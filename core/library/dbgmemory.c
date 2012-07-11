#include "display.h"
#include "string.h"
#include "memory.h"

struct mem_list {
  struct mem_list *next;
  struct mem_list *prev;
  unsigned long size;
  unsigned long chksum;
};

void *dbg_malloc(unsigned long size)
{
  char s[32];
  char *m;
  unsigned long areasize=size;
  unsigned long blockstart;

  areasize += sizeof(long);
  if(areasize<sizeof(struct mem_list))
    areasize=sizeof(struct mem_list);

  if(areasize%sizeof(int) != 0) {
    areasize += sizeof(int) - (areasize%sizeof(int));
  }

  m=malloc(size);
  blockstart = (unsigned long)(((int)m)-sizeof(long));

  display_puts("[a:");
  long2hex((unsigned long)m,s);
  display_puts(s);
  display_puts(":");
  long2hex(blockstart,s);
  display_puts(s);
  display_puts("-");
  long2hex(blockstart+areasize-1,s);
  display_puts(s);
  display_puts("]");
  return m;
}
void dbg_mfree(void *mm)
{
  char s[32];
  char *m=mm;
  unsigned int freearea,areasize;
  unsigned long blockstart;

  freearea=(unsigned int)((int)m-sizeof(long));
  areasize=*((int *)freearea);

  blockstart = (unsigned long)(((int)m)-sizeof(long));

  display_puts("[f:");
  long2hex((unsigned long)m,s);
  display_puts(s);
  display_puts(":");
  long2hex(blockstart,s);
  display_puts(s);
  display_puts("-");
  long2hex(blockstart+areasize-1,s);
  display_puts(s);
  display_puts("]");
  mfree(m);
  return;
}
