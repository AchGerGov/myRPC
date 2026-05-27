#include "../common/protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void
print_help(const char *prog)
{
  printf("Usage: %s -h HOST -p PORT (-s|-d) -c COMMAND\n", prog);
  printf("  -c, --command CMD   bash command\n");
  printf("  -h, --host HOST     server address\n");
  printf("  -p, --port PORT     server port\n");
  printf("  -s, --stream        TCP stream socket\n");
  printf("  -d, --dgram         UDP datagram socket\n");
  printf("      --help          show help\n");
}

static const char *
current_login(void)
{
  struct passwd *pw;

  pw = getpwuid(getuid());
  if (pw == NULL)
    return "unknown";

  return pw->pw_name;
}

static int
make_addr(const char *host, const char *port, int socktype,
          struct addrinfo **result)
{
  struct addrinfo hints;

  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = socktype;

  return getaddrinfo(host, port, &hints, result);
}

int
main(int argc, char **argv)
{
  const char *command = NULL;
  const char *host = NULL;
  const char *port = NULL;
  int socktype = 0;
  int opt;
  int option_index = 0;
  int fd;
  int code;
  ssize_t nread;
  struct addrinfo *ai = NULL;
  char request[MYRPC_MAX_MESSAGE];
  char response[MYRPC_MAX_MESSAGE * 3];
  char result[MYRPC_MAX_RESULT];

  static struct option long_options[] = {
    {"command", required_argument, 0, 'c'},
    {"host", required_argument, 0, 'h'},
    {"port", required_argument, 0, 'p'},
    {"stream", no_argument, 0, 's'},
    {"dgram", no_argument, 0, 'd'},
    {"help", no_argument, 0, 1000},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "c:h:p:sd", long_options,
                            &option_index)) != -1)
    {
      switch (opt)
        {
        case 'c':
          command = optarg;
          break;
        case 'h':
          host = optarg;
          break;
        case 'p':
          port = optarg;
          break;
        case 's':
          socktype = SOCK_STREAM;
          break;
        case 'd':
          socktype = SOCK_DGRAM;
          break;
        case 1000:
          print_help(argv[0]);
          return EXIT_SUCCESS;
        default:
          print_help(argv[0]);
          return EXIT_FAILURE;
        }
    }

  if (command == NULL || host == NULL || port == NULL || socktype == 0)
    {
      print_help(argv[0]);
      return EXIT_FAILURE;
    }

  if (make_addr(host, port, socktype, &ai) != 0)
    {
      perror("getaddrinfo");
      return EXIT_FAILURE;
    }

  fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (fd < 0)
    {
      perror("socket");
      freeaddrinfo(ai);
      return EXIT_FAILURE;
    }

  if (myrpc_build_request(current_login(), command, request, sizeof (request)) != 0)
    {
      fprintf(stderr, "request is too long\n");
      close(fd);
      freeaddrinfo(ai);
      return EXIT_FAILURE;
    }

  if (socktype == SOCK_STREAM)
    {
      if (connect(fd, ai->ai_addr, ai->ai_addrlen) != 0)
        {
          perror("connect");
          close(fd);
          freeaddrinfo(ai);
          return EXIT_FAILURE;
        }
      if (write(fd, request, strlen(request)) < 0)
        {
          perror("write");
          close(fd);
          freeaddrinfo(ai);
          return EXIT_FAILURE;
        }
      shutdown(fd, SHUT_WR);
      nread = read(fd, response, sizeof (response) - 1);
    }
  else
    {
      if (sendto(fd, request, strlen(request), 0, ai->ai_addr, ai->ai_addrlen) < 0)
        {
          perror("sendto");
          close(fd);
          freeaddrinfo(ai);
          return EXIT_FAILURE;
        }
      nread = recvfrom(fd, response, sizeof (response) - 1, 0, NULL, NULL);
    }

  if (nread < 0)
    {
      perror("recv/read");
      close(fd);
      freeaddrinfo(ai);
      return EXIT_FAILURE;
    }

  response[nread] = '\0';

  if (myrpc_parse_response(response, &code, result, sizeof (result)) != 0)
    {
      fprintf(stderr, "bad server response: %s\n", response);
      close(fd);
      freeaddrinfo(ai);
      return EXIT_FAILURE;
    }

  printf("code=%d\n%s\n", code, result);

  close(fd);
  freeaddrinfo(ai);
  return code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
