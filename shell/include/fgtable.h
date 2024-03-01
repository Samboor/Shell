#ifndef _FGTABLE_H_
#define _FGTABLE_H_

#include <stdbool.h>
#include <signal.h>

#include "config.h"

// fg_table

typedef struct fg_table {
    int id[FG_TABLE_SIZE];
    int cnt;
} fg_table;

extern fg_table FG_TABLE;

bool remove_fg(fg_table* table, int pid);

void insert_fg(fg_table* table, int pid);

void prepare_fg(fg_table* table);

void handle_sigchld(int signum);

// bg_table

typedef struct bg_table {
    int id[BG_TABLE_SIZE];
    int ret_status[BG_TABLE_SIZE];
    int size;
} bg_table;

extern bg_table BG_TABLE;

void prepare_bg(bg_table* table);

void insert_bg(bg_table* table, int pid, int ret);

void flush_bg(bg_table* table);

// sigchld

extern sigset_t BLOCK_SIGCHLD, REG_SIGSET;
extern struct sigaction SIGCHLD_SIGACTION;

void initialize_signal_handlers();

void block_sigchld();

void unblock_sigchld();

// sigint

extern struct sigaction PRV_SIGINT_HANDLER;

void block_sigint();

void unblock_sigint();

#endif /* !_FGTABLE_H_ */