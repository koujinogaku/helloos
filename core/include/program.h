#ifndef PROGRAM_H
#define PROGRAM_H
#include "kmessage.h"


#define PROGRAM_ARGSLEN  80
union program_msg {
  struct msg_head h;
  struct {
    struct msg_head h;
    int arg2;
    char file[PROGRAM_FILELEN];
  } load;
  struct  {
    struct msg_head h;
    char s[PROGRAM_ARGLEN];
  } run;
  struct  {
    struct msg_head h;
    unsigned short type;
    unsigned short id;
  } reg;
  struct  {
    struct msg_head h;
    unsigned short taskid;
    unsigned short exitcode;
  } wait;
};

/* console */
int display_init(void);
//void display_setup(void);
int display_putc(int c);
int display_puts(char *str);
//void display_scroll(void);
int display_locatepos(int posx, int posy);
int display_getpos(void);
int display_putpos(int pos);

#endif
