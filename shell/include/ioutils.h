#ifndef _IOUTILS_H_
#define _IOUTILS_H_

#include <sys/types.h>
#include <stdbool.h>

ssize_t read_next_command(char* buffer, char** begin, char** end, bool* open_file, bool print_prompt);

bool update_pointers(char* buffer, char** begin, char** end, char** endl, bool* open_file, bool* discard_command);

#endif /* !_IOUTILS_H_ */
