/* Memory Managemant */

#include "config.h"
#include "memory.h"
#include "string.h"
#include "list.h"
#include "syscall.h"
#include "errno.h"

#include "display.h"
static char s[16];

struct mem_list {
  struct mem_list *next;
  struct mem_list *prev;
  unsigned long size;
  unsigned long chksum;
};

struct mem_control_block {
  struct mem_list memfreelist;
  unsigned long heap_low;
  unsigned long heap_high;
};

static char mem_std_status=0;

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

static inline struct mem_control_block *mem_get_ctl(void *u_ctl)
{
  if(u_ctl==0) {
    if(mem_std_status==0) {
      memory_init((void*)CFG_MEM_USERHEAP,CFG_MEM_USERHEAPSZ);
      mem_std_status=1;
    }
    return (struct mem_control_block *)CFG_MEM_USERHEAP;
  }
  else {
    return u_ctl;
  }
}

void memory_free(void *address, void* u_ctl)
{
  struct mem_list *list, *freearea, *tmp_area;
  unsigned int areasize;
  int i;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

  if((unsigned int)address < ctl->heap_low ||  ctl->heap_high <= (unsigned int)address ) {
    display_puts("mfree:bad address[");
    long2hex((long)address,s);
    display_puts(s);
    display_puts("]\n");
    syscall_exit(1);
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

void *memory_calloc(unsigned long count,unsigned long bufsize, void* u_ctl)
{
  char *m,*r;
  int areasize;
  struct mem_control_block *ctl;

  ctl=mem_get_ctl(u_ctl);

  areasize=count*bufsize;
  r=m=memory_alloc(areasize,ctl);
  if(r!=0) {
    for(;areasize>0;--areasize)
      *m++ = 0;
  }
  return r;
}

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

