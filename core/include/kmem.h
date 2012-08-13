#ifndef KMEM_H
#define KMEM_H

struct kmem_program
{
  unsigned short id;
  unsigned short taskid;
  unsigned short status;
  unsigned short exitque;
  void *pgd;
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
