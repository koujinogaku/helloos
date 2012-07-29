#include "syscall.h"
#include "display.h"
#include "string.h"
#include "errno.h"
#include "kmem.h"
#include "memory.h"

#define PLIST_SIZE 64


int start(int argc, char *argv[])
{
  struct kmem_queue *plist;
  int i,num;
  char strbuf[16];

  plist = malloc(sizeof(struct kmem_queue)*PLIST_SIZE);
  if(plist==0) {
    display_puts("error=malloc\n");
    return 1;
  }

  num = syscall_que_list(0,PLIST_SIZE,plist);

  display_puts("ID   IN   OUT  NAME ST WAIT\n");
  for(i=0;i<num;i++) {
    word2hex(plist[i].id,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].in,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].out,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].name,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    byte2hex(plist[i].status,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].numwait,strbuf);
    display_puts(strbuf);
    display_putc('\n');

  }

  mfree(plist);

  return 0;
}
