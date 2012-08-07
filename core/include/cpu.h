/*
** cpu.h --- CPU control operations
*/
#ifndef CPU_H
#define CPU_H

#include "config.h"

static inline byte
cpu_in8( unsigned int port )
{
  byte ret;
  Asm("inb %%dx, %%al": "=a"(ret): "d"(port));
  return ret;
}

static inline void
cpu_out8( unsigned int port, byte data )
{
  Asm("outb %%al, %%dx":: "a"(data), "d"(port));
}

static inline word16
cpu_in16( unsigned int port )
{
  word16 data;
  Asm("inw %%dx, %%ax": "=a"(data): "d"(port));
  return data;
}

static inline void
cpu_out16( unsigned int port, word16 data )
{
  Asm("outw %%ax, %%dx":: "a"(data), "d"(port));
}

static inline void
cpu_lock(void)
{
  Asm("cli");
}

static inline void
cpu_unlock(void)
{
  Asm("sti");
}

static inline bool
cpu_has_locked(void)
{
  unsigned int eflags;
//  Asm("pushf");
//  Asm("popl %0": "=m"(eflags));
  Asm("pushf\n"
      "popl %0": "=m"(eflags));
  return((eflags & 0x200) == 0);
}

#define cpu_hold_lock( flag )	\
{				\
  flag = cpu_has_locked();	\
  if( flag == 0 )		\
    cpu_lock();			\
}

#define cpu_hold_unlock( flag )	\
{				\
  if( flag == 0 )		\
    cpu_unlock();		\
}

#define BEGIN_CPULOCK()\
  {\
    bool hold_locked;\
    cpu_hold_lock(hold_locked);

#define END_CPULOCK()\
    cpu_hold_unlock(hold_locked);\
  }

static inline void
cpu_halt(void)
{
  Asm("hlt");
}

static inline void
cpu_disable_nmi(void)
{
  cpu_out8(0x70, 0x80);
}

static inline void
cpu_enable_nmi(void)
{
  cpu_out8(0x70, 0x00);
}

#endif /* CPU_H */
