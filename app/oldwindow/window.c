#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "display.h"
#include "environm.h"
#include "message.h"
#include "window.h"

static unsigned short window_queid=0;
static unsigned short window_clientq=0;
//static char s[10];

int window_init(void)
{
  int r;

  if(window_queid != 0)
    return 0;

  for(;;) {
    r = syscall_que_lookup(WIN_QNM_WINDOW);
    if(r==ERRNO_NOTEXIST) {
      syscall_wait(10);
      continue;
    }
    if(r<0) {
      return r;
    }
    window_queid = r;

    window_clientq=environment_getqueid();
    break;
  }
  return 0;
}

static int window_sndcmd4(int cmd,int winid,int arg2,int x1,int y1,int x2,int y2)
{
  union win_msg msg;
  int r;

  if(window_queid==0) {
    r=window_init();
    if(r<0)
      return r;
  }

  msg.req.h.size=msg_sizeof(struct win_req_s);
  msg.req.h.service=WIN_SRV_WINDOW;
  msg.req.h.command=cmd;
  msg.req.h.arg=window_clientq;
  msg.req.winid=winid;
  msg.req.arg2=arg2;
  msg.req.x1=x1;
  msg.req.y1=y1;
  msg.req.x2=x2;
  msg.req.y2=y2;
  for(;;) {
    r=message_send(window_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    return r;
  }

  msg.res.h.size=msg_sizeof(struct msg_head);
  r=message_receive(0,WIN_SRV_WINDOW, cmd, &msg);
  if(r<0) {
    return r;
  }

  return msg.res.h.arg;
}


static int window_sndcmd2(int cmd, int winid, int x, int y)
{
  union win_msg msg;
  int r;

  if(window_queid==0) {
    r=window_init();
    if(r<0)
      return r;
  }

  msg.req.h.size=msg_sizeof(struct win_req_s2);
  msg.req.h.service=WIN_SRV_WINDOW;
  msg.req.h.command=cmd;
  msg.req.h.arg=window_clientq;
  msg.req.winid=winid;
  msg.req.x1=x;
  msg.req.y1=y;
  for(;;) {
    r=message_send(window_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    return r;
  }

  msg.res.h.size=msg_sizeof(struct msg_head);
  r=message_receive(0,WIN_SRV_WINDOW, cmd, &msg);
  if(r<0) {
    return r;
  }

  return msg.res.h.arg;
}

static int window_sndcmd1(int cmd, int winid,int fgcolor)
{
  union win_msg msg;
  int r;

  if(window_queid==0) {
    r=window_init();
    if(r<0)
      return r;
  }

  msg.req.h.size=msg_sizeof(struct win_req_s1);
  msg.req.h.service=WIN_SRV_WINDOW;
  msg.req.h.command=cmd;
  msg.req.h.arg=window_clientq;
  msg.req.winid=winid;
  msg.req.x1=fgcolor;
  for(;;) {
    r=message_send(window_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    return r;
  }

  msg.res.h.size=msg_sizeof(struct msg_head);
  r=message_receive(0,WIN_SRV_WINDOW, cmd, &msg);
  if(r<0) {
    return r;
  }

  return msg.res.h.arg;
}

static int window_sndcmd0(int cmd,int winid)
{
  union win_msg msg;
  int r;

  if(window_queid==0) {
    r=window_init();
    if(r<0)
      return r;
  }

  msg.req.h.size=msg_sizeof(struct win_req_s0);
  msg.req.h.service=WIN_SRV_WINDOW;
  msg.req.h.command=cmd;
  msg.req.h.arg=window_clientq;
  msg.req.winid=winid;
  for(;;) {
    r=message_send(window_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    return r;
  }

  msg.res.h.size=msg_sizeof(struct msg_head);
  r=message_receive(0,WIN_SRV_WINDOW, cmd, &msg);
  if(r<0) {
    return r;
  }

  return msg.res.h.arg;
}

int window_sndtext(int cmd,int winid,int gc,int x, int y, char *str)
{
  static union win_msg_stext msg;
  short len;
  int r;

  if(window_queid==0) {
    r=window_init();
    if(r<0)
      return r;
  }

  len=strlen(str);
  if(len>=WIN_STRLEN-1)
    len=WIN_STRLEN-1;

  msg.req.h.size=msg_size(sizeof(struct win_req_stext)-WIN_STRLEN+len+5);
  msg.req.h.service=WIN_SRV_WINDOW;
  msg.req.h.command=cmd;
  msg.req.h.arg=window_clientq;
  msg.req.winid=winid;
  msg.req.x1=x;
  msg.req.y1=y;
  msg.req.arg2=gc;
  strncpy(msg.req.text, str, WIN_STRLEN-1);
  msg.req.text[len]=0;
  for(;;) {
    r=message_send(window_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    return r;
  }

  msg.res.h.size=msg_sizeof(struct msg_head);
  r=message_receive(0,WIN_SRV_WINDOW, cmd, &msg);
  if(r<0) {
    return r;
  }

  return msg.res.h.arg;
}

int window_create(int d1,int d2,int width,int height,char *title)
{
//  return window_sndcmd4(WIN_CMD_CREATE,0,0,x,y,width,height);
  return window_sndtext(WIN_CMD_CREATE,0,0,width,height,title);
}

int window_delete(int winid)
{
  return window_sndcmd0(WIN_CMD_DELETE,winid);
}

int window_raise(int winid)
{
  return window_sndcmd0(WIN_CMD_RAISE,winid);
}

int window_hide(int winid)
{
  return window_sndcmd0(WIN_CMD_HIDE,winid);
}

int window_move(int winid, int x, int y)
{
  return window_sndcmd2(WIN_CMD_MOVE,winid,x,y);
}

int window_add_event(int winid,int evtnum,int x1,int y1,int x2,int y2)
{
  return window_sndcmd4(WIN_CMD_ADD_EVENT,winid,evtnum,x1,y1,x2,y2);
}

int window_gc_create(void)
{
  return window_sndcmd0(WIN_CMD_GC_CREATE,0);
}
int window_gc_delete(int gcid)
{
  return window_sndcmd0(WIN_CMD_GC_DELETE,gcid);
}

int window_set_color(int gcid,int fgcolor)
{
  return window_sndcmd1(WIN_CMD_SET_COLOR,gcid,fgcolor);
}
/*
int window_set_bgcolor(int gcid,int bgcolor)
{
  return window_sndcmd1(WIN_CMD_SET_BGCOLOR,winid,bgcolor);
}
*/
int window_get_color(int winid,int defcolor)
{
  return window_sndcmd1(WIN_CMD_GET_COLOR,winid,defcolor);
}

int window_draw_box(int winid,int gc,int x1,int y1,int x2,int y2)                                                     
{
  return window_sndcmd4(WIN_CMD_DRAW_BOX,winid,gc,x1,y1,x2,y2);
}

int window_draw_boxfill(int winid,int gc,int x1,int y1,int x2,int y2)
{
  return window_sndcmd4(WIN_CMD_DRAW_BOXFILL,winid,gc,x1,y1,x2,y2);
}

int window_draw_horzline(int winid,int gc,int x1,int x2,int y)
{
  return window_sndcmd4(WIN_CMD_DRAW_HORZLINE,winid,gc,x1,y,x2,0);
}

int window_draw_vertline(int winid,int gc,int x, int y1, int y2)
{
  return window_sndcmd4(WIN_CMD_DRAW_VERTLINE,winid,gc,x,y1,0,y2);
}

int window_draw_pixel(int winid,int gc,int x,int y)
{
  return window_sndcmd4(WIN_CMD_DRAW_PIXEL,winid,gc,x,y,0,0);
}

int window_draw_text(int winid,int gc,int x, int y, char *str)
{
  return window_sndtext(WIN_CMD_DRAW_TEXT,winid, gc, x, y, str);
}

int window_sndcmdres(int cmd, int winid, int *event, int *x,int *y, int *button)
{
  union win_msg msg;
  int r;

  if(window_queid==0) {
    r=window_init();
    if(r<0)
      return r;
  }

  msg.req.h.size=msg_sizeof(struct win_req_s0);
  msg.req.h.service=WIN_SRV_WINDOW;
  msg.req.h.command=cmd;
  msg.req.h.arg=window_clientq;
  msg.req.winid=winid;
  for(;;) {
    r=message_send(window_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    return r;
  }

  msg.res.h.size=msg_sizeof(struct win_res_s);
  r=message_receive(0,WIN_SRV_WINDOW, cmd, &msg);
  if(r<0) {
    return r;
  }

  *event=msg.res.h.arg;
  *x=msg.res.x;
  *y=msg.res.y;
  *button=msg.res.button;

  return 0;
}

int window_get_event(int winid, int *event, int *x,int *y, int *button)
{
  return window_sndcmdres(WIN_CMD_GET_EVENT, winid, event, x, y, button);
}
int window_get_winpos(int winid, int *x,int *y, int *width, int *height)
{
  return window_sndcmdres(WIN_CMD_GET_WINPOS, winid, width, x, y, height);
}

int window_regist_winmgr(void)
{
  return window_sndcmd0(WIN_CMD_REGIST_WINMGR,0);
}
