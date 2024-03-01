#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

#include "builtins.h"
#include "config.h"

int get_argc(char** argv);

int builtin_exit(char*[]);
int echo(char*[]);
int lcd(char*[]);
int lkill(char*[]);
int lls(char*[]);
int undefined(char*[]);

builtin_pair builtins_table[]={
	{"exit",	&builtin_exit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};

int get_argc(char** argv) {
	int argc = 0;
	while(argv[argc]) {
		argc++;
	}
	return argc;
}

int builtin_exit(char** argv) {
	if(get_argc(argv) > 1) {
		return BUILTIN_ERROR;
	}
	exit(0);
}

int lcd(char** argv) {
	int argc = get_argc(argv);
	if(argc > 2) {
		return BUILTIN_ERROR;
	}

	char path_buffer[MAX_LINE_LENGTH];
	if(argc == 1) {	
		strcpy(path_buffer, getenv("HOME"));
		if(path_buffer == NULL) {
			return BUILTIN_ERROR;
		}
		argv[1] = path_buffer; 
	}

	int ret = chdir(argv[1]);
	if(ret == -1) {
		return BUILTIN_ERROR;
	}
	return 0;
}

int lkill(char** argv) {
	int argc = get_argc(argv);
	if(argc > 3 || argc <= 1) {
		return BUILTIN_ERROR;
	}

	int sig = SIGTERM;
	char* pid_str = argv[1];
	if(argc == 3) {
		pid_str = argv[2];

		if(argv[1] == NULL || argv[1][0] != '-') {
			return BUILTIN_ERROR;
		}

		char* endptr = NULL;
		errno = 0;
		long new_sig = strtol(argv[1]+1, &endptr, 10);
		if(errno != 0) {
			return BUILTIN_ERROR;
		}
		if(argv[1] == NULL || *endptr != '\0') {
			return BUILTIN_ERROR;
		}

		sig = new_sig;
	}

	char* endptr = NULL;
	errno = 0;
	long pid = strtol(pid_str, &endptr, 10);
	if(errno != 0) {
		return BUILTIN_ERROR;
	}
	if(pid_str == NULL || *endptr != '\0') {
		return BUILTIN_ERROR;
	}

	int ret = kill(pid, sig);
	if(ret != 0) {
		return BUILTIN_ERROR;
	}
	return 0;
}

int lls(char** argv) {
	int argc = get_argc(argv);
	if(argc != 1) {
		return BUILTIN_ERROR;
	}

	char path_buffer[MAX_LINE_LENGTH];
	char* cwd = getcwd(path_buffer, MAX_LINE_LENGTH);
	if(cwd == NULL) {
		return BUILTIN_ERROR;
	}

	DIR* directory = opendir(path_buffer);
	if(directory == NULL) {
		return BUILTIN_ERROR;
	}

	struct dirent* file = readdir(directory);
	while(file) {
		if((file->d_name)[0] != '.') {
			printf("%s\n", file->d_name);
		}
		file = readdir(directory);
	}

	closedir(directory);
	fflush(stdout);

	return 0;
}

int echo(char* argv[]) {
	int i = 1;
	if (argv[i]) {
		printf("%s", argv[i++]);
	} 
	while(argv[i]) {
		printf(" %s", argv[i++]);
	}
	printf("\n");
	fflush(stdout);
	return 0;
}

int undefined(char* argv[]) {
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}
