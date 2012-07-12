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

static struct mem_list memfreelist;
//static int mem_alloc_mutex=0;
static char mem_status=0;

void mfree(void *address)
{
  struct mem_list *list, *freearea, *tmp_area;
  unsigned int areasize;
  int i;

  if(mem_status==0)
    return;
//  mutex_lock(&mem_alloc_mutex);

  if((unsigned int)address < CFG_MEM_USERHEAP ||  CFG_MEM_USERHEAPMAX <= (unsigned int)address ) {
    display_puts("mfree:bad address[");
    long2hex((long)address,s);
    display_puts(s);
    display_puts("]\n");
    syscall_exit(1);
  }

  freearea=(struct mem_list *)((int)address-sizeof(long));
  areasize=*((int *)freearea);

  freearea->size=areasize;

  list_for_each((&memfreelist), list) {
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
  if(&memfreelist==list) {  // Not found 
    list_add_tail((&memfreelist),freearea); // Add to tail becouse it is biggest 
    freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;
    tmp_area=freearea->next;
    tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
    tmp_area=freearea->prev;
    tmp_area->chksum=(unsigned int)tmp_area->next+(unsigned int)tmp_area->prev+tmp_area->size;
  }

  freearea = freearea->prev;
  for(i=0;i<2;i++) { // perform 2 times
    if(&memfreelist!=freearea) {
      if((((char*)freearea)+freearea->size) == (char*)freearea->next) { // has continuous
        if(&memfreelist!=freearea->next) {  // Join if not last
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

void *malloc(unsigned long areasize)
{
  struct mem_list *freearea, *list, *tmp_area;
  unsigned long *address;

  if(mem_status==0) {
    mem_init();
  }

//  mutex_lock(&mem_alloc_mutex);

  areasize += sizeof(long);
  if(areasize<sizeof(struct mem_list))
    areasize=sizeof(struct mem_list);

  if(areasize%sizeof(int) != 0) {
    areasize += sizeof(int) - (areasize%sizeof(int));
  }

  list_for_each((&memfreelist), list) {
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

void *realloc(void *address,unsigned long areasize)
{
  struct mem_list *list, *freearea;
  struct mem_list list_temp;
  unsigned long orig_areasize,old_areasize,del_areasize;
  unsigned long *newaddress,*outeraddress;

  orig_areasize=areasize;

  if(mem_status==0) {
    mem_init();
  }
  if(address==0) {
    return malloc(areasize);
  }
  if(areasize==0) {
    mfree(address);
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
    mfree(&outeraddress[1]);
    return address;
  }

  outeraddress=(unsigned long *)((unsigned int)address - sizeof(unsigned long) + old_areasize);
  list_for_each((&memfreelist), list) {
    if((struct mem_list *)outeraddress <=list)
      break;
  }

  if((struct mem_list *)outeraddress!=list ||
     ((struct mem_list *)outeraddress==list && list->size < areasize-old_areasize)) {
    newaddress=malloc(orig_areasize);
    if(newaddress==0)
      return 0;
    memcpy((void*)newaddress,address,orig_areasize);
    mfree(address);
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

void *calloc(unsigned long count,unsigned long bufsize)
{
  char *m,*r;
  int areasize;
  areasize=count*bufsize;
  r=m=malloc(areasize);
  if(r!=0) {
    for(;areasize>0;--areasize)
      *m++ = 0;
  }
  return r;
}

unsigned long mem_get_heapfree(void)
{
  unsigned long size=0;
  struct mem_list *list;

  if(mem_status==0) {
    mem_init();
  }

  list_for_each((&memfreelist),list)
  {
    size += list->size;
  }
  return size;
}

int mem_check_heapfree(void)
{
  unsigned long chksum;
  struct mem_list *list;

  if(mem_status==0) {
    mem_init();
  }

  list_for_each((&memfreelist),list)
  {
    chksum=(unsigned int)(list->next)+(unsigned int)(list->prev)+list->size;
    if(list->chksum != chksum)
      return ERRNO_CTRLBLOCK;
  }
  return 0;
}

int mem_get_status(void)
{
  return mem_status;
}

void mem_init(void)
{
  struct mem_list *freearea;

  list_init((&memfreelist));

  freearea=(struct mem_list *)CFG_MEM_USERHEAP;
  freearea->size=CFG_MEM_USERHEAPSZ;

  list_add_tail((&memfreelist),freearea);
  freearea->chksum=(unsigned int)(freearea->next)+(unsigned int)(freearea->prev)+freearea->size;

  mem_status=1;
}

#define console_puts  display_puts
void mem_dumpfree(void)
{
  struct mem_list *list;
  char s[32];

  if(mem_status==0) {
    mem_init();
  }

    console_puts("HD=");
    long2hex((int)(&memfreelist),s);
    console_puts(s);
    console_puts("(");
    long2hex((int)(&memfreelist)->prev,s);
    console_puts(s);
    console_puts(",");
    long2hex((int)(&memfreelist)->next,s);
    console_puts(s);
    console_puts("),");

  console_puts("FREE=");

  list_for_each((&memfreelist),list)
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

