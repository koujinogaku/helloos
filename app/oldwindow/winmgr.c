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
#include "message.h"
#include "winmgr.h"



int winmgr_queid=0;

int winmgr_target_winid=0;
int winmgr_winx=0;
int winmgr_winy=0;
int winmgr_action=0;
int winmgr_orgx=0;
int winmgr_orgy=0;
int winmgr_gc=0;

static char s[16];


void draw_window(int winid, int gc, int ww, int wh, char *title)
{

  int fgcolor=window_get_color(winid,WIN_COLOR_BLACK);
  int bgcolor=window_get_color(winid,WIN_COLOR_WHITE);
  int grcolor=window_get_color(winid,WIN_COLOR_GLAY);
  int titlelen=strlen(title);

  window_set_color(gc,bgcolor);
  window_draw_boxfill(winid,gc,0,0,ww-1,wh-1);
  window_set_color(gc,fgcolor);
  window_draw_box(winid,gc,0,0,ww-2,wh-2);
  window_draw_horzline(winid,gc,0,ww-2,18);
  window_draw_horzline(winid,gc,2,ww-2-2,4);
  window_draw_horzline(winid,gc,2,ww-2-2,6);
  window_draw_horzline(winid,gc,2,ww-2-2,8);
  window_draw_horzline(winid,gc,2,ww-2-2,10);
  window_draw_horzline(winid,gc,2,ww-2-2,12);
  window_draw_horzline(winid,gc,2,ww-2-2,14);
  window_set_color(gc,bgcolor);
  window_draw_boxfill(winid,gc,7,4,19,14);
  window_set_color(gc,fgcolor);
  window_draw_box(winid,gc,8,4,18,14);
  window_draw_horzline(winid,gc,1,ww-2,19);
  window_set_color(gc,grcolor);
  window_draw_horzline(winid,gc,1,ww-1,wh-1);
  window_draw_vertline(winid,gc,ww-1,1,wh-1);
  window_set_color(gc,bgcolor);
  window_draw_boxfill(winid,gc,ww/2-(titlelen*8/2)-3,2,ww/2+(titlelen*8/2)+1,17);

  window_set_color(gc,fgcolor);
//  window_set_bgcolor(winid,bgcolor);
  window_draw_text(winid,gc,ww/2-(titlelen*8/2),2,title);

}



int winmgr_init(void)
{
  return 0;
}


int winmgr_cmd_create(int winid, int d2,int width,int height,char *title)
{
  draw_window(winid,winmgr_gc,width,height,title);
  window_add_event(winid,WIN_EVT_MOU_CMD_MOVEWIN,0,0,width,20);

  return 0;
}

int winmgr_cmd_delete(int winid)
{
  return 0;
}

int winmgr_cmd_raise(int winid)
{
  return 0;
}

int winmgr_cmd_hide(int winid)
{
  return 0;
}

int winmgr_cmd_move(int winid, int x, int y)
{
  return 0;
}

int winmgr_cmd_get_event(int winid,int event,int x,int y,int button)
{
  int tmpw,tmph;

  switch(event) {
  case WIN_EVT_MOU_CMD_MOVEWIN:
    winmgr_target_winid=winid;
    window_raise(winid);
    window_get_winpos(winid, &winmgr_winx, &winmgr_winy, &tmpw, &tmph);


    sint2dec(event,s);
    window_draw_text(winid,winmgr_gc,1,36,s);
    sint2dec(y,s);
    window_draw_text(winid,winmgr_gc,50,36,s);



    winmgr_action = WIN_EVT_MOU_CMD_MOVEWIN;
    winmgr_orgx = x;
    winmgr_orgy = y;

    break;
  case WIN_EVT_MOU_LEFT_UP:
    winmgr_target_winid=0;
    winmgr_action = 0;
    break;
  case WIN_EVT_MOU_RIGHT_UP:
    winmgr_target_winid=0;
    winmgr_action = 0;
    break;

  case WIN_EVT_MOU_MOVE:
    if(winmgr_action==WIN_EVT_MOU_CMD_MOVEWIN)
    {
      window_move(winmgr_target_winid,winmgr_winx+(x-winmgr_orgx),winmgr_winy+(y-winmgr_orgy));
    }
  }
  return 0;
}

int winmgr_initsrv(void)
{
  int r;

  winmgr_queid = environment_getqueid();
  if(winmgr_queid==0) {
    syscall_puts("win_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }

  r = syscall_que_setname(winmgr_queid, WINMGR_QNM_WINMGR);
  if(r<0) {
    winmgr_queid = 0;
    display_puts("win_init msgq=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  r=window_regist_winmgr();
  if(r<0)
    return r;


  return 0;
}


int winmgr_cmd(union win_msg *msg)
{
  int r;
  int clientq;
  union win_msg_stext *msgtext;

  clientq=msg->req.h.arg;

  switch(msg->req.h.command)
  {
    case WIN_CMD_CREATE:
      msgtext=(union win_msg_stext *)msg;
      r = winmgr_cmd_create(msgtext->req.winid, msgtext->req.arg2, msgtext->req.x1,msgtext->req.y1,msgtext->req.text);
      break;

    case WIN_CMD_DELETE:
      r = winmgr_cmd_delete(msg->req.winid);
      break;

    case WIN_CMD_RAISE:
      r = winmgr_cmd_raise(msg->req.winid);
      break;

    case WIN_CMD_HIDE:
      r = winmgr_cmd_hide(msg->req.winid);
      break;

    case WIN_CMD_MOVE:
      r = winmgr_cmd_move(msg->req.winid,msg->req.x1,msg->req.y1);
      break;

    case WIN_CMD_GET_EVENT:
      r = winmgr_cmd_get_event(msg->res.winid,msg->res.h.arg,msg->res.x,msg->res.y,msg->res.button);
      break;

    default:
      return ERRNO_CTRLBLOCK;
  }

  return 0;
}




int start(void)
{
  int r;
  union win_msg_stext msgmax;
  union win_msg *msg=(union win_msg *)&msgmax;

  r=winmgr_initsrv();
  if(r<0) {
    return 255;
  }

  winmgr_gc=window_gc_create();

  for(;;)
  {
    msg->req.h.size=sizeof(union win_msg_stext);
    r=message_receive(0, 0, 0, msg);
    if(r<0)
      break;

/*
word2hex(msg->req.h.service,s);
w_text(400,0,s);
word2hex(msg->req.h.command,s);
w_text(450,0,s);
*/
    switch(msg->req.h.service)
    {
    case WINMGR_SRV_WINMGR:
      r=winmgr_cmd(msg);
      break;

    default:
      r=ERRNO_CTRLBLOCK;
      break;
    }
//    display_puts("end!");    
//    if(r>=0 && win_has_request!=0)
//      r=mouse_response();

  }

  window_gc_delete(winmgr_gc);

  return 0;
}

