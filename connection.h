#ifndef CONNECTION_H
#define CONNECTION_H
#define _GNU_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>


#define MAX_BUFFER_SIZE 1024

struct connection {
    int socket;
    SSL *ssl;
    SSL_CTX *ctx;
};

struct connection create_connection(const char *hostname, bool tls);
void close_connection(struct connection *connection);

int conn_read(struct connection *connection, void *buffer, int buffer_size);
int conn_write(struct connection *connection, const void *buffer, int buffer_size);
char* conn_readline(struct connection *connection);
#endif