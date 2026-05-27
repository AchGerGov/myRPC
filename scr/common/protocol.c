#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int build_request(char *buffer, int size, const char *login,
                  const char *command)
{
    int written;

    written = snprintf(buffer, size, "%s:%s", login, command);

    if (written < 0 || written >= size)
    {
        return 1;
    }

    return 0;
}

int parse_request(const char *buffer, struct rpc_request *request)
{
    const char *separator;
    size_t login_len;
    size_t command_len;

    separator = strchr(buffer, ':');
    if (separator == NULL)
    {
        return 1;
    }

    login_len = (size_t)(separator - buffer);
    command_len = strlen(separator + 1);

    if (login_len == 0 || login_len >= LOGIN_SIZE)
    {
        return 1;
    }

    if (command_len == 0 || command_len >= COMMAND_SIZE)
    {
        return 1;
    }

    memcpy(request->login, buffer, login_len);
    request->login[login_len] = '\0';

    memcpy(request->command, separator + 1, command_len);
    request->command[command_len] = '\0';

    return 0;
}

int build_response(char *buffer, int size, int code, const char *result)
{
    int written;

    written = snprintf(buffer, size, "%d:%s", code, result);

    if (written < 0 || written >= size)
    {
        return 1;
    }

    return 0;
}

int parse_response(const char *buffer, struct rpc_response *response)
{
    const char *separator;
    size_t result_len;

    separator = strchr(buffer, ':');
    if (separator == NULL)
    {
        return 1;
    }

    response->code = atoi(buffer);

    result_len = strlen(separator + 1);
    if (result_len >= RESULT_SIZE)
    {
        return 1;
    }

    memcpy(response->result, separator + 1, result_len);
    response->result[result_len] = '\0';

    return 0;
}
