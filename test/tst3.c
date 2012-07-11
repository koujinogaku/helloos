#include "memory.h"
#include "display.h"
#include "syscall.h"
#include "print.h"
#include "string.h"

void tst_args(int i, ...);
void tst_vargs(int i, va_list args);

int start()
{
/*
  char *a,*b,*c,*d;

  display_puts("malloc\n");
  a=malloc(64-4);
{
  char s[16];
  long2hex(a,s);
  display_puts("a=");
  display_puts(s);
  display_puts("\n");
}
  c=malloc(32-4);
  d=malloc(16-4);
  mfree(c);

  mem_dumpfree();

  b=realloc(a,64-4+1);
{
  char s[16];
  long2hex(b,s);
  display_puts("b=");
  display_puts(s);
  display_puts("\n");
}
  mem_dumpfree();

*/
/*
  int i;

  for(i=0;i<256;i++) {
    if(i%32==0)
      display_putc('\n');
    display_setattr(i);
    display_putc((i%10)+'0');
    //syscall_putc((i%10)+'0');
    //syscall_wait(10);
  }
  display_setattr(0x7);
  display_putc('\n');
  return 0;
*/
  int i=1;
  int x=2,y=3;
  char s[32];

  for(i=0;i<10;i++) {
    print_format("i=%s,","abc");
  }

  return 0;
}
