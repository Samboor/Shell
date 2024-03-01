#ifndef _EXECUTILS_H_
#define _EXECUTILS_H_

#include <sys/types.h>

#include "config.h"
#include "siparse.h"

command* get_command(char* begin);

void prepare_argv(command* com, char** argv);

void execute_line(pipelineseq* pipeline_seq, char** argv);

int execute_command(char** argv, int fd_in, int fd_out, bool close_in, bool close_out, int prv_pipe[2], int nxt_pipe[2], bool bg_command);

bool try_builtins(char** argv);

#endif /* !_EXECUTILS_H_ */