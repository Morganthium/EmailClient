#include "imap.h"
#include "strutil.h"
#include <ctype.h>

void imap_result_free(struct imap_result *result) {
  if (result && result->responses) {
    for (int i = 0; i < result->n; i++) {
      free(result->responses[i]);
    }
    free(result->responses);
  }
  free(result);
}

struct imap_result *imap_command(struct connection *connection,
                                 const char *command, const char *tag) {
  size_t len = strlen(command);
  if (conn_write(connection, command, len) <= 0) {
    return NULL;
  }
  struct imap_result *result = malloc(sizeof(struct imap_result));
  char *require, *require_ok;
  asprintf(&require, "%s ", tag);
  asprintf(&require_ok, "%s OK ", tag);
  result->n = 0;
  result->responses = NULL;

  while (1) {
    char *newline = conn_readline(connection);
    if (!newline) {
      free(require);
      free(require_ok);
      imap_result_free(result);
      return NULL;
    }

    if (startswith(newline, require_ok)) {
      free(require);
      free(require_ok);
      free(newline);
      return result;
    } else if (startswith(newline, require)) {
      free(require);
      free(require_ok);
      free(newline);
      imap_result_free(result);
      return NULL;
    }

    if (strfind(newline, "BODY[") && strfind(newline, "FETCH")) {
      char *body = strfind(newline, "BODY[");
      char *size = strfind(body, "{");
      int readsize = atoi(size + 1);
      result->responses =
          realloc(result->responses, sizeof(char *) * (result->n + 1));
      result->responses[result->n] = malloc(readsize + 1);
      if (conn_read(connection, result->responses[result->n], readsize) !=
          readsize) {
        free(require);
        free(require_ok);
        free(newline);
        imap_result_free(result);
        return NULL;
      }
      result->responses[result->n][readsize] = '\0';
      result->n++;
    } else {
    }
    free(newline);
  }
  return result;
}