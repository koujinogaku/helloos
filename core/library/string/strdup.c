#include "string.h"
#include "memory.h"

char *strdup(const char *src)
{
  char *d,*dst;

  dst = d = malloc(strlen(src)+1);
  if(d==0)
    return d;

  while(1) {
    *dst = *src;
    if(*src==0)
      break;
    src++;
    dst++;
  }

  return dst;
}
