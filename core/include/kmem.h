#ifndef KMEM_H
#define KMEM_H

struct kmem_program
{
  unsigned short id;
  unsigned short status;
  unsigned short taskid;
  unsigned short exitque;
  void *pgd;
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

#endif /* KMEM_H */
