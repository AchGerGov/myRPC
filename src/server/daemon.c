#include "daemon.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int daemonize_process(void)
{
    pid_t pid;
    int fd;

    pid = fork();
    if (pid < 0)
    {
        return 1;
    }

    if (pid > 0)
    {
        exit(0);
    }

    if (setsid() < 0)
    {
        return 1;
    }

    pid = fork();
    if (pid < 0)
    {
        return 1;
    }

    if (pid > 0)
    {
        exit(0);
    }

    umask(0);
    chdir("/");

    fd = open("/dev/null", O_RDWR);
    if (fd >= 0)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        if (fd > STDERR_FILENO)
        {
            close(fd);
        }
    }

    return 0;
}
