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
	ch = stdlog_open("tester", 0, 17, argv[1]);
	stdlog_log(ch, 7, "Test %s, %d, %c, %p", "abc", 4712, 'T', NULL);
	return 0;
}
