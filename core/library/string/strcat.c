#include "string.h"

char *strcat(char *str1, const char *str2)
{
  char *p = str1 + strlen(str1);

  while(*str2)
    *p++ = *str2++;

  return str1;
}
