#include "config.h"
#include "cpu.h"
#include "syscall.h"
#include "string.h"
#include "display.h"
#include "errno.h"
#include "environm.h"
#include "list.h"

#define ENVIRONM_QNM_ENVIRONM 0x8003
#define ENVIRONM_SRV_ENVIRONM ENVIRONM_QNM_ENVIRONM
#define ENVIRONM_CMD_REGPGM   0x0001
#define ENVIRONM_CMD_REGQUE   0x0001
#define ENVIRONM_ARGLEN 128

union environment_msg {
  struct msg_head h;
  struct  {
    struct msg_head h;
    unsigned short type;
    unsigned short taskid;
    char s[ENVIRONM_ARGLEN];
  } regpgm;
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


struct env_resource {
  struct env_resource *next;
  struct env_resource *prev;
  int type;
  int id;
  char *str;
  struct env_resource *children_head;
};

struct env_resource env_type_head;

static int environment_queid=0;
static char s[16];

int environment_initsrv(void)
{
  int r;

  environment_queid = environment_getqueid();
  if(display_queid==0) {
    syscall_puts("display_init que get error=");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }
  r = syscall_que_setname(environment_queid, ENVIRONM_QNM_ENVIRONM);
  if(r<0) {
    syscall_puts("environment_init que create error=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }
  list_init(&env_type_head);

  return 0;
}

struct env_resource *
environment_find_type( int type )
{
  struct env_resource *resrc;

  list_for_each(&env_type_head,resrc);
  {
    if(resrc->type==type)
      return resrc->chidren_head;
  }
  resrc=malloc(sizeof(struct env_resource));
  if(resrc==NULL) {
    syscall_puts("env findtype malloc error\n");
    return NULL;
  }
  memset(resrc,0,sizeof(struct env_resource));
  resrc->type=type;
  resrc->children_head=malloc(sizeof(struct env_resource));
  if(resrc==NULL) {
    syscall_puts("env findtype malloc error\n");
    return NULL;
  }
  memset(resrc->children_head,0,sizeof(struct env_resource));
  list_init(resrc->children_head)
  resrc>children_head->type=type;

  list_add_tail(&env_type_head,resrc);
  return resrc->children_head;
}

struct env_resource *
environment_find_resource( int type )
{
  struct env_resource *resrc;

}

int display_putc(int chr)
{
//  int i;
  switch(chr) {
  case '\n':
    display_pos = (display_pos/display_size_x + 1)*display_size_x;
    break;
  case '\b':
    if(display_pos>0)
      display_pos--;
    break;
  case '\t':
    display_pos = (display_pos+8)&0xfffffff8;
    if(display_pos>=display_size)
      display_pos=display_size-1;
    break;
  case '\r':
    display_pos = display_pos-(display_pos%display_size_x);
    break;
  case '\v':
    display_pos += display_size_x;
    if(display_pos>=display_size)
      display_pos -= display_size_x;
    break;
  case '\f':
    display_pos = 0;
    break;
  case '\a':
    ;
    break;
  default:
    display_scrn[display_pos].chr = chr;
    display_scrn[display_pos].attr = display_attr;
    ++display_pos;
  }
/*
  if(display_pos >= display_size)
    display_pos = 0;
  if((display_pos%display_size_x)==0){
    for(i=0;i<display_size_x;i++){
      display_scrn[display_pos+i].chr = ' ';
      display_scrn[display_pos+i].attr = display_attr;
    }
  }
*/
  if(display_pos >= display_size) {
    display_scroll();
    display_pos = display_size - display_size_x;
  }
  display_set_cursorpos(display_pos);
  return 0;
}

int display_puts(char *str)
{
  struct msg_head *msg =(void*)((unsigned int)str-sizeof(struct msg_head));
  int i;
/*
syscall_puts("arg=");
int2dec(msg->arg,s);
syscall_puts(s);
*/
  for(i=0; *str!=0 && i<msg->arg; ++i,++str)
    display_putc(*str);
  return 0;
}

int display_locatepos(int posx, int posy)
{
  if(posx >= display_size_x)
    posx = display_size_x - 1;
  if(posy >= display_size_y)
    posy = display_size_y - 1;
  display_pos = posx + posy * display_size_x;
  return 0;
}

int display_getpos(void)
{
  return display_pos;
}
int display_setpos(int pos)
{
  display_pos = pos;
  return 0;
}

int display_response(unsigned short clientq, union display_msg *msg)
{
  int r;

  r=syscall_que_put(clientq, &msg);
  if(r<0) {
    syscall_puts("resp queput error=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts(" qid=");
    int2dec(clientq,s);
    syscall_puts(s);
    syscall_puts("\n");
  }
  return 0;
}


int display_setup(void)
{
  int pos;
/*
	if(binfo->vmode != 2)
		return;
	display_size_x = binfo->scrnx;
	if(display_size_x <= 0)
		display_size_x = 1;
	display_size_y = binfo->scrny;
	if(display_size_y <= 0)
		display_size_y = 1;
	display_scrn   = (void *)binfo->vram;
	display_size = display_size_x * display_size_y;
	display_pos = 0;
	display_attr = 0x0f;
*/
  pos=syscall_getcursor();
/*
      syscall_puts("console pos=");
      int2dec(pos,s);
      syscall_puts(s);
      syscall_puts("\n");
*/
  display_setpos(pos);
  display_set_cursorpos(pos);
  return 0;
}

int start(void)
{
  int r;
  union display_msg cmd;
  short clientq;

  r=display_initsrv();
  if(r<0) {
    return 255;
  }

  display_setup();

  for(;;)
  {
//    cmd.h.size=msg_size(sizeof(cmd));
    cmd.h.size=sizeof(cmd);
    r=syscall_que_get(display_queid,&cmd);
    if(r<0) {
      syscall_puts("que receive error=");
      int2dec(-r,s);
      syscall_puts(s);
      syscall_puts("\n");
      return 255;
    }
/*
syscall_puts("cmd=");
int2dec(cmd.h.command,s);
syscall_puts(s);
*/
    if(cmd.h.service!=DISPLAY_SRV_DISPLAY) {
      syscall_puts("srv name error=");
      word2hex(cmd.h.service,s);
      syscall_puts(s);
      syscall_puts(" sz=");
      word2hex(cmd.h.size,s);
      syscall_puts(s);
      syscall_puts(" cmd=");
      long2hex(cmd.h.command,s);
      syscall_puts(s);
      syscall_puts(" arg=");
      long2hex(cmd.h.arg,s);
      syscall_puts(s);
      syscall_puts("\n");
      continue;
    }
    switch(cmd.h.command) {
    case DISPLAY_CMD_PUTC:
      display_putc(cmd.h.arg);
      break;
    case DISPLAY_CMD_PUTS:
      display_puts(cmd.s.s);
/*
syscall_puts("sz=");
int2dec(cmd.s.h.size,s);
syscall_puts(s);
syscall_puts("arg=");
int2dec(cmd.s.h.arg,s);
syscall_puts(s);
syscall_puts("str=");
byte2hex(cmd.s.s[0],&s[0]);
byte2hex(cmd.s.s[1],&s[2]);
syscall_puts(s);
//for(;;) syscall_wait(100);
*/
      break;
    case DISPLAY_CMD_LOCATE:
      display_locatepos(cmd.h.arg,cmd.n.arg2);
      break;
    case DISPLAY_CMD_GETPOS:
      clientq=cmd.h.arg;
      cmd.h.arg=display_getpos();
//      cmd.h.size=msg_size(sizeof(struct msg_head));
      cmd.h.size=sizeof(struct msg_head);
      display_response(clientq,&cmd);
      break;
    case DISPLAY_CMD_SETPOS:
      display_setpos(cmd.h.arg);
      break;
    default:
      syscall_puts("cmd number error srv=");
      word2hex(cmd.h.service,s);
      syscall_puts(s);
      syscall_puts(" sz=");
      word2hex(cmd.h.size,s);
      syscall_puts(s);
      syscall_puts(" cmd=");
      long2hex(cmd.h.command,s);
      syscall_puts(s);
      syscall_puts(" arg=");
      long2hex(cmd.h.arg,s);
      syscall_puts(s);
      syscall_puts("\n");
      break;
    }
  }
}

