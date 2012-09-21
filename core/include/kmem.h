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

#endif /* KMEM_H */
