#include "memory.h"
#include "list.h"
#include "memory_s.h"

void *memory_alloc(unsigned long areasize, void* u_ctl)
{
  struct mem_list *freearea, *list, *tmp_area;
  unsigned long *address;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

//  mutex_lock(&mem_alloc_mutex);

  areasize += sizeof(long);
  if(areasize<sizeof(struct mem_list))
    areasize=sizeof(struct mem_list);

  if(areasize%sizeof(int) != 0) {
    areasize += sizeof(int) - (areasize%sizeof(int));
  }

  list_for_each((&(ctl->memfreelist)), list) {
    if(list->size >= areasize) {
      if(list->size >= (areasize+sizeof(struct mem_list)) && list->size >= sizeof(struct mem_list)*2 ) {
        freearea=(struct mem_list*)(((char*)list)+areasize);
        freearea->size=list->size-areasize;
        list_add_next(list,freearea);
      }
      freearea=list->next;
      tmp_area=list->prev;
      list_del(list);
      freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
      tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;

//      mutex_unlock(&mem_alloc_mutex);
      address=(unsigned long *)list;
      address[0] = areasize;
      return &address[1];
    }
  }

//  mutex_unlock(&mem_alloc_mutex);
  return 0;
}

