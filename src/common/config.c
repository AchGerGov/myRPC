#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LINE_SIZE 512

static char * trim_spaces(char *str)
{
    char *end;

    while (isspace((unsigned char)*str))
    {
        str++;
    }

    if (*str == '\0')
    {
        return str;
    }

    end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    return str;
}

static void set_default_config(struct server_config *config)
{
    config->port = 1234;
    config->socket_type = SOCKET_TYPE_STREAM;
    config->daemon_mode = 0;
    snprintf(config->log_file, sizeof(config->log_file),
             "/var/log/myRPC-server.log");
}

static int parse_socket_type(const char *value)
{
    if (strcmp(value, "stream") == 0)
    {
        return SOCKET_TYPE_STREAM;
    }

    if (strcmp(value, "dgram") == 0)
    {
        return SOCKET_TYPE_DGRAM;
    }

    return 0;
}

static int parse_daemon_mode(const char *value)
{
    if (strcmp(value, "yes") == 0 || strcmp(value, "1") == 0)
    {
        return 1;
    }

    return 0;
}

int load_server_config(const char *path, struct server_config *config)
{
    FILE *file;
    char line[LINE_SIZE];

    set_default_config(config);

    file = fopen(path, "r");
    if (file == NULL)
    {
        perror("fopen");
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *key;
        char *value;
        char *eq;

        key = trim_spaces(line);

        if (key[0] == '#' || key[0] == '\0')
        {
            continue;
        }

        eq = strchr(key, '=');
        if (eq == NULL)
        {
            continue;
        }

        *eq = '\0';
        value = eq + 1;

        key = trim_spaces(key);
        value = trim_spaces(value);

        if (strcmp(key, "port") == 0)
        {
            config->port = atoi(value);
        }
        else if (strcmp(key, "socket_type") == 0)
        {
            config->socket_type = parse_socket_type(value);
        }
        else if (strcmp(key, "daemon") == 0)
        {
            config->daemon_mode = parse_daemon_mode(value);
        }
        else if (strcmp(key, "log_file") == 0)
        {
            snprintf(config->log_file, sizeof(config->log_file), "%s", value);
        }
    }

    fclose(file);

    if (config->port <= 0)
    {
        fprintf(stderr, "Invalid port in config\n");
        return 1;
    }

    if (config->socket_type == 0)
    {
        fprintf(stderr, "Invalid socket_type in config\n");
        return 1;
    }

    return 0;
}

void print_server_config(const struct server_config *config)
{
    printf("myRPC-server config:\n");
    printf("  port: %d\n", config->port);

    if (config->socket_type == SOCKET_TYPE_STREAM)
    {
        printf("  socket_type: stream\n");
    }
    else if (config->socket_type == SOCKET_TYPE_DGRAM)
    {
        printf("  socket_type: dgram\n");
    }

    printf("  daemon: %s\n", config->daemon_mode ? "yes" : "no");
    printf("  log_file: %s\n", config->log_file);
}
