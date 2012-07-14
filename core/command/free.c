#include "syscall.h"
#include "display.h"
#include "string.h"
#include "errno.h"

static char s[16];

int start(int argc, char *argv[])
{
  int r;
  unsigned long status[4];

  r = syscall_kernel_memory_status(status);

  if(r<0) {
    display_puts("error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return -r;
  }

  display_puts("Total Mem=");
  int2dec(status[0]/1024,s);
  display_puts(s);
  display_puts("KB\n");

  display_puts("Free  Mem=");
  int2dec(status[1]/1024,s);
  display_puts(s);
  display_puts("KB\n");

  display_puts("Kernel Free Mem=");
  int2dec(status[2]/1024,s);
  display_puts(s);
  display_puts("KB\n");

  return 0;
}
