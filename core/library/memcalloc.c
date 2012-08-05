#include "memory.h"
#include "list.h"
#include "memory_s.h"

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

