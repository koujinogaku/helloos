#include "environm.h"
#include "display.h"
#include "errno.h"
#include "window.h"
#include "wrect.h"
#include "memory.h"
#include "string.h"
#include "keyboard.h"
#include "list.h"
#include "mouse.h"
#include "winmgr.h"
//#include "message.h"


struct WIN_ID {
  struct WIN *win;
};
struct GC_ID {
  struct GC *gc;
};

struct WIN_EVT {
  struct WIN_EVT *prev;
  struct WIN_EVT *next;
  short evtnum;
  short x;
  short y;
  short width;
  short height;
};

struct WIN {
  struct WIN *next;
  struct WIN *prev;
  short id;
  short qid;
  char status;
  char waitevent;
  struct wrect_driver_info *drv;
  short x;
  short y;
  short width;
  short height;
  struct WIN_EVT *evthead;
  short curgcid;
};

struct GC {
  short fgcolor;
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
static struct GC_ID  *gcidtbl;
static struct wrect_driver_info *windrv;

static int window_mouse_x;
static int window_mouse_y;
//static int window_mouse_status=WIN_STAT_INVISIBLE;
static int window_mouse_button=0;

static int win_queid=0;
static int win_winmgrq=0;
static int win_winmgr_mode=0;
static int win_winmgr_target_winid=0;

//static struct msg_list *win_bufferq=NULL;
//static struct msg_list *win_requestq=NULL;
static char s[16];


void ww_text(int x,int y,char *str)
{
  struct WRECT rect;
  int curcolor;

  rect.x1=0;
  rect.y2=0;
  rect.x2=windrv->width-1;
  rect.y2=windrv->height-1;

  curcolor=windrv->color;
  wrect_set_color(windrv,15);
  wrect_draw_boxfill(windrv,&rect, x, y, (x+strlen(str)*8) ,y+16);
  wrect_set_color(windrv,1);
  wrect_draw_text(windrv,&rect,x,y,str);
  wrect_set_color(windrv,curcolor);
}
void ww_textln(char *str)
{
  static int x=400;
  static int y=2;
  char s[2];

  s[1]=0;
  while(*str!=0) {
    s[0]=*str++;
    if(*s=='\n') {
      x=400;
      y+=20;
      if(y>460) {
        y=2;
      }
    }
    else {
      ww_text(x,y,s);
      x+=8;
    }
  }
}

int window_init(void)
{
  struct WRECT rect;

  winidtbl=malloc(sizeof(struct WIN_ID)*WIN_IDTBLSZ);
  memset(winidtbl,0,sizeof(struct WIN_ID)*WIN_IDTBLSZ);
  winhead=malloc(sizeof(struct WIN));
  list_init(winhead);
  gcidtbl=malloc(sizeof(struct GC_ID)*WIN_IDTBLSZ*2);
  memset(gcidtbl,0,sizeof(struct GC_ID)*WIN_IDTBLSZ*2);

  windrv=malloc(sizeof(struct wrect_driver_info));
  wrect_init(windrv,&rect);

//  window_create(0, 0,windrv->width,windrv->height);
  wrect_draw_wall(windrv,&rect,winwall);
  window_mouse_x= windrv->width /2;
  window_mouse_y= windrv->height /2;

  return 0;
}

int window_shutdown(void)
{
  ww_textln("shutdown\n");
  for(;;);

  wrect_textmode(windrv);
  return 0;
}

int window_gc_create(void)
{
  int gcid=0;

  for(gcid=1;gcid<WIN_IDTBLSZ*2;++gcid)
  {
    if(gcidtbl[gcid].gc==NULL)
      break;
  }

  if(gcid>=WIN_IDTBLSZ*2) {
    return ERRNO_RESOURCE;
  }

  gcidtbl[gcid].gc=malloc(sizeof(struct GC));
  memset(gcidtbl[gcid].gc,0,sizeof(struct GC));

  return gcid;
}
int window_gc_delete(int gcid)
{
  if(gcid<1 || gcid>=WIN_IDTBLSZ*2)
    return ERRNO_CTRLBLOCK;
  
  if(gcidtbl[gcid].gc!=NULL){
    mfree(gcidtbl[gcid].gc);
    gcidtbl[gcid].gc=NULL;
  }
  return 0;
}

int window_gc_change(struct WIN *win,int gcid)
{
  struct GC *gc;

  if(gcid<1 || gcid>=WIN_IDTBLSZ*2) {
ww_textln("chg_gc=");
sint2dec(gcid,s);
ww_textln(s);
ww_textln(",win=");
sint2dec(win->id,s);
ww_textln(s);
ww_textln("\n");
    return ERRNO_CTRLBLOCK;
  }
  gc=gcidtbl[gcid].gc;
  win->curgcid=gcid;
  wrect_set_color(win->drv,gc->fgcolor);
  if(win->status==WIN_STAT_VISIBLE && win->next==winhead)
    wrect_set_color(windrv,gc->fgcolor);
  return 0;
}

struct WIN *window_get_win(int winid)
{
  if(winid<=1 && winid>=WIN_IDTBLSZ) {
ww_textln("get_win=");
sint2dec(winid,s);
ww_textln(s);
ww_textln("\n");
    return NULL;
  }
  if(winidtbl[winid].win==NULL) {
ww_textln("get_win=");
sint2dec(winid,s);
ww_textln(s);
ww_textln("\n");
    return NULL;
  }

  return winidtbl[winid].win;
}

int window_create(int d1, int d2,int width,int height,char *title)
{
  int winid;
  struct WIN *win_new;
  struct WRECT rect;
//  struct WRECT *rect;

//  rect=malloc(sizeof(struct WRECT));

/*
ww_textln("cre_win=");
sint2dec(width,s);
ww_textln(s);
ww_textln(",");
sint2dec(height,s);
ww_textln(s);
ww_textln("\n");
*/

if(mem_check_heapfree()) {
  ww_textln("chk_heap_err\n");
  for(;;);
}


  for(winid=1;winid<WIN_IDTBLSZ;++winid)
  {
    if(winidtbl[winid].win==NULL)
      break;
  }
  if(winid>=WIN_IDTBLSZ) {
    return ERRNO_RESOURCE;
  }

/*
ww_textln("cre_win2=");
sint2dec(winid,s);
ww_textln(s);
ww_textln(",");
sint2dec(sizeof(struct WIN),s);
ww_textln(s);
ww_textln("\n");
*/

if(mem_check_heapfree()) {
  ww_textln("chk_heap_err\n");
  for(;;);
}

  win_new=malloc(sizeof(struct WIN));
  if(win_new==NULL)
  {
ww_textln("cre_win_res_err\n");
    return ERRNO_RESOURCE;
  }
  winidtbl[winid].win=win_new;
  win_new->id=winid;
  win_new->x=0;
  win_new->y=0;
  win_new->width=width;
  win_new->height=height;
  win_new->status=WIN_STAT_INVISIBLE;
  win_new->waitevent=0;
//for(;;);
  win_new->drv=malloc(sizeof(struct wrect_driver_info));
//for(;;);
  if(win_new->drv==NULL)
  {
ww_textln("cre_win_res_drv_err\n");
    return ERRNO_RESOURCE;
  }
//for(;;);
  win_new->evthead=malloc(sizeof(struct list_head));
//for(;;);
  if(win_new->evthead==NULL)
  {
ww_textln("cre_win_res_evthead_err\n");
for(;;);
    return ERRNO_RESOURCE;
  }
//for(;;);
/*
ww_textln("malloc_end\n");
*/
if(mem_check_heapfree()) {
  ww_textln("chk_heap_err\n");
  for(;;);
}
  list_init((win_new->evthead));
  win_new->prev=win_new;
  win_new->next=win_new;
  list_add(winhead,win_new);


  wrect_create_pixmap(win_new->drv,&rect,0,0,width,height);
  wrect_adjust_pixmap(win_new->drv,windrv);

  wrect_set_color(win_new->drv,win_new->drv->colorcode_black);
  wrect_draw_boxfill(win_new->drv,&rect,0,0,rect.x2,rect.y2);
/*
  wrect_create_pixmap(win_new->drv,rect,0,0,width,height);
  wrect_adjust_pixmap(win_new->drv,windrv);

  wrect_set_color(win_new->drv,win_new->drv->colorcode_black);
  wrect_draw_boxfill(win_new->drv,rect,0,0,rect->x2,rect->y2);
*/
/*
ww_textln("cre_end=");
sint2dec(winid,s);
ww_textln(s);
ww_textln("\n");
*/
if(mem_check_heapfree()) {
  ww_textln("chk_heap_err\n");
  for(;;);
}

  return winid;
}

int window_delete(int winid)
{
  struct WIN *win;
  struct WIN_EVT *evt;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  if(win->status==WIN_STAT_VISIBLE)
  {
    window_hide(winid);
  }

  for(;;)
  {
    if(list_empty(win->evthead))
      break;
    evt=win->evthead->prev;
    list_del(evt);
    mfree(evt);
  }
  mfree(win->evthead);

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

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  win->status=WIN_STAT_VISIBLE;
  list_del(win);
  list_add_tail(winhead,win);

  wrect_set_color(windrv,win->drv->color);
  rect.x1=win->x;
  rect.y1=win->y;
  rect.x2=win->x+win->width-1;
  rect.y2=win->y+win->height-1;

  if(rect.x1>=windrv->width)  return 0;
  if(rect.y1>=windrv->height) return 0;
  if(rect.x2<0) return 0;
  if(rect.y2<0) return 0;
  if(rect.x1<0) rect.x1=0;
  if(rect.y1<0) rect.y1=0;
  if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
  if(rect.y2>=windrv->height) rect.y2=windrv->height-1;

  wrect_draw_pixmap(windrv, &rect, 
		win->x,win->y, win->width,win->height, win->drv->videobuff);
  return 0;
}

int window_hide(int winid)
{
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  win->status=WIN_STAT_INVISIBLE;
  list_del(win);
  list_add(winhead,win);

  rect.x1=win->x;
  rect.y1=win->y;
  rect.x2=win->x+win->width-1;
  rect.y2=win->y+win->height-1;
  if(rect.x1>=windrv->width)  return 0;
  if(rect.y1>=windrv->height) return 0;
  if(rect.x2<0) return 0;
  if(rect.y2<0) return 0;
  if(rect.x1<0) rect.x1=0;
  if(rect.y1<0) rect.y1=0;
  if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
  if(rect.y2>=windrv->height) rect.y2=windrv->height-1;

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

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  window_hide(winid);

//  if(x<0) x=0;
//  if(y<0) y=0;
  win->x=x;
  win->y=y;

  window_raise(winid);

  return 0;
}

int window_add_event(int winid, int evtnum, int x, int y,int width,int height)
{
  struct WIN *win;
  struct WIN_EVT *evt;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  evt=malloc(sizeof(struct WIN_EVT));
  evt->x=x;
  evt->y=y;
  evt->evtnum=evtnum;
  evt->width=width;
  evt->height=height;
  list_add_tail(win->evthead,evt);

  return 0;
}

int window_cmd_get_winpos(int clientq, int winid, int *x,int *y, int *width, int *height)
{
  struct WIN *win;
  union win_msg msgres;
  int r;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;
  
    msgres.res.h.size = sizeof(struct win_res_s);
    msgres.res.h.service=WIN_SRV_WINDOW;
    msgres.res.h.command=WIN_CMD_GET_WINPOS;
    msgres.res.h.arg = win->width;
    msgres.res.winid = win->id;
    msgres.res.x =win->x ;
    msgres.res.y =win->y ;
    msgres.res.button = win->height;

    r=syscall_que_put(clientq, &msgres);
    if(r<0)
      return r;

  return 0;
}

int window_set_color(int gcid,int fgcolor)
{
  struct GC *gc;

  if(gcid<1 || gcid>=WIN_IDTBLSZ*2) {
ww_textln("set_gc=");
sint2dec(gcid,s);
ww_textln(s);
ww_textln("\n");
    return ERRNO_CTRLBLOCK;
  }

  gc=gcidtbl[gcid].gc;
  gc->fgcolor=fgcolor;

  return 0;
}
/*
int window_set_bgcolor(int gcid,int bgcolor)
{
  struct GC *gc;

  gc=gcidtbl[gcid].gc;
  gc->bgcolor=bgcolor;

  return 0;
}
*/
int window_get_color(int winid,int defcolor)
{
  struct WIN *win;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  switch(defcolor)
  {
    case WIN_COLOR_FGCOLOR:
      return win->drv->color;
    case WIN_COLOR_BGCOLOR:
      return win->drv->text_bgcolor;
    case WIN_COLOR_WHITE:
      return win->drv->colorcode_white;
    case WIN_COLOR_BLACK:
      return win->drv->colorcode_black;
    case WIN_COLOR_GLAY:
      return win->drv->colorcode_gray;
  }
  return ERRNO_MODE;
}

int window_draw_box(int winid,int gcid,int x1,int y1,int x2,int y2)
{
  int r;
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("draw_box err\n");
    return ERRNO_CTRLBLOCK;
  }

  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("draw_box err\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_box(windrv, &rect, x1+win->x, y1+win->y, x2+win->x, y2+win->y);
  }

  return 0;
}

int window_draw_boxfill(int winid,int gcid,int x1,int y1,int x2,int y2)
{
  int r;
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("boxfil err\n");
    return ERRNO_CTRLBLOCK;
  }
  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("boxfil err\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_boxfill(windrv, &rect, x1+win->x, y1+win->y, x2+win->x, y2+win->y);
  }

  return 0;
}

int window_draw_pixmap(int winid,int x,int y,int w,int h, unsigned char *pixmap)
{
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_pixmap(windrv, &rect, x+win->x, y+win->y, w, h, pixmap);
  }

  return 0;
}

int window_draw_horzline(int winid,int gcid,int x1,int x2,int y)
{
  int r;
  struct WIN *win;
  struct WRECT rect;


  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("draw_hor\n");
    return ERRNO_CTRLBLOCK;
  }
  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("draw_hor\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_horzline(windrv, &rect, x1+win->x, x2+win->x, y+win->y);
  }

  return 0;
}

int window_draw_vertline(int winid,int gcid,int x, int y1, int y2)
{
  int r;
  struct WIN *win;
  struct WRECT rect;


  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("draw_ver\n");
    return ERRNO_CTRLBLOCK;
  }

  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("draw_ver\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_vertline(windrv, &rect, x+win->x, y1+win->y, y2+win->y);
  }

  return 0;
}

int window_draw_pixel(int winid,int gcid,int x,int y)
{
  int r;
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("draw_pix\n");
    return ERRNO_CTRLBLOCK;
  }

  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("draw_pix\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_pixel(windrv, &rect, x+win->x, y+win->y);
  }

  return 0;
}

int window_draw_bitmap(int winid,int gcid,int x, int y,unsigned char *map,int ysize)
{
  int r;
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("draw_bit\n");
    return ERRNO_CTRLBLOCK;
  }
  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("draw_bit\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
    wrect_draw_bitmap(windrv, &rect, x+win->x, y+win->y, map, ysize);
  }

  return 0;
}

int window_draw_text(int winid,int gcid,int x, int y, char *str)
{
  int r;
  struct WIN *win;
  struct WRECT rect;

  win=window_get_win(winid);
  if(win==NULL) {
ww_textln("draw_txt\n");
    return ERRNO_CTRLBLOCK;
  }
  r=window_gc_change(win,gcid);
  if(r<0) {
ww_textln("draw_txt\n");
    return r;
  }

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
    if(rect.x1>=windrv->width)  return 0;
    if(rect.y1>=windrv->height) return 0;
    if(rect.x2<0) return 0;
    if(rect.y2<0) return 0;
    if(rect.x1<0) rect.x1=0;
    if(rect.y1<0) rect.y1=0;
    if(rect.x2>=windrv->width)  rect.x2=windrv->width-1;
    if(rect.y2>=windrv->height) rect.y2=windrv->height-1;
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

struct WIN *window_find_window(int x, int y)
{
  struct WIN *win;

  win=winhead->prev;
  for(;;)
  {
    if(win==winhead)
      return NULL;
    if(win->x <= x && x <= win->x+win->width-1 &&
       win->y <= y && y <= win->y+win->height-1   )
    {
      break;
    }
    win=win->prev;
  }
  return win;
}

struct WIN_EVT *window_find_evt(struct WIN *win,int x, int y)
{
  struct WIN_EVT *evt;

  evt=win->evthead->prev;
  for(;;)
  {
    if(evt==win->evthead)
      return NULL;
    if(evt->x <= x && x <= evt->x+evt->width-1 &&
       evt->y <= y && y <= evt->y+evt->height-1   )
    {
      break;
    }
    evt=evt->prev;
  }
  return evt;
}


int window_initsrv(void)
{
  int r;
//  union kbd_msg msg;
  union mou_msg msgmou;

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
/*
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
*/
//  r=keyboard_request_key(&msg);
//  if(r<0)
//    return r;
  r=mouse_request_code(&msgmou);
  if(r<0)
    return r;

  window_init();

  return 0;
}


int window_get_event(int winid, int *event, int *x,int *y,int *button)
{
  struct WIN *win;

  win=window_get_win(winid);
  if(win==NULL)
    return ERRNO_CTRLBLOCK;

  win->waitevent=1;
  return 0;
}

int window_command(union win_msg *msg)
{
  int r;
  int clientq;
  union win_msg_stext *msgtext;
  int event,ex,ey,btn;

  clientq=msg->req.h.arg;

  switch(msg->req.h.command)
  {
    case WIN_CMD_CREATE:
//ww_textln("win_cre\n");
      msgtext=(union win_msg_stext *)msg;
      r = window_create(0, 0, msgtext->req.x1,msgtext->req.y1,msgtext->req.text);
//ww_textln("win_cre_end=");
//sint2dec(r,s);
//ww_textln(s);
//ww_textln("\n");
//for(;;);
      if(r>=0){
        winidtbl[r].win->qid=clientq;
        if(win_winmgrq!=0 && win_winmgrq!=clientq)
        {
          msgtext->req.h.service=WINMGR_SRV_WINMGR;
          msgtext->req.winid=r;
          syscall_que_put(win_winmgrq, msg);
          msg->req.h.service=WIN_SRV_WINDOW;
ww_textln("win_sndmgr\n");
        }
      }
      else {
ww_textln("win_ope_err\n");
      }
      break;

    case WIN_CMD_DELETE:
      r = window_delete(msg->req.winid);
      if(win_winmgrq!=0 && win_winmgrq!=clientq)
      {
        msg->req.h.service=WINMGR_SRV_WINMGR;
        syscall_que_put(win_winmgrq, msg);
        msg->req.h.service=WIN_SRV_WINDOW;
      }
      break;

    case WIN_CMD_RAISE:
      r = window_raise(msg->req.winid);
      if(win_winmgrq!=0 && win_winmgrq!=clientq)
      {
        msg->req.h.service=WINMGR_SRV_WINMGR;
        syscall_que_put(win_winmgrq, msg);
        msg->req.h.service=WIN_SRV_WINDOW;
      }
      break;

    case WIN_CMD_HIDE:
      r = window_hide(msg->req.winid);
      if(win_winmgrq!=0 && win_winmgrq!=clientq)
      {
        msg->req.h.service=WINMGR_SRV_WINMGR;
        syscall_que_put(win_winmgrq, msg);
        msg->req.h.service=WIN_SRV_WINDOW;
      }
      break;

    case WIN_CMD_MOVE:
      r = window_move(msg->req.winid,msg->req.x1,msg->req.y1);
      if(win_winmgrq!=0 && win_winmgrq!=clientq)
      {
        msg->req.h.service=WINMGR_SRV_WINMGR;
        syscall_que_put(win_winmgrq, msg);
        msg->req.h.service=WIN_SRV_WINDOW;
      }
      break;

    case WIN_CMD_GET_EVENT:
      r = window_get_event(msg->req.winid,&event,&ex,&ey,&btn);
      break;

    case WIN_CMD_ADD_EVENT:
      r = window_add_event(msg->req.winid,msg->req.arg2,msg->req.x1,msg->req.y1,msg->req.x2,msg->req.y2);
      break;

    case WIN_CMD_GET_WINPOS:
      r = window_cmd_get_winpos(clientq, msg->req.winid,&ex,&ey,&event,&btn);
      break;

    case WIN_CMD_REGIST_WINMGR:
      win_winmgrq = clientq;
      r = 0;
      break;

    case WIN_CMD_GC_CREATE:
      r = window_gc_create();
      break;

    case WIN_CMD_GC_DELETE:
      r = window_gc_delete(msg->req.winid);
      break;

    case WIN_CMD_SET_COLOR:
      r = window_set_color(msg->req.winid,msg->req.x1);

      break;
/*
    case WIN_CMD_SET_BGCOLOR:
      r = window_set_bgcolor(msg->req.winid,msg->req.x1);
      break;
*/
    case WIN_CMD_GET_COLOR:
      r = window_get_color(msg->req.winid,msg->req.x1);
      break;

    case WIN_CMD_DRAW_BOX:
      r = window_draw_box(msg->req.winid, msg->req.arg2, msg->req.x1, msg->req.y1, msg->req.x2,msg->req.y2);
      break;

    case WIN_CMD_DRAW_BOXFILL:
      r = window_draw_boxfill(msg->req.winid, msg->req.arg2, msg->req.x1, msg->req.y1, msg->req.x2,msg->req.y2);
      break;

    case WIN_CMD_DRAW_HORZLINE:
      r = window_draw_horzline(msg->req.winid, msg->req.arg2, msg->req.x1, msg->req.x2 ,msg->req.y1);
      break;

    case WIN_CMD_DRAW_VERTLINE:
      r = window_draw_vertline(msg->req.winid, msg->req.arg2, msg->req.x1, msg->req.y1, msg->req.y2);
      break;

    case WIN_CMD_DRAW_PIXEL:
      r = window_draw_pixel(msg->req.winid, msg->req.arg2, msg->req.x1, msg->req.y1);
      break;

    case WIN_CMD_DRAW_TEXT:
      msgtext=(union win_msg_stext *)msg;
      r = window_draw_text(msgtext->req.winid, msgtext->req.arg2, msgtext->req.x1, msgtext->req.y1, msgtext->req.text);
      break;

    default:
      return ERRNO_CTRLBLOCK;
  }

  if(msg->req.h.command!=WIN_CMD_GET_EVENT && msg->req.h.command!=WIN_CMD_GET_WINPOS)
  {
    msg->res.h.arg = r;
    msg->res.h.size = sizeof(struct msg_head);
    r=syscall_que_put(clientq, msg);
    if(r<0)
      return r;
  }
  return 0;
}

int window_mouse(union win_msg *msgv)
{
  union mou_msg *msg=(union mou_msg *)msgv;
  int button,dx,dy,r;
  union mou_msg msgreq;
  unsigned short event,evbtn;
  struct WIN *win;
  union win_msg msgres;
  struct WIN_EVT *evt;
//  int l;
  int queid=0;

  mouse_decode_code(msg,&button,&dx,&dy);
  r=mouse_request_code(&msgreq);
  if(r<0)
    return r;

/*
sint2dec(dx,s);
l=strlen(s); s[l]=' ';s[l+1]=0;
ww_text(500,0,s);
sint2dec(dy,s);
l=strlen(s); s[l]=' ';s[l+1]=0;
ww_text(550,0,s);
*/

  window_mouse_x += dx;
  window_mouse_y += dy;
  if(window_mouse_x<0)
    window_mouse_x=0;
  if(window_mouse_x>=windrv->width)
    window_mouse_x=windrv->width-1;
  if(window_mouse_y<0)
    window_mouse_y=0;
  if(window_mouse_y>=windrv->height)
    window_mouse_y=windrv->height-1;

  event=0;
  evbtn=0;
  if((window_mouse_button&MOU_BTN_LEFT)!=(button&MOU_BTN_LEFT))
  {
    if(button&MOU_BTN_LEFT)
      event=WIN_EVT_MOU_LEFT_DOWN;
    else
      event=WIN_EVT_MOU_LEFT_UP;
  }
  else if((window_mouse_button&MOU_BTN_RIGHT)!=(button&MOU_BTN_RIGHT))
  {
    if(button&MOU_BTN_RIGHT)
      event=WIN_EVT_MOU_RIGHT_DOWN;
    else
      event=WIN_EVT_MOU_RIGHT_UP;
  }
  else if(dx!=0 || dy!=0)
  {
    event=WIN_EVT_MOU_MOVE;
  }

  if(button&MOU_BTN_LEFT)
    evbtn |= WIN_EVT_MOU_STAT_LEFT;
  if(button&MOU_BTN_RIGHT)
    evbtn |= WIN_EVT_MOU_STAT_RIGHT;

  window_mouse_button=button;

  if(win_winmgr_mode)
  {
    msgres.res.h.size = sizeof(struct win_res_s);
    msgres.res.h.service=WINMGR_SRV_WINMGR;
    msgres.res.h.command=WIN_CMD_GET_EVENT;
    msgres.res.h.arg = event;
    msgres.res.winid = win_winmgr_target_winid;
    msgres.res.x =window_mouse_x;
    msgres.res.y =window_mouse_y;
    msgres.res.button = evbtn;
    r=syscall_que_put(win_winmgrq, &msgres);
ww_text(500,0,"move ");
    if((evbtn&WIN_EVT_MOU_STAT_LEFT)==0 && (evbtn&WIN_EVT_MOU_STAT_RIGHT)==0) {
      win_winmgr_mode=0;
ww_text(500,0,"end  ");
}
    if(r<0)
      return r;
    return 0;
  }

  if(event==WIN_EVT_MOU_MOVE && (evbtn&WIN_EVT_MOU_STAT_LEFT)==0 && (evbtn&WIN_EVT_MOU_STAT_RIGHT)==0)
    return 0;
//ww_text(500,0,"ttt");


  win=window_find_window(window_mouse_x,window_mouse_y);
  if(win==NULL)
    return 0;
  if(win!=winhead->prev && event!=WIN_EVT_MOU_LEFT_DOWN)
    return 0;
  queid=win->qid;

//ww_text(500,0,"uuu");

  evt=window_find_evt(win,window_mouse_x-win->x,window_mouse_y-win->y);
  if(evt!=NULL && event==WIN_EVT_MOU_LEFT_DOWN) {
    event = evt->evtnum;
    if(win_winmgrq!=0)
    {
      if(event==WIN_EVT_MOU_CMD_MOVEWIN) {
        win_winmgr_mode=WIN_WINMGR_MODE_MOVE;
        win_winmgr_target_winid=win->id;
      }
    }
  }
  msgres.res.h.size = sizeof(struct win_res_s);
  msgres.res.h.service=WIN_SRV_WINDOW;
  msgres.res.h.command=WIN_CMD_GET_EVENT;
  msgres.res.h.arg = event;
  msgres.res.winid = win->id;
  msgres.res.x =window_mouse_x - win->x ;
  msgres.res.y =window_mouse_y - win->y ;
  msgres.res.button = evbtn;

  if(win_winmgr_mode)
  {
    msgres.res.h.service=WINMGR_SRV_WINMGR;
    msgres.res.x =window_mouse_x;
    msgres.res.y =window_mouse_y;
    r=syscall_que_put(win_winmgrq, &msgres);
ww_text(500,0,"start ");
    if((evbtn&WIN_EVT_MOU_STAT_LEFT)==0 && (evbtn&WIN_EVT_MOU_STAT_RIGHT)==0) {
      win_winmgr_mode=0;
ww_text(500,0,"end  ");
    }
    if(r<0)
      return r;
  }
  else if(win->waitevent)
  {
    r=syscall_que_put(queid, &msgres);
    if(r<0)
      return r;
    win->waitevent=0;
  }

  return 0;
}

int window_keyboard(union win_msg *msg)
{
  return 0;
}

int start(void)
{
  int r;
//  union win_msg_stext msgmax;
  union win_msg *msg;

  msg=malloc(sizeof(union win_msg_stext));

  r=window_initsrv();
  if(r<0) {
    return 255;
  }

//ww_textln("win_init\n");
if(mem_check_heapfree()) {
  ww_textln("chk_heap_err\n");
}

  for(;;)
  {
//  if(syscall_que_trypeeksize(win_queid)==0) {
      window_draw_mouse_cursor(window_mouse_x,window_mouse_y);
//    window_mouse_status=WIN_STAT_VISIBLE;
//  }
    msg->req.h.size=sizeof(union win_msg_stext);
    r=syscall_que_get(win_queid,msg);

    if(mem_check_heapfree()) {
      ww_textln("chk_heap_err\n");
      for(;;);
    }
//  if(window_mouse_status==WIN_STAT_VISIBLE) {
      window_draw_mouse_cursor(window_mouse_x,window_mouse_y);
//    window_mouse_status=WIN_STAT_INVISIBLE;
//  }

//    for(;;);

    if(r<0) {

      ww_textln("que_err\n");
      for(;;);

      break;
    }
/*
word2hex(msg->req.h.service,s);
ww_text(400,0,s);
word2hex(msg->req.h.command,s);
ww_text(450,0,s);
*/
//for(;;);
if(mem_check_heapfree()) {
  ww_textln("msg_switch_bf\n");
  ww_textln("chk_heap_err\n");
  for(;;);
}

//for(;;);

    switch(msg->req.h.service)
    {
    case WIN_SRV_WINDOW:
      r=window_command(msg);
      break;
    case KBD_SRV_KEYBOARD:
      r=window_keyboard(msg);
      break;
    case MOU_SRV_MOUSE:
      r=window_mouse(msg);
      break;

    default:
      r=ERRNO_CTRLBLOCK;
      break;
    }
//    display_puts("end!");    
//    if(r>=0 && win_has_request!=0)
//      r=mouse_response();

    if(r<0) {
      window_shutdown();
      display_puts("*** window terminate ***\n");
      return 255;
    }
  }
  window_shutdown();
  display_puts("*** window terminate ***\n");
  return 0;
}

