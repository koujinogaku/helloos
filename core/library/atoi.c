#include "stdlib.h"

int atoi(const char* str)
{
    int re = 0;
    const char* p = str;

    if(*str == '-') p++;
    for(; *p != '\0'; p++){
        if(*p < '0' || *p > '9'){
            re = 0;
            break;
        }else{
            re *= 10;
            re += (int)*p - 0x30; // '0' = 0x30
        }
    }
    if(*str == '-') re = -re;

    return re;
}
