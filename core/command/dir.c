#include "syscall.h"
#include "string.h"
#include "keyboard.h"
#include "display.h"
#include "errno.h"

static char s[10];

int start()
{
  unsigned long dp;
  int r;
  char dirent[12];
  unsigned int filesize;
  int fp;

  r = syscall_file_opendir(&dp,"");
  if(r<0) {
    display_puts("opendir error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return -r;
  }
  for(;;) {
    r = syscall_file_readdir(&dp,dirent);
    if(r==ERRNO_OVER)
      break;
    if(r<0) {
      display_puts("readdir error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return -r;
    }
    display_puts(dirent);

    r =syscall_file_open(dirent, SYSCALL_FILE_O_RDONLY);
    if(r<0) {
      display_puts("fileopen error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return -r;
    }
    fp = r;
    r=syscall_file_size(fp, &filesize);
    if(r<0) {
      display_puts("filesize error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return -r;
    }
    display_puts("\t");
/*
    int2dec(strlen(dirent),s);
    display_puts(s);
    display_puts("\t");
*/
    if(strlen(dirent)<8)
      display_puts("\t");
    int2dec(filesize,s);
    display_puts(s);
    display_puts("\n");

    r=syscall_file_close(fp);
  
  }

  r = syscall_file_closedir(&dp);
  if(r<0) {
    display_puts("closedir error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return -r;
  }

  return 0;
}

