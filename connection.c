#include "connection.h"

static struct addrinfo *host2addr(const char *hostname, int family) {
  struct addrinfo *result = NULL;
  struct addrinfo hints = {
      .ai_family = family,
      .ai_socktype = SOCK_STREAM,
      .ai_protocol = IPPROTO_TCP,
  };
  if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
    return NULL;
  }
  return result;
}

static bool connect_ipv6(struct connection *connection, const char *hostname,
                         int port) {
  int s = socket(AF_INET6, SOCK_STREAM, 0);
  if (s == -1) {
    perror("ipv6: socket");
    return false;
  }
  struct addrinfo *result = host2addr(hostname, AF_INET6);
  if (!result) {
    perror("ipv6: get_address_info");
    close(s);
    return false;
  }

  struct sockaddr_in6 *addr = (struct sockaddr_in6 *)result->ai_addr;
  addr->sin6_port = htons(port);
  addr->sin6_family = AF_INET6;
  if (connect(s, result->ai_addr, result->ai_addrlen) == -1) {
    perror("ipv6: connect");
    close(s);
    freeaddrinfo(result);
    return false;
  }
  freeaddrinfo(result);
  connection->socket = s;
  return true;
}

static bool connect_ipv4(struct connection *connection, const char *hostname,
                         int port) {
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("ipv4: socket");
    return false;
  }
  struct addrinfo *result = host2addr(hostname, AF_INET);
  if (!result) {
    perror("ipv4: get_address_info");
    close(s);
    return false;
  }

  struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
  addr->sin_port = htons(port);
  addr->sin_family = AF_INET;
  if (connect(s, result->ai_addr, result->ai_addrlen) == -1) {
    freeaddrinfo(result);
    perror("ipv4: connect");
    close(s);
    return false;
  }
  freeaddrinfo(result);
  connection->socket = s;
  return true;
}

static bool connect_tls(struct connection *c) {
  SSL_library_init();
  c->ctx = SSL_CTX_new(TLS_client_method());
  if (c->ctx == NULL) {
    perror("init_tls: SSL_CTX_new");
    return false;
  }
  c->ssl = SSL_new(c->ctx);
  if (!c->ssl) {
    perror("init_tls: SSL_new");
    close(c->socket);
    SSL_CTX_free(c->ctx);
    return false;
  }
  if (SSL_set_fd(c->ssl, c->socket) == 0) {
    perror("init_tls: SSL_set_fd");
    close(c->socket);
    SSL_CTX_free(c->ctx);
    SSL_free(c->ssl);
    return false;
  }
  if (SSL_connect(c->ssl) == 0) {
    perror("init_tls: SSL_connect");
    close(c->socket);
    SSL_CTX_free(c->ctx);
    SSL_free(c->ssl);
    return false;
  }
  return true;
}

struct connection create_connection(const char *hostname, bool tls) {
  int port = tls ? 993 : 143;
  struct connection c = {0};
  if (!connect_ipv6(&c, hostname, port)) {
    if (!connect_ipv4(&c, hostname, port)) {
      exit(1);
    }
  }
  if (tls && !connect_tls(&c)) {
    exit(2);
  }
  return c;
}

void close_connection(struct connection *connection) {
  close(connection->socket);
  if (connection->ssl) {
    SSL_shutdown(connection->ssl);
    SSL_free(connection->ssl);
  }
  if (connection->ctx) {
    SSL_CTX_free(connection->ctx);
  }
}

int conn_read(struct connection *connection, void *buffer, int buffer_size) {
  int read_bytes = 0;

  while (read_bytes < buffer_size) {
    int t = 0;
    if (connection->ssl) {
      t = SSL_read(connection->ssl, buffer + read_bytes,
                   buffer_size - read_bytes);
    } else {
      t = read(connection->socket, buffer + read_bytes,
               buffer_size - read_bytes);
    }
    if (t <= 0) {
      return t;
    }
    read_bytes += t;
  }
  return read_bytes;
}
int conn_write(struct connection *connection, const void *buffer,
               int buffer_size) {
  if (connection->ssl) {
    return SSL_write(connection->ssl, buffer, buffer_size);
  } else {
    return write(connection->socket, buffer, buffer_size);
  }
}

char *conn_readline(struct connection *connection) {
  char *buffer = malloc(1000);
  int size = 1000;
  char *p = buffer;
  while (1) {
    if (p - buffer >= size) {
      size_t old = size;
      size *= 2;
      buffer = realloc(buffer, size);
      p = buffer + old;
    }

    if (conn_read(connection, p, 1) <= 0) {
      free(buffer);
      return NULL;
    } else if (*p == '\n' && p > buffer && p[-1] == '\r') {
      *p = '\0';
      return buffer;
    } else {
      p++;
    }
  }
  return buffer;
}