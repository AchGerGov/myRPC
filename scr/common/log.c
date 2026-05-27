#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>

static FILE *log_file = NULL;

void log_init(const char *path)
{
    openlog("myRPC-server", LOG_PID | LOG_CONS, LOG_DAEMON);

    if (path != NULL && path[0] != '\0')
    {
        log_file = fopen(path, "a");
    }
}

void log_close(void)
{
    if (log_file != NULL)
    {
        fclose(log_file);
        log_file = NULL;
    }

    closelog();
}

static void write_file_log(const char *level, const char *fmt, va_list args)
{
    time_t now;

    if (log_file == NULL)
    {
        return;
    }

    now = time(NULL);

    fprintf(log_file, "%ld [%s] ", (long)now, level);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");
    fflush(log_file);
}

void log_info(const char *fmt, ...)
{
    va_list args_file;
    va_list args_syslog;

    va_start(args_file, fmt);
    va_copy(args_syslog, args_file);

    write_file_log("INFO", fmt, args_file);
    vsyslog(LOG_INFO, fmt, args_syslog);

    va_end(args_syslog);
    va_end(args_file);
}

void log_error(const char *fmt, ...)
{
    va_list args_file;
    va_list args_syslog;

    va_start(args_file, fmt);
    va_copy(args_syslog, args_file);

    write_file_log("ERROR", fmt, args_file);
    vsyslog(LOG_ERR, fmt, args_syslog);

    va_end(args_syslog);
    va_end(args_file);
}
