#ifndef ASSERT_H
#define ASSERT_H
#include "display.h"
#include "string.h"
#include "syscall.h"

#define assert(c) if((int)(c)==0) { char s[8]; display_puts("Assertion failed : ");display_puts(__FILE__);display_puts(":");int2dec(__LINE__,s);display_puts(s);exit(0); }

#endif /* ASSERT_H */
