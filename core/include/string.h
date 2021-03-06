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
int strlen(const char *chr);

char *int2dec(unsigned int memsz, char *buff);
char *sint2dec(int num, char *str);
char *byte2hex(unsigned char num, char *str);
char *word2hex(unsigned short num, char *str);
char *long2hex(unsigned long num, char *str);
char numtohex1(char chr);
char uppercase(char c);

// external object code
int dbl2dec(double val, char *str, int width);
char *strrchr(const char *str, char ch);
char *strcat(char *str1, const char *str2);
int strcmp( void *dest, const void *src);
char *strdup(const char *src);

#endif
