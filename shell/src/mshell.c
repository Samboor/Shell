#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include "config.h"
#include "siparse.h"
#include "ioutils.h"
#include "executils.h"
#include "builtins.h"
#include "fgtable.h"

int main(int argc, char* argv[]) {
	initialize_signal_handlers();
	block_sigint();

	char buffer[BUFFER_SIZE];
	char *begin = buffer, *end = buffer, *endl = NULL;

	char* command_argv[MAX_LINE_LENGTH/2 + 2];
	bool discard_command = false, open_file = true;

	prepare_bg(&BG_TABLE);

	while(open_file || begin < end) {
		if(!update_pointers(buffer, &begin, &end, &endl, &open_file, &discard_command)) {
			continue;
		}
		// [begin, end) should be executed

		pipelineseq* pipeline_seq = parseline(begin);
		begin = endl + 1;
		if(pipeline_seq == NULL || pipeline_seq->pipeline == NULL) {
			continue;
		}
		execute_line(pipeline_seq, command_argv);
		fflush(stdout);
		fflush(stderr);
	}

	return 0;
}
