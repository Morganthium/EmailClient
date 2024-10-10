#ifndef STRUTIL_H
#define STRUTIL_H

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

bool startswith(const char *str1, const char *str2);
char *strfind(char *str1, const char *str2);
char *strslice(const char *str1, int start, int end);
char *strgetline(char **str1);
char *strgetline_space(char **str1);
#endif