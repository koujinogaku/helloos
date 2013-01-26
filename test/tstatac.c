#include "ata.h"
#include "display.h"
#include "string.h"
#include "config.h"
#include "shm.h"

#define ATA_SHMNAME 10001

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

int start(int ac, char *av[])
{
  int rc;

  unsigned short seq=1;
  unsigned short devid=0;
  int typecode;

  for(devid=0;devid<4;devid++) {
    rc=ata_getinfo(seq++,devid,&typecode);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      break;
    }
    display_puts("\n=====");
    display_puts("devid=");
    int2dec(devid,s);
    display_puts(s);
    display_puts(",type=");
    sint2dec(typecode,s);
    display_puts(s);
    display_puts("=====\n");
  }

  unsigned long lba;
  int count;
  int shmname;
  int shmid;
  unsigned long shmaddr;
  unsigned long offset;
  unsigned long bufsize;
  int resultcode;
  unsigned short res_seq;

  devid=0;
  //lba=0x10;
  lba=0x00;
  count=1;
  shmname=ATA_SHMNAME;
  offset=0;
  bufsize=2048;
  shmid=shm_create(shmname,bufsize*2);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      return 0;
    }
  rc=shm_bind(shmid,&shmaddr);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      return 0;
    }
  rc=ata_read_request(seq++,devid,lba,count,shmname,offset,bufsize);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      return 0;
    }

  devid=1;
//  lba=0x11;
  lba=0x00;
  count=1;
  shmname=ATA_SHMNAME;
  offset=2048;
  rc=ata_read_request(seq++,devid,lba,count,shmname,offset,bufsize);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      return 0;
    }

  rc=ata_read_response(&res_seq, &resultcode);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      return 0;
    }
  display_puts("seq=");
  sint2dec(res_seq,s);
  display_puts(s);
  display_puts("\n");
  

  rc=ata_read_response(&res_seq, &resultcode);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      return 0;
    }
  display_puts("seq=");
  sint2dec(res_seq,s);
  display_puts(s);
  display_puts("\n");

  display_puts("\n");
  syscall_wait(50);
  char *addr=(void*)shmaddr;
  dump(addr);
  syscall_wait(50);
  display_puts("-------\n");
  dump(addr+2048);
  return 0;
}
