#ifndef DISPLAY_H
#define DISPLAY_H
#include "kmessage.h"
#include "msgquenames.h"

#define DISPLAY_QNM_DISPLAY MSGQUENAMES_DISPLAY
#define DISPLAY_SRV_DISPLAY DISPLAY_QNM_DISPLAY
#define DISPLAY_CMD_PUTC    0x0001
#define DISPLAY_CMD_PUTS    0x0002
#define DISPLAY_CMD_LOCATE  0x0003
#define DISPLAY_CMD_GETPOS  0x0004
#define DISPLAY_CMD_SETPOS  0x0005
#define DISPLAY_CMD_SETATTR 0x0006

#define DISPLAY_STRLEN  80
union display_msg {
  struct msg_head h;
  struct n {
    struct msg_head h;
    unsigned short arg2;
  } n;
  struct s {
    struct msg_head h;
    char s[DISPLAY_STRLEN];
  } s;
};

/* console */
int display_init(void);
//void display_setup(void);
int display_putc(int c);
int display_puts(char *str);
//void display_scroll(void);
int display_locatepos(int posx, int posy);
int display_getpos(void);
int display_setpos(int pos);
int display_setattr(int attr);

#endif
