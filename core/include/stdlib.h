#ifndef STDLIB_H
#define STDLIB_H

#define abs(n) (((n)<0 )? -(n) : (n))
#define tolower(n) ( ( (n)>='A' && (n)<='Z') ? ((n)+0x20) : (n))
#define toupper(n) ( ( (n)>='a' && (n)<='z') ? ((n)-0x20) : (n))

void qsort(void* base, unsigned int n, unsigned int size, int(*fnc)(const void*, const void*));
int atoi(const char* str);
void exit(int r);

#endif
