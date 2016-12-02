#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdlog.h"

int main(int argc, char *argv[])
{
	char buf[40];
	stdlog_channel_t ch;
	stdlog_channel_t ch2;
	int dflt_option = STDLOG_SIGSAFE;
	int option = 0;
	char *chanspec = NULL;

	if (3 == argc && (0 == strcmp(argv[1], "-p"))) {
		dflt_option |= STDLOG_PID;
		option |= STDLOG_PID;
		chanspec = argv[2];
	} else if (2 == argc) {
		chanspec = argv[1];
	} else if(argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: tester [-p] channelspec\n");
		exit(1);
	}

	stdlog_init(dflt_option);
	ch = stdlog_open("tester", option, STDLOG_LOCAL0, chanspec);
	ch2 = stdlog_open("tester", STDLOG_USE_DFLT_OPTS, STDLOG_LOCAL0, chanspec);
	stdlog_log(ch, STDLOG_DEBUG, "Test %10.6s, %u, %d, %c, %x, %p, %f",
		   "abc", 4712, -4712, 'T', 0x129abcf0, NULL, 12.0345);
	stdlog_log_b(ch2, STDLOG_DEBUG, buf, sizeof(buf), "Test %100.50s, %u, %d, %c, %x, %p, %f",
		   "abc", 4712, -4712, 'T', 0x129abcf0, NULL, 12.03);
	stdlog_deinit();
	return 0;
}
