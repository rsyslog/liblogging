#include <stdio.h>
#include <stdlib.h>
#include "stdlog.h"

int main(int argc, char *argv[])
{
	stdlog_channel_t ch;
	if(argc != 2) {
		fprintf(stderr, "Usage: tester channelspec\n");
		exit(1);
	}

	stdlog_init(STDLOG_SIGSAFE);
	ch = stdlog_open("tester", 0, STDLOG_LOCAL0, argv[1]);
	stdlog_log(ch, STDLOG_DEBUG, "Test %100.50s, %u, %d, %c, %x, %p, %f",
		   "abc", 4712, -4712, 'T', 0x129abcf0, NULL, 12.03);
	stdlog_deinit();
	return 0;
}
