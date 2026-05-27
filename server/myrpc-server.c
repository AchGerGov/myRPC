#include "../common/protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT_CONFIG "/etc/myRPC/myRPC.conf"
#define DEFAULT_USERS "/etc/myRPC/users.conf"
#define DEFAULT_LOG NULL

struct config
{
  int port;
  int socktype;
  int daemon_mode;
  char log_file[256];
};

static volatile sig_atomic_t stop_flag = 0;
static volatile sig_atomic_t reload_flag = 0;
static FILE *log_fp = NULL;
static struct config cfg;

static void
log_msg(int priority, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if (log_fp != NULL)
    {
      vfprintf(log_fp, fmt, ap);
      fprintf(log_fp, "\n");
      fflush(log_fp);
    }
  else
    vsyslog(priority, fmt, ap);
  va_end(ap);
}

static char *
trim(char *s)
{
  char *end;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
    s++;
  end = s + strlen(s);
  while (end > s && (end[-1] == ' ' || end[-1] == '\t'
                     || end[-1] == '\n' || end[-1] == '\r'))
    *--end = '\0';
  return s;
}

static int
load_config(const char *path, struct config *out)
{
  FILE *fp;
  char line[256];

  out->port = 1234;
  out->socktype = SOCK_STREAM;
  out->daemon_mode = 0;
  out->log_file[0] = '\0';

  fp = fopen(path, "r");
  if (fp == NULL)
    return -1;

  while (fgets(line, sizeof (line), fp) != NULL)
    {
      char *eq;
      char *key;
      char *value;

      key = trim(line);
      if (key[0] == '#' || key[0] == '\0')
        continue;

      eq = strchr(key, '=');
      if (eq == NULL)
        continue;

      *eq = '\0';
      value = trim(eq + 1);
      key = trim(key);

      if (strcmp(key, "port") == 0)
        out->port = atoi(value);
      else if (strcmp(key, "socket_type") == 0)
        out->socktype = strcmp(value, "dgram") == 0 ? SOCK_DGRAM : SOCK_STREAM;
      else if (strcmp(key, "daemon") == 0)
        out->daemon_mode = strcmp(value, "yes") == 0 || strcmp(value, "1") == 0;
      else if (strcmp(key, "log_file") == 0)
        snprintf(out->log_file, sizeof (out->log_file), "%s", value);
    }

  fclose(fp);
  return 0;
}

static int
user_allowed(const char *login)
{
  FILE *fp;
  char line[MYRPC_MAX_LOGIN + 8];

  fp = fopen(DEFAULT_USERS, "r");
  if (fp == NULL)
    return 0;

  while (fgets(line, sizeof (line), fp) != NULL)
    {
      char *name = trim(line);
      if (name[0] != '#' && strcmp(name, login) == 0)
        {
          fclose(fp);
          return 1;
        }
    }

  fclose(fp);
  return 0;
}

static void
read_file_to_buf(const char *path, char *buf, size_t size)
{
  FILE *fp;
  size_t n;

  buf[0] = '\0';
  fp = fopen(path, "r");
  if (fp == NULL)
    return;

  n = fread(buf, 1, size - 1, fp);
  buf[n] = '\0';
  fclose(fp);
}

static int
execute_command(const char *command, char *result, size_t result_size)
{
  char out_template[] = "/tmp/myRPC_XXXXXX.stdout";
  char err_template[] = "/tmp/myRPC_XXXXXX.stderr";
  char shell_command[MYRPC_MAX_COMMAND + 256];
  int out_fd;
  int err_fd;
  int status;

  out_fd = mkstemps(out_template, 7);
  err_fd = mkstemps(err_template, 7);
  if (out_fd < 0 || err_fd < 0)
    {
      if (out_fd >= 0)
        close(out_fd);
      if (err_fd >= 0)
        close(err_fd);
      snprintf(result, result_size, "cannot create temp files");
      return 1;
    }

  close(out_fd);
  close(err_fd);
  snprintf(shell_command, sizeof (shell_command), "%s >%s 2>%s", command,
           out_template, err_template);

  status = system(shell_command);
  if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
      read_file_to_buf(err_template, result, result_size);
      if (result[0] == '\0')
        snprintf(result, result_size, "command failed");
      unlink(out_template);
      unlink(err_template);
      return 1;
    }

  read_file_to_buf(out_template, result, result_size);
  unlink(out_template);
  unlink(err_template);
  return 0;
}

static void
handle_request(const char *request, char *response, size_t response_size)
{
  char login[MYRPC_MAX_LOGIN];
  char command[MYRPC_MAX_COMMAND];
  char result[MYRPC_MAX_RESULT];
  int code;

  if (myrpc_parse_request(request, login, sizeof (login), command,
                          sizeof (command)) != 0)
    {
      myrpc_build_response(1, "bad request", response, response_size);
      return;
    }

  log_msg(LOG_INFO, "request from user=%s command=%s", login, command);

  if (!user_allowed(login))
    {
      log_msg(LOG_WARNING, "access denied for user=%s", login);
      myrpc_build_response(1, "user is not allowed", response, response_size);
      return;
    }

  code = execute_command(command, result, sizeof (result));
  myrpc_build_response(code, result, response, response_size);
}

static void
on_signal(int signo)
{
  if (signo == SIGINT || signo == SIGTERM)
    stop_flag = 1;
  else if (signo == SIGHUP)
    reload_flag = 1;
  else if (signo == SIGCHLD)
    while (waitpid(-1, NULL, WNOHANG) > 0)
      ;
}

static void
setup_signals(void)
{
  struct sigaction sa;
  memset(&sa, 0, sizeof (sa));
  sa.sa_handler = on_signal;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGCHLD, &sa, NULL);
}

static int
daemonize(void)
{
  pid_t pid;

  pid = fork();
  if (pid < 0)
    return -1;
  if (pid > 0)
    exit(EXIT_SUCCESS);

  if (setsid() < 0)
    return -1;

  pid = fork();
  if (pid < 0)
    return -1;
  if (pid > 0)
    exit(EXIT_SUCCESS);

  chdir("/");
  umask(0);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  open("/dev/null", O_RDONLY);
  open("/dev/null", O_WRONLY);
  open("/dev/null", O_RDWR);
  return 0;
}

static int
make_socket(void)
{
  int fd;
  int yes = 1;
  struct sockaddr_in addr;

  fd = socket(AF_INET, cfg.socktype, 0);
  if (fd < 0)
    return -1;

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes));

  memset(&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((uint16_t) cfg.port);

  if (bind(fd, (struct sockaddr *) &addr, sizeof (addr)) != 0)
    {
      close(fd);
      return -1;
    }

  if (cfg.socktype == SOCK_STREAM && listen(fd, 16) != 0)
    {
      close(fd);
      return -1;
    }

  return fd;
}

static void
serve_stream(int fd)
{
  int cfd;
  ssize_t nread;
  char request[MYRPC_MAX_MESSAGE];
  char response[MYRPC_MAX_MESSAGE * 3];

  cfd = accept(fd, NULL, NULL);
  if (cfd < 0)
    return;

  if (fork() == 0)
    {
      nread = read(cfd, request, sizeof (request) - 1);
      if (nread > 0)
        {
          request[nread] = '\0';
          handle_request(request, response, sizeof (response));
          write(cfd, response, strlen(response));
        }
      close(cfd);
      _exit(EXIT_SUCCESS);
    }

  close(cfd);
}

static void
serve_dgram(int fd)
{
  struct sockaddr_in peer;
  socklen_t peer_len = sizeof (peer);
  ssize_t nread;
  char request[MYRPC_MAX_MESSAGE];
  char response[MYRPC_MAX_MESSAGE * 3];

  nread = recvfrom(fd, request, sizeof (request) - 1, 0,
                   (struct sockaddr *) &peer, &peer_len);
  if (nread <= 0)
    return;

  request[nread] = '\0';
  handle_request(request, response, sizeof (response));
  sendto(fd, response, strlen(response), 0, (struct sockaddr *) &peer, peer_len);
}

static void
print_help(const char *prog)
{
  printf("Usage: %s [-f] [-c /etc/myRPC/myRPC.conf] [-l /path/log]\n", prog);
  printf("  -f, --foreground     do not daemonize\n");
  printf("  -c, --config FILE    config file\n");
  printf("  -l, --log FILE       log to file instead of syslog\n");
  printf("      --help           show help\n");
}

int
main(int argc, char **argv)
{
  const char *config_path = DEFAULT_CONFIG;
  int foreground = 0;
  int fd;
  int opt;
  int option_index = 0;

  static struct option long_options[] = {
    {"foreground", no_argument, 0, 'f'},
    {"config", required_argument, 0, 'c'},
    {"log", required_argument, 0, 'l'},
    {"help", no_argument, 0, 1000},
    {0, 0, 0, 0}
  };

  openlog("myRPC-server", LOG_PID | LOG_NDELAY, LOG_DAEMON);
  if (load_config(config_path, &cfg) != 0)
    load_config("./server/myRPC.conf", &cfg);

  while ((opt = getopt_long(argc, argv, "fc:l:", long_options,
                            &option_index)) != -1)
    {
      switch (opt)
        {
        case 'f':
          foreground = 1;
          break;
        case 'c':
          config_path = optarg;
          load_config(config_path, &cfg);
          break;
        case 'l':
          snprintf(cfg.log_file, sizeof (cfg.log_file), "%s", optarg);
          break;
        case 1000:
          print_help(argv[0]);
          return EXIT_SUCCESS;
        default:
          print_help(argv[0]);
          return EXIT_FAILURE;
        }
    }

  if (cfg.log_file[0] != '\0')
    log_fp = fopen(cfg.log_file, "a");

  setup_signals();

  if (!foreground && cfg.daemon_mode)
    {
      if (daemonize() != 0)
        return EXIT_FAILURE;
    }

  fd = make_socket();
  if (fd < 0)
    {
      log_msg(LOG_ERR, "cannot create socket on port %d: %s", cfg.port,
              strerror(errno));
      return EXIT_FAILURE;
    }

  log_msg(LOG_INFO, "server started port=%d socket=%s", cfg.port,
          cfg.socktype == SOCK_STREAM ? "stream" : "dgram");

  while (!stop_flag)
    {
      if (reload_flag)
        {
          load_config(config_path, &cfg);
          log_msg(LOG_INFO, "SIGHUP received, config reloaded");
          reload_flag = 0;
        }

      if (cfg.socktype == SOCK_STREAM)
        serve_stream(fd);
      else
        serve_dgram(fd);
    }

  log_msg(LOG_INFO, "server stopped by signal");
  close(fd);
  if (log_fp != NULL)
    fclose(log_fp);
  closelog();
  return EXIT_SUCCESS;
}
