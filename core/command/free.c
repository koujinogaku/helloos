#include "syscall.h"
#include "display.h"
#include "string.h"
#include "errno.h"
#include "print.h"

static char s[16];

int start(int argc, char *argv[])
{
  int r;
  unsigned long status[5];

  r = syscall_krn_memory_status(status);

  if(r<0) {
    display_puts("error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return -r;
  }

  display_puts("          total     used     free\n");
  print_format("Total  %6dKB %6dKB %6dKB\n",status[0]/1024,(status[0]-status[1])/1024,status[1]/1024);
  print_format("Kernel %6dKB %6dKB %6dKB\n",status[2]/1024,(status[2]-status[3])/1024,status[3]/1024);

  return 0;
}
