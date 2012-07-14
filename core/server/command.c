#include "syscall.h"
#include "string.h"
#include "keyboard.h"
#include "display.h"
#include "errno.h"
#include "environm.h"
#include "message.h"
#include "memory.h"

#define COMMAND_LINESIZE    127
#define COMMAND_ARGMAX      32
#define COMMAND_PGMNAMEMAX  32
#define COMMAND_BUFFSZ     512

static char s[10];
static char command_batch_args[COMMAND_LINESIZE+1];
static int  command_batch_filep=0;
static char command_readbuf[COMMAND_BUFFSZ];
static int  command_bufpoint=COMMAND_BUFFSZ;
static int  command_bufusesz=0;

inline static int getc(void)
{
  return keyboard_getcode();
}

int command_editline(char *cmdline)
{
  int key;
  int col=0;

  for(;;) {
    key = getc();
    if(key<0)
      return key;
    if(key>=' ' && key<='~' && col<COMMAND_LINESIZE-5) {
      cmdline[col]=key;
      col++;
      display_putc(key);
    }
    else {
      switch(key) {
      case KBS:
        if(col>0) {
          col--;
          display_puts("\b \b");
        }
        break;
      case '\n':
        cmdline[col]=0;
        display_putc('\n');
        return col;
      }
    }
  }
  return 0;
}
int command_readline(char *cmdline)
{
  char c;
  int col=0;
  int r;

  for(;;) 
  {
    if(command_bufpoint>=command_bufusesz)
    {
      r = syscall_file_read(command_batch_filep, command_readbuf, COMMAND_BUFFSZ);
      if(r==ERRNO_OVER) {
        r = syscall_file_close(command_batch_filep);
        if(r<0) {
          display_puts("closebatch error=");
          int2dec(-r,s);
          display_puts(s);
          display_puts("\n");
          return r;
        }
        command_batch_filep=0;
        break;
      }
      if(r<0) {
        display_puts("readdir error=");
        int2dec(-r,s);
        display_puts(s);
        display_puts("\n");
        return r;
      }
      command_bufpoint=0;
      command_bufusesz=r;
    }

    c=command_readbuf[command_bufpoint++];
    if(c=='\n')
      break;
    if(col<COMMAND_LINESIZE-5)
    {
      cmdline[col] = c;
      col++;
    }

  }
  cmdline[col] = 0;
  display_puts(cmdline);
  display_puts("\n");
  
  return col;

}

int command_getcmdline(char *cmdline)
{
  if(command_batch_filep==0) {
    //display_puts("$");
    return command_editline(cmdline);
  }
  else {
    //display_puts("#");
    return command_readline(cmdline);
  }
}

int command_split(char *cmdline, int len, char *argv[])
{
  int i;
  int argc=0;
  int is_arg=0;

  for(i=0;i<len;i++,cmdline++) {
    if(*cmdline==0)
      break;
    if(*cmdline==' ') {
      *cmdline=0;
      is_arg=0;
      if(argc>=COMMAND_ARGMAX)
        break;
    }
    else {
      if(is_arg==0) {
        argv[argc++]=cmdline;
        is_arg=1;
      }
    }
  }
  return argc;
}


int command_exec(int argc, char *argv[])
{
  static char pgmname[COMMAND_PGMNAMEMAX];
  int len,n,i;
  int r;
  int taskid;
  int queid;
  char userargs[COMMAND_LINESIZE+1];

  len=strlen(argv[0]);
  if(len>=COMMAND_PGMNAMEMAX-6) {
    return ERRNO_NOTEXIST;
  }
  strncpy(pgmname, argv[0], COMMAND_PGMNAMEMAX-6);
  strncpy(&pgmname[len],".out",5);
/*
  display_puts("filename=[");
  display_puts(pgmname);
  display_puts("]\n");
*/

  r=syscall_pgm_load(pgmname, SYSCALL_PGM_TYPE_VGA|SYSCALL_PGM_TYPE_IO);
  if(r==ERRNO_NOTEXIST) {
    return r;
  }
  if(r<0) {
    display_puts("load error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }
  taskid=r;

  n=0;
  memset(userargs,0,COMMAND_LINESIZE+1);
  for(i=0;i<argc;i++) {
    len=strlen(argv[i]);
    strncpy(&userargs[n],argv[i],COMMAND_LINESIZE-n);
    n=n+len+1;
    if(n>=COMMAND_LINESIZE)
      break;
  }
  r = syscall_pgm_setargs(taskid, userargs, n);
  if(r<0) {
    display_puts("setargs error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  queid = environment_getqueid();
  r=syscall_pgm_start(taskid, queid);
  if(r<0) {
    display_puts("start error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  return 0;
}

int command_wait(tryflg)
{
  struct msg_head msg;
  int taskid;
  int exitcode;
  int r;

  msg.size=sizeof(msg);
  r=message_receive(tryflg, MSG_SRV_KERNEL, MSG_CMD_KRN_EXIT, &msg);
  if(r==ERRNO_OVER)
    return r;

  if(r<0) {
    display_puts("wait que error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  taskid=msg.arg;
  exitcode = syscall_pgm_delete(taskid);
  if(exitcode<0) {
    display_puts("wait del error=");
    int2dec(-exitcode,s);
    display_puts(s);
    display_puts("\n");
    return exitcode;
  }

  return exitcode;
}

int command_builtin(int argc, char *argv[])
{
  //if(strncmp(argv[0],"mem", 4)==0) {
  //  memory_dumpfree(0);
  //  return 0;
  //}

  return ERRNO_NOTEXIST;

}

int command_batch_open(int argc, char *argv[])
{
  int r,i,n,len;
  static char pgmname[COMMAND_PGMNAMEMAX];

  len=strlen(argv[0]);
  if(len>=COMMAND_PGMNAMEMAX-6) {
    return ERRNO_NOTEXIST;
  }
  strncpy(pgmname, argv[0], COMMAND_PGMNAMEMAX-6);
  strncpy(&pgmname[len],".bat",5);
  r = syscall_file_open(pgmname,SYSCALL_FILE_O_RDONLY);
  if(r==ERRNO_NOTEXIST) {
    return r;
  }
  if(r<0) {
    display_puts("open error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  if( command_batch_filep != 0) {
    r = syscall_file_close(command_batch_filep);
    if(r<0) {
      display_puts("closebatch error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      command_batch_filep=0;
      return r;
    }
  }

  n=0;
  memset(command_batch_args,0,COMMAND_LINESIZE+1);
  for(i=0;i<argc;i++) {
    len=strlen(argv[i]);
    strncpy(&command_batch_args[n],argv[i],COMMAND_LINESIZE-n);
    n=n+len+1;
    if(n>=COMMAND_LINESIZE)
      break;
  }

  command_batch_filep=r;
  command_bufpoint=COMMAND_BUFFSZ;
  command_bufusesz=0;
  return 0;
}


int start(void)
{
  static char cmdline[COMMAND_LINESIZE];
  static char *argv[COMMAND_ARGMAX];
  int argc;
  int len;
//  int i;
  int r;
  int exitcode;
  int bg;

  display_puts("hello world\n");

  for(;;) {
    display_puts("o(^^)o>");
    cmdline[0]=0;
    len=command_getcmdline(cmdline);
    if(len<0) {
      display_puts("editline error=");
      int2dec(-len,s);
      display_puts(s);
      display_puts("\n");
      break;
    }

    exitcode=command_wait(MESSAGE_MODE_TRY);
    if(exitcode>0) {
      display_puts("program terminate exitcode=");
      int2dec(exitcode,s);
      display_puts(s);
      display_puts("\n");
    }
/*
    display_puts("command=[");
    display_puts(cmdline);
    display_puts("] len=");
    int2dec(len,s);
    display_puts(s);
    display_puts("\n");
*/
    argc=command_split(cmdline, len, argv);
/*
    display_puts("split argc=");
    int2dec(argc,s);
    display_puts(s);
    for(i=0;i<argc;i++) {
      display_puts(" argv[");
      int2dec(i,s);
      display_puts(s);
      display_puts("]=[");
      display_puts(argv[i]);
      display_puts("]");
    }
    display_puts("\n");
*/
    if(argc<=0)
      continue;

    if(strncmp(argv[argc-1],"&",2)==0) {
      bg=1;
      argc--;
      if(argc<=0)
        continue;
    }
    else {
      bg=0;
    }

    r=command_builtin(argc, argv);
    if(r==0)
      continue;

    r=command_exec(argc, argv);
    if(r==0) {

      if(bg==1)
        continue;

      exitcode=command_wait(MESSAGE_MODE_WAIT);
      if(exitcode>0) {
        display_puts("program terminate exitcode=");
        int2dec(exitcode,s);
        display_puts(s);
        display_puts("\n");
      }
      continue;
    }

    r=command_batch_open(argc, argv);
    if(r==0)
      continue;

    if(r==ERRNO_NOTEXIST) {
      display_puts("command not found\n");
    }

  }

  display_puts("end command.out\n");
  return 0;

}
