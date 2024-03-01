#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#include "executils.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"
#include "ioutils.h"
#include "fgtable.h"

void prepare_argv(command* com, char** argv) {
    int argc = 0;
    argseq* args = com->args;
	do {
		argv[argc] = args->arg;
		args = args->next;
        argc++;
	} while(args != com->args);
    argv[argc] = NULL;
}

int number_of_commands(commandseq* commands) {
	if(commands == NULL) {
		return 0;
	}
	int N = 0;
	commandseq* end = commands;
	do {
		N++;
		commands = commands->next;
	} while(commands != end);
	return N;
}

bool valid_line(pipelineseq* pipeline_seq) {
	pipelineseq* end = pipeline_seq;

	do { // iterating over pipelines
		pipeline* pipe = pipeline_seq->pipeline;
		commandseq*  com_seq = pipe->commands;

		int pipeline_length = 0;
		bool empty_command = false;

		do { // iterating over commands in pipeline
			pipeline_length++;
			command* com = com_seq->com;
			if(com == NULL) {
				empty_command = true;
			}

			com_seq = com_seq->next;
		} while(com_seq != pipe->commands);

		if(pipeline_length > 1 && empty_command) {
			fprintf(stderr, "empty command\n");
			return false;
		}

		pipeline_seq = pipeline_seq->next;
	} while(pipeline_seq != end);

	return true;
}

void safe_close(int* fd) {
	if(*fd == -1) {
		return;
	}
	int ret = close(*fd);
	if(ret == -1) {
		exit(EXIT_FAILURE);
	}
	*fd = -1;
}

void execute_line(pipelineseq* pipeline_seq, char** argv) {
	if(!valid_line(pipeline_seq)) {
		fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
		return;
	}

	pipelineseq* end = pipeline_seq;
	do {
		pipeline* cur_pipeline = pipeline_seq->pipeline;
		commandseq* commands = cur_pipeline->commands;

		// clear fg_table
		prepare_fg(&FG_TABLE);

		int N = number_of_commands(commands), cnt = 0;
		if(N == 1 && commands->com == NULL) {
			pipeline_seq = pipeline_seq->next;
			continue;
		}

		int prv_pipe[2], nxt_pipe[2];
		prv_pipe[0] = prv_pipe[1] = -1;
		nxt_pipe[0] = nxt_pipe[1] = -1;

		for(int i = 0; i < N; i++, commands = commands->next) {
			command* com = commands->com;
			prepare_argv(com, argv);

			if(N == 1 && try_builtins(argv)) {
				// builtin was used
				break;
			}

			int fd[2];
			fd[0] = 0, fd[1] = 1;
			bool discard_command = false;

			if(i > 0) { // use previous pipe out as input
				fd[0] = prv_pipe[0];
			}

			if(i+1 < N) { // create new pipe :)
				errno = 0;
				int ret = pipe(nxt_pipe);
				if(ret == -1) {
					fprintf(stderr, "error occured while creating pipe\n");
					exit(EXIT_FAILURE);
				}
				fd[1] = nxt_pipe[1];
			}

			// apply redirects
			redirseq* redirs = com->redirs;
			bool close_in = false, close_out = false;
			if(redirs) {
				redirseq* end = redirs;
				do {
					redir* r = redirs->r;
					if(IS_RIN(r->flags)) {
						close_in = true;
						safe_close(&prv_pipe[0]);

						errno = 0;
						fd[0] = open(r->filename, O_RDONLY);

						if(fd[0] == -1) {
							discard_command = true;
							if(errno == ENOENT) {
								fprintf(stderr, "%s: no such file or directory\n", r->filename);
							}
							else if(errno == EACCES) {
								fprintf(stderr, "%s: permission denied\n", r->filename);
							}
						}
					}
					else if(IS_ROUT(r->flags) || IS_RAPPEND(r->flags)) {
						close_out = true;
						safe_close(&nxt_pipe[1]);

						int flag = O_TRUNC;
						if(IS_RAPPEND(r->flags)) {
							flag = O_APPEND;
						}
						flag |= O_CREAT | O_WRONLY;

						errno = 0;
						fd[1] = open(r->filename, flag, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);

						if(fd[1] == -1) {
							discard_command = true;
							if(errno == EACCES) {
								fprintf(stderr, "%s: permission denied\n", r->filename);
							}
						}
					}
					redirs = redirs->next;
				} while(redirs != end);
			}

			if(!discard_command) {
				int pid = execute_command(argv, fd[0], fd[1], close_in, close_out, prv_pipe, nxt_pipe, cur_pipeline->flags != INBACKGROUND);
			}
			
			for(int j = 0; j < 2; j++) {
				safe_close(&prv_pipe[j]);
				prv_pipe[j] = nxt_pipe[j];
				nxt_pipe[j] = -1;
			}
		}

		block_sigchld();
		while(FG_TABLE.cnt > 0) {
			errno = 0;
			int ret = sigsuspend(&REG_SIGSET);
		}
		unblock_sigchld();

		pipeline_seq = pipeline_seq->next;
	} while(pipeline_seq != end);
}

int execute_command(char** argv, int fd_in, int fd_out, bool close_in, bool close_out, int prv_pipe[2], int nxt_pipe[2], bool fg_command) {
	//block sigchld 
	block_sigchld();

    pid_t pid = fork();
	if(pid == 0) { // child
		unblock_sigint();
		unblock_sigchld();

		if(!fg_command) {
			errno = 0;
			pid_t gpid = setpgid(0, 0);
			if(gpid == -1) {
				exit(EXIT_FAILURE);
			}
		}

		if(fd_in != 0) {
			int ret = dup2(fd_in, 0);
			if(ret != 0) {
				exit(EXEC_FAILURE);
			}
		}
		if(fd_out != 1) {
			int ret = dup2(fd_out, 1);
			if(ret != 1) {
				exit(EXEC_FAILURE);
			}
		}

		// close pipes
		safe_close(&prv_pipe[0]);
		safe_close(&prv_pipe[1]);
		safe_close(&nxt_pipe[0]);
		safe_close(&nxt_pipe[1]);

		// close redirects
		if(close_in) {
			safe_close(&fd_in);
		}
		if(close_out) {
			safe_close(&fd_out);
		}

		execvp(argv[0], argv);

		assert(argv != NULL && argv[0] != NULL);
		if(errno == 2) {
			fprintf(stderr, "%s: no such file or directory\n", argv[0]);
		}
		else if(errno == 13) {
			fprintf(stderr, "%s: permission denied\n", argv[0]);
		}
		else {
			fprintf(stderr, "%s: exec error\n", argv[0]);
		}
			
		exit(EXEC_FAILURE);
	}
	else if(pid == -1) { // error
		fprintf(stderr, "fork() failure\n");
		exit(EXIT_FAILURE);
	}
	else { // parent
		if(close_in) {
			safe_close(&fd_in);
		}
		if(close_out) {
			safe_close(&fd_out);
		}

		if(fg_command) {
			insert_fg(&FG_TABLE, pid);
		}

		unblock_sigchld();
	}

	return pid;
}

bool try_builtins(char** argv) {
	if(argv == NULL || argv[0] == NULL) {
		return false;
	}

	for(int i = 0; builtins_table[i].name; i++) {
		if(strcmp(argv[0], builtins_table[i].name) != 0) {
			continue;
		}
		int ret = builtins_table[i].fun(argv);
		if(ret != 0) {
			fprintf(stderr, "Builtin %s error.\n", builtins_table[i].name);
		}
		return true;
	}

	return false;
}