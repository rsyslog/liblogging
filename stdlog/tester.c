#include <stdio.h>
#include "stdlog.h"

int main(int __attribute__((unused)) argc, char __attribute__((unused)) *argv[])
{
	stdlog_log(NULL, 7, "Test %s, %d, %c, %p", "abc", 4712, 'T', NULL);
	return 0;
}
