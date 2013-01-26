#include "string.h"

char *strrchr(const char *str, char ch)
{
  int len = strlen(str);

  for(;len>0;) {
    --len;
    if(str[len]==ch)
      return (char *)(&(str[len]));
  }
  return 0;
}
