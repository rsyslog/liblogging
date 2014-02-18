#include <stdio.h>
#include "stdlog.h"

void main()
{
	stdlog_log(NULL, 7, "Test %s, %d, %c, %p\n", "abc", 4712, 'T', NULL);
}
