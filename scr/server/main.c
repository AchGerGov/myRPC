#include "../common/config.h"
#include "../common/log.h"
#include "../common/protocol.h"
#include "../common/users.h"
#include "daemon.h"
#include "worker.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#define DEFAULT_CONFIG_PATH "/etc/myRPC/myRPC.conf"
#define DEFAULT_USERS_PATH "/etc/myRPC/users.conf"
#define BACKLOG 10

static volatile sig_atomic_t need_stop = 0;
static volatile sig_atomic_t need_reload = 0;

static void handle_sigint(int signo)
{
    (void)signo;
    need_stop = 1;
}

static void handle_sighup(int signo)
{
    (void)signo;
    need_reload = 1;
}

static void handle_sigchld(int signo)
{
    (void)signo;

    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
}

static int setup_signals(void)
{
    struct sigaction sa_int;
    struct sigaction sa_hup;
    struct sigaction sa_chld;

    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);

    if (sigaction(SIGINT, &sa_int, NULL) < 0)
    {
        return 1;
    }

    if (sigaction(SIGTERM, &sa_int, NULL) < 0)
    {
        return 1;
    }

    memset(&sa_hup, 0, sizeof(sa_hup));
    sa_hup.sa_handler = handle_sighup;
    sigemptyset(&sa_hup.sa_mask);

    if (sigaction(SIGHUP, &sa_hup, NULL) < 0)
    {
        return 1;
    }

    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0)
    {
        return 1;
    }

    return 0;
}

static int handle_client(int client_fd, const struct user_list *users)
{
    char request_buffer[REQUEST_SIZE];
    char response_buffer[RESPONSE_SIZE];
    struct rpc_request request;
    ssize_t bytes_read;

    memset(request_buffer, 0, sizeof(request_buffer));
    memset(response_buffer, 0, sizeof(response_buffer));

    bytes_read = recv(client_fd, request_buffer, sizeof(request_buffer) - 1, 0);
    if (bytes_read <= 0)
    {
        log_error("recv failed");
        return 1;
    }

    request_buffer[bytes_read] = '\0';

    if (parse_request(request_buffer, &request) != 0)
    {
        build_response(response_buffer, sizeof(response_buffer), 1,
                       "invalid request format");
        send(client_fd, response_buffer, strlen(response_buffer), 0);
        log_error("invalid request format");
        return 1;
    }

    log_info("worker pid %d: user=%s command=%s",
             getpid(), request.login, request.command);

    if (!is_user_allowed(users, request.login))
    {
        build_response(response_buffer, sizeof(response_buffer), 1,
                       "user is not allowed");
        send(client_fd, response_buffer, strlen(response_buffer), 0);
        log_error("user denied: %s", request.login);
        return 1;
    }

    {
        char command_result[RESULT_SIZE];
        int command_code;

        memset(command_result, 0, sizeof(command_result));

        command_code = execute_command(request.command, command_result,
                                       sizeof(command_result));

        build_response(response_buffer, sizeof(response_buffer), command_code,
                       command_result);
        send(client_fd, response_buffer, strlen(response_buffer), 0);

        return command_code;
    }
}

static int reload_users(struct user_list *users)
{
    struct user_list new_users;

    if (load_user_list(DEFAULT_USERS_PATH, &new_users) != 0)
    {
        log_error("failed to reload users whitelist");
        return 1;
    }

    *users = new_users;
    log_info("users whitelist reloaded");
    return 0;
}

static int run_stream_server(int port, struct user_list *users)
{
    int server_fd;
    int client_fd;
    int opt;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;

    if (setup_signals() != 0)
    {
        log_error("failed to setup signals");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        log_error("socket failed");
        return 1;
    }

    opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        log_error("setsockopt failed");
        close(server_fd);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        log_error("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0)
    {
        log_error("listen failed");
        close(server_fd);
        return 1;
    }

    log_info("server pid %d listening on port %d", getpid(), port);

    while (!need_stop)
    {
        pid_t pid;

        if (need_reload)
        {
            need_reload = 0;
            reload_users(users);
        }

        client_len = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           &client_len);
        if (client_fd < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            log_error("accept failed");
            continue;
        }

        log_info("client connected: %s", inet_ntoa(client_addr.sin_addr));

        pid = fork();
        if (pid < 0)
        {
            log_error("fork failed");
            close(client_fd);
            continue;
        }

        if (pid == 0)
        {
            close(server_fd);
            handle_client(client_fd, users);
            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }

    log_info("server stopping, waiting worker processes");

    while (waitpid(-1, NULL, 0) > 0)
    {
    }

    close(server_fd);
    log_info("server stopped");

    return 0;
}

static int run_dgram_server(int port, struct user_list *users)
{
    int server_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    char request_buffer[REQUEST_SIZE];
    char response_buffer[RESPONSE_SIZE];

    if (setup_signals() != 0)
    {
        log_error("failed to setup signals");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0)
    {
        log_error("dgram socket failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        log_error("dgram bind failed");
        close(server_fd);
        return 1;
    }

    log_info("dgram server pid %d listening on port %d", getpid(), port);

    while (!need_stop)
    {
        struct rpc_request request;
        char command_result[RESULT_SIZE];
        int command_code;
        ssize_t bytes_read;

        if (need_reload)
        {
            need_reload = 0;
            reload_users(users);
        }

        memset(request_buffer, 0, sizeof(request_buffer));
        memset(response_buffer, 0, sizeof(response_buffer));
        memset(command_result, 0, sizeof(command_result));

        client_len = sizeof(client_addr);

        bytes_read = recvfrom(server_fd, request_buffer,
                              sizeof(request_buffer) - 1, 0,
                              (struct sockaddr *)&client_addr, &client_len);

        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            log_error("recvfrom failed");
            continue;
        }

        request_buffer[bytes_read] = '\0';

        if (parse_request(request_buffer, &request) != 0)
        {
            build_response(response_buffer, sizeof(response_buffer), 1,
                           "invalid request format");
            sendto(server_fd, response_buffer, strlen(response_buffer), 0,
                   (struct sockaddr *)&client_addr, client_len);
            continue;
        }

        log_info("dgram request from user=%s command=%s",
                 request.login, request.command);

        if (!is_user_allowed(users, request.login))
        {
            build_response(response_buffer, sizeof(response_buffer), 1,
                           "user is not allowed");
            sendto(server_fd, response_buffer, strlen(response_buffer), 0,
                   (struct sockaddr *)&client_addr, client_len);
            continue;
        }

        command_code = execute_command(request.command, command_result,
                                       sizeof(command_result));

        build_response(response_buffer, sizeof(response_buffer), command_code,
                       command_result);

        sendto(server_fd, response_buffer, strlen(response_buffer), 0,
               (struct sockaddr *)&client_addr, client_len);
    }

    close(server_fd);
    log_info("dgram server stopped");

    return 0;
}

int main(void)
{
    struct server_config config;
    struct user_list users;

    if (load_server_config(DEFAULT_CONFIG_PATH, &config) != 0)
    {
        fprintf(stderr, "Failed to load server config\n");
        return 1;
    }

    log_init(config.log_file);

    if (load_user_list(DEFAULT_USERS_PATH, &users) != 0)
    {
        log_error("failed to load users whitelist");
        log_close();
        return 1;
    }

    if (config.daemon_mode)
    {
        log_info("daemon mode enabled");

        if (daemonize_process() != 0)
        {
            log_error("failed to daemonize process");
            log_close();
            return 1;
        }
    }

    if (config.socket_type == SOCKET_TYPE_STREAM)
    {
        run_stream_server(config.port, &users);
    }
    else if (config.socket_type == SOCKET_TYPE_DGRAM)
    {
        run_dgram_server(config.port, &users);
    }
    else
    {
        log_error("unknown socket type");
        log_close();
        return 1;
    }

    run_stream_server(config.port, &users);

    log_close();
    return 0;
}
