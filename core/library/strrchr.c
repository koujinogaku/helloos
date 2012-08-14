#include "string.h"

char *strrchr(char *str, char ch)
{
  int len = strlen(str);

  for(;len>0;) {
    --len;
    if(str[len]==ch)
      return &(str[len]);
  }
  return 0;
}
