#ifndef IMAP_H
#define IMAP_H


#include "connection.h"

struct imap_result {
    char **responses;
    int n;
};

struct imap_result* imap_command(struct connection *connection, const char *command, const char *tag);
void imap_result_free(struct imap_result *result);

#endif