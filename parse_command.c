#include "parse_command.h"
#include <stdlib.h>

struct command parse_command(int argc, char **argv) {
  char *username = NULL;
  char *password = NULL;
  char *hostname = NULL;
  char *command = NULL;
  char *message = "*";
  char *folder = "INBOX";
  bool tls = false;

  for (int i = 1; i < argc; i++) {
    char *option = argv[i];
    char *arg = i + 1 != argc ? argv[i + 1] : NULL;
    if (!strcmp(option, "-u") && arg) {
      username = arg;
      ++i;
    } else if (!strcmp(option, "-p") && arg) {
      password = arg;
      ++i;
    } else if (!strcmp(option, "-f") && arg) {
      folder = arg;
      ++i;
    } else if (!strcmp(option, "-t")) {
      tls = true;
    } else if (!strcmp(option, "-n") && arg) {
      message = arg;
      ++i;
      if (!strcmp(arg, "*")) {
        continue;
      }
      for (int j = 0; message[j]; ++j) {
        if (message[j] < '0' || message[j] > '9') {
          exit(1);
        }
      }
    } else if (option[0] == '-') {
      exit(1);
    } else if (command == NULL) {
      command = option;
    } else if (hostname == NULL) {
      hostname = option;
    } else {
      exit(1);
    }
  }
  if (!command || !hostname || !username || !password) {
    exit(1);
  }
  if (strcmp(command, "retrieve") && strcmp(command, "list") &&
      strcmp(command, "parse") && strcmp(command, "mime")) {
    exit(1);
  }
  struct command cmd = {
      .command = command,
      .message = message,
      .folder = folder,
      .tls = tls,
      .username = username,
      .password = password,
      .hostname = hostname,
  };
  return cmd;
}