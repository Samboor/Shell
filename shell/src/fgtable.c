#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fgtable.h"
#include "config.h"

// FG_TABLE

fg_table FG_TABLE;

bool remove_fg(fg_table* table, int pid) {
    for(int i = 0; i < FG_TABLE_SIZE; i++) {
        if(table->id[i] == pid) {
            table->cnt--;
            table->id[i] = -1;
            return true;
        }
    }
    return false;
}

void insert_fg(fg_table* table, int pid) {
    for(int i = 0; i < FG_TABLE_SIZE; i++) {
        if(table->id[i] == -1) {
            table->cnt++;
            table->id[i] = pid;
            return;
        }
    }
}

void prepare_fg(fg_table* table) {
    for(int i = 0; i < FG_TABLE_SIZE; i++) {
        table->id[i] = -1;
    }
    table->cnt = 0;
}

// BG_TABLE

bg_table BG_TABLE;

void prepare_bg(bg_table* table) {
    table->size = 0;
}

void insert_bg(bg_table* table, int pid, int ret) {
    int i = table->size++;
    table->id[i] = pid;
    table->ret_status[i] = ret;
}

void flush_bg(bg_table* table) {
    if(isatty(0) == 0) {
        table->size = 0;
        return;
    }

    for(int i = 0; i < table->size; i++) {
        int wstatus = table->ret_status[i];
        printf("Background process %d terminated. ", table->id[i]);
        if(WIFEXITED(wstatus)) {
            printf("(exited with status %d)\n", wstatus);
        }
        else if(WIFSIGNALED(wstatus)) {
            printf("(killed by signal %d)\n", wstatus);
        }
    }
    table->size = 0;
    fflush(stdout);
}

// SIGCHLD
sigset_t BLOCK_SIGCHLD, REG_SIGSET;
struct sigaction SIGCHLD_SIGACTION;
struct sigaction IGNORE_SIGINT, PRV_SIGINT_HANDLER;

void initialize_signal_handlers() {
    int ret = sigemptyset(&BLOCK_SIGCHLD);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }

    ret = sigaddset(&BLOCK_SIGCHLD, SIGCHLD);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }

    SIGCHLD_SIGACTION.sa_handler = handle_sigchld;
    SIGCHLD_SIGACTION.sa_mask = BLOCK_SIGCHLD;
    SIGCHLD_SIGACTION.sa_flags = 0;
    ret = sigaction(SIGCHLD, &SIGCHLD_SIGACTION, NULL);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }

    IGNORE_SIGINT.sa_handler = SIG_IGN;
    ret = sigemptyset(&IGNORE_SIGINT.sa_mask);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }
    IGNORE_SIGINT.sa_flags = SA_RESTART;
}

void handle_sigchld(int signum) {
    while(true) {
        int ret = 0;
        int pid = waitpid(-1, &ret, WNOHANG);
        if(pid == -1 || pid == 0) {
            return;
        }
        if(!remove_fg(&FG_TABLE, pid)) {
            insert_bg(&BG_TABLE, pid, ret);
        }
    }
}

void block_sigchld() {
    int ret = sigprocmask(SIG_BLOCK, &BLOCK_SIGCHLD, &REG_SIGSET);
	if(ret == -1) {
		exit(EXIT_FAILURE);
	}
}

void unblock_sigchld() {
    int ret = sigprocmask(SIG_UNBLOCK, &BLOCK_SIGCHLD, NULL);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }
}

// SIGINT

void block_sigint() {
    int ret = sigaction(SIGINT, &IGNORE_SIGINT, &PRV_SIGINT_HANDLER);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }
}

void unblock_sigint() {
    int ret = sigaction(SIGINT, &PRV_SIGINT_HANDLER, NULL);
    if(ret == -1) {
        exit(EXIT_FAILURE);
    }
}

