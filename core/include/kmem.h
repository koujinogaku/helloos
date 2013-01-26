#ifndef KMEM_H
#define KMEM_H

struct bios_info {
  unsigned short vmode;     /* Video Mode */
  unsigned short depth;     /* num of bit in 1dot */
  void *vram;               /* video frame physical address */
  unsigned short scrnx;     /* video X size */
  unsigned short scrny;     /* video Y size */
  unsigned short cursorx;   /* cursor X */
  unsigned short cursory;   /* cursor Y */
  void *loaderscreen;       /* backup of loader screen */
  void *vesainfo;
  void *vesamodeinfo;
};

struct kmem_program
{
  unsigned short id;
  unsigned short taskid;
  unsigned short status;
  unsigned short exitque;
  void *pgd;
  unsigned long  tick;
  unsigned short taskque;
  char pgmname[8];
};

struct kmem_queue
{
  short id;
  short in;
  short out;
  unsigned short name;
  char status;
  char dmy;
  short numwait;
};

struct kmem_systime
{
  unsigned long low;
  unsigned long high;
};

struct kmem_except_reg {
  unsigned long gs, fs, es, ds, ss;		// push values of each register
  unsigned long edi, esi, ebp, esp;		// pushad info
  unsigned long ebx, edx, ecx, eax;		// ..
  unsigned long errcode, eip, cs, eflags;	// exception info
  unsigned long app_esp, app_ss;		// appl stack info.(if user mode)
};
struct kmem_except_info {
  unsigned long excpno;
  unsigned long cr0;
  unsigned long cr2;
  unsigned long cr3;
  unsigned int  taskid;
  unsigned int  lastfunc;
  struct kmem_except_reg regs;
};
#endif /* KMEM_H */
