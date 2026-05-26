#include "../common/protocol.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET_STREAM 1
#define SOCKET_DGRAM 2

struct client_options
{
    char *host;
    int port;
    int socket_type;
    char *command;
};

static void print_help(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -h, --host HOST        Server IP address\n");
    printf("  -p, --port PORT        Server port\n");
    printf("  -c, --command COMMAND  Bash command\n");
    printf("  -s, --stream           Use stream socket\n");
    printf("  -d, --dgram            Use datagram socket\n");
    printf("      --help             Show this help\n");
}

static const char *get_current_username(void)
{
    struct passwd *pw;

    pw = getpwuid(getuid());
    if (pw == NULL)
    {
        return NULL;
    }

    return pw->pw_name;
}

static int parse_arguments(int argc, char **argv, struct client_options *options)
{
    int opt;
    int option_index = 0;

    static struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"command", required_argument, 0, 'c'},
        {"stream", no_argument, 0, 's'},
        {"dgram", no_argument, 0, 'd'},
        {"help", no_argument, 0, 1000},
        {0, 0, 0, 0}};

    options->host = NULL;
    options->port = 0;
    options->socket_type = 0;
    options->command = NULL;

    while ((opt = getopt_long(argc, argv, "h:p:c:sd", long_options,
                              &option_index)) != -1)
    {
        switch (opt)
        {
        case 'h':
            options->host = optarg;
            break;

        case 'p':
            options->port = atoi(optarg);
            break;

        case 'c':
            options->command = optarg;
            break;

        case 's':
            options->socket_type = SOCKET_STREAM;
            break;

        case 'd':
            options->socket_type = SOCKET_DGRAM;
            break;

        case 1000:
            print_help(argv[0]);
            exit(0);

        default:
            print_help(argv[0]);
            return 1;
        }
    }

    if (options->host == NULL)
    {
        fprintf(stderr, "Error: host is required\n");
        return 1;
    }

    if (options->port <= 0)
    {
        fprintf(stderr, "Error: valid port is required\n");
        return 1;
    }

    if (options->command == NULL)
    {
        fprintf(stderr, "Error: command is required\n");
        return 1;
    }

    if (options->socket_type == 0)
    {
        fprintf(stderr, "Error: socket type is required\n");
        return 1;
    }

    return 0;
}

static int send_stream_request(const struct client_options *options,
                               const char *request)
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char response_buffer[RESPONSE_SIZE];
    struct rpc_response response;
    ssize_t bytes_read;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)options->port);

    if (inet_pton(AF_INET, options->host, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock_fd);
        return 1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    if (send(sock_fd, request, strlen(request), 0) < 0)
    {
        perror("send");
        close(sock_fd);
        return 1;
    }

    memset(response_buffer, 0, sizeof(response_buffer));

    bytes_read = recv(sock_fd, response_buffer, sizeof(response_buffer) - 1, 0);
    if (bytes_read <= 0)
    {
        perror("recv");
        close(sock_fd);
        return 1;
    }

    response_buffer[bytes_read] = '\0';

    if (parse_response(response_buffer, &response) != 0)
    {
        fprintf(stderr, "Error: invalid response format\n");
        close(sock_fd);
        return 1;
    }

    printf("server response code: %d\n", response.code);
    printf("server response result: %s\n", response.result);

    close(sock_fd);
    return response.code;
}

static int send_dgram_request(const struct client_options *options,
                              const char *request)
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char response_buffer[RESPONSE_SIZE];
    struct rpc_response response;
    ssize_t bytes_read;
    socklen_t server_len;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)options->port);

    if (inet_pton(AF_INET, options->host, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock_fd);
        return 1;
    }

    if (sendto(sock_fd, request, strlen(request), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("sendto");
        close(sock_fd);
        return 1;
    }

    memset(response_buffer, 0, sizeof(response_buffer));
    server_len = sizeof(server_addr);

    bytes_read = recvfrom(sock_fd, response_buffer,
                          sizeof(response_buffer) - 1, 0,
                          (struct sockaddr *)&server_addr, &server_len);

    if (bytes_read <= 0)
    {
        perror("recvfrom");
        close(sock_fd);
        return 1;
    }

    response_buffer[bytes_read] = '\0';

    if (parse_response(response_buffer, &response) != 0)
    {
        fprintf(stderr, "Error: invalid response format\n");
        close(sock_fd);
        return 1;
    }

    printf("server response code: %d\n", response.code);
    printf("server response result: %s\n", response.result);

    close(sock_fd);
    return response.code;
}

int main(int argc, char **argv)
{
    struct client_options options;
    char request[REQUEST_SIZE];
    const char *login;

    if (parse_arguments(argc, argv, &options) != 0)
    {
        return 1;
    }

    login = get_current_username();
    if (login == NULL)
    {
        fprintf(stderr, "Error: failed to get current username\n");
        return 1;
    }

    if (build_request(request, sizeof(request), login, options.command) != 0)
    {
        fprintf(stderr, "Error: failed to build request\n");
        return 1;
    }

    if (options.socket_type == SOCKET_STREAM)
    {
        return send_stream_request(&options, request);
    }

    if (options.socket_type == SOCKET_DGRAM)
    {
        return send_dgram_request(&options, request);
    }

    fprintf(stderr, "Error: unknown socket type\n");
    return 1;
}
