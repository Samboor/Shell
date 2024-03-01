#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 2048

#define BUFFER_SIZE 2 * MAX_LINE_LENGTH

#define FG_TABLE_SIZE MAX_LINE_LENGTH/2 + 2

#define BG_TABLE_SIZE 2048

#define SYNTAX_ERROR_STR "Syntax error."

#define EXEC_FAILURE 127

#define PROMPT_STR "$ \0"
#define PROMPT_LENGTH strlen(PROMPT_STR)

#endif /* !_CONFIG_H_ */
