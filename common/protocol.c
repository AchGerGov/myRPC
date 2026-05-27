#include "protocol.h"

#include <stdio.h>
#include <string.h>

int
myrpc_escape(const char *src, char *dst, size_t dst_size)
{
  size_t i;
  size_t j;

  if (src == NULL || dst == NULL || dst_size == 0)
    return -1;

  for (i = 0, j = 0; src[i] != '\0'; i++)
    {
      if (src[i] == '\\' || src[i] == '"' || src[i] == '\n' || src[i] == ':')
        {
          if (j + 2 >= dst_size)
            return -1;
          dst[j++] = '\\';
          if (src[i] == '\n')
            dst[j++] = 'n';
          else
            dst[j++] = src[i];
        }
      else
        {
          if (j + 1 >= dst_size)
            return -1;
          dst[j++] = src[i];
        }
    }

  dst[j] = '\0';
  return 0;
}

int
myrpc_unescape(const char *src, char *dst, size_t dst_size)
{
  size_t i;
  size_t j;

  if (src == NULL || dst == NULL || dst_size == 0)
    return -1;

  for (i = 0, j = 0; src[i] != '\0'; i++)
    {
      if (src[i] == '\\' && src[i + 1] != '\0')
        {
          i++;
          if (j + 1 >= dst_size)
            return -1;
          dst[j++] = src[i] == 'n' ? '\n' : src[i];
        }
      else
        {
          if (j + 1 >= dst_size)
            return -1;
          dst[j++] = src[i];
        }
    }

  dst[j] = '\0';
  return 0;
}

int
myrpc_build_request(const char *login, const char *command, char *out, size_t out_size)
{
  char escaped[MYRPC_MAX_COMMAND * 2];

  if (myrpc_escape(command, escaped, sizeof (escaped)) != 0)
    return -1;

  if (snprintf(out, out_size, "%s: %s", login, escaped) >= (int) out_size)
    return -1;

  return 0;
}

int
myrpc_parse_request(const char *request, char *login, size_t login_size,
                    char *command, size_t command_size)
{
  const char *sep;
  size_t login_len;

  sep = strchr(request, ':');
  if (sep == NULL)
    return -1;

  login_len = (size_t) (sep - request);
  if (login_len == 0 || login_len >= login_size)
    return -1;

  memcpy(login, request, login_len);
  login[login_len] = '\0';

  sep++;
  while (*sep == ' ')
    sep++;

  return myrpc_unescape(sep, command, command_size);
}

int
myrpc_build_response(int code, const char *result, char *out, size_t out_size)
{
  char escaped[MYRPC_MAX_RESULT * 2];

  if (myrpc_escape(result == NULL ? "" : result, escaped, sizeof (escaped)) != 0)
    return -1;

  if (snprintf(out, out_size, "%d: %s", code, escaped) >= (int) out_size)
    return -1;

  return 0;
}

int
myrpc_parse_response(const char *response, int *code, char *result, size_t result_size)
{
  const char *sep;

  sep = strchr(response, ':');
  if (sep == NULL)
    return -1;

  if (sscanf(response, "%d", code) != 1)
    return -1;

  sep++;
  while (*sep == ' ')
    sep++;

  return myrpc_unescape(sep, result, result_size);
}
