#ifndef MYRPC_CONFIG_H
#define MYRPC_CONFIG_H
#define SOCKET_TYPE_STREAM 1
#define SOCKET_TYPE_DGRAM 2

struct server_config
{
    int port;
    int socket_type;
    int daemon_mode;
    char log_file[256];
};

int load_server_config(const char *path, struct server_config *config);
void print_server_config(const struct server_config *config);

#endif
