#include "cdfs.h"
#include "display.h"
#include "string.h"
#include "config.h"
#include "block.h"
#include "print.h"
#include "keyboard.h"
#include "environm.h"
#include "errno.h"
#include "memory.h"
#include "excptdmp.h"
#include "message.h"

static char s[16];

void dump(char *addr)
{
  int i,j;
  for(j=0;j<2;j++) {
    long2hex((unsigned long)addr,s);
    display_puts(s);
    for(i=0;i<16;i++,addr++) {
      byte2hex(*addr,s);
      display_puts(" ");
      display_puts(s);
    }
    display_puts("\n");
    syscall_wait(50);
  }

}

void dsp_open_error(int errno)
{
  switch(errno) {
  case ERRNO_NOTEXIST:
    display_puts("Not found\n");
    break;
  case ERRNO_SCSI_NotReady:
    display_puts("Drive is not ready\n");
    break;
  default:
    print_format("Error(%d)\n",errno);
  }
}

void usage(void)
{
    display_puts("usage: tstcdfs command fullpath\n");
    display_puts("commands:\n");
    display_puts("type - display file\n");
    display_puts("dir  - list directory\n");
}

int cmd_type(int ac, char **av)
{
  int rc;
  int handle=0;
  block_t buf_blk=0;
  block_t blk;
  int readbyte;
  unsigned long pos;

//display_puts("cdfs_open\n");
  handle = cdfs_open(av[0]);
  if(handle<0) {
    dsp_open_error(handle);
    goto EXITCMD;
  }

//display_puts("block_init\n");
  blk = block_init(BLOCK_SHM_ID);
  if(blk==0) {
    display_puts("error:block_open\n");
    goto EXITCMD;
  }

//display_puts("block_alloc\n");
  buf_blk = block_alloc(blk);
  if(buf_blk==0) {
    display_puts("rc=");
    sint2dec(handle,s);
    display_puts(s);
    goto EXITCMD;
  }

  char *buf=block_addr(blk,buf_blk);

//display_puts("cdfs_read\n");
  pos=0;
  readbyte=cdfs_read(handle,pos,BLOCK_SHM_ID,buf_blk,BLOCK_SIZE);
  if(readbyte<0) {
    display_puts("rc=");
    sint2dec(readbyte,s);
    display_puts(s);
    goto EXITCMD;
  }


//  display_puts("\n");
//syscall_wait(10);
  //dump(buf);

  int i;
  for(i=0;i<readbyte;i++) {
    display_putc(*buf);
//syscall_wait(10);
    buf++;
  }

//display_puts("\n");
//display_puts("-------\n");
//display_puts("len=");
//sint2dec(readbyte,s);
//display_puts(s);
//display_puts("\n");
//syscall_wait(50);


EXITCMD:

  if(handle>0) {
//display_puts("cdfs_close\n");
    rc = cdfs_close(handle);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
    }
  }

  if(buf_blk) {
    block_free(blk,buf_blk);
  }

  return 0;
}


int cmd_dir(int ac, char **av)
{
  int handle=0;
  int rc;
  char filter=0;
  char paging=0;
  char all_list=0;

//display_puts("av[2]=");
//display_puts(av[0]);

  for(;;) {
    if(strncmp(av[0],"-d",3)==0) {
      filter = 'D';
    }
    else if(strncmp(av[0],"-p",3)==0) {
      paging = 1;
    }
    else if(strncmp(av[0],"-a",3)==0) {
      all_list = 1;
    }
    else {
      break;
    }

    av++;
    ac--;
    if(ac<1) {
      usage();
      return 0;
    }

  }

//display_puts("cdfs_open directory=");
//display_puts(av[0]);

//display_puts("all_list=");
//sint2dec(all_list,s);
//display_puts(s);

//display_puts("\n");

  handle = cdfs_open(av[0]);
  if(handle<0) {
    dsp_open_error(handle);
    goto EXITCMD;
  }

//  int i;
//  for(i=0;i<3;i++) {
  char linecnt=0;
  for(;;) {
    int rc;
    char type;
    char buf[sizeof(struct file_info)+CDFS_FULLPATH_MAXLENGTH];
    struct file_info *info=(void*)buf;
    rc = cdfs_readdir(handle,info,sizeof(buf));
    if(rc<0)
      break;
    if(info->attr & CDFS_ATTR_DIRECTORY)
      type='D';
    else
      type=' ';

    if((!(info->attr & CDFS_ATTR_HIDDEN)) || all_list) {
      if(filter==0 || filter==type) {
        print_format("%c %9d %s\n",type,info->size,info->name);
        linecnt++;
//syscall_wait(10);
      }
    }
    if(linecnt>20) {
      linecnt=0;
      if(paging) {
        char key;
        display_puts("--page(q=quit)--\n");
        key=keyboard_getcode();
        if(key=='q')
          break;
      }
    }
  }

EXITCMD:
  if(handle>0) {
//display_puts("cdfs_close\n");
    rc = cdfs_close(handle);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
    }
  }

  return 0;
}

int cmd_stat(int ac, char **av)
{
  int rc;
  int handle;
  char type;
  char buf[sizeof(struct file_info)+CDFS_FULLPATH_MAXLENGTH];
  struct file_info *info=(void*)buf;

//display_puts("cdfs_open\n");
  handle = cdfs_open(av[0]);
  if(handle<0) {
    dsp_open_error(handle);
    goto EXITCMD;
  }

//display_puts("cdfs_stat\n");
  rc = cdfs_stat(handle,info,sizeof(buf));
  if(rc<0) {
    display_puts("rc=");
    sint2dec(rc,s);
    display_puts(s);
    goto EXITCMD;
  }


  if(info->attr & CDFS_ATTR_DIRECTORY)
    type='D';
  else
    type='F';

  unsigned char *datestr=info->mtime;
  print_format("type=%c(%02x) date=%d-%02d-%02d %02d:%02d:%02d size=%d\n",type,info->attr,1900+datestr[0],datestr[1],datestr[2],datestr[3],datestr[4],datestr[5],info->size);

EXITCMD:
  if(handle>0) {
//display_puts("cdfs_close\n");
    rc = cdfs_close(handle);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
    }
  }

  return 0;
}

char *get_filename(char *fullpath)
{
  int idx;
  idx=strlen(fullpath);
  if(idx==0)
    return fullpath;

  for(;;) {
    if(idx-1<0)
      break;
    if(fullpath[idx-1]=='/')
      break;
    idx--;
  }

  return &fullpath[idx];
}


static int program_exec(int taskid,int argc, char *argv[])
{
  char *session;

  session=environment_copy_session();
  environment_make_args(session, argc, argv);
  taskid = environment_execimage(taskid, session);
  mfree(session);
  if(taskid==ERRNO_NOTEXIST)
    return taskid;
  if(taskid<0) {
    display_puts("exec error error=");
    int2dec(-taskid,s);
    display_puts(s);
    display_puts("\n");
    return taskid;
  }

  return taskid;
}

static int program_wait(int *exitcode, int tryflg)
{
  int taskid;
  char *userargs;
  int usersize;

  usersize = environment_get_session_size();
  userargs = malloc(usersize);
  if(userargs==0)
    return ERRNO_RESOURCE;

  taskid=environment_wait(exitcode, tryflg, userargs, usersize);
  if(taskid==ERRNO_OVER) {
    mfree(userargs);
    return taskid;
  }

  if(taskid<0) {
    display_puts("wait error=");
    int2dec(-taskid,s);
    display_puts(s);
    display_puts("\n");
    mfree(userargs);
    return taskid;
  }

  if(*exitcode & 0x8000) {
    exception_dump((void*)userargs, display_puts);
  }
  mfree(userargs);
  return taskid;
}

int cmd_exec(int ac, char **av)
{
  int handle=0;
  int pgmid=0;
  block_t buf_blk=0;
  int rc;
  block_t blk;
  int readbyte;
  unsigned long pos;

//display_puts("cdfs_open\n");
  handle = cdfs_open(av[0]);
  if(handle<0) {
    dsp_open_error(handle);
    goto EXITCMD;
  }

//display_puts("cdfs_stat\n");
  char infobuf[sizeof(struct file_info)+CDFS_FULLPATH_MAXLENGTH];
  struct file_info *info=(void*)infobuf;
  rc = cdfs_stat(handle,info,sizeof(infobuf));
  if(rc<0) {
    display_puts("rc=");
    sint2dec(rc,s);
    display_puts(s);
    goto EXITCMD;
  }

//display_puts("block_init\n");
  blk = block_init(BLOCK_SHM_ID);
  if(blk==0) {
    display_puts("error:block_init\n");
    goto EXITCMD;
  }

//display_puts("block_alloc\n");
  buf_blk = block_alloc(blk);
  if(buf_blk==0) {
    display_puts("error:block_alloc\n");
    goto EXITCMD;
  }

  char *buf=block_addr(blk,buf_blk);

  char *pgname = get_filename(av[0]);
//display_puts("pgm_alloc\n");
  pgmid = environment_allocimage(pgname,info->size);
  if(pgmid<0) {
    display_puts("rc=");
    sint2dec(pgmid,s);
    display_puts(s);
    goto EXITCMD;
  }
//display_puts("size=");
//sint2dec(info->size,s);
//display_puts(s);
//display_puts("\n");

//display_puts("cdfs_read\n");
  pos=0;

  for(;;) {
    readbyte=cdfs_read(handle,pos,BLOCK_SHM_ID,buf_blk,BLOCK_SIZE);
    if(readbyte<0) {
      break;
    }

//display_puts(" readbyte=");
//sint2dec(readbyte,s);
//display_puts(s);

    rc=environment_loadimage(pgmid,buf,readbyte);
    if(rc<0) {
      display_puts(" loadimage rc=");
      sint2dec(rc,s);
      display_puts(s);
      goto EXITCMD;
    }

    pos += readbyte;
  }

  rc = program_exec(pgmid,ac, av);
  if(rc<0) {
    goto EXITCMD;
  }

  int exitcode;
  program_wait(&exitcode, MESSAGE_MODE_WAIT);


EXITCMD:

  if(handle>0) {
//display_puts("cdfs_close\n");
    rc = cdfs_close(handle);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
    }
  }

  if(buf_blk) {
    block_free(blk,buf_blk);
  }

//display_puts("\n");
//syscall_wait(10);
  //dump(buf);


  return 0;
}


int start(int ac, char *av[])
{
  if(ac < 3) {
    usage();
    return 0;
  }

  char *cmd = av[1];
  ac-=2;
  av+=2;
  if(strncmp(cmd,"type",5)==0)
    return cmd_type(ac, av);
  if(strncmp(cmd,"dir",4)==0)
    return cmd_dir(ac, av);
  if(strncmp(cmd,"stat",5)==0)
    return cmd_stat(ac, av);
  if(strncmp(cmd,"exec",5)==0)
    return cmd_exec(ac, av);

  usage();
  return 0;
}

