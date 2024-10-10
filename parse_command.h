#ifndef PARSE_COMMAND_H
#define PARSE_COMMAND_H
#include <stdbool.h>
#include <string.h>
struct command {
    char *username;
    char *password;
    char *hostname;
    char *command;
    char *message;
    char *folder;
    bool tls;
};

struct command parse_command(int argc, char **argv);

#endif