#ifndef MYRPC_USERS_H
#define MYRPC_USERS_H
#define MAX_USERS 128
#define USERNAME_SIZE 64

struct user_list
{
    char names[MAX_USERS][USERNAME_SIZE];
    int count;
};

int load_user_list(const char *path, struct user_list *users);
int is_user_allowed(const struct user_list *users, const char *username);
void print_user_list(const struct user_list *users);

#endif
