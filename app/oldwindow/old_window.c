#include "errno.h"
#include "window.h"
#include "wrect.h"
#include "memory.h"
#include "string.h"
#include "keyboard.h"
#include "list.h"
#include "mouse.h"

#define TEST 1

struct WIN_ID {
  struct WIN *win;
};

struct WIN {
  struct WIN *next;
  struct WIN *prev;
  short id;
  short status;
  struct wrect_driver_info *drv;
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
};

static unsigned char winwall[] = { 0xef, 0xff, 0xfe, 0xff, 0xbf, 0xff, 0xfb, 0xff };
static unsigned char window_mouse_cursor[] = {
   0xe0, 0xfc, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0x3f,
   0x3f, 0x1f, 0x1f, 0x1f, 0x0e, 0x0c, 0x08, 0x00,
   0x00, 0x00, 0x80, 0xf0, 0xfe, 0xfc, 0xf8, 0xf0,
   0xe0, 0xf0, 0xf8, 0x7c, 0x3e, 0x1f, 0x0e, 0x04
};


#define WIN_IDTBLSZ 1024

static struct WIN *winhead;
static struct WIN_ID *winidtbl;
static struct wrect_driver_info *windrv;

static int win_queid=0;
static struct msg_list *win_bufferq=NULL;
static struct msg_list *win_requestq=NULL;
static int win_has_request=0;
static char s[16];

int window_init(void)
{
  struct WRECT rect;

  winidtbl=malloc(sizeof(struct WIN_ID)*WIN_IDTBLSZ);
  memset(winidtbl,0,sizeof(struct WIN_ID)*WIN_IDTBLSZ);
  winhead=malloc(sizeof(struct WIN));
  list_init(winhead);

  windrv=malloc(sizeof(struct wrect_driver_info));
  wrect_init(windrv,&rect);

//  window_create(0, 0,windrv->width,windrv->height);
  wrect_draw_wall(windrv,&rect,winwall);

  return 0;
}

int window_shutdown(void)
{
  wrect_textmode(windrv);
  return 0;
}

int window_create(int x, int y,int width,int height)
{
  int winid;
  struct WIN *win_new;
  struct WRECT rect;

  for(winid=1;winid<WIN_IDTBLSZ;++winid)
  {
    if(winidtbl[winid].win==NULL)
    break;
  }
  if(winid>=WIN_IDTBLSZ) {
    return ERRNO_RESOURCE;
  }

  winidtbl[winid].win=malloc(sizeof(struct WIN));
  win_new=winidtbl[winid].win;
  win_new->id=winid;
  win_new->x=x;
  win_new->y=y;
  win_new->width=width;
  win_new->height=height;
  win_new->status=WIN_STAT_INVISIBLE;
  win_new->drv=malloc(sizeof(struct wrect_driver_info));
  win_new->prev=win_new;
  win_new->next=win_new;
  list_add(winhead,win_new);

  wrect_create_pixmap(win_new->drv,&rect,0,0,width,height);
  wrect_adjust_pixmap(win_new->drv,windrv);

  wrect_set_color(win_new->drv,win_new->drv->colorcode_black);
  wrect_draw_boxfill(win_new->drv,&rect,0,0,rect.x2,rect.y2);

  return winid;
}

int window_delete(int winid)
{
  struct WIN *win;

  win=winidtbl[winid].win;

  if(win->status==WIN_STAT_VISIBLE)
  {
    window_hide(winid);
  }

  list_del(win);
  wrect_delete_pixmap(win->drv);
  mfree(win->drv);
  mfree(win);
  winidtbl[winid].win=NULL;

  return 0;
}

int window_raise(int winid)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  win->status=WIN_STAT_VISIBLE;
  list_del(win);
  list_add_tail(winhead,win);

  wrect_set_color(windrv,win->drv->color);
  rect.x1=win->x;
  rect.y1=win->y;
  rect.x2=win->x+win->width-1;
  rect.y2=win->y+win->height-1;

  wrect_draw_pixmap(windrv, &rect, 
		win->x,win->y, win->width,win->height, win->drv->videobuff);
  return 0;
}

int window_hide(int winid)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  win->status=WIN_STAT_INVISIBLE;
  list_del(win);
  list_add(winhead,win);

  rect.x1=win->x;
  rect.y1=win->y;
  rect.x2=win->x+win->width-1;
  rect.y2=win->y+win->height-1;

  wrect_draw_wall(windrv,&rect,winwall);

  list_for_each(winhead,win)
  {
    if(win->status==WIN_STAT_VISIBLE)
    {
      wrect_draw_pixmap(windrv, &rect, 
		win->x,win->y, win->width,win->height, win->drv->videobuff);
    }
  }
  return 0;
}

int window_move(int winid, int x, int y)
{
  struct WIN *win;

  window_hide(winid);

  win=winidtbl[winid].win;
  win->x=x;
  win->y=y;

  window_raise(winid);

  return 0;
}


int window_set_color(int winid,int fgcolor)
{
  struct WIN *win;

  win=winidtbl[winid].win;
  wrect_set_color(win->drv,fgcolor);
  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    wrect_set_color(windrv,win->drv->color);
  }

  return 0;
}
int window_set_bgcolor(int winid,int fgcolor)
{
  struct WIN *win;

  win=winidtbl[winid].win;
  wrect_set_bgcolor(win->drv,fgcolor);
  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    wrect_set_bgcolor(windrv,win->drv->color);
  }

  return 0;
}

int window_get_color(int winid)
{
  struct WIN *win;

  win=winidtbl[winid].win;
  return win->drv->color;
}
int window_get_color_white(int winid)
{
  struct WIN *win;

  win=winidtbl[winid].win;
  return win->drv->colorcode_white;
}
int window_get_color_black(int winid)
{
  struct WIN *win;

  win=winidtbl[winid].win;
  return win->drv->colorcode_black;
}
int window_get_color_gray(int winid)
{
  struct WIN *win;

  win=winidtbl[winid].win;
  return win->drv->colorcode_gray;
}

int window_draw_box(int winid,int x1,int y1,int x2,int y2)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_box(win->drv, &rect, x1, y1, x2, y2);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_box(windrv, &rect, x1+win->x, y1+win->y, x2+win->x, y2+win->y);
  }

  return 0;
}

int window_draw_boxfill(int winid,int x1,int y1,int x2,int y2)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_boxfill(win->drv, &rect, x1, y1, x2, y2);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_boxfill(windrv, &rect, x1+win->x, y1+win->y, x2+win->x, y2+win->y);
  }

  return 0;
}

int window_draw_pixmap(int winid, int x,int y,int w,int h, unsigned char *pixmap)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_pixmap(win->drv, &rect, x, y, w, h, pixmap);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_pixmap(windrv, &rect, x+win->x, y+win->y, w, h, pixmap);
  }

  return 0;
}

int window_draw_horzline(int winid,int x1,int x2,int y)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_horzline(win->drv, &rect, x1, x2, y);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_horzline(windrv, &rect, x1+win->x, x2+win->x, y+win->y);
  }

  return 0;
}

int window_draw_vertline(int winid, int x, int y1, int y2)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_vertline(win->drv, &rect, x, y1, y2);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_vertline(windrv, &rect, x+win->x, y1+win->y, y2+win->y);
  }

  return 0;
}

int window_draw_pixel(int winid, int x,int y)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_pixel(win->drv, &rect, x, y);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_pixel(windrv, &rect, x+win->x, y+win->y);
  }

  return 0;
}

int window_draw_bitmap(int winid, int x, int y,unsigned char *map,int ysize)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_bitmap(win->drv, &rect, x, y, map, ysize);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_bitmap(windrv, &rect, x+win->x, y+win->y, map, ysize);
  }

  return 0;
}

int window_draw_text(int winid, int x, int y, char *str)
{
  struct WIN *win;
  struct WRECT rect;

  win=winidtbl[winid].win;

  rect.x1=0;
  rect.y1=0;
  rect.x2=win->width-1;
  rect.y2=win->height-1;
  wrect_draw_text(win->drv, &rect, x, y, str);

  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
  {
    rect.x1=win->x;
    rect.y1=win->y;
    rect.x2=win->x+win->width-1;
    rect.y2=win->y+win->height-1;
    wrect_draw_text(windrv, &rect, x+win->x, y+win->y, str);
  }

  return 0;
}
int window_draw_mouse_cursor(int x,int y)
{
  struct WRECT rect;
  int colorbackup;
  rect.x1=0;
  rect.y1=0;
  rect.x2=windrv->width-1;
  rect.y2=windrv->height-1;

  colorbackup=windrv->color;
  wrect_set_color(windrv,windrv->colorcode_white);
  wrect_set_calcmode(windrv,WRECT_GC_MODE_XOR);
  wrect_draw_bitmap(windrv,&rect,x,y,&window_mouse_cursor[0],16);
  wrect_draw_bitmap(windrv,&rect,x+8,y,&window_mouse_cursor[16],16);
  wrect_set_calcmode(windrv,WRECT_GC_MODE_PSET);
  wrect_set_color(windrv,colorbackup);

  return 0;
}

int window_initsrv(void)
{
  int r;
  int data;

  win_queid = environment_getqueid();
  if(win_queid==0) {
    syscall_puts("win_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }
  r = syscall_que_setname(win_queid, WIN_QNM_WINDOW);
  if(r<0) {
    win_queid = 0;
    display_puts("win_init msgq=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }
  win_bufferq = message_create_userqueue();
  if(win_bufferq==NULL) {
    display_puts("win_init bufq\n");
    return ERRNO_RESOURCE;
  }
  win_requestq = message_create_userqueue();
  if(win_requestq==NULL) {
    display_puts("win_init reqq\n");
    return ERRNO_RESOURCE;
  }

  window_init();

  return 0;
}


int window_cmd_getevent(union win_msg *msg)
{
  int r;

  if(win_has_request==0) {
/*
display_puts("setfirst");
int2dec(msg->req.queid,s);
display_puts(s);
*/
   win_has_request=msg->req.queid;
  }
  else {
    r=message_put_userqueue(win_requestq, msg);
    if(r<0) {
      display_puts("getcode putreq=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
  }
  return 0;
}


int mouse_response(void)
{
  int r;
  union win_msg msg;

  if(win_has_request==0)
    return 0;
/*
display_puts("resin");
*/
  for(;;) {
    msg.res.h.size=sizeof(msg);
    r=message_get_userqueue(win_bufferq, &msg);
    if(r==ERRNO_OVER)
      return 0;
    if(r<0) {
      display_puts("resp getkey=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
/*
display_puts("snd=");
word2hex(msg.res.h.cmd,s);
display_puts(s);
*/
    r=syscall_que_put(win_has_request, &msg);
    if(r<0) {
      display_puts("resp sndkey=");
      int2dec(-r,s);
      display_puts(s);
      display_puts(" qid=");
      int2dec(win_has_request,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    msg.req.h.size=sizeof(msg);
    r=message_get_userqueue(win_requestq, &msg);
    if(r==ERRNO_OVER) {
      win_has_request=0;
      return 0;
    }
    if(r<0) {
      display_puts("resp getreq=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    win_has_request=msg.req.queid;
  }
  return 0;
}

int start(void)
{
  int r;
  union win_msg cmd;

  r=mouse_initsrv();
  if(r<0) {
    return 255;
  }

  for(;;)
  {
    cmd.req.h.size=sizeof(cmd);
    r=syscall_que_get(win_queid,&cmd);
    if(r<0) {
      display_puts("cmd receive error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return 255;
    }

//display_puts("srv.cmd=");
//word2hex(cmd.req.h.service,s);
//display_puts(s);
//display_puts(".");
//word2hex(cmd.req.h.command,s);
//display_puts(s);

    switch(cmd.req.h.service<<16 | cmd.req.h.command) {
    case MSG_SRV_KERNEL<<16 | MSG_CMD_KRN_INTERRUPT:
//    display_puts("intr!");
      r=mouse_intr(&cmd);
      break;
    case WIN_SRV_MOUSE<<16 | WIN_CMD_GETCODE:
//    display_puts("get!");
      r=mouse_cmd_getcode(&cmd);
      break;
    default:
      display_puts("cmd number error srv=");
      word2hex(cmd.req.h.service,s);
      display_puts(s);
      display_puts(" sz=");
      word2hex(cmd.req.h.size,s);
      display_puts(s);
      display_puts(" cmd=");
      long2hex(cmd.req.h.command,s);
      display_puts(s);
      display_puts(" arg=");
      long2hex(cmd.req.h.arg,s);
      display_puts(s);
      display_puts("\n");
      r=-1;
      break;
    }
//    display_puts("end!");    
    if(r>=0 && win_has_request!=0)
      r=mouse_response();

    if(r<0) {
      display_puts("*** keyboard terminate ***\n");
      return 255;
    }
  }
  return 0;
}



#if 0
void draw_window(int winid, int wx, int wy, int ww, int wh, char *title)
{
  int fgcolor=window_get_color_black(winid);
  int bgcolor=window_get_color_white(winid);
  int grcolor=window_get_color_gray(winid);

  int titlelen=strlen(title);
  window_set_color(winid,bgcolor);
  window_draw_boxfill(winid,0,0,ww-1,wh-1);
  window_set_color(winid,fgcolor);
  window_draw_box(winid,0,0,ww-2,wh-2);
  window_draw_horzline(winid,0,ww-2,18);
  window_draw_horzline(winid,2,ww-2-2,4);
  window_draw_horzline(winid,2,ww-2-2,6);
  window_draw_horzline(winid,2,ww-2-2,8);
  window_draw_horzline(winid,2,ww-2-2,10);
  window_draw_horzline(winid,2,ww-2-2,12);
  window_draw_horzline(winid,2,ww-2-2,14);
  window_set_color(winid,bgcolor);
  window_draw_boxfill(winid,7,4,19,14);
  window_set_color(winid,fgcolor);
  window_draw_box(winid,8,4,18,14);
  window_draw_horzline(winid,1,ww-2,19);
  window_set_color(winid,grcolor);
  window_draw_horzline(winid,1,ww-1,wh-1);
  window_draw_vertline(winid,ww-1,1,wh-1);
  window_set_color(winid,bgcolor);
  window_draw_boxfill(winid,ww/2-(titlelen*8/2)-3,2,ww/2+(titlelen*8/2)+1,17);
  window_set_color(winid,fgcolor);
  window_set_bgcolor(winid,bgcolor);
  window_draw_text(winid,ww/2-(titlelen*8/2),2,title);

}


int start(void)
{
  int r;
  int winid,winid2;
  int win_x,win_y,win_w,win_h,drv_w,drv_h;
  int button_L,button_R,win_x,win_y,color;
  int button,dx,dy;
  int fgcolor,bgcolor;

  window_init();

  win_x=2;
  win_y=2;
  win_w=300;
  win_h=150;

  drv_w=windrv->width;
  drv_h=windrv->height;

  win_x = drv_w/2;
  win_y = drv_h/2;
  button_L=button_R=0;
  color=1;
  winid =window_create(win_x, win_y, win_w, win_h);
  window_raise(winid);
  draw_window(winid,0,0,win_w,win_h,"Title");

  fgcolor=window_get_color_black(winid);
  bgcolor=window_get_color_white(winid);

  for(;;) {
    char s[16];

    window_draw_mouse_cursor(mou_x,mou_y);
    r=mouse_getcode(&button,&dx,&dy);
    window_draw_mouse_cursor(mou_x,mou_y);
    if(r<0)
      break;
    window_set_color(winid,bgcolor);
    window_draw_boxfill(winid,1,20,50,68);
    window_set_color(winid,fgcolor);

    sint2dec(dx,s);
    window_draw_text(winid,1,20,s);
    sint2dec(dy,s);
    window_draw_text(winid,1,36,s);
    byte2hex(button,s);
    window_draw_text(winid,1,52,s);

    mou_x += dx;
    mou_y += dy;
    if(mou_x<0) mou_x=0;
    if(mou_x>=drv_w) mou_x=drv_w-1;
    if(mou_y<0) mou_y=0;
    if(mou_y>=drv_h) mou_y=drv_h-1;

    window_set_color(winid,color);
    window_draw_pixel(winid,mou_x-win_x,mou_y-win_y);
 
    if(button&0x1) {
      if(button_L==0) {
        button_L=1;
        color++;
        if(color>15) {
          color=1;
        }
        window_set_color(winid,color);
      }
    }
    else {
      button_L=0;
    }
    if(button&0x2) {
      if(button_R==0) {
        button_R=1;
        break;
      }
    }
    else {
      button_R=0;
    }
  }

  window_shutdown();
  return 0;
}
#endif

