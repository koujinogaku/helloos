#ifndef STRING_H
#define STRING_H

#ifndef NULL
#define NULL   (0)
#endif

void *memcpy( void *dest, const void *src, unsigned int n );
void *memset( void *s, int c, unsigned int n );
int memcmp( void *dest, const void *src, unsigned int n );
void *memmove( void *dest, const void *src, unsigned int n );
void *strncpy( void *dest, const void *src, unsigned int n );
int strncmp( void *dest, const void *src, unsigned int n );
int strncmpi( void *dest, const void *src, unsigned int n );
int strlen(char *chr);

void int2dec(unsigned int memsz, char *buff);
void sint2dec(int num, char *str);
void byte2hex(unsigned char num, char *str);
void word2hex(unsigned short num, char *str);
void long2hex(unsigned long num, char *str);
char numtohex1(char chr);
char uppercase(char c);
int atoi(const char* str);

#endif
