#include "syscall.h"
#include "display.h"
#include "string.h"
#include "errno.h"
#include "kmem.h"
#include "memory.h"

#include "config.h"
#include "environm.h"

#define PLIST_SIZE 64


int start(int argc, char *argv[])
{
  struct kmem_program *plist;
  int i,num;
  char strbuf[16];

  plist = malloc(sizeof(struct kmem_program)*PLIST_SIZE);
  if(plist==0) {
    display_puts("error=malloc\n");
    return 1;
  }

  num = syscall_pgm_list(0,PLIST_SIZE,plist);

  display_puts("ID   ST TASK TSKQ EXTQ PGDADDR  NAME\n");
  for(i=0;i<num;i++) {
    word2hex(plist[i].id,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    byte2hex(plist[i].status,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].taskid,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].taskque,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].exitque,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    long2hex(((unsigned long)(plist[i].pgd)),strbuf);
    display_puts(strbuf);
    display_putc(' ');

    strncpy(strbuf,plist[i].pgmname,sizeof(plist[i].pgmname));
    strbuf[sizeof(plist[i].pgmname)]=0;
    display_puts(strbuf);
    display_putc('\n');

  }

  mfree(plist);

  return 0;
}
