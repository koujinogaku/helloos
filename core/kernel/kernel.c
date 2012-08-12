/*
** kernel.c --- kernel main
*/

/* startup code */

#include "config.h"
#include "vesa.h"

#include "console.h"
#include "cpu.h"
#include "desc.h"
#include "exception.h"
#include "page.h"
#include "string.h"
#include "kmemory.h"
#include "task.h"
#include "queue.h"
#include "alarm.h"
#include "pic.h"
#include "timer.h"
#include "dma.h"
#include "floppy.h"
#include "fat.h"
#include "syscall.h"
#include "shrmem.h"
#include "kernel.h"
#include "kprogram.h"
#include "fpu.h"

static char s[64];
int kernel_queid=0;

static struct bios_info bios_info;
static char vesa_info[512];
static vbe_info_t  *vesa_info_table;
static mode_info_t *vesa_mode_info_table;

void kernel_dspfree(void)
{
  unsigned int memsz;

  memsz=mem_get_totalsize();
  console_puts("Total Mem=");
  int2dec(memsz,s);
  console_puts(s);
  console_puts("(");
  int2dec(memsz/1024,s);
  console_puts(s);
  console_puts("KB) ");

  memsz = mem_get_kernelfree();
  console_puts("Kernel Free=");
  int2dec(memsz/1024,s);
  console_puts(s);
  console_puts("KB ");

  memsz = page_get_totalfree();
  console_puts("Virtual Free=");
  int2dec(memsz*4,s);
  console_puts(s);
  console_puts("KB\n");

}

void taskb(void)
{
  for(;;) {

//console_puts("HELLO TASKB!!\n");

    Asm("nop");
    cpu_halt();
  }
}

int kernel_get_bios_info(char *binfo)
{
  memcpy(binfo,&bios_info,sizeof(struct bios_info));
  return 0;
}

struct bios_info *kernel_get_bios_info_addr(void)
{
  return &bios_info;
}

void start(void)
{
  int r;
  int taskid;
//  int taskcid;
//  int i;
//  int c;
//  char *a,*b,*c;
//  int fp;
//  char *loadbuf;
//  void *useraddr,*tstpgd;
//  char *pbad;
//    int exitcode;

  memcpy((char *)&bios_info,(char *)CFG_MEM_TMPBIOSINFO,sizeof(struct bios_info));
  memcpy((char *)&vesa_info,(char *)CFG_MEM_TMPVESAINFO,512);
  vesa_info_table = (vbe_info_t *)&vesa_info;
  vesa_mode_info_table = (mode_info_t *)(((unsigned long)&vesa_info)+16);

  console_setup(&bios_info);

  desc_init_gdtidt();
  exception_init();
  mem_init();
  page_init();

  shm_init();
  task_init();
  queue_init();
  alarm_init();
  pic_init();
  timer_init();
  syscall_init();
  fpu_init();

  cpu_unlock();

  timer_enable();

  kernel_queid = queue_create(MSG_QNM_KERNEL);
  if(kernel_queid<0) {
    console_puts("kernel que create error=");
    int2dec(kernel_queid,s);
    console_puts(s);
    console_puts("\n");
    for(;;);
  }

//r=task_create(taskb);
//task_start(r);

  task_dispatch_start();

  console_puts("Start tasks\n");

//console_puts("HELLO WORLD!!\n");
//for(;;);

  console_puts("Initializing DMA");
  dma_init();
  console_puts("...Ok\n");

  console_puts("Initializing Floppy");
  r=floppy_init();
  if(r<0) {
    console_puts("floppy init error=");
    int2dec(-r,s);
    console_puts(s);
    console_puts("\n");
  }
  console_puts("...Ok\n");

  console_puts("Initializing FAT");
  r=fat_init();
  if(r<0) {
    console_puts("fat init error=");
    int2dec(-r,s);
    console_puts(s);
    console_puts("\n");
  }
  console_puts("...Ok\n");

/*
// debug
  console_puts("VESA RET=");
  word2hex(bios_info.depth,s);
  console_puts(s);
  console_puts("\n");
  for(r=0;r<64;r++) {
    byte2hex(vesa_info[r],s);
    if(r%16==0)
      console_puts("\n");
    console_puts(s);
    console_puts(" ");
  }
  console_puts("\n");
  console_puts("SIG=");
  for(r=0;r<4;r++) {
    console_putc(vesa_info_table->sig[r]);
  }
  console_puts(" miner=");
  byte2hex(vesa_info_table->ver_minor,s);
  console_puts(s);
  console_puts(" major=");
  byte2hex(vesa_info_table->ver_major,s);
  console_puts(s);
  console_puts(" oem_name_off=");
  word2hex(vesa_info_table->oem_name_off,s);
  console_puts(s);
  console_puts(" oem_name_seg=");
  word2hex(vesa_info_table->oem_name_seg,s);
  console_puts(s);
  console_puts("\n");
  console_puts(" capabilities=");
  long2hex(vesa_info_table->capabilities,s);
  console_puts(s);

  console_puts(" mode_list_off=");
  word2hex(vesa_info_table->mode_list_off,s);
  console_puts(s);
  console_puts(" mode_list_seg=");
  word2hex(vesa_info_table->mode_list_seg,s);
  console_puts(s);
  console_puts("\n");
  console_puts(" vid_mem_size=");
  word2hex(vesa_info_table->vid_mem_size,s);
  console_puts(s);
  console_puts("(");
  int2dec(vesa_info_table->vid_mem_size*64,s);
  console_puts(s);
  console_puts("KB)");

  console_puts("\n");
  /////////////////////////

  console_puts(" mode_attrib=");
  word2hex(vesa_mode_info_table->mode_attrib,s);
  console_puts(s);

  console_puts(" win_a_attrib=");
  byte2hex(vesa_mode_info_table->win_a_attrib,s);
  console_puts(s);
  console_puts(" win_b_attrib=");
  byte2hex(vesa_mode_info_table->win_b_attrib,s);
  console_puts(s);
  console_puts(" k_per_gran=");
  word2hex(vesa_mode_info_table->k_per_gran,s);
  console_puts(s);
  console_puts(" win_size=");
  word2hex(vesa_mode_info_table->win_size,s);
  console_puts(s);
  console_puts(" win_a_seg=");
  word2hex(vesa_mode_info_table->win_a_seg,s);
  console_puts(s);
  console_puts(" win_b_seg=");
  word2hex(vesa_mode_info_table->win_b_seg,s);
  console_puts(s);
  console_puts(" bytes_per_row=");
  word2hex(vesa_mode_info_table->bytes_per_row,s);
  console_puts(s);
  console_puts(" wd=");
  word2hex(vesa_mode_info_table->wd,s);
  console_puts(s);
  console_puts(" ht=");
  word2hex(vesa_mode_info_table->ht,s);
  console_puts(s);
  console_puts(" depth=");
  word2hex(vesa_mode_info_table->depth,s);
  console_puts(s);
  console_puts(" memory_model=");
  word2hex(vesa_mode_info_table->memory_model,s);
  console_puts(s);
  console_puts(" lfb_adr=");
  long2hex(vesa_mode_info_table->lfb_adr,s);
  console_puts(s);

  console_puts("\n");
*/

  console_puts("vmode=");
  word2hex(bios_info.vmode,s);
  console_puts(s);
  console_puts("h depth=");
  int2dec(bios_info.depth,s);
  console_puts(s);
  console_puts(" vram=");
  long2hex(bios_info.vram,s);
  console_puts(s);
  console_puts("h scrnx=");
  int2dec(bios_info.scrnx,s);
  console_puts(s);
  console_puts(" scrny=");
  int2dec(bios_info.scrny,s);
  console_puts(s);

  console_puts("\n");

  program_init();

  kernel_dspfree();

  console_puts("HELLO WORLD!!\n");

  for(;;) {

    taskid=program_load("display.out",PGM_TYPE_IO|PGM_TYPE_VGA);

    if(taskid<0) {
      console_puts("display load error=");
      int2dec(taskid,s);
      console_puts(s);
      console_puts("\n");
      break;
    }
    if(program_start(taskid,0)<0)
      break;
/*
    console_puts("start display id=");
    int2dec(taskid,s);
    console_puts(s);
    console_puts("\n");
*/
    taskid=program_load("keyboard.out",PGM_TYPE_IO);
    if(taskid<0) {
      console_puts("display load error=");
      int2dec(taskid,s);
      console_puts(s);
      console_puts("\n");
      break;
    }
    if(program_start(taskid,0)<0)
      break;
/*
    console_puts("start keyboard id=");
    int2dec(taskid,s);
    console_puts(s);
    console_puts("\n");
*/

    taskid=program_load("command.out",0);
    if(taskid<0) {
      console_puts("display load error=");
      int2dec(taskid,s);
      console_puts(s);
      console_puts("\n");
      break;
    }
    if(program_start(taskid,0)<0)
      break;
/*
    console_puts("start command id=");
    int2dec(taskid,s);
    console_puts(s);
    console_puts("\n");
*/
/*
    taskid=program_load("vga.out",PGM_TYPE_IO|PGM_TYPE_VGA);
    if(taskid<0)
      break;
    if(program_start(taskid,0))
      break;
    console_puts("start vga id=");
    int2dec(taskid,s);
    console_puts(s);
    console_puts("\n");
*/

    break;
  }


  for(;;) {
    int exitcode;
    struct msg_head qcmd;

    qcmd.size=sizeof(qcmd);
    r = queue_get(kernel_queid,&qcmd);
    if(qcmd.service==MSG_SRV_KERNEL && qcmd.command==MSG_CMD_KRN_EXIT) {
      exitcode=program_exitevent(&qcmd);
      if(exitcode>=0) {
        console_puts("delete process=");
        int2dec(taskid,s);
        console_puts(s);
        console_puts(" exitcode=");
        int2dec(exitcode,s);
        console_puts(s);
        console_puts("\n");
      }
    }
  }

}
