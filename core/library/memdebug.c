#include "memory.h"
#include "memory_s.h"
#include "string.h"
#include "list.h"
#include "syscall.h"
#include "errno.h"

#include "display.h"
unsigned long memory_get_heapfree(void* u_ctl)
{
  unsigned long size=0;
  struct mem_list *list;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

  list_for_each((&(ctl->memfreelist)),list)
  {
    size += list->size;
  }
  return size;
}

int memory_check_heapfree(void* u_ctl)
{
  unsigned long chksum;
  struct mem_list *list;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

  list_for_each((&(ctl->memfreelist)),list)
  {
    chksum=(unsigned int)(list->next)+(unsigned int)(list->prev)+list->size;
    if(list->chksum != chksum)
      return ERRNO_CTRLBLOCK;
  }
  return 0;
}

#define console_puts  display_puts
void memory_dumpfree(void* u_ctl)
{
  struct mem_list *list;
  char s[32];
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

    console_puts("HD=");
    long2hex((int)(&(ctl->memfreelist)),s);
    console_puts(s);
    console_puts("(");
    long2hex((int)(&(ctl->memfreelist))->prev,s);
    console_puts(s);
    console_puts(",");
    long2hex((int)(&(ctl->memfreelist))->next,s);
    console_puts(s);
    console_puts("),");

  console_puts("FREE=");

  list_for_each((&(ctl->memfreelist)),list)
  {
    long2hex((int)list,s);
    console_puts(s);
    console_puts("(");
    long2hex((int)list->prev,s);
    console_puts(s);
    console_puts(",");
    long2hex((int)list->next,s);
    console_puts(s);
    console_puts(",");
    long2hex(list->size,s);
    console_puts(s);
    console_puts("),");
  }
  console_puts("\n");
}

