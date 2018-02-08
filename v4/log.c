#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "log.h"

int log_level=DEBUG;

void set_log_level(int level)
{
	log_level=level;
}

int
fill_time_string(char *time_string, int len)
{
        time_t nowtime;
        struct tm result;

        nowtime = time(NULL);
        /* Format is month day hour:minute:second (24 time) */
        return strftime(time_string, len, "%b %d %H:%M:%S",
                localtime_r(&nowtime, &result));
}

void
log_me(FILE *fd, const char *ident, const char *fmt, ...)
{
	char buf[16];
	va_list t;
	fill_time_string(buf, sizeof(buf));

	fprintf(fd, "%s %s ", ident, buf);

	va_start(t, fmt);
	vfprintf(fd, fmt, t);
	va_end(t);
}

