#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "ioutils.h"
#include "config.h"
#include "fgtable.h"

ssize_t read_next_command(char* buffer, char** begin, char** end, bool* open_file, bool print_prompt) {
    block_sigchld();
    flush_bg(&BG_TABLE);
    if(print_prompt) {
        ssize_t prompt_status = write(1, PROMPT_STR, PROMPT_LENGTH - 1);
        assert(prompt_status == PROMPT_LENGTH - 1);
    }
    unblock_sigchld();
    
    if(BUFFER_SIZE - (*end - buffer) <= MAX_LINE_LENGTH) {
        int N = *end - *begin;
        if(N < 0) {
            N = 0;
        }
        memmove(buffer, *begin, N);
        *begin = buffer;
        *end = *begin + N;
    }

    ssize_t ret = read(0, *end, BUFFER_SIZE - (*end - buffer) - 1);    
    assert(ret != -1);

    *end += ret;

    if(ret == 0) {
        *open_file = false;
    }

    return ret;
}

bool update_pointers(char* buffer, char** begin, char** end, char** endl, bool* open_file, bool* discard_command) {
	int len = *end - *begin;
	if(*open_file && len < MAX_LINE_LENGTH) {
		read_next_command(buffer, begin, end, open_file, isatty(0));
	}

    len = *end - *begin;
	*endl = memchr(*begin, '\n', len);
	if(*endl == NULL && *open_file && len >= MAX_LINE_LENGTH) {
		*begin = *end;
		*discard_command = true;
		return false;
	}
	else if(*endl == NULL && *open_file == false) {
		**end = '\0';
		*endl = *end;
	}
    else if(*endl == NULL) {
        return false;
    }
	else {
		**endl = '\0';
	}

	if((*endl - *begin) > MAX_LINE_LENGTH) {
		*discard_command = true;
	}

	if(*discard_command) {
		*begin = *endl + 1;
		*discard_command = false;
		fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
		return false;
	}
	return true;
}