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
  struct kmem_program *plist0;
  int i,j,num,num0,len;
  char strbuf[20];
  int *cputimelist;
  int cputimetotal;
  int percent;

  plist0 = malloc(sizeof(struct kmem_program)*PLIST_SIZE);
  if(plist0==0) {
    display_puts("error=malloc\n");
    return 1;
  }
  plist = malloc(sizeof(struct kmem_program)*PLIST_SIZE);
  if(plist==0) {
    display_puts("error=malloc\n");
    return 1;
  }
  cputimelist = malloc(sizeof(int)*PLIST_SIZE);
  if(cputimelist==0) {
    display_puts("error=malloc\n");
    return 1;
  }

  num0 = syscall_pgm_list(0,PLIST_SIZE, plist0);
  syscall_wait(100);
  num = syscall_pgm_list(0,PLIST_SIZE, plist);

  cputimetotal=0;
  j=0;
  for(i=0;i<num;i++) {
    while(plist[j].id < plist[i].id && j<num0) {
      j++;
    }

    if(j<num0 && plist0[j].id == plist[i].id)
      cputimelist[i] = plist[i].tick - plist0[j].tick;
    else
      cputimelist[i] = 0;

    cputimetotal += cputimelist[i];
  }

  display_puts("ID   TASK STAT TSKQ EXTQ PGDADDR  CPU% NAME\n");
  for(i=0;i<num;i++) {
    word2hex(plist[i].id,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].taskid,strbuf);
    display_puts(strbuf);
    display_putc(' ');

    word2hex(plist[i].status,strbuf);
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

    percent = (cputimelist[i]*100)/cputimetotal;
    int2dec(percent,strbuf);
    len=strlen(strbuf);
    for(j=0;j<3-len;j++)
      display_putc(' ');
    display_puts(strbuf);
    display_puts("% ");

    strncpy(strbuf,plist[i].pgmname,sizeof(plist[i].pgmname));
    strbuf[sizeof(plist[i].pgmname)]=0;
    display_puts(strbuf);
    display_putc('\n');

  }

  mfree(plist);

  return 0;
}
