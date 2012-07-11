/*
** floppy.c --- floppy disk driver
*/

#include "config.h"
#include "cpu.h"
#include "string.h"
#include "pic.h"
#include "intr.h"
#include "kmemory.h"
#include "console.h"
#include "alarm.h"
#include "queue.h"
#include "dma.h"
#include "floppy.h"
#include "errno.h"
#include "iplinfo.h"

/* FDC registers */
enum {
  FDC_SEC_SRA = 0x370, // Status Register A
  FDC_SEC_SRB = 0x371, // Status Register B
  FDC_SEC_DOR = 0x372, // Digital Output Register
  FDC_SEC_DSR = 0x373, // Drive Status Register
  FDC_SEC_MSR = 0x374, // Main Status Register
  FDC_SEC_IOR = 0x375, // Data port
  FDC_SEC_DIR = 0x377, // Digital Input Register
  FDC_SEC_CCR = 0x377, // Configuration Control Register

  FDC_PRI_SRA = 0x3f0, // Status Register A
  FDC_PRI_SRB = 0x3f1, // Status Register B
  FDC_PRI_DOR = 0x3f2, // Digital Output Register
  FDC_PRI_DSR = 0x3f3, // Drive Status Register
  FDC_PRI_MSR = 0x3f4, // Main Status Register
  FDC_PRI_IOR = 0x3f5, // Data port
  FDC_PRI_DIR = 0x3f7, // Digital Input Register
  FDC_PRI_CCR = 0x3f7, // Configuration Control Register
};

/* definition DOR */
#define FDC_DMA_ENABLE  0x08
#define FDC_REST_RESET  0x00
#define FDC_REST_ENABLE 0x04
#define FDC_MOTA_START  0x10

/* definition MSR */
#define FDC_MRQ_READY    0x80
#define FDC_DIO_TO_CPU   0x40
#define FDC_NDMA_NOT_DMA 0x20
#define FDC_BUSY_ACTIVE  0x10
#define FDC_ACTD_ACTIVE  0x08
#define FDC_ACTC_ACTIVE  0x04
#define FDC_ACTB_ACTIVE  0x02
#define FDC_ACTA_ACTIVE  0x01

/* FDC Commands */
#define FDC_CMD_SPECIFY         0x03
#define FDC_CMD_WRITE_DATA      0xc5
#define FDC_CMD_READ_DATA       0xe6
#define FDC_CMD_RECALIBRATE     0x07
#define FDC_CMD_SENSE_INTERRUPT_STAT 0x08
#define FDC_CMD_SEEK            0x0f

#define FDC_NUM_DRIVE  (FLOPPY_NUM_DRIVE)


enum {
  FDC_SECSIZE  = 512, /* 1 sector is 512 bytes */
  FDC_SECPTRK  = 18,  /* 1 track is 18 sectors */
  FDC_HEADPCYL = 2,   /* 1 drive is 1 head */
  FDC_TRKSIZE  = FDD_SECPTRK*FDD_SECSIZE, /* bytes of 1 track */
};

/* data rate */
enum {
  FDC_DRATE_500K = 0,
  FDC_DRATE_300K = 1,
  FDC_DRATE_250K = 2,
  FDC_DRATE_1M = 3,
};

/* sector size code */
enum {
  FDC_SECSZCODE_128  = 0,
  FDC_SECSZCODE_256  = 1,
  FDC_SECSZCODE_512  = 2,
  FDC_SECSZCODE_1024 = 3,
};

/* media number */
enum {
  MEDIA_UNDEF = 0,
  MEDIA_2M88 = 1,
  MEDIA_1M44 = 2,
  MEDIA_720K = 3,
};

typedef struct {
  byte sector_size_code;
  byte sectors_per_track;
  byte gap_length;
  byte format_gap_length;
  byte data_rate;
} MediaInfo;

static const MediaInfo media_info[4] =
  {
    [MEDIA_UNDEF]{ FDC_SECSZCODE_512, 18, 0x1b, 0x6c, FDC_DRATE_500K },
    [MEDIA_2M88] { FDC_SECSZCODE_512, 36, 0x1b, 0x50, FDC_DRATE_1M   },
    [MEDIA_1M44] { FDC_SECSZCODE_512, 18, 0x1b, 0x6c, FDC_DRATE_500K },
    [MEDIA_720K] { FDC_SECSZCODE_512,  9, 0x2a, 0x50, FDC_DRATE_250K },
  };

struct fdc_drive_stat{
//  int motor_count;
  short current_cylinder;
};

static struct fdc_drive_stat fdc_drivestat[FDC_NUM_DRIVE];
static int fdc_intr_qid=0;
static void *fdc_dma_buffer;
static byte fdc_current_drive=255;


//static char s[64];

static void
fdc_iodelay(int n)
{
  int i;
  for(i=0;i<n;i++)
    cpu_in8(0x80);
}

static void
fdc_motor_on( byte drive )
{
  byte n;

  n = cpu_in8(FDC_PRI_DOR) | FDC_REST_ENABLE | FDC_DMA_ENABLE | (FDC_MOTA_START << drive);
  cpu_out8(FDC_PRI_DOR, n);
  fdc_iodelay(4);
}

static void
fdc_motor_off( byte drive )
{
  byte n;
  
  n = (cpu_in8(FDC_PRI_DOR) | FDC_REST_ENABLE | FDC_DMA_ENABLE)& ~(FDC_MOTA_START << drive);
  cpu_out8(FDC_PRI_DOR, n);
  fdc_iodelay(4);

}

static void
fdc_select_drive( byte drive )
{
  byte n;

  if(fdc_current_drive==drive)
    return;
  fdc_current_drive=drive;

  n = FDC_REST_ENABLE | FDC_DMA_ENABLE | (FDC_MOTA_START << drive ) | drive;
  cpu_out8(FDC_PRI_DOR, n);
  fdc_iodelay(4);
}

/*
static int
fdc_check_disk_changed(void)
{
  if((cpu_in8(FDC_PRI_DIR) & 0x80)!=0)
    return 1;
  else
    return 0;
}
*/

static void
fdc_set_data_rate( byte rate )
{
  cpu_out8(FDC_PRI_CCR, rate);
}

static int
fdc_wait_stat(byte mask, byte stat)
{
  byte ret;
  byte retry=200;

  for(;;) {
    ret=cpu_in8(FDC_PRI_MSR);
    if((ret & mask) == stat)
      break;
    retry--;
    if(retry==0)
      return ERRNO_TIMEOUT;  // timeout
    fdc_iodelay(1);
 }
  return 0; // success
}

static int
fdc_receive( byte *data, int count )
{
  int i;
  for( i = 0 ; i < count ; i++ ){
    if(fdc_wait_stat(FDC_MRQ_READY|FDC_DIO_TO_CPU, FDC_MRQ_READY|FDC_DIO_TO_CPU)<0)
      return ERRNO_TIMEOUT; // timeout
    data[i] = cpu_in8(FDC_PRI_IOR);
  }
  return 0; // success
}

static int
fdc_send( byte *data, int count )
{
  int i;
  for( i = 0 ; i < count ; i++ ){
    if(fdc_wait_stat(FDC_MRQ_READY|FDC_DIO_TO_CPU, FDC_MRQ_READY)<0)
      return ERRNO_TIMEOUT; // timeout
    cpu_out8(FDC_PRI_IOR, data[i]);
  }
  return 0;  // success
}

static void
fdc_set_stdcmd( byte *cmd, byte cmd_num, byte drive, byte head, byte cylinder,
	      byte first_sector )
{
  cmd[0] = cmd_num;
  cmd[1] = (head << 2) | drive;  // MT,MFM,SK,head,drive
  cmd[2] = cylinder;             // C(cylinder)
  cmd[3] = head;                 // H(head)
  cmd[4] = first_sector;         // R(sector address)
  cmd[5] = FDC_SECSZCODE_512;    // N(sector size code=512KB)
  cmd[6] = FDC_SECPTRK;          // EOT(The final sector number of the current track)
  cmd[7] = 0x1b;                 // GPL(gap length)
  cmd[8] = 0xff;                 // DTL(special sector size=n/a)
}

//#define FDC_INTR_TIMEOUT 1
//#define FDC_INTR_DONE    0

/*
//#pragma interrupt
INTR_INTERRUPT( fdc_intr );
void fdc_intr(void)
{
  struct msg_head msg;
  if(fdc_intr_qid!=0) {
    msg.size=sizeof(msg);
    msg.cmd=MSG_CMD_INTERRUPT;
    msg.opt=PIC_IRQ_FDC;
    queue_put(fdc_intr_qid, &msg);
  }
  pic_eoi(PIC_IRQ_FDC);
}
*/

static int
fdc_wait_intr(void)
{
  int a;
  int r;
  struct msg_head msg;

  if(fdc_intr_qid==0)
    return ERRNO_NOTINIT;
  
  for(;;) {
    msg.size=sizeof(msg);
    r = queue_tryget(fdc_intr_qid,&msg);
    if(r<0)
      break;
    if(msg.service==MSG_SRV_KERNEL && msg.command==MSG_CMD_KRN_INTERRUPT && msg.arg==PIC_IRQ_FDC) {
      return 0; // success
    }
  }

  a = alarm_set(10000,fdc_intr_qid,0);
  if(a<0) {
    return ERRNO_RESOURCE;
  }

  msg.size=sizeof(msg);
  r = queue_get(fdc_intr_qid,&msg);
  if(r<0)
    return ERRNO_NOTINIT;

  if(msg.service==MSG_SRV_KERNEL && msg.command==MSG_CMD_KRN_INTERRUPT && msg.arg==PIC_IRQ_FDC) {
    alarm_unset(a);
//    fdc_intr_cnt=0;
    return 0; // success
  }
  console_puts("fdc wait: *** TIMEOUT ***\n");
  return ERRNO_TIMEOUT; // timeout
}

static int
fdc_wait_sense_intr_stat( byte *stat )
{
  int r;
  byte cmd[] = { FDC_CMD_SENSE_INTERRUPT_STAT };

  if( (r=fdc_wait_intr())<0 )
    return r;

  if( (r=fdc_send(cmd, sizeof(cmd)))<0 )
    return r;

  if( (r=fdc_receive(stat,2))<0 )
    return r;

  return 0; // success
}

static int
fdc_wait_receive_result( byte *result )
{
  int r;

  if( (r=fdc_wait_intr())<0 )
    return r;

  if( (r=fdc_receive(result,7))<0 )
    return r;

  return 0; // success
}

static int
fdc_reset(void)
{
  int r;
  byte statdmy[2];

  cpu_out8(FDC_PRI_DOR, FDC_DMA_ENABLE);                   // FDC DMA enable
  fdc_iodelay(1);
  cpu_out8(FDC_PRI_DOR, FDC_DMA_ENABLE | FDC_REST_ENABLE); // REST enable
  fdc_iodelay(1);
  cpu_out8(FDC_PRI_CCR, 0x00);                             // CCR reset
  fdc_iodelay(1);

  if( (r=fdc_wait_sense_intr_stat(statdmy))<0 )
    return r;

  return 0; //success
}

static int
fdc_cmd_seek( byte drive, byte head, byte cylinder )
{
  byte cmd[3], stat[2];
  int r;

  if(drive >= FDC_NUM_DRIVE)
    return ERRNO_NOTEXIST;

  if(fdc_drivestat[drive].current_cylinder == cylinder) {
    return 0; // don't need to seek
  }

  cmd[0] = FDC_CMD_SEEK;
  cmd[1] = (head << 2) | drive;
  cmd[2] = cylinder;
  if( (r=fdc_send(cmd, sizeof(cmd)))<0 )
    return r;

  if( (r=fdc_wait_sense_intr_stat(stat))<0 )
    return r;
/*
    console_puts("fdc_cmd_seek_result ST0=");
    console_putc(numtohex1((stat[0]&0xf0)/16));
    console_putc(numtohex1(stat[0]&0x0f));
    console_puts(" NCN=");
    console_putc(numtohex1((stat[1]&0xf0)/16));
    console_putc(numtohex1(stat[1]&0x0f));
    console_puts("\n");
*/
  if( (stat[0] & 0xe3) != (0x20 | drive) ) // Status Register IC=00 SE=1 DS=drive
    return ERRNO_SEEK;

  fdc_drivestat[drive].current_cylinder=cylinder;

  return 0; // success
}

static int
fdc_cmd_recalibrate( byte drive )
{
  byte stat[2];
  byte cmd[2];
  int r;

  if(drive >= FDC_NUM_DRIVE)
    return 0;

  cmd[0] = FDC_CMD_RECALIBRATE;
  cmd[1] = drive;

  if( (r=fdc_send(cmd,sizeof(cmd)))<0 )
    return r;
  
  if( (r=fdc_wait_sense_intr_stat(stat))<0 )
    return r;

  if( (stat[0] & 0xe3) != (0x20 | drive) ) // Status Register IC=00 SE=1 DS=drive
    return ERRNO_RECALIBRATE;

  fdc_drivestat[drive].current_cylinder=0;
  return 0; // success
}

static int
fdc_cmd_write_data( byte drive, byte head, byte cylinder, byte sector )
{
  byte cmd[9], result[7];
  int r;
  
  fdc_set_stdcmd(cmd, FDC_CMD_WRITE_DATA,
                 drive, head, cylinder, sector );
  if( (r=fdc_send(cmd,sizeof(cmd)))<0 )
    return r;

  if( (r=fdc_wait_receive_result(result))<0 )
    return r;

  if( (result[0] & 0xc3) != (0x00 | drive) ) // Status Register IC=00 SE=1 DS=drive
    return ERRNO_WRITE;

  return 0; // success
}

static int
fdc_cmd_read_data( byte drive, byte head, byte cylinder, byte sector )
{
  byte cmd[9], result[7];
  int r;

  fdc_set_stdcmd(cmd, FDC_CMD_READ_DATA,
                 drive, head, cylinder, sector);
  if( (r=fdc_send(cmd,sizeof(cmd)))<0 )
    return r;

  if( (r=fdc_wait_receive_result(result))<0 )
    return r;
/*
    console_puts("fdc_cmd_read_result ST0=");
    console_putc(numtohex1((result[0]&0xf0)/16));
    console_putc(numtohex1(result[0]&0x0f));
    console_puts(" ST1=");
    console_putc(numtohex1((result[1]&0xf0)/16));
    console_putc(numtohex1(result[1]&0x0f));
    console_puts(" ST2=");
    console_putc(numtohex1((result[2]&0xf0)/16));
    console_putc(numtohex1(result[2]&0x0f));
    console_puts(" C=");
    console_putc(numtohex1((result[3]&0xf0)/16));
    console_putc(numtohex1(result[3]&0x0f));
    console_puts(" H=");
    console_putc(numtohex1((result[4]&0xf0)/16));
    console_putc(numtohex1(result[4]&0x0f));
    console_puts(" R=");
    console_putc(numtohex1((result[5]&0xf0)/16));
    console_putc(numtohex1(result[5]&0x0f));
    console_puts(" N=");
    console_putc(numtohex1((result[6]&0xf0)/16));
    console_putc(numtohex1(result[6]&0x0f));
    console_puts("\n");
*/
  if( (result[0] & 0xc3) != (0x00 | drive) ) // Status Register IC=00 SE=1 DS=drive
    return ERRNO_READ;

  return 0; // success
}

int
floppy_init( void )
{
  int i;
  int r;
  byte cmd[] = { FDC_CMD_SPECIFY,
                 0xc1, // SRT = 4ms HUT = 16ms
                 0x10  // HLT = 16ms DMA
               };

  fdc_intr_qid = queue_create(0);
  if(fdc_intr_qid<0) {
    fdc_intr_qid=0;
    return ERRNO_RESOURCE;
  }
  memset( &fdc_drivestat, 0, sizeof(fdc_drivestat) );
  fdc_dma_buffer = (byte*)mem_alloc(FDC_SECSIZE);
  if(fdc_dma_buffer==0) {
    queue_destroy(fdc_intr_qid);
    fdc_intr_qid=0;
    return ERRNO_RESOURCE;
  }
//  intr_define(0x20+PIC_IRQ_FDC, INTR_INTR_ENTRY(fdc_intr),INTR_DPL_SYSTEM);
  r=intr_regist_receiver(PIC_IRQ_FDC,fdc_intr_qid);
  if(r<0) {
    queue_destroy(fdc_intr_qid);
    mem_free(fdc_dma_buffer,FDC_SECSIZE);
    fdc_intr_qid=0;
    fdc_dma_buffer=0;
    return ERRNO_RESOURCE;
  }

  pic_enable(PIC_IRQ_FDC);

  /* reset controller */
  if( (r=fdc_reset())<0)
    return r;

  for(i=0;i<FDC_NUM_DRIVE;i++)
    fdc_motor_on(i);

  /* specify */
  if( (r=fdc_send(cmd,sizeof(cmd)))<0 )
    return r;

  for(i=0;i<FDC_NUM_DRIVE;i++)
    fdc_motor_off(i);

  return 0; // success
}

int
floppy_open( byte drive )
{
  int r;
  if( drive >= FDC_NUM_DRIVE )
    return ERRNO_NOTEXIST;
  
  fdc_select_drive(drive);

  /* recalibrate 2 times */
  fdc_cmd_recalibrate(drive);
  if((r=fdc_cmd_recalibrate(drive))<0) {
    fdc_motor_off(drive);
    return r;
  }

  /* set data rate for 144MBfloppydisk */
  fdc_set_data_rate( FDC_DRATE_500K );

  return 0; // success
}

int
floppy_close(byte drive)
{
  if( drive >= FDC_NUM_DRIVE )
    return ERRNO_NOTEXIST;
  
  /* motor off */
  fdc_motor_off(drive);
  return 0;
}

int
floppy_write_sector( byte drive, void *buf, unsigned int sector, unsigned int count )
{
  byte h, c, s;
  int i;
  int r;

  if( drive >= FDC_NUM_DRIVE )
    return ERRNO_NOTEXIST;
  
  /* change drive */
  fdc_select_drive( drive );

  /* write sectors */
  for( i = 0 ; i < count ; i++ )
  {
    c = (sector + i) / FDC_SECPTRK / FDC_HEADPCYL;
    h = (sector + i) / FDC_SECPTRK % FDC_HEADPCYL;
    s = (sector + i) % FDC_SECPTRK + 1;

    /* copy to DMA buffer */
    memcpy( fdc_dma_buffer, buf + i*FDC_SECSIZE, FDC_SECSIZE );

    /* setup DMA channel 2 */
    BEGIN_CPULOCK;
    dma_disable(2);
    dma_set_area(2, (unsigned int)fdc_dma_buffer, FDC_SECSIZE-1);
    dma_set_mode(2, DMA_MODE_READ | DMA_MODE_SINGLE | DMA_MODE_INCL);
    dma_enable(2);
    END_CPULOCK;

    /* seek */
    if( (r=fdc_cmd_seek(drive, h, c))<0 )
      return r;
    
    /* write sector */
    if( (r=fdc_cmd_write_data(drive, h, c, s))<0 )
      return r;
  }

  return 0;
}

int
floppy_read_sector( byte drive, void *buf, unsigned int sector, unsigned int count )
{
  byte h, c, s;
  int i;
  int r;
//  int ii;

  if( drive >= FDC_NUM_DRIVE )
    return ERRNO_NOTEXIST;
  
  /* change drive */
  fdc_select_drive( drive );

  /* read sectors */
  for( i = 0 ; i < count ; i++ )
  {
    c = (sector + i) / FDC_SECPTRK / FDC_HEADPCYL;
    h = (sector + i) / FDC_SECPTRK % FDC_HEADPCYL;
    s = (sector + i) % FDC_SECPTRK + 1;

    /* setup DMA channel 2 */
    BEGIN_CPULOCK;
    dma_disable(2);
    dma_set_area(2, (unsigned int)fdc_dma_buffer, FDC_SECSIZE-1);
    dma_set_mode(2, DMA_MODE_WRITE | DMA_MODE_SINGLE | DMA_MODE_INCL);
    dma_enable(2);
    END_CPULOCK;

    /* seek */
    if( (r=fdc_cmd_seek(drive, h, c))<0)
      return r;
    
    /* read sector */
    if( (r=fdc_cmd_read_data(drive, h, c, s))<0)
      return r;

//  console_puts("fdc_dma_buffer=");
//  for(ii=508;ii<508+8;ii++) {
//    console_putc(numtohex1((((char*)fdc_dma_buffer)[ii]&0xf0)/16));
//    console_putc(numtohex1( ((char*)fdc_dma_buffer)[ii]&0x0f    ));
//    console_puts("  ");
//  }
//  console_puts("\n");


    /* copy from DMA buffer */
    memcpy( buf + i*FDC_SECSIZE, fdc_dma_buffer, FDC_SECSIZE );
  }

  return 0;
}
