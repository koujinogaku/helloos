#include "syscall.h"
#include "display.h"
#include "string.h"
#include "errno.h"

static char s[10];
#define BUFFSZ 512

int start(int argc, char *argv[])
{
  int r;
  int fp;
  char buf[BUFFSZ+1];

  if(argc<2) {
    display_puts("Usage: type <filename>\n");
    return 1;
  }

  r = syscall_file_open(argv[1],SYSCALL_FILE_O_RDONLY);
  if(r==ERRNO_NOTEXIST) {
    display_puts("file not found\n");
    return 1;
  }
  if(r<0) {
    display_puts("open error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return -r;
  }
  fp = r;
  for(;;) {
    r = syscall_file_read(fp, buf, BUFFSZ);
    if(r==ERRNO_OVER)
      break;
    if(r<0) {
      display_puts("readdir error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return -r;
    }
    buf[r]=0;
    display_puts(buf);
  }

  r = syscall_file_close(fp);
  if(r<0) {
    display_puts("closedir error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return -r;
  }

  return 0;
}

