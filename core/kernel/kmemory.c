/* Memory Management */

#include "config.h"
#include "cpu.h"
#include "kmemory.h"
#include "string.h"
#include "console.h"
#include "list.h"
#include "mutex.h"

struct mem_freelist {
  struct mem_freelist *next;
  struct mem_freelist *prev;
  unsigned long size;
};

static struct mem_freelist memfreelist;
//static int mem_alloc_mutex=0;
static char mem_kmem_status=0;
static unsigned long mem_totalsize;
static unsigned long mem_kernel_totalsize;

void mem_free(void *vfreearea, unsigned long areasize)
{
  struct mem_freelist *list, *freearea=vfreearea;
  int i;
  int hold_locked;

  cpu_hold_lock(hold_locked)
//  mutex_lock(&mem_alloc_mutex);

  if(areasize%MEM_PAGESIZE != 0) {
    areasize += MEM_PAGESIZE - (areasize%MEM_PAGESIZE);
  }

  freearea->size=areasize;

  list_for_each((&memfreelist), list) {
    if(freearea<list) { // Has discovered a area that is comparison in ascending order
      list_insert_prev(list,freearea);
      break; // end becouse it has inserted
    }
  }
  if(&memfreelist==list) {  // Not found 
    list_add_tail((&memfreelist),freearea); // Add to tail becouse it is biggest 
  }

  freearea = freearea->prev;
  for(i=0;i<2;i++) { // perform 2 times
    if(&memfreelist!=freearea) {
      if((((char*)freearea)+freearea->size) == (char*)freearea->next) { // has continuous
        if(&memfreelist!=freearea->next) {  // Join if not last
          freearea->size += freearea->next->size;
          list=freearea->next;
          list_del(list);
          freearea = freearea->prev; // back to previous becouse delete next
        }
      }
    }
    freearea = freearea->next; // go next
  }

  cpu_hold_unlock(hold_locked);
//  mutex_unlock(&mem_alloc_mutex);
}

void *mem_alloc(unsigned long areasize)
{
  struct mem_freelist *freearea, *list;
  int hold_locked;

  cpu_hold_lock(hold_locked)
//  mutex_lock(&mem_alloc_mutex);

  if(areasize%MEM_PAGESIZE != 0) {
    areasize += MEM_PAGESIZE - (areasize%MEM_PAGESIZE);
  }

  list_for_each((&memfreelist), list) {
    if(list->size >= areasize) {
      if(list->size >= (areasize+sizeof(struct mem_freelist)) && list->size >= sizeof(struct mem_freelist)*2) {
        freearea=(struct mem_freelist*)(((char*)list)+areasize);
        freearea->size=list->size-areasize;
        list_insert_prev(list,freearea);
      }
      list_del(list);

      cpu_hold_unlock(hold_locked);
//      mutex_unlock(&mem_alloc_mutex);
      return list;
    }
  }

  cpu_hold_unlock(hold_locked);
//  mutex_unlock(&mem_alloc_mutex);
  return 0;
}

unsigned long mem_get_kernelfree(void)
{
  unsigned long size=0;
  struct mem_freelist *list;

  list_for_each((&memfreelist),list)
  {
    size += list->size;
  }
  return size;
}

static inline void
mem_cache_off(void)
{
  asm volatile(
    "mov %%cr0, %%eax\n"
    "or  $0x60000000, %%eax\n"
    "mov %%eax, %%cr0"
     :::"eax");
}
static inline void
mem_cache_on(void)
{
  asm volatile(
    "mov %%cr0, %%eax\n"
    "and $0x9fffffff, %%eax\n"
    "mov %%eax, %%cr0"
     :::"eax");
}

static inline unsigned int
mem_get_eflags(void)
{
  unsigned int eflags;
  asm volatile(
    "pushf\n"
    "popl %0"
    :"=m"(eflags));
  return eflags;
}

static inline void
mem_set_eflags(unsigned int eflags)
{
  asm volatile(
    "pushl %0\n"
    "popf"
    :"=m"(eflags));
}

unsigned int mem_peek(unsigned int addr)
{
  unsigned int data;

  asm volatile(
      "movl (%%ebx),  %%eax\n"
      :"=eax"(data):"ebx"(addr));
  return data;
}
void mem_poke(unsigned int addr,unsigned int data)
{
  asm volatile(
      "movl %%eax, (%%ebx)\n"
      ::"eax"(data),"ebx"(addr));
}

#define MEM_AC_BIT 0x00040000
static unsigned int
mem_check_memsize(void)
{
  unsigned int eflags;
  int cache_cfg=0;
  unsigned int adr,data,pat,ex,xdata;

  eflags = mem_get_eflags();
  eflags |= MEM_AC_BIT;
  mem_set_eflags(eflags);
  eflags = mem_get_eflags();
  if(eflags & MEM_AC_BIT) {
    cache_cfg = 1;
    mem_cache_off();
    eflags &= ~MEM_AC_BIT;
    mem_set_eflags(eflags);
  }

  adr = CFG_MEM_KERNELHEAP;

  for(;;)
  {
    data = mem_peek(adr);
    pat = 0xf0a50f5a;
    mem_poke(adr,pat);
    xdata=mem_peek(adr);
    if(pat==xdata)
      ex = 1;
    else
      ex = 0;

    mem_poke(adr,data);
    if(ex==0)
      break;
    if(adr > 0xffff0000)
      break;
    adr+=MEM_PAGESIZE;
  }

  if(cache_cfg) {
    mem_cache_on();
  }

  return adr;
}

int mem_get_status(void)
{
  return mem_kmem_status;
}
unsigned long mem_get_totalsize(void)
{
  return mem_totalsize;
}
unsigned long mem_get_kernelsize(void)
{
  return mem_kernel_totalsize;
}

void mem_init(void)
{
  mem_totalsize = mem_check_memsize();

  list_init((&memfreelist));

  if(mem_totalsize>(CFG_MEM_KERNELHEAP+CFG_MEM_KERNELHEAPSZ)) {
    /* pool under 8MB */
    mem_free((void*)CFG_MEM_KERNELHEAP , CFG_MEM_KERNELHEAPSZ );
    mem_kmem_status=2;
    mem_kernel_totalsize=CFG_MEM_KERNELHEAPSZ;
  }
  else {
    /* pool under 640KB */
    mem_free((void*)CFG_MEM_KERNELHEAP2, CFG_MEM_KERNELHEAP2SZ);
    mem_kmem_status=1;
    mem_kernel_totalsize=CFG_MEM_KERNELHEAP2SZ;
  }
}

void mem_dumpfree(void)
{
  struct mem_freelist *list;
  char s[32];

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

