#ifndef MYRPC_PROTOCOL_H
#define MYRPC_PROTOCOL_H
#define REQUEST_SIZE 1024
#define RESPONSE_SIZE 4096
#define LOGIN_SIZE 64
#define COMMAND_SIZE 768
#define RESULT_SIZE 3840

struct rpc_request
{
    char login[LOGIN_SIZE];
    char command[COMMAND_SIZE];
};

struct rpc_response
{
    int code;
    char result[RESULT_SIZE];
};

int build_request(char *buffer, int size, const char *login,
                  const char *command);
int parse_request(const char *buffer, struct rpc_request *request);

int build_response(char *buffer, int size, int code, const char *result);
int parse_response(const char *buffer, struct rpc_response *response);

#endif
