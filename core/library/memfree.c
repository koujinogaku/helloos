#include "memory.h"
#include "list.h"
#include "memory_s.h"
#include "syscall.h"

void memory_free(void *address, void* u_ctl)
{
  struct mem_list *list, *freearea, *tmp_area;
  unsigned int areasize;
  int i;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

  if((unsigned int)address < ctl->heap_low ||  ctl->heap_high <= (unsigned int)address ) {
/*
    display_puts("mfree:bad address[");
    long2hex((long)address,s);
    display_puts(s);
    display_puts("]\n");
*/
    syscall_exit(333);
    return;
  }

  freearea=(struct mem_list *)((int)address-sizeof(long));
  areasize=*((int *)freearea);

  freearea->size=areasize;

  list_for_each((&(ctl->memfreelist)), list) {
    if(freearea<list) { // Has discovered a area that is comparison in ascending order
      list_insert_prev(list,freearea);
      freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
      tmp_area=freearea->next;
      tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
      tmp_area=freearea->prev;
      tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
      break; // end becouse it has inserted
    }
  }
  if(&(ctl->memfreelist)==list) {  // Not found 
    list_add_tail((&(ctl->memfreelist)),freearea); // Add to tail becouse it is biggest 
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
    tmp_area=freearea->next;
    tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
    tmp_area=freearea->prev;
    tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
  }

  freearea = freearea->prev;
  for(i=0;i<2;i++) { // perform 2 times
    if(&(ctl->memfreelist)!=freearea) {
      if((((char*)freearea)+freearea->size) == (char*)freearea->next) { // has continuous
        if(&(ctl->memfreelist)!=freearea->next) {  // Join if not last
          freearea->size += freearea->next->size;
          list=freearea->next;
          list_del(list);
          freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
          tmp_area=freearea->next;
          tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
          freearea = freearea->prev; // back to previous becouse delete next
        }
      }
    }
    freearea = freearea->next; // go next
  }

//  mutex_unlock(&mem_alloc_mutex);
}
