#ifndef MYRPC_PROTOCOL_H
#define MYRPC_PROTOCOL_H

#include <stddef.h>

#define MYRPC_MAX_LOGIN 64
#define MYRPC_MAX_COMMAND 2048
#define MYRPC_MAX_MESSAGE 4096
#define MYRPC_MAX_RESULT 8192

int myrpc_escape(const char *src, char *dst, size_t dst_size);
int myrpc_unescape(const char *src, char *dst, size_t dst_size);
int myrpc_build_request(const char *login, const char *command, char *out, size_t out_size);
int myrpc_parse_request(const char *request, char *login, size_t login_size,
                        char *command, size_t command_size);
int myrpc_build_response(int code, const char *result, char *out, size_t out_size);
int myrpc_parse_response(const char *response, int *code, char *result, size_t result_size);

#endif
