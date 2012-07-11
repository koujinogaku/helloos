#ifndef PRINT_H
#define PRINT_H

#include "stdarg.h"

int print_format(const char* fmt, ...);
int print_sformat(char* buff,const char* fmt, ...);
int print_vsformat(char* buff,const char* fmt,va_list arg);
void print_null(const char* fmt);

#endif /* PRINT_H */
