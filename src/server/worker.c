#include "worker.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define TMP_TEMPLATE_SIZE 64


static int read_file_to_buffer(const char *path, char *buffer, int buffer_size)
{
    FILE *file;
    size_t bytes_read;

    file = fopen(path, "r");
    if (file == NULL)
    {
        snprintf(buffer, buffer_size, "failed to open result file");
        return 1;
    }

    bytes_read = fread(buffer, 1, (size_t)buffer_size - 1, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return 0;
}

int execute_command(const char *command, char *result, int result_size)
{
    char stdout_template[TMP_TEMPLATE_SIZE] = "/tmp/myRPC_stdout_XXXXXX";
    char stderr_template[TMP_TEMPLATE_SIZE] = "/tmp/myRPC_stderr_XXXXXX";
    int stdout_fd;
    int stderr_fd;
    pid_t pid;
    int status;

    stdout_fd = mkstemp(stdout_template);
    if (stdout_fd < 0)
    {
        snprintf(result, result_size, "failed to create stdout temp file");
        return 1;
    }

    stderr_fd = mkstemp(stderr_template);
    if (stderr_fd < 0)
    {
        close(stdout_fd);
        unlink(stdout_template);
        snprintf(result, result_size, "failed to create stderr temp file");
        return 1;
    }

    pid = fork();
    if (pid < 0)
    {
        close(stdout_fd);
        close(stderr_fd);
        unlink(stdout_template);
        unlink(stderr_template);
        snprintf(result, result_size, "fork failed");
        return 1;
    }

    if (pid == 0)
    {
        dup2(stdout_fd, STDOUT_FILENO);
        dup2(stderr_fd, STDERR_FILENO);

        close(stdout_fd);
        close(stderr_fd);

        execl("/bin/bash", "bash", "-c", command, (char *)NULL);
        _exit(127);
    }

    close(stdout_fd);
    close(stderr_fd);

    if (waitpid(pid, &status, 0) < 0)
    {
        unlink(stdout_template);
        unlink(stderr_template);
        snprintf(result, result_size, "waitpid failed");
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
        read_file_to_buffer(stdout_template, result, result_size);
        unlink(stdout_template);
        unlink(stderr_template);
        return 0;
    }

    read_file_to_buffer(stderr_template, result, result_size);
    unlink(stdout_template);
    unlink(stderr_template);

    if (strlen(result) == 0)
    {
        snprintf(result, result_size, "command failed");
    }

    return 1;
}
