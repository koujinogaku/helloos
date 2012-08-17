#include "string.h"

int
strcmp( void *dest, const void *src)
{
  const char *d = (char*)dest;
  const char *s = (const char*)src;

  for(;; d++,s++ ) {
    if(*d != *s)
      break;
    if(*d==0 || *s==0)
      break;
  }

  if( *d > *s )
    return 1;
  else if( *d < *s )
    return -1;
  else
    return 0;
}
