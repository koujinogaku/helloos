#include "string.h"
#include "stdlib.h"

void int2dec(unsigned int num, char *str)
{
  int i;
  char s[16];

  for(i=0;i<15;i++) {
    s[i] = (num % 10) + '0';
    num /= 10;
    if(num == 0)
      break;
  }
  for(;i>=0;i--) {
    *str++ = s[i];
  }
  *str=0;
}

void sint2dec(int num, char *str)
{
  if(num<0) {
    *str='-';
    str++;
    num = -num;
  }
  int2dec(num,str);
}

void byte2hex(unsigned char num, char *str)
{
    str[0]=numtohex1((num&0xf0)/16);
    str[1]=numtohex1(num&0x0f);
    str[2]=0;
}

void word2hex(unsigned short num, char *str)
{
    str[0]=numtohex1((num&0xf000)/16/256);
    str[1]=numtohex1((num&0x0f00)/256);
    str[2]=numtohex1((num&0x00f0)/16);
    str[3]=numtohex1( num&0x000f    );
    str[4]=0;
}
void long2hex(unsigned long num, char *str)
{
  word2hex((num&0xffff0000)>>16,str);
  word2hex(num&0x0000ffff,&str[4]);
}

/* coping memory blocks */
void *
memcpy( void *dest, const void *src, unsigned int n )
{
  char *d = (char*)dest;
  const char *s = (const char*)src;

  for( ; n>0 ; n-- )
    *d++ = *s++;

  return dest;
}

void *
memset( void *s, int c, unsigned int n )
{
  char *p = (char*)s;

  for( ; n>0 ; n-- )
    *p++ = (char)c;

  return s;
}

int
memcmp( void *dest, const void *src, unsigned int n )
{
  const char *d = (char*)dest;
  const char *s = (const char*)src;

  for(; n>0 ; n--,d++,s++ )
    if(*d != *s)
      break;
  if(n==0)
    return 0;

  if( *d > *s )
    return 1;
  else if( *d < *s )
    return -1;
  else
    return 0;
}

void *
memmove( void *dest, const void *src, unsigned int n )
{
  char *d;
  const char *s;

  if((unsigned int)dest < (unsigned int)src) {
    d = (char*)dest;
    s = (const char*)src;
    for( ; n>0 ; n-- )
      *d++ = *s++;
  }
  else if((unsigned int)dest > (unsigned int)src) {
    d = (char*)((unsigned int)dest+n);
    s = (const char*)((unsigned int)src+n);
    for( ; n>0 ; n-- )
      *(--d) = *(--s);
  }

  return dest;
}

void *
strncpy( void *dest, const void *src, unsigned int n )
{
  char *d = (char*)dest;
  const char *s = (const char*)src;

  for( ; n>0 ; n--,d++,s++ ) {
    *d = *s;
    if(*s==0)
      break;
  }

  return dest;
}

int
strncmp( void *dest, const void *src, unsigned int n )
{
  const char *d = (char*)dest;
  const char *s = (const char*)src;

  for(; n>0 ; n--,d++,s++ ) {
    if(*d != *s)
      break;
    if(*d==0 || *s==0)
      break;
  }
  if(n==0)
    return 0;

  if( *d > *s )
    return 1;
  else if( *d < *s )
    return -1;
  else
    return 0;
}
int
strncmpi( void *dest, const void *src, unsigned int n )
{
  const char *d = (char*)dest;
  const char *s = (const char*)src;

  for(; n>0 ; n--,d++,s++ ) {
    if(tolower(*d) != tolower(*s))
      break;
    if(*d==0 || *s==0)
      break;
  }
  if(n==0)
    return 0;

  if( tolower(*d) > tolower(*s) )
    return 1;
  else if( tolower(*d) < tolower(*s) )
    return -1;
  else
    return 0;
}

int strlen(char *chr)
{
  int len;

  for(len=0;*chr!=0;len++,chr++)
    ;
  return len;
}

char numtohex1(char chr)
{
	chr = chr & 0x0f;
	if(chr < 10)
		return chr + '0';
	else
		return chr - 10 + 'A';
}

char uppercase(char c)
{
  if(c>='a' && c<='z')
    c -= 0x20;
  return c;
}
