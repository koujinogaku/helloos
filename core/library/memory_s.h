#ifndef MEMORY_S_H
#define MEMORY_S_H 1

#include "config.h"

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

extern char mem_std_status;

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

#endif
