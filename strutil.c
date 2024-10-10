#include "strutil.h"

bool startswith(const char *str1, const char *str2) {
  int i = 0;
  while (str2[i] != '\0') {
    if (tolower(str1[i]) != tolower(str2[i])) {
      return false;
    }
    i++;
  }
  return true;
}

char *strfind(char *str1, const char *str2) {
  for (int i = 0; str1[i] != '\0'; i++) {
    if (startswith(&str1[i], str2)) {
      return &str1[i];
    }
  }
  return NULL;
}

char *strslice(const char *str1, int start, int end) {
  char *newstr = malloc(end - start + 1);
  memcpy(newstr, &str1[start], end - start);
  newstr[end - start] = '\0';
  return newstr;
}

char *strgetline(char **str1) {
  char *n = strfind(*str1, "\r\n");
  if (n) {
    char *newstr = strslice(*str1, 0, n - *str1);
    *str1 = n + 2;
    return newstr;
  }
  return NULL;
}

char *strgetline_space(char **str1) {
  char *s = strgetline(str1);
  if (!s) {
    return NULL;
  }
  while (**str1 == '\t' || **str1 == ' ') {
    char *t = strgetline(str1);
    s = realloc(s, strlen(s) + strlen(t) + 1);
    strcat(s, t);
    free(t);
  }
  return s;
}