#include "users.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#define LINE_SIZE 256

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

int load_user_list(const char *path, struct user_list *users)
{
    FILE *file;
    char line[LINE_SIZE];

    users->count = 0;

    file = fopen(path, "r");
    if (file == NULL)
    {
        perror("fopen");
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *name;

        name = trim_spaces(line);

        if (name[0] == '#' || name[0] == '\0')
        {
            continue;
        }

        if (users->count >= MAX_USERS)
        {
            fprintf(stderr, "Too many users in whitelist\n");
            fclose(file);
            return 1;
        }

        snprintf(users->names[users->count],
                 sizeof(users->names[users->count]), "%s", name);
        users->count++;
    }

    fclose(file);
    return 0;
}

int is_user_allowed(const struct user_list *users, const char *username)
{
    int i;

    for (i = 0; i < users->count; i++)
    {
        if (strcmp(users->names[i], username) == 0)
        {
            return 1;
        }
    }

    return 0;
}

void print_user_list(const struct user_list *users)
{
    int i;

    printf("allowed users:\n");

    for (i = 0; i < users->count; i++)
    {
        printf("  %s\n", users->names[i]);
    }
}
