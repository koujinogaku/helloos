#include "memory.h"
#include "list.h"
#include "memory_s.h"
#include "string.h"

void *memory_realloc(void *address,unsigned long areasize, void* u_ctl)
{
  struct mem_list *list, *freearea;
  struct mem_list list_temp;
  unsigned long orig_areasize,old_areasize,del_areasize;
  unsigned long *newaddress,*outeraddress;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

  orig_areasize=areasize;

  if(address==0) {
    return memory_alloc(areasize,ctl);
  }
  if(areasize==0) {
    memory_free(address,ctl);
    return 0;
  }

  freearea=(struct mem_list *)((int)address-sizeof(long));
  old_areasize=*((int *)freearea);

  areasize += sizeof(long);
  if(areasize<sizeof(struct mem_list))
    areasize=sizeof(struct mem_list);

  if(areasize%sizeof(int) != 0) {
    areasize += sizeof(int) - (areasize%sizeof(int));
  }

  if(old_areasize == areasize) {
    return address;
  }
  else if(old_areasize > areasize) {
    del_areasize = old_areasize - areasize;
    if(del_areasize < sizeof(struct mem_list)){
      return address;
    }
    newaddress=(unsigned long *)((unsigned int)address-sizeof(unsigned long));
    outeraddress=(unsigned long *)((unsigned int)newaddress + areasize);
    *newaddress=areasize;
    *outeraddress=del_areasize;
    memory_free(&outeraddress[1],ctl);
    return address;
  }

  outeraddress=(unsigned long *)((unsigned int)address - sizeof(unsigned long) + old_areasize);
  list_for_each((&(ctl->memfreelist)), list) {
    if((struct mem_list *)outeraddress <=list)
      break;
  }

  if((struct mem_list *)outeraddress!=list ||
     ((struct mem_list *)outeraddress==list && list->size < areasize-old_areasize)) {
    newaddress=memory_alloc(orig_areasize,ctl);
    if(newaddress==0)
      return 0;
    memcpy((void*)newaddress,address,orig_areasize);
    memory_free(address,ctl);
    return (void*)newaddress;
  }

  if(list->size - (areasize-old_areasize) > sizeof(struct mem_list)) {
    freearea=(struct mem_list *)((unsigned int)list + (areasize-old_areasize));

    memcpy(&list_temp,list,sizeof(struct mem_list));
    memcpy(freearea,&list_temp,sizeof(struct mem_list));
    freearea->size -= (areasize-old_areasize);
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
    list = freearea;
    freearea=list->prev;
    freearea->next = list;
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
    freearea=list->next;
    freearea->prev = list;
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;

    newaddress=(unsigned long *)((unsigned int)address-sizeof(unsigned long));
    *newaddress=areasize;
    return address;
  }
  else {
    freearea=list->next;
    freearea->prev = list->prev;
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
    freearea=list->prev;
    freearea->next = list->next;
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;

    newaddress=(unsigned long *)((unsigned int)address-sizeof(unsigned long));
    *newaddress=old_areasize + list->size;
    return address;
  }

}

