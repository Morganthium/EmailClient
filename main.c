#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"
#include "imap.h"
#include "parse_command.h"
#include "strutil.h"
#include <stdbool.h>

bool login(const char *username, const char *password, struct connection *c,
           const char *folder) {
  char *str = NULL;
  asprintf(&str, "LOGIN LOGIN \"%s\" \"%s\"\r\n", username, password);
  struct imap_result *result = imap_command(c, str, "LOGIN");
  if (!result) {
    printf("Login failure\n");
    return false;
  }
  free(str);
  imap_result_free(result);
  asprintf(&str, "SELECT SELECT \"%s\"\r\n", folder);
  result = imap_command(c, str, "SELECT");
  if (!result) {
    printf("Folder not found\n");
    return false;
  }
  free(str);
  imap_result_free(result);
  return true;
}

void retrieve(const char *msgid, struct connection *c) {
  char *str = NULL;
  asprintf(&str, "FETCH FETCH %s BODY[]\r\n", msgid);
  struct imap_result *result = imap_command(c, str, "FETCH");
  free(str);
  if (!result || result->n == 0) {
    printf("Message not found\n");
    imap_result_free(result);
    exit(3);
  }
  printf("%s", result->responses[0]);
  imap_result_free(result);
}

void print_field(const char *header, const char *ifempty, const char *data) {
  printf("%s", header);
  if (data[0] == '\r') {
    printf("%s\n", ifempty);
  } else {
    char *r = strstr(data, " ");
    char *s = strgetline_space(&r);
    if (s) {
      printf("%s", s);
      free(s);
    }
    printf("\n");
  }
}

void parse(const char *msgid, struct connection *c) {
  char *str = NULL;
  asprintf(&str, "FROM FETCH %s BODY[HEADER.FIELDS (FROM)]\r\n", msgid);
  struct imap_result *result_from = imap_command(c, str, "FROM");
  free(str);

  asprintf(&str, "TO FETCH %s BODY[HEADER.FIELDS (TO)]\r\n", msgid);
  struct imap_result *result_to = imap_command(c, str, "TO");
  free(str);

  asprintf(&str, "DATE FETCH %s BODY[HEADER.FIELDS (DATE)]\r\n", msgid);
  struct imap_result *result_date = imap_command(c, str, "DATE");
  free(str);

  asprintf(&str, "SUBJECT FETCH %s BODY[HEADER.FIELDS (SUBJECT)]\r\n", msgid);
  struct imap_result *result_subject = imap_command(c, str, "SUBJECT");
  free(str);

  bool ok = false;
  if (result_from && result_to && result_date && result_subject) {
    if (result_from->n > 0 && result_date->n > 0) {
      ok = true;
      print_field("From:", "", result_from->responses[0]);
      print_field("To:", "", result_to->n > 0 ? result_to->responses[0] : "\r");
      print_field("Date:", "", result_date->responses[0]);
      print_field("Subject:", " <No subject>",
                  result_subject->n > 0 ? result_subject->responses[0] : "\r");
    }
  }

  imap_result_free(result_from);
  imap_result_free(result_to);
  imap_result_free(result_date);
  imap_result_free(result_subject);
  if (!ok) {
    printf("Message not found\n");
    exit(3);
  }
}

void list(struct connection *c) {
  char *str = NULL;
  asprintf(&str, "FETCH FETCH 1:* BODY[HEADER.FIELDS (SUBJECT)]\r\n");
  struct imap_result *result = imap_command(c, str, "FETCH");
  free(str);
  if (!result || result->n == 0) {
    printf("Message not found\n");
    imap_result_free(result);
    exit(3);
  }
  for (int i = 0; i < result->n; i++) {
    printf("%d:", i + 1);
    if (result->responses[i][0] == '\r') {
      printf(" <No subject>\n");
    } else {
      char *r = strstr(result->responses[i], " ");
      char *s = strgetline_space(&r);
      if (!s) {
        printf("\n");
      } else {
        printf("%s\n", s);
        free(s);
      }
    }
  }
  imap_result_free(result);
}

void test_mime(char *block) {
  char *line1 = strgetline_space(&block);
  char *line2 = strgetline_space(&block);
  if (strfind(line1, "Content-Type:")) {
    char *tmp = line1;
    line1 = line2;
    line2 = tmp;
  }
  if (strfind(line1, "Content-Transfer-Encoding:") &&
      (strfind(line1, "quoted-printable") || strfind(line1, "7bit") ||
       strfind(line1, "8bit"))) {
    if (strfind(line2, "Content-Type:") && strfind(line2, "text/plain") &&
        strfind(line2, "utf-8")) {
      char *q = strgetline_space(&block);
      free(q);
      printf("%s", block);
    }
  }

  free(line1);
  free(line2);
}
void output_mime(char *data, char *open, char *close) {
  char **starts = NULL;
  int size = 0;
  while (data) {
    char *s = strfind(data, open);
    if (s) {
      starts = realloc(starts, sizeof(char *) * (size + 1));
      starts[size] = s;
      size++;
      data = s + 1;
    } else {
      char *s = strfind(data, close);
      if (s) {
        starts = realloc(starts, sizeof(char *) * (size + 1));
        starts[size] = s;
        size++;
      }
      break;
    }
  }
  for (int i = 0; i < size - 1; ++i) {
    char *block = strslice(starts[i], 0, starts[i + 1] - starts[i]);
    test_mime(block + strlen(open));
    free(block);
  }
}

void extract_mime(char *data) {
  char *mimestart = strfind(data, "MIME-Version: 1.0");
  if (!mimestart) {
    return;
  }
  char *mimeline = strgetline_space(&mimestart);
  free(mimeline);
  char *contentline = strgetline_space(&mimestart);
  char *boundary = strfind(contentline, "boundary=");
  if (!boundary) {
    free(contentline);
    return;
  }
  boundary += 9;
  char *bstr = NULL;
  if (*boundary == '"') {
    boundary++;
    char *end = strfind(boundary, "\"");
    bstr = strslice(boundary, 0, end - boundary);
  } else {
    bstr = strdup(boundary);
  }
  free(contentline);
  char *open, *close;
  asprintf(&open, "\r\n--%s\r\n", bstr);
  asprintf(&close, "\r\n--%s--\r\n", bstr);
  free(bstr);
  output_mime(mimestart, open, close);
  free(open);
  free(close);
}

void mime(const char *msgid, struct connection *c) {
  char *str = NULL;
  asprintf(&str, "FETCH FETCH %s BODY[]\r\n", msgid);
  struct imap_result *result = imap_command(c, str, "FETCH");
  free(str);
  if (!result || result->n == 0) {
    printf("Message not found\n");
    imap_result_free(result);
    exit(3);
  }
  extract_mime(result->responses[0]);
  imap_result_free(result);
}

int main(int argc, char **argv) {
  struct command cmd = parse_command(argc, argv);
  struct connection conn = create_connection(cmd.hostname, cmd.tls);
  if (!login(cmd.username, cmd.password, &conn, cmd.folder)) {
    close_connection(&conn);
    return 3;
  }
  switch (cmd.command[0]) {
  case 'r':
    retrieve(cmd.message, &conn);
    break;
  case 'l':
    list(&conn);
    break;
  case 'p':
    parse(cmd.message, &conn);
    break;
  case 'm':
    mime(cmd.message, &conn);
    break;
  }
  close_connection(&conn);
}
