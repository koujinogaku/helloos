#ifndef WINDOW_H
#define WINDOW_H
#include "kmessage.h"
#include "msgquenames.h"

#define WIN_QNM_WINDOW  MSGQUENAMES_WINDOW
#define WIN_SRV_WINDOW  WIN_QNM_WINDOW

#define WIN_CMD_CREATE		0x0001
#define WIN_CMD_DELETE		0x0002
#define WIN_CMD_SHUTDOWN	0x0003
#define WIN_CMD_RAISE		0x0004
#define WIN_CMD_HIDE		0x0005
#define WIN_CMD_MOVE		0x0006
#define WIN_CMD_GET_EVENT	0x0007
#define WIN_CMD_ADD_EVENT       0x0008
#define WIN_CMD_GET_WINPOS      0x0009

#define WIN_CMD_GC_CREATE	0x000A
#define WIN_CMD_GC_DELETE	0x000B
#define WIN_CMD_SET_COLOR	0x000C
#define WIN_CMD_SET_BGCOLOR	0x000D
#define WIN_CMD_GET_COLOR	0x000E

#define WIN_CMD_DRAW_BOX	0x000F
#define WIN_CMD_DRAW_BOXFILL	0x0010
#define WIN_CMD_DRAW_PIXMAP	0x0011
#define WIN_CMD_DRAW_HORZLINE	0x0012
#define WIN_CMD_DRAW_VERTLINE	0x0013
#define WIN_CMD_DRAW_PIXEL	0x0014
#define WIN_CMD_DRAW_BITMAP	0x0015
#define WIN_CMD_DRAW_TEXT	0x0016
#define WIN_CMD_REGIST_WINMGR	0x0017

#define WIN_COLOR_FGCOLOR	0x0001
#define WIN_COLOR_BGCOLOR	0x0002
#define WIN_COLOR_WHITE		0x0003
#define WIN_COLOR_BLACK		0x0004
#define WIN_COLOR_GLAY		0x0005

#define WIN_STRLEN  80


#define WIN_EVT_MOU_LEFT_DOWN	0x0001
#define WIN_EVT_MOU_RIGHT_DOWN	0x0002
#define WIN_EVT_MOU_LEFT_UP	0x0003
#define WIN_EVT_MOU_RIGHT_UP	0x0004
#define WIN_EVT_MOU_MOVE	0x0005

#define WIN_EVT_MOU_CMD_MOVEWIN	0x0006


#define WIN_EVT_MOU_STAT_LEFT	0x0100
#define WIN_EVT_MOU_STAT_RIGHT	0x0200

#define WIN_WINMGR_MODE_MOVE	0x0001

union win_msg {
  struct win_req_s {
    struct msg_head h;
    unsigned short winid;
    short x1;
    short y1;
    short x2;
    short y2;
    short arg2;
  } req;
  struct win_req_s0 {
    struct msg_head h;
    unsigned short winid;
  } req0;
  struct win_req_s1 {
    struct msg_head h;
    unsigned short winid;
    short x1;
  } req1;
  struct win_req_s2 {
    struct msg_head h;
    unsigned short winid;
    short x1;
    short y1;
    short arg2;
  } req2;
  struct win_res_s {
    struct msg_head h;
    unsigned short winid;
    short x;
    short y;
    unsigned short button;
  } res;
};
union win_msg_stext {
  struct win_req_stext {
    struct msg_head h;
    unsigned short winid;
    short x1;
    short y1;
    short arg2;
    char text[WIN_STRLEN];
  } req;
  struct win_res_stext {
    struct msg_head h;
    unsigned short winid;
    short x;
    short y;
    unsigned short button;
  } res;
};


#define WIN_STAT_INVISIBLE 0
#define WIN_STAT_VISIBLE   1

int window_init(void);

int window_create(int d1,int d2,int width,int height,char *title);
int window_delete(int winid);
int window_shutdown(void);
int window_raise(int winid);
int window_hide(int winid);
int window_move(int winid, int x, int y);

int window_get_event(int winid,int *event,int *x,int *y,int *button);
int window_add_event(int winid,int evtnum,int x1,int y1,int x2,int y2);
int window_get_winpos(int winid, int *x,int *y, int *width, int *height);
int window_regist_winmgr(void);

int window_gc_create(void);
int window_gc_delete(int gcid);
int window_set_color(int gcid,int fgcolor);
//int window_set_bgcolor(int gcid,int bgcolor);
int window_get_color(int gcid,int defcolor);

int window_draw_box(int winid,int gc,int x1,int y1,int x2,int y2);
int window_draw_boxfill(int winid,int gc,int x1,int y1,int x2,int y2);
int window_draw_pixmap(int winid,int x,int y,int w,int h, unsigned char *pixmap);
int window_draw_horzline(int winid,int gc,int x1,int x2,int y);
int window_draw_vertline(int winid,int gc,int x, int y1, int y2);
int window_draw_pixel(int winid,int gc,int x,int y);
int window_draw_bitmap(int winid,int gc,int x, int y,unsigned char *map,int ysize);
int window_draw_text(int winid,int gc,int x, int y, char *str);


#endif
