#include "memory.h"
#include "list.h"
#include "memory_s.h"

char mem_std_status=0;

void memory_init(void *heap_start, unsigned long heap_size)
{
  struct mem_list *freearea;
  struct mem_control_block *ctl;

  ctl = heap_start;
  list_init((&(ctl->memfreelist)));

  freearea=(struct mem_list *)((unsigned long)heap_start+sizeof(struct mem_control_block));
  freearea->size=heap_size-sizeof(struct mem_control_block);

  list_add_tail((&(ctl->memfreelist)),freearea);
  freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;

  ctl->heap_low  = (unsigned long)freearea;
  ctl->heap_high = ((unsigned long)freearea)+freearea->size;
}

