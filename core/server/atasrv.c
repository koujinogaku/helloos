#define DEBUG 1
//
// ATA controller
//
#include "cpu.h"
#include "display.h"
#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "pic.h"
#include "environm.h"
#include "memory.h"
#include "ata.h"
#include "message.h"
#include "list.h"
#include "shm.h"
#include "config.h"

enum {
  ATA_ADR_DATA		= 0x0,
  ATA_ADR_ERROR		= 0x1, // read
  ATA_ADR_FEATURES	= 0x1, // write
  ATA_ADR_SECTORCNT	= 0x2,
  ATA_ADR_SECTORNUM	= 0x3,
  ATA_ADR_CYLINDERLOW	= 0x4,
  ATA_ADR_CYLINDERHIGH	= 0x5,
  ATA_ADR_LBALOW	= 0x3,
  ATA_ADR_LBAMID	= 0x4,
  ATA_ADR_LBAHIGH	= 0x5,

  ATA_ADR_HEAD_DEV	= 0x6,
  ATA_ADR_STATUS	= 0x7, // read
  ATA_ADR_COMMAND	= 0x7, // write
  ATA_ADR_ALTSTATUS	= 0x206, // read
  ATA_ADR_DEVCONTROL	= 0x206, // write

  ATA_ADR_PRIMARY	= 0x1f0,
  ATA_ADR_SECONDARY	= 0x170,

  ATA_DEV_MASTER	= 0x00,
  ATA_DEV_SLAVE		= 0x10,
  ATA_DEV_LBA		= 0x40,
  ATA_DEV_ATAPI		= 0xa0,

  ATA_DEVC_nIEN		= 0x02, // nIEN bit: The enable bit for the device assertion of INTRQ to the host.
  ATA_DEVC_SRST		= 0x04, // SRST bit: The host software reset bit
  ATA_DEVC_BIT3		= 0x08,
  ATA_DEVC_RST_DR0	= 0x10,
  ATA_DEVC_RST_DR1	= 0x20,
  ATA_DEVC_HOB		= 0x80, // HOB bit: High order byte is defined by the 48-bit Address feature set.

  // ATA Status
  ATA_STAT_ERR		= 0x01, // Error
  ATA_STAT_INDEX	= 0x02,
  ATA_STAT_COR		= 0x04, // Corrected Error
  ATA_STAT_DRQ		= 0x08, // Data Transfer Request
  ATA_STAT_DSC		= 0x10, // Device Seek Complete
  ATA_STAT_DF		= 0x20, // Device Fault
  ATA_STAT_DRDY		= 0x40, // Drive Ready
  ATA_STAT_BUSY		= 0x80, // Device Busy
  ATA_STAT_IDLE		= 0x50, // DSC+DRDY+~BUSY+~ERR = IDLE

  // ATA Error Status
  ATA_ERR_ILI		= 0x01,  // illegal length indicator
  ATA_ERR_EOM		= 0x02,  // end of media
  ATA_ERR_ABRT		= 0x04,  // command aborted
  ATA_ERR_MCR		= 0x08,  // media change request
  ATA_ERR_IDNF		= 0x10,  // id not found
  ATA_ERR_MC		= 0x20,  // media change
  ATA_ERR_UNC		= 0x40,  // data is uncorrectable
  ATA_ERR_ICRC		= 0x80,  // interface CRC error

  // Device Reset and Execute Diagnostics 
  //                   Count-HIGH-MID-LOW
  ATA_DIAG_CODE_GENERAL	= 0x01000001,//ATA-HDD
  ATA_DIAG_CODE_PACKET	= 0x01EB1401,//CD-ROM 
  ATA_DIAG_CODE_SATA1	= 0x013CC301,//Reserved for SATA
  ATA_DIAG_CODE_SATA2	= 0x01699601,//Reserved for SATA
  ATA_DIAG_CODE_CEATA	= 0x01CEAA01,//Reserved for CE-ATA

  // ATA command for HDD
  ATA_ICMD_EXECUTE_DEVICE_DIAGNOSTIC=0x90,
  ATA_ICMD_IDENTIFY_DEVICE	= 0xec,
  ATA_ICMD_READ_SECTORS		= 0x20,
  // ATA command for ATAPI device
  ATA_ICMD_DEVICE_RESET		= 0x08,
  ATA_ICMD_PACKET		= 0xa0,
  ATA_ICMD_IDENTIFY_PACKET_DEVICE= 0xa1,
  ATA_ICMD_SERVICE		= 0xa2,

  ATA_ICMD_INITIALIZE_DEVICE_PARAMETERS = 0x91, // Legacy command; set device sector size for ATAPI

  // Features
  ATA_FEATURE_DMA	= 0x01,  // Use DMA
  ATA_FEATURE_OVL	= 0x02,  // Overlapped command

  // ATA Interupt Reason
  ATA_IREASON_CD	= 0x01,  // 0:Data. 1:Command packet.
  ATA_IREASON_IO	= 0x02,  // 0:To the device. 1:To the host.
  ATA_IREASON_REL	= 0x04,  // 1:Bus release

  // SCSI/ATAPI command
  ATAPI_CMD_TEST_UNIT_READY	= 0x00,
  ATAPI_CMD_REQUEST_SENSE	= 0x03,
  ATAPI_CMD_START_STOP_UNIT	= 0x1b,
  ATAPI_CMD_ALLOW_MEDIUM_REMOVAL= 0x1e,
  ATAPI_CMD_READ_CAPACITY_10	= 0x25,
  ATAPI_CMD_MODE_SENSE_10	= 0x5a,
  ATAPI_CMD_MODE_SELECT_10	= 0x55,
  ATAPI_CMD_GET_EVENT_STATUS_NOTIFICATION = 0x4a,

  ATAPI_CMD_READ_10		= 0x28,
  ATAPI_CMD_READ_CD		= 0xbe,
//  ATAPI_CMD_LOAD_UNLOAD	= 0xa6,
  ATAPI_CMD_READTOC		= 0x43,
  ATAPI_CMD_READ_12		= 0xa8,
  ATAPI_CMD_INQUIRY		= 0x12,
  //ATAPI_CMD_REPORT_LUNS		= 0xa0,

  // Expected Sector Type for READ_CD command
  ATAPI_TYPE_ANY_TYPE		= 0x00, // No check of the Sector Type
  ATAPI_TYPE_CD_DA		= 0x04, // Red Book(CD-DA)
  ATAPI_TYPE_MODE1		= 0x08, // Yellow Book Sectors, 2048 bytes data
  ATAPI_TYPE_MODE2		= 0x0c, // Yellow Book Sectors, 2336 bytes data
  ATAPI_TYPE_MODE2_FORM1	= 0x10, // Green Book Sectors, 2048 bytes data
  ATAPI_TYPE_MODE2_FORM2	= 0x14, // Green Book Sectors, 2324 bytes data
};

#define ATAPI_SECTOR_SIZE 2048
#define ATA_SECTOR_SIZE   512

#define ATA_TIMEOUT_COUNT 100000

#define ATA_ITYPE_UNKOWN      0
#define ATA_ITYPE_ATA         1
#define ATA_ITYPE_ATAPI       2

#define ATA_RC_DONE           1
#define ATA_RC_WAITINTR       2
#define ATA_RC_ABORT          3

enum ata_func_id {
  ATA_FUNC_READ_PIO_TRANS,
  ATA_FUNC_READ_PIO_DONE,
  ATA_FUNC_PKT_PIO_TRANS,
  ATA_FUNC_PKT_PIO_DONE,
};

struct ata_device {
  int  id;
  int  port;
  int  dev;
  int  sectorsize;
  int  word_p_sec;
  void  *buf;
  unsigned int bufsize;
  int icmd;
  int next_func;
  struct ata_interface *interface;
  unsigned short service;
  unsigned short command;
  unsigned short queid;
  unsigned short seq;
  short resultcode;
  char p_type;
  char i_type;
  unsigned removable  :1;
  unsigned lba        :1;
  unsigned dma        :1;
  unsigned pio        :1;
  unsigned mwdma      :1;
  unsigned mwdma_mode :2;
  unsigned udma       :1;
  unsigned udma_mode  :3;
  unsigned dmadir     :1;
  int *info;
};

struct ata_interface {
  struct ata_device *intr_device;
  int queing;
  struct msg_list *requestq;
};


struct atapi_toc_header {
	unsigned short toc_length;
	byte first_track;
	byte last_track;
};

struct atapi_toc_entry {
	byte reserved1;
	unsigned control : 4;
	unsigned adr     : 4;
	byte track;
	byte reserved2;
	unsigned long lba;
};

static char *atapi_sense_key_message[16] = {
  "No Sense.",         // 0h
  "Recovered Error.",  // 1h
  "Not Ready.",        // 2h
  "Medium Error.",     // 3h
  "Hardware Error.",   // 4h
  "Illegal Request.",  // 5h
  "Unit Attention.",   // 6h
  "Data Protect.",     // 7h
  "Blank Check.",      // 8h
  "Vendor Specific.",  // 9h
  "Copy Aborted.",     // Ah
  "Aborted Command.",  // Bh
  "Equal.",            // Ch
  "Volume Overflow.",  // Dh
  "Miscompare.",       // Eh
  "Reserved.",         // Fh
};

struct ata_shm_ctrl {
  struct ata_shm_ctrl *next;
  struct ata_shm_ctrl *prev;
  unsigned int shmname;
  char *addr;
  unsigned long size;
};

#define ATA_INTERFACE_MAX 2
static struct ata_interface ata_interface[ATA_INTERFACE_MAX];
#define ATA_DEVICE_MAX 4
static struct ata_device ata_device[ATA_DEVICE_MAX];
static int ata_device_num=0;
static char s[16];
static int ata_queid=0;
//static char *ata_shm_last=(char*)CFG_MEM_BUCKETHEAPMAX;
static struct ata_shm_ctrl ata_shm_ctrl;

//
// ****************** Pre Declare *********************
//
int ata_cmd_response(struct ata_device *device);
int ata_read_pio_done(struct ata_device *device);
int ata_packet_pio_done(struct ata_device *device);
int ata_cmd_read_response(struct ata_device *device);



//
// ****************** Shared Memory Driver *********************
//
void *ata_shm_mapping(unsigned int shmname, unsigned long offset, unsigned long bufsize)
{
  int rc;
  struct ata_shm_ctrl *ctrl;
  unsigned long size;
  unsigned long shmaddr;
  int shmid;

//display_puts("*shm(");
//int2dec(shmid,s);
//display_puts(s);
//display_puts(":");
//int2dec(offset,s);
//display_puts(s);
//display_puts(":");
//int2dec(bufsize,s);
//display_puts(s);
//display_puts(")*");
  list_for_each(&ata_shm_ctrl, ctrl) {
    if(ctrl->shmname == shmname) {
      return (ctrl->addr) + offset;
    }
  }
  shmid=shm_lookup(shmname);
  if(shmid<0) {
    display_puts("*invalid shmname*");
    return NULL;
  }
  rc=shm_bind(shmid,&shmaddr);
  if(rc<0) {
    display_puts("*invalid shmid=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("*");
    return NULL;
  }
  rc=shm_getsize(shmid, &size);
  if(rc<0) {
    display_puts("*invalid shmid*");
    return NULL;
  }
  ctrl=malloc(sizeof(struct ata_shm_ctrl));
  ctrl->shmname=shmname;
  ctrl->addr=(char*)shmaddr;
  ctrl->size=size;
  list_add(&ata_shm_ctrl, ctrl);

  return (ctrl->addr) + offset;
}

//
// ****************** Low Level Access to I/O Port *********************
//
static void ata_io_delay_400ns(int port)
{
   cpu_in8(ATA_ADR_ALTSTATUS|port);
   cpu_in8(ATA_ADR_ALTSTATUS|port);
   cpu_in8(ATA_ADR_ALTSTATUS|port);
   cpu_in8(ATA_ADR_ALTSTATUS|port);
   //syscall_wait(1);
}

static int ata_io_wait_stat(int port,int mask,int stat)
{
  int i;
  char status;
  for(i=0; i<ATA_TIMEOUT_COUNT ; i++) {
    status = cpu_in8(ATA_ADR_ALTSTATUS|port);
    if((status&(mask)) == stat)
      return 0;
  }
  if(status==(char)0xff)
    return ERRNO_NOTEXIST;
  return ERRNO_TIMEOUT;
}

static void ata_io_read_data(int port, int readsize, void *bufp, int bytesize)
{
  int size,bufsize;
  int i;
  unsigned short *p;

  size=readsize/2+(readsize&1);
  bufsize=bytesize/2;
  p=(unsigned short*)(bufp);
  for(i=0;i<size;i++) {
    short tmp;
    tmp = cpu_in16(ATA_ADR_DATA | port);
    if(i<bufsize) {
      *p=tmp;
      p++;
    }
  }
}

static void ata_io_write_data(int port, void *bufp, int bytesize)
{
  int bufsize;
  int i;
  unsigned short *p;

  bufsize=bytesize/2;
  p=(unsigned short*)(bufp);
  for(i=0;i<bufsize;i++,p++) {
    cpu_out16(ATA_ADR_DATA | port, *p);
  }
}

static int ata_io_select_device(int port, int dev)
{
  int rc;
  unsigned char res8;
  //static int lastport = 0;

  //if(port==lastport)
  //  portp=0;
  //else
  //  portp=1;
  //if(lastdev[portp]!=dev) {
  //  // select device
  //  cpu_out8(ATA_ADR_HEAD_DEV | port, dev);
  //  ata_io_delay_400ns(port);
  //  return 1;
  //}
  //lastdev[portp]=dev;


  rc=ata_io_wait_stat(port,ATA_STAT_BUSY|ATA_STAT_DRQ,0);
  if(rc<0) {
    if(rc != ERRNO_NOTEXIST) {
display_puts("*wait_stat1inselect=");
res8 = cpu_in8(ATA_ADR_ALTSTATUS|port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
      return rc;// return at not (ERRNO_NOTEXIST)
    }
  }

  // select device
  cpu_out8(ATA_ADR_HEAD_DEV | port, dev);
  ata_io_delay_400ns(port);

  rc=ata_io_wait_stat(port,ATA_STAT_BUSY|ATA_STAT_DRQ,0);
  if(rc<0) {
    if(rc != ERRNO_NOTEXIST) {
display_puts("*wait_stat2inselect");
res8 = cpu_in8(ATA_ADR_ALTSTATUS|port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    }
    return rc;// return at all error
  }

  res8 = cpu_in8(ATA_ADR_STATUS | port);//reset interrupt
  if(res8 == (unsigned char)0xff) {
display_puts("*ATA_ADR_STATUSinselect*");
    return ERRNO_NOTEXIST;
  }

//display_puts("*selected(");
//byte2hex(res8,s);
//display_puts(s);
//display_puts(")");
  return 1;
}

//
// ****************** ATA Protocol *********************
//
static int ata_io_wait_intr(struct ata_device *device,int timeout)
{
  int rc;
  int alarm;
  struct msg_head msg;

//display_puts("*ata_io_wait_intr*");
  alarm=syscall_alarm_set(timeout,ata_queid,device->id);

  msg.size=sizeof(msg);
  rc=syscall_que_get(ata_queid,&msg);
  syscall_alarm_unset(alarm,ata_queid);
  if(rc<0) {
    return ERRNO_CTRLBLOCK;
  }
  if(msg.service==MSG_SRV_ALARM) {
    return ERRNO_TIMEOUT;
  }
//    display_puts("+");
//    word2hex(msg.service,s);
//    display_puts(s);
//    word2hex(msg.command,s);
//    display_puts(s);
//    word2hex(msg.arg,s);
//    display_puts(s);
//    msg.size=sizeof(msg);
  return 1;
}

int ata_read_pio_start(struct ata_device *device, int command, unsigned long long *lba, int *feature, int *count, void *buf, int bufsize)
{
  int rc;

//display_puts("*read_s*");
  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0) {
display_puts("*error selectdev*");
    return rc;
  }

  // Feature
  if(feature)
    cpu_out8(ATA_ADR_FEATURES | device->port, *feature);

  // Count
  if(count)
    cpu_out8(ATA_ADR_SECTORCNT | device->port, *count);

  if(lba) {
    cpu_out8(ATA_ADR_LBALOW   | device->port, (*lba)&0x00ff);
    cpu_out8(ATA_ADR_LBAMID   | device->port, ((*lba)>>8)&0x00ff);
    cpu_out8(ATA_ADR_LBAHIGH  | device->port, ((*lba)>>16)&0x00ff);
    cpu_out8(ATA_ADR_HEAD_DEV | device->port, device->dev|(((*lba)>>24)&0x000f) );
  }

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, command);

  device->icmd = command;
  device->buf = buf;
  device->bufsize = bufsize;
  device->next_func = ATA_FUNC_READ_PIO_TRANS;

//display_puts("*ret read_s*");
  return ATA_RC_WAITINTR;
}

  
int ata_read_pio_transfer(struct ata_device *device)
{
  unsigned char res8;
  int rc;

//display_puts("*read_trn*");
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
  if(res8&ATA_STAT_ERR) {
display_puts("*Status=");
byte2hex(res8,s);
display_puts(s);
display_puts("*");
display_puts("*ATA_STAT_ERR=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
/*
display_puts("*");
  rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
display_puts("*ireason=");
byte2hex(rc,s);
display_puts(s);
display_puts("*");
*/
    return ERRNO_NOTEXIST;
  }

  if((res8&ATA_STAT_DRQ)==0) {
    rc=ata_read_pio_done(device);
    return rc;
  }

  ata_io_read_data(device->port, ATA_SECTOR_SIZE, device->buf, device->bufsize);
//display_puts("[bufsize=");
//int2dec(device->bufsize,s);
//display_puts(s);
//display_puts("]");

  res8 = cpu_in8(ATA_ADR_ALTSTATUS|device->port);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERR2=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERR3=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }
//if(lba) {
//display_puts("[done=");
//byte2hex(res8,s);
//display_puts(s);
//display_puts("]");
//}

  device->next_func = ATA_FUNC_READ_PIO_DONE;

  return ATA_RC_WAITINTR;
}

int ata_read_pio_done(struct ata_device *device)
{
  unsigned char res8;
  int rc;
  // Status
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERRP=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }

  rc=ata_cmd_response(device);

  return rc;
}

int ata_read_pio(struct ata_device *device, int command, unsigned long long *lba, int *feature, int *count, void *buf, int bytesize)
{
  int rc;

  rc=ata_read_pio_start(device, command, lba, feature, count, buf, bytesize);
  if(rc<0)
    return rc;
  rc=ata_io_wait_intr(device,100);
  if(rc<0)
    return rc;
  rc=ata_read_pio_transfer(device);
  if(rc<0)
    return rc;
  rc=ata_read_pio_done(device);
  return rc;
}

int ata_error_dsp_packet(struct ata_device *device, unsigned char res8)
{
//  int rc;
//  int readsize;
//  unsigned int readsize_h,readsize_l;
  int sensekey;

//display_puts("*Status=");
//byte2hex(res8,s);
//display_puts(s);
//if(res8&ATA_STAT_ERR)
//  display_puts(" ERR");
//if(res8&ATA_STAT_DRQ)
//  display_puts(" DRQ");
//if(res8&ATA_STAT_DSC)
//  display_puts(" SERV");
//if(res8&ATA_STAT_DF)
//  display_puts(" DF");
//if(res8&ATA_STAT_DRDY)
//  display_puts(" DRDY");
//if(res8&ATA_STAT_BUSY)
//  display_puts(" BUSY");
//display_puts("*");

  res8 = cpu_in8(ATA_ADR_ERROR | device->port);
  sensekey = res8>>4;

//display_puts("*ERR=");
//byte2hex(res8,s);
//display_puts(s);
//display_puts(" SenseKey=");
//int2dec(sensekey,s);
//display_puts(s);
//display_puts(":");
//display_puts(atapi_sense_key_message[sensekey]);
//if(res8&ATA_ERR_ILI)
//  display_puts(" ILI");
//if(res8&ATA_ERR_EOM)
//  display_puts(" EOM");
//if(res8&ATA_ERR_ABRT)
//  display_puts(" ABRT");
//display_puts("*");
//  rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
//display_puts("*ireason=");
//byte2hex(rc,s);
//display_puts(s);
//display_puts("*");

  // Receive data size
//  readsize_h = cpu_in8(ATA_ADR_CYLINDERHIGH | device->port);
//  readsize_l = cpu_in8(ATA_ADR_CYLINDERLOW  | device->port);
//  readsize = (readsize_h << 8)|readsize_l;
//display_puts("*resbyte=");
//int2dec(readsize,s);
//display_puts(s);
//display_puts("*");

  return sensekey;
}

int ata_packet_pio_start(struct ata_device *device, void *cmd, int cmdsize, void *buf, int bufsize)
{
  unsigned char res8;
  int rc;

//display_puts("*pkt_start*");

  if(device->i_type != ATA_ITYPE_ATAPI)
    return ERRNO_NOTEXIST;

/*
display_puts("[port=");
word2hex(device->port,s);
display_puts(s);
display_puts("]");
display_puts("[dev=");
byte2hex(device->dev,s);
display_puts(s);
display_puts("]");
*/
  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0)
    return rc;

/*
  msg.size=sizeof(msg);
  while(syscall_que_tryget(ata_queid,&msg)>=0) {
    display_puts("@");
    word2hex(msg.service,s);
    display_puts(s);
    word2hex(msg.command,s);
    display_puts(s);
    word2hex(msg.arg,s);
    display_puts(s);
    msg.size=sizeof(msg);
  }
*/

//display_puts("*alloc=");
//int2dec(bufsize,s);
//display_puts(s);
//display_puts("*");

  // Features = PIO
  cpu_out8(ATA_ADR_FEATURES  | device->port, 0);    // OVL=No DMA=No

  cpu_out8(ATA_ADR_SECTORCNT    | device->port, 0x00); // Tag for the ireason when queuing
  cpu_out8(ATA_ADR_SECTORNUM    | device->port, 0); // N/A

  // buffer size for PIO mode
  cpu_out8(ATA_ADR_CYLINDERLOW  | device->port, bufsize&0x00ff);
  cpu_out8(ATA_ADR_CYLINDERHIGH | device->port, (bufsize>>8)&0x00ff);

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_ICMD_PACKET);
  ata_io_delay_400ns(device->port);

  rc=ata_io_wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
    return rc;
  }

//display_puts("*pktsnd*");

  res8 = cpu_in8(ATA_ADR_ALTSTATUS | device->port);
  if(res8&ATA_STAT_ERR) {
    // clear interrupt message in error
    res8 = cpu_in8(ATA_ADR_STATUS | device->port);
    return ERRNO_DEVICE;
  }

//display_puts("*noerror*");

  // data request status
  if((res8&ATA_STAT_DRQ)==0) {
    return ERRNO_DEVICE;
  }

//display_puts("*datareq*");

// ******************** PACKET command transfer *****************
  // send scsi/atapi command
  ata_io_write_data(device->port, cmd, cmdsize);

//    display_puts("*scsi");
//    byte2hex(((char*)cmd)[0],s);
//    display_puts(s);
//    display_puts("*");
//display_puts("*sndcmd[sz=");
//int2dec(cmdsize,s);
//display_puts(s);
//display_puts("]*");

  device->buf = buf;
  device->bufsize = bufsize;
  device->next_func = ATA_FUNC_PKT_PIO_TRANS;

// ******************** Awaiting PACKET command *****************
  return ATA_RC_WAITINTR;
}


int ata_packet_pio_transfer(struct ata_device *device)
{
  unsigned char res8;
  int rc;
  unsigned int readsize_h,readsize_l;
  int readsize;
  int sensekey;

//display_puts("*pkt_trn*");

// ******************** Check command status *****************
  // Status
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
  if(res8&ATA_STAT_ERR) {
    sensekey = ata_error_dsp_packet(device, res8);
    device->resultcode = ERRNO_SCSI_NoSense - sensekey;
    rc = ata_cmd_response(device);
    if(rc<0)
      return rc;
    return ATA_RC_ABORT;
  }

  rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
//display_puts("*ireason=");
//int2dec(rc,s);
//display_puts(s);
//display_puts("*");

  if((res8&ATA_STAT_DRQ)==0 && device->bufsize!=0) {
display_puts("*NO DATA");
display_puts(" ATA_STAT_ERRP=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_DEVICE;
  }
//display_puts("*noerror*");

  if((res8&ATA_STAT_DRQ)==0) {
    rc = ata_cmd_response(device);
    if(rc<0)
      return rc;
    return ATA_RC_DONE;
  }

  // Receive data size
  readsize_h = cpu_in8(ATA_ADR_CYLINDERHIGH | device->port);
  readsize_l = cpu_in8(ATA_ADR_CYLINDERLOW  | device->port);
  readsize = (readsize_h << 8)|readsize_l;

//display_puts("*recv=");
//int2dec(readsize,s);
//display_puts(s);
//display_puts("*");
  ata_io_read_data(device->port, readsize, device->buf, device->bufsize);

  device->next_func = ATA_FUNC_PKT_PIO_DONE;

  return ATA_RC_WAITINTR;
}

int ata_packet_pio_done(struct ata_device *device)
{
  unsigned char res8;
  int rc;

//display_puts("*pkt_done*");

  // Status
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERRP=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }

  rc=ata_cmd_response(device);
  if(rc<0)
    return rc;

  return ATA_RC_DONE;
}

int ata_packet_pio(struct ata_device *device, void *cmd, int cmdsize, void *buf, int bufsize)
{
  int rc;

  rc=ata_packet_pio_start(device, cmd, cmdsize, buf, bufsize);
  if(rc!=ATA_RC_WAITINTR)
    return rc;
  rc=ata_io_wait_intr(device,100);
  if(rc<0)
    return rc;
  rc=ata_packet_pio_transfer(device);
  if(rc!=ATA_RC_WAITINTR)
    return rc;
  rc=ata_io_wait_intr(device,100);
  if(rc<0)
    return rc;
  rc=ata_packet_pio_done(device);
  return rc;
}

//
// ****************** ATA Protocol as HDD Command *********************
//
int ata_read_sectors_start(struct ata_device *device, unsigned long lba, int count, void *buf, unsigned long bufsize)
{
  int rc;
  unsigned long long llba = lba;

  rc = ata_read_pio_start(device, ATA_ICMD_READ_SECTORS, &llba, 0, &count, buf, bufsize);

  return rc;
}
//
// ****************** ATAPI Protocol as SCSI Command *********************
//
int atapi_test_unit_ready_start(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_TEST_UNIT_READY, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int rc;

  rc = ata_packet_pio_start(device, cmd, sizeof(cmd), 0, 0);
  if(rc<0) {
display_puts("*test unit error*");
    return rc;
  }

  return 1;
}

int atapi_request_sense(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_REQUEST_SENSE, 0, 0, 0, 18, 0, 0,0,0,0,0,0 };
  char res[18];
  int rc;
  int rescode;
  int sensekey;
  int ili=0;

  memset(&res,0,sizeof(res));
  cmd[4] = sizeof(res);
  rc = ata_packet_pio(device, cmd, sizeof(cmd), &res, sizeof(res));
  if(rc<0) {
display_puts("*request sense error*");
    return rc;
  }

  rescode=(res[0])&0x7f;

  if(rescode==0x70 || rescode==0x71) {
    sensekey = (res[2])&0x0f;
    ili = (res[2])&0x20;
  }
  else {
    sensekey = (res[1])&0x0f;
  }

  if(sensekey) {
    display_puts(" Res=");
    byte2hex(rescode,s);
    display_puts(s);
    display_puts(" SenseKey=");
    display_puts(atapi_sense_key_message[sensekey]);
    if(ili)
      display_puts(" ILI");
  }

  return 1;
}

int atapi_start_stop(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_START_STOP_UNIT, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int rc;

  cmd[1] = 0; // Immed=0
  cmd[4] = 0x2 | 0x1;  // LoEj=1, Start=1 => Load the Disc(Close Tray)

  rc = ata_packet_pio(device, cmd, sizeof(cmd), 0, 0);
  if(rc<0) {
display_puts("*start stop error*");
    return rc;
  }

  return 1;
}

int atapi_read_capacity_10(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_READ_CAPACITY_10, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  unsigned char res[18];
  int rc;
  unsigned long lba;
  unsigned long bsz;

  rc = ata_packet_pio(device, cmd, sizeof(cmd), &res, sizeof(res));
  if(rc<0) {
display_puts("*read_capacity_10 error*");
    return rc;
  }

  lba =  res[0]<<24;
  lba |= res[1]<<16;
  lba |= res[2]<<8;
  lba |= res[3];

  bsz =  res[4]<<24;
  bsz |= res[5]<<16;
  bsz |= res[6]<<8;
  bsz |= res[7];

  display_puts(" *Capacity=");
  int2dec(lba,s);
  display_puts(s);
  display_puts(" BlockSize=");
  int2dec(bsz,s);
  display_puts(s);
  display_puts("*");

  return 1;
}

int atapi_allow_medium_removal(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_ALLOW_MEDIUM_REMOVAL, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int rc;

  cmd[4] = 0x01; // Prevent=1 => Lock

  rc = ata_packet_pio(device, cmd, sizeof(cmd), 0, 0);
  if(rc<0) {
display_puts("*allow_medium_removal error*");
    return rc;
  }

  return 1;
}

int atapi_read_10_start(struct ata_device *device, unsigned long lba, unsigned long count, void *buf, unsigned long bufsize)
{
  static char cmd[12] = { ATAPI_CMD_READ_10, 0, 0, 0, 0, 0,  0,0,0,0,0,0};

  int rc;

  cmd[2] = (lba >> 24) & 0xFF;   // most sig. byte of LBA
  cmd[3] = (lba >> 16) & 0xFF;
  cmd[4] = (lba >>  8) & 0xFF;
  cmd[5] = (lba >>  0) & 0xFF;   // least sig. byte of LBA
  cmd[7] = (count >> 8) & 0xFF;  // sector count high
  cmd[8] = (count >> 0) & 0xFF;  // sector count low

  rc = ata_packet_pio_start(device, cmd, sizeof(cmd), buf, bufsize);
  return rc;
}


int atapi_inquiry(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_INQUIRY, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };

  int allocsize = 32768; //ATAPI_SECTOR_SIZE;
  char *scsibuff;
  int readcount;
  int i;

  scsibuff=malloc(allocsize);
  memset(scsibuff,0,allocsize);

  cmd[3] = allocsize&0xff;
  cmd[4] = allocsize>>8;

  readcount = ata_packet_pio(device, cmd, sizeof(cmd), scsibuff, allocsize);
  if(readcount<0) {
display_puts("*inquiry error*");
    mfree(scsibuff);
    return readcount;
  }

  display_puts("\n");
  for(i=0;i<readcount && i<16;i++) {
    if(i!=0)
      display_puts(":");
    byte2hex(scsibuff[i],s);
    display_puts(s);
  }
  display_puts("\n");
  mfree(scsibuff);

  return 1;
}


//
// ****************** ATA Device Setting *********************
//

static void swap_string(void *bufp, int size)
{
  short *p=bufp;
  while(size>0) {
    short tmp=*p;
    *p = ((tmp>>8)&0x00ff) | ((tmp<<8)&0xff00);
    p++;
    size -= 2;
  }
}


int ata_execute_diagnostic(struct ata_device *device)
{
  int rc;
  // select device
  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0) {
    if(rc!=ERRNO_NOTEXIST) {
      display_puts(" *DIAG ERROR1*");
sint2dec(rc,s);
display_puts(s);
    }
    return rc;
  }

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_ICMD_EXECUTE_DEVICE_DIAGNOSTIC);
  ata_io_delay_400ns(device->port);

  rc=ata_io_wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
display_puts(" *DIAG ERROR2*");
sint2dec(rc,s);
display_puts(s);
    return rc;
  }
  return 0;
}

int ata_reset_atapi_device(struct ata_device *device)
{
  int rc;
  // select device
  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0) {
    if(rc!=ERRNO_NOTEXIST) {
      display_puts(" *RESET ERROR1*");
sint2dec(rc,s);
display_puts(s);
    }
    return rc;
  }

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_ICMD_DEVICE_RESET);
  ata_io_delay_400ns(device->port);

  rc=ata_io_wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
    display_puts(" *RESET ERROR2*");
    return rc;
  }
  return 0;
}

int ata_check_diag_status(struct ata_device *device, unsigned long *type)
{
  int rc;
  unsigned long device_type;

  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0) {
    if(rc!=ERRNO_NOTEXIST) {
      display_puts(" DEVSELECTERROR");
    }
    return rc;
  }

  device_type  = (cpu_in8(ATA_ADR_SECTORCNT | device->port) << 24);
  device_type |= (cpu_in8(ATA_ADR_LBAHIGH   | device->port) << 16);
  device_type |= (cpu_in8(ATA_ADR_LBAMID    | device->port) <<  8);
  device_type |= (cpu_in8(ATA_ADR_LBALOW    | device->port) <<  0);

//  switch(device_type) {
//  case ATA_DIAG_CODE_GENERAL:
//    display_puts("ATA Device:");
//    break;
//  case ATA_DIAG_CODE_PACKET:
//    display_puts("ATAPI Device:");
//    break;
//  case ATA_DIAG_CODE_SATA1:
//    display_puts("SATA1 Device:");
//    break;
//  case ATA_DIAG_CODE_SATA2:
//    display_puts("SATA2 Device:");
//    break;
//  case ATA_DIAG_CODE_CEATA:
//    display_puts("CEATA Device:");
//    break;
//  default:
//    display_puts("Unknown Device(");
//    long2hex(device_type,s);
//    display_puts(s);
//    display_puts("):");
//    break;
//  }

  *type = device_type;
  return 0;
}

int ata_decode_device_info(struct ata_device *device,unsigned short *databuff)
{
  if((databuff[0]&0x8000)==0) {
    device->i_type = ATA_ITYPE_ATA;
    display_puts("ATA:");
  }
  else if((databuff[0]&0xc000)==0x8000) {
    device->i_type = ATA_ITYPE_ATAPI;
    display_puts("ATAPI:");
  }
  else if((databuff[0]&0xc000)==0xc000) {
    device->i_type = ATA_ITYPE_UNKOWN;
    display_puts(" Reserved:");
  }

  char infostr[80];
/*
  display_puts(" serial=");
  memcpy(infostr,&databuff[10],20);
  swap_string(infostr,20);
  infostr[20]=0;
  display_puts(infostr);
  display_puts(" farmware=");
  memcpy(infostr,&databuff[23],8);
  swap_string(infostr,8);
  infostr[8]=0;
  display_puts(infostr);
*/
  display_puts(" Model=");
  memcpy(infostr,&databuff[27],40);
  swap_string(infostr,40);
  infostr[40]=0;
  display_puts(infostr);


  device->p_type = (databuff[0]>>8)&0x001f;
  if(device->p_type==0)
    device->p_type=ATA_DEVTYPE_CD;
  display_puts(" Type=");
  switch(device->p_type) {
  case ATA_DEVTYPE_CD:
    display_puts("Cdrom");
    device->sectorsize=ATAPI_SECTOR_SIZE;
    break;
  case ATA_DEVTYPE_TAPE:
    display_puts("Tape");
    device->sectorsize=0;
    break;
  case ATA_DEVTYPE_HDD:
    display_puts("HardDisk");
    device->sectorsize=ATA_SECTOR_SIZE;
    break;
  default:
    display_puts("Unknown");
    device->sectorsize=0;
    break;
  }

  display_puts("(");
  int2dec(device->p_type,s);
  display_puts(s);
  display_puts(")");

  if(databuff[0]&0x0080) {
    device->removable = 1;
    display_puts(" Removable");
  }
  else {
    device->removable = 0;
    display_puts(" Fixed");
  }

  if((databuff[0]&0x0020)) {
    display_puts(" IntDRQ");
  }

  if((databuff[0]&0x0060)==0x0040) {
    display_puts(" 50us");
  }
  else if((databuff[0]&0x0060)==0x0000) {
    display_puts(" 3ms");
  }

  if((databuff[0]&0x0003)==0x0000) {
    display_puts(" 12byte");
  }
  else if((databuff[0]&0x0003)==0x0001) {
    display_puts(" 16byte");
  }

  if(databuff[49]&0x0200) {
    device->lba = 1;
    display_puts(" LBA");
  }
  else {
    device->dev = device->dev & ~ATA_DEV_LBA;
  }

  if(databuff[49]&0x0100) {
    device->dma = 1;
    display_puts(" DMA");
  }

  display_puts(" TOTALSEC=");
  int2dec((databuff[60])+(databuff[61]<<16),s);
  display_puts(s);

  if(databuff[62]&0x8000) {
    device->dmadir = 1;
    display_puts(" DMADIR");
  }

  if(databuff[62]&0x0001) {
    device->udma = 1;
    device->udma_mode = 0;
  }
  if(databuff[62]&0x0002) {
    device->udma = 1;
    device->udma_mode = 1;
  }
  if(databuff[62]&0x0004) {
    device->udma = 1;
    device->udma_mode = 2;
  }
  if(databuff[62]&0x0008) {
    device->udma = 1;
    device->udma_mode = 3;
  }
  if(databuff[62]&0x0010) {
    device->udma = 1;
    device->udma_mode = 4;
  }
  if(databuff[62]&0x0020) {
    device->udma = 1;
    device->udma_mode = 5;
  }
  if(databuff[62]&0x0040) {
    device->udma = 1;
    device->udma_mode = 6;
  }

  if(databuff[63]&0x0001) {
    device->mwdma = 1;
    device->mwdma_mode = 0;
  }
  if(databuff[63]&0x0002) {
    device->mwdma = 1;
    device->mwdma_mode = 1;
  }
  if(databuff[63]&0x0004) {
    device->mwdma = 1;
    device->mwdma_mode = 2;
  }

  if(device->mwdma) {
    display_puts(" MWDMA");
    int2dec(device->mwdma_mode,s);
    display_puts(s);
  }
  if(device->udma) {
    display_puts(" UDMA");
    int2dec(device->udma_mode,s);
    display_puts(s);
  }

  if(databuff[64]&0x000f) {
    device->pio = 1;
    display_puts(" PIO");
  }

  if(databuff[82]&0x0200) {
    display_puts(" DRST");
  }

  if(databuff[83]&0x0400) {
    display_puts(" LBA48");
  }

  device->word_p_sec = databuff[117] | databuff[118] << 16;
  display_puts(" Word/Sec=");
  int2dec(device->word_p_sec,s);
  display_puts(s);


  device->info = malloc(sizeof(databuff));
  memcpy(device->info,databuff,sizeof(databuff));


  display_puts("\n");

  return 0;
}

int ata_probe_device(struct ata_device *device, struct ata_interface *interface, int id, int port, int dev)
{
  unsigned short databuff[256];
  int rc;
  unsigned long device_type;

//display_puts("\n******id=");
//int2dec(id,s);
//display_puts(s);
//display_puts("******\n");

  memset(device,0,sizeof(struct ata_device));
  device->id   = id;
  device->port = port;
  device->dev  = dev|ATA_DEV_LBA;
  device->next_func = -1;
  device->interface = interface;

  rc=ata_execute_diagnostic(device);
  if(rc!=ERRNO_NOTEXIST) {
    if(ata_io_wait_intr(device,100)<0)
      display_puts("(reset timeout)");
  }
  if(rc<0)
    return rc;

  rc=ata_check_diag_status(device,&device_type);
  if(rc<0)
    return rc;

  int cmd;
  if(device_type==ATA_DIAG_CODE_PACKET) {
    device->dev  = dev|ATA_DEV_ATAPI;
    cmd = ATA_ICMD_IDENTIFY_PACKET_DEVICE;
  }
  else {
    device->dev  = dev|ATA_DEV_LBA;
    cmd = ATA_ICMD_IDENTIFY_DEVICE;
  }
  rc = ata_read_pio(device, cmd, 0, 0, 0, &databuff, sizeof(databuff));
  if(rc<0)
    return rc;

  ata_decode_device_info(device,databuff);

  return 0;
}


int ata_init(void)
{
  int rc;
  int i;

  ata_queid = environment_getqueid();
  if(ata_queid==0) {
    syscall_puts("ata_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }

  memset(&ata_interface,0,sizeof(ata_interface));
  memset(&ata_device,0,sizeof(ata_device));

  list_init(&ata_shm_ctrl);


  for(i=0;i<ATA_INTERFACE_MAX;i++) {
    ata_interface[i].requestq=message_create_userqueue();
    if(ata_interface[i].requestq==NULL) {
      display_puts("ata_init create userqueue\n");
      return ERRNO_RESOURCE;
    }
  }

  // Reset Primary & Secondary
  cpu_out8(ATA_ADR_DEVCONTROL|ATA_ADR_PRIMARY, ATA_DEVC_nIEN|ATA_DEVC_SRST|ATA_DEVC_BIT3);
  syscall_wait(1);
  cpu_out8(ATA_ADR_DEVCONTROL|ATA_ADR_PRIMARY, ATA_DEVC_nIEN|ATA_DEVC_BIT3);
  syscall_wait(5);

  rc=syscall_intr_regist(PIC_IRQ_IDE0, ata_queid);
  if(rc<0) {
    ata_queid = 0;
    display_puts("ata_init intrreg=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  rc=syscall_intr_regist(PIC_IRQ_IDE1, ata_queid);
  if(rc<0) {
    ata_queid = 0;
    display_puts("ata_init intrreg=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  pic_enable(PIC_IRQ_IDE0);
  pic_enable(PIC_IRQ_IDE1);

  // Enable intrupt
  cpu_out8(ATA_ADR_DEVCONTROL|ATA_ADR_PRIMARY, ATA_DEVC_BIT3);

  // probe devices
  display_puts("atad:Start probe\n");

  ata_device_num=0;
  if(ata_probe_device(&ata_device[ata_device_num], &ata_interface[0], ata_device_num, ATA_ADR_PRIMARY,   ATA_DEV_MASTER)>=0) {
    ata_device_num++;
  }
  if(ata_probe_device(&ata_device[ata_device_num], &ata_interface[0], ata_device_num, ATA_ADR_PRIMARY,   ATA_DEV_SLAVE)>=0) {
    ata_device_num++;
  }
  if(ata_probe_device(&ata_device[ata_device_num], &ata_interface[1], ata_device_num, ATA_ADR_SECONDARY, ATA_DEV_MASTER)>=0) {
    ata_device_num++;
  }
  if(ata_probe_device(&ata_device[ata_device_num], &ata_interface[1], ata_device_num, ATA_ADR_SECONDARY, ATA_DEV_SLAVE)>=0) {
    ata_device_num++;
  }
  if(ata_device_num<ATA_DEVICE_MAX)
    ata_device[ata_device_num].interface = NULL;

  display_puts("atad:End probe\n");


  rc = syscall_que_setname(ata_queid, ATA_QNM_ATA);
  if(rc<0) {
    syscall_puts("ata_init que create error=");
    int2dec(-rc,s);
    syscall_puts(s);
    syscall_puts("\n");
    return rc;
  }

  return 0;
}

/*
static int test_ata_access(struct ata_device *device)
{
    unsigned long long lba=0;
    int count=1;
    unsigned char *diskbuff;
    int rc;

    diskbuff=malloc(ATA_SECTOR_SIZE);
    memset(diskbuff,0,ATA_SECTOR_SIZE);

    rc = ata_io_wait_stat(device->port,ATA_STAT_DRDY|ATA_STAT_DRDY,ATA_STAT_DRDY);
    if(rc<0) {
      display_puts("*drive not ready*");
      return ERRNO_DEVICE;
    }
    rc = ata_read_pio(device, ATA_ICMD_READ_SECTORS, &lba, 0, &count, diskbuff, ATA_SECTOR_SIZE);
    if(rc<0) {
      display_puts("read error=");
      int2dec(-rc,s);
      display_puts(s);
    }
    display_puts(" readsec\n");
//int n;
//for(n=0;n<80;n++) {
//  if(diskbuff[n]>0x20)
//    display_putc(diskbuff[n]);
//  else
//    display_putc('.');
//}
    display_puts("\n");
    return 1;

}

static int test_atapi_access(struct ata_device *device)
{
  int rc;
//  rc = atapi_request_sense(device);
//  if(rc<0)
//    return rc;

//  ata_reset_atapi_device(device);

  rc = atapi_test_unit_ready(device);
  if(rc<0) {
//display_puts("*error exit*");
    return rc;
  }

//  rc = atapi_start_stop(device);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }

  rc = atapi_read_capacity_10(device);
  if(rc<0) {
//display_puts("*error exit*");
    return rc;
  }

//  rc = atapi_allow_medium_removal(device);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }

  rc = atapi_inquiry(device);
  if(rc<0) {
display_puts("*error exit*");
    return rc;
  }

//  unsigned long lba;
//  lba = 0x10;
//  rc = atapi_read_10(device,lba);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }


  ///////////////////////////////////////////////////////////////////////
  //    Access to ATAPI device
  ///////////////////////////////////////////////////////////////////////
  //int count = ATAPI_SECTOR_SIZE;
  //rc = ata_read_pio(device, ATA_ICMD_INITIALIZE_DEVICE_PARAMETERS, 0, 0, &count, 0, 0);
  //if(rc<0) {
  //  display_puts("*NotSupINITDEVPARM*");
  //}

  // Inquiry The Peripheral device type from ATAPI Device
//  static char cmd[12] = { 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0};

//  static char cmd[] = { ATAPI_CMD_READ12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//  long long lba;
//  static char cmd[6] = { 0, 0, 0, 0, 200, 0 };
//  static char cmd[12] = { ATAPI_CMD_REPORT_LUNS, 0, 0, 0, 0, 0, 0,0,ATAPI_SECTOR_SIZE>>8,ATAPI_SECTOR_SIZE&0xff,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READ_12, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READ_CD, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READTOC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//
// Correct Command Set
//  static char cmd[12] = { ATAPI_CMD_MODE_SENSE_10, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };

// READ12
//  int allocsize = ATAPI_SECTOR_SIZE;
//  cmd[0] = ATAPI_CMD_READ_12;
//  cmd[2] = (lba >> 0x18) & 0xFF;   // most sig. byte of LBA
//  cmd[3] = (lba >> 0x10) & 0xFF;
//  cmd[4] = (lba >> 0x08) & 0xFF;
//  cmd[5] = (lba >> 0x00) & 0xFF;   // least sig. byte of LBA
//  cmd[9] = 1;                      // sector count

// READ_CD
//  int allocsize = ATAPI_SECTOR_SIZE;
//  cmd[0] = ATAPI_CMD_READ_CD;
//  cmd[1] = ATAPI_TYPE_MODE1;
//  cmd[2] = (lba >> 0x18) & 0xFF;   // most sig. byte of LBA
//  cmd[3] = (lba >> 0x10) & 0xFF;
//  cmd[4] = (lba >> 0x08) & 0xFF;
//  cmd[5] = (lba >> 0x00) & 0xFF;   // least sig. byte of LBA
//  cmd[8] = 1;                      // sector count
//  cmd[9] = 0x10;                   // User Data

// MODE_SENSE_10
//  int allocsize = 24;
//  cmd[0] = ATAPI_CMD_MODE_SENSE_10;
//  cmd[1] = 0; // Immed=0
//  cmd[2] = 0x0e;

// READTOC
//  int allocsize = sizeof(struct atapi_toc_header)+sizeof(struct atapi_toc_entry);
//  cmd[0] = ATAPI_CMD_READTOC;
//  cmd[1] = 0x02;             // msf
//  cmd[2] = 0x00;
//  cmd[6] = 0;                // trackno
//  cmd[7] = allocsize>>8;
//  cmd[8] = allocsize&0xff;   // alloc length
//  cmd[9] = 0x0;              // format 0x80=Returns all Sub-channel Q data    0x40=first session


  if(allocsize) {
    scsibuff=malloc(allocsize);
    memset(scsibuff,0,allocsize);
  }
  else {
    scsibuff=0;
  }
  rc = ata_packet_pio(device, cmd, sizeof(cmd), scsibuff, allocsize);
  //rc = ata_packet_pio(device, cmd, sizeof(cmd), scsibuff, 12);
  //rc = ata_packet_pio(device, cmd, sizeof(cmd), 0, 0);
  if(rc<0) {
    mfree(scsibuff);
display_puts(" *packet comand error*");
    atapi_request_sense(device);
    ata_check_diag_status(device);
    ata_reset_atapi_device(device);
    ata_check_diag_status(device);
    return rc;
  }
  if(rc<0)
    return rc;

  device->p_type = scsibuff[0]&0x1f;
  display_puts(" PTYPE=");
  byte2hex(device->p_type,s);
  display_puts(s);

  display_puts(" PQUAL=");
  byte2hex((scsibuff[0]>>5)&0x7,s);
  display_puts(s);

  display_puts(" VendorId=");
  memcpy(infostr,&scsibuff[8],8);
  swap_string(infostr,8);
  infostr[8]=0;
  display_puts(infostr);

  display_puts(" ProductId=");
  memcpy(infostr,&scsibuff[8],8);
  swap_string(infostr,8);
  infostr[8]=0;
  display_puts(infostr);


  mfree(scsibuff);


  return 1;
}
*/

//
// ****************** Interrupt hundler *********************
//
int ata_alarm_recvmsg(union ata_msg *msg)
{
  display_puts("*ata_alarm_recvmsg*");
  return 0;
}

int ata_intr_handler(union ata_msg *msg)
{
  int rc;
  struct ata_interface *interface;
  struct ata_device *device;
//display_puts("*intr*");

  if(msg->h.command!=MSG_CMD_KRN_INTERRUPT) {
    display_puts("intr bad cmd=");
    goto error_exception;
  }
  switch(msg->h.arg) {
  case PIC_IRQ_IDE0:
    interface=&ata_interface[0];
    break;
  case PIC_IRQ_IDE1:
    interface=&ata_interface[1];
    break;
  default:
    display_puts("intr bad irq=");
    goto error_exception;
  }

  device=interface->intr_device;
  if(device==NULL) {
    display_puts("intr not ready device=");
    goto error_exception;
  }

  switch(device->next_func) {
  case ATA_FUNC_READ_PIO_TRANS:
    rc=ata_read_pio_transfer(device);
    //break;// No Intr
  case ATA_FUNC_READ_PIO_DONE:
    rc=ata_read_pio_done(device);
    break;
  case ATA_FUNC_PKT_PIO_TRANS:
    rc=ata_packet_pio_transfer(device);
    break;
  case ATA_FUNC_PKT_PIO_DONE:
    rc=ata_packet_pio_done(device);
    break;
  default:
    display_puts("intr unknown func=");
    goto error_exception;
  }
  return rc;

error_exception:
    long2hex(msg->h.command,s);
    display_puts(s);
    display_puts(":");
    long2hex(msg->h.arg,s);
    display_puts(s);
    display_puts("from kernel\n");
    return ERRNO_CTRLBLOCK;
}

//
// ****************** Commands *********************
//
int ata_get_device(union ata_msg *msg, struct ata_device **devicep)
{
  int devid=msg->read.req.devid;
  int rc;
//display_puts("*read_req(");
//int2dec(msg->read.req.seq,s);
//display_puts(s);
//display_puts(")*");

  if(devid >= ata_device_num) {
    struct ata_device dummydevice;
    dummydevice.interface = NULL;
    dummydevice.service   = msg->h.service;
    dummydevice.command   = msg->h.command;
    dummydevice.queid     = msg->h.arg;
    dummydevice.seq       = msg->read.req.seq;
    dummydevice.resultcode= ERRNO_NOTEXIST;
    rc=ata_cmd_response(&dummydevice);
    if(rc<0)
      return rc;
    return ERRNO_NOTEXIST;
  }

  struct ata_device *device=&ata_device[devid];
  if(device->interface->intr_device != NULL) {
display_puts("*queing request*");
    rc=message_put_userqueue(device->interface->requestq, msg);
    if(rc<0)
      return rc;
    device->interface->queing++;
    return ERRNO_INUSE;
  }

  device->interface->intr_device=device;
  device->service = msg->h.service;
  device->command = msg->h.command;
  device->queid   = msg->h.arg;
  device->seq     = msg->read.req.seq;
  device->resultcode=0;

  *devicep = device;
  return 0;
}

int ata_cmd_read_request(union ata_msg *msg)
{
  int rc;
  int devid=msg->read.req.devid;
//display_puts("*read_req(");
//int2dec(msg->read.req.seq,s);
//display_puts(s);
//display_puts(")*");

  if(devid >= ata_device_num) {
    struct ata_device dummydevice;
    dummydevice.interface = NULL;
    dummydevice.service   = msg->h.service;
    dummydevice.command   = msg->h.command;
    dummydevice.queid     = msg->h.arg;
    dummydevice.seq       = msg->read.req.seq;
    dummydevice.resultcode= ERRNO_NOTEXIST;
    rc=ata_cmd_read_response(&dummydevice);
    return rc;
  }

  struct ata_device *device=&ata_device[devid];
  if(device->interface->intr_device != NULL) {
display_puts("*queing request*");
    rc=message_put_userqueue(device->interface->requestq, msg);
    device->interface->queing++;
    return rc;
  }
  device->interface->intr_device=device;
  device->service = msg->h.service;
  device->command = msg->h.command;
  device->queid   = msg->h.arg;
  device->seq     = msg->read.req.seq;
  device->resultcode=0;

  char *buf;
  buf = ata_shm_mapping(msg->read.req.shmname,msg->read.req.offset,msg->read.req.bufsize);
//display_puts("*buf(");
//long2hex((unsigned long)buf,s);
//display_puts(s);
//display_puts(")*");
//*buf=0x55;
  if(device->i_type==ATA_ITYPE_ATAPI) {
    rc=atapi_read_10_start(device, msg->read.req.lba, msg->read.req.count, buf, msg->read.req.bufsize);
  }
  else {
    rc=ata_read_sectors_start(device, msg->read.req.lba, msg->read.req.count, buf, msg->read.req.bufsize);
  }
  return rc;
}

int ata_cmd_read_response(struct ata_device *device)
{
  int rc;
  union ata_msg res;
//display_puts("*read_res*");

  memset(&res,0,sizeof(res));
  res.h.size       = sizeof(res);
  res.h.service    = device->service;
  res.h.command    = device->command;
  res.h.arg        = device->queid;
  res.read.res.seq = device->seq;
  res.read.res.resultcode=device->resultcode;

  rc=syscall_que_put(res.h.arg, &res);

  // Release
  if(device->interface!=NULL) {
    device->interface->intr_device=NULL;
  }
  return rc;
}

int ata_cmd_getinfo(union ata_msg *msg)
{
  int rc;
  union ata_msg res;
  int devid=msg->getinfo.req.devid;
//display_puts("*ata_cmd_getinfo*");
  memset(&res,0,sizeof(res));
  res.h.size=sizeof(res);
  res.h.service=ATA_SRV_ATA;
  res.h.command=ATA_CMD_GETINFO;
  res.h.arg=msg->h.arg;
  res.getinfo.res.seq=msg->getinfo.req.seq;

  if(devid >= ata_device_num) {
    res.getinfo.res.type=ERRNO_NOTEXIST;
  }
  else {
    res.getinfo.res.type=ata_device[devid].p_type;
  }
//display_puts("*snd info*");
  rc=syscall_que_put(res.h.arg, &res);
  return rc;
}

int ata_cmd_checkmedia_request(union ata_msg *msg)
{
  struct ata_device *device;
  int rc;

  rc = ata_get_device(msg, &device);
  if(rc==ERRNO_NOTEXIST||rc==ERRNO_INUSE)
    return 0;
  if(rc<0)
    return rc;

  if(device->i_type==ATA_ITYPE_ATAPI) {
    rc=atapi_test_unit_ready_start(device);
  }
  else {
    rc=0;
  }
  return rc;
}

int ata_cmd_checkmedia_response(struct ata_device *device)
{
  int rc;
  union ata_msg res;
//display_puts("*read_res*");

  memset(&res,0,sizeof(res));
  res.h.size       = sizeof(res);
  res.h.service    = device->service;
  res.h.command    = device->command;
  res.h.arg        = device->queid;
  res.checkmedia.res.seq = device->seq;
  res.checkmedia.res.resultcode=device->resultcode;

  rc=syscall_que_put(res.h.arg, &res);

  // Release
  if(device->interface!=NULL) {
    device->interface->intr_device=NULL;
  }
  return rc;
}

int ata_cmd_handler(union ata_msg *msg)
{
  int rc;
  switch(msg->h.command) {
  case ATA_CMD_GETINFO:
    rc=ata_cmd_getinfo(msg);
    break;
  case ATA_CMD_READ:
    rc=ata_cmd_read_request(msg);
    break;
  case ATA_CMD_CHECKMEDIA:
    rc=ata_cmd_checkmedia_request(msg);
    break;
  default:
    display_puts("unknown cmd error=");
    int2dec(msg->h.command,s);
    display_puts(s);
    display_puts("\n");
    return ERRNO_CTRLBLOCK;
  }
  return rc;
}

int ata_cmd_response(struct ata_device *device)
{
  int rc;
  switch(device->command) {
  case 0:
    rc=0;
    break;
  case ATA_CMD_READ:
    rc=ata_cmd_read_response(device);
    break;
  case ATA_CMD_CHECKMEDIA:
    rc=ata_cmd_checkmedia_response(device);
    break;
  default:
    display_puts("unknown cmd error=");
    int2dec(device->command,s);
    display_puts(s);
    display_puts("\n");
    return ERRNO_CTRLBLOCK;
  }
  return rc;
}

int start(int ac, char *av[])
{
  int rc;
  union ata_msg cmd;

  ata_init();

/*
  display_puts("***************************\n");

  int i;
  for(i=0;i<ata_device_num;i++) {
    if(ata_device[i].i_type == ATA_ITYPE_ATAPI)
      test_atapi_access(&ata_device[i]);
    else
      test_ata_access(&ata_device[i]);

    display_puts("***************************\n");
  }
*/

  for(;;)
  {
    cmd.h.size=sizeof(cmd);
    if(ata_interface[0].intr_device==NULL && ata_interface[0].queing > 0) {
      ata_interface[0].queing--;
      rc=message_get_userqueue(ata_interface[0].requestq, &cmd);
//display_puts("*pull request*");
    }
    else if(ata_interface[1].intr_device==NULL && ata_interface[1].queing > 0) {
      ata_interface[1].queing--;
      rc=message_get_userqueue(ata_interface[1].requestq, &cmd);
//display_puts("*pull request*");
    }
    else {
//display_puts("%");
      rc=syscall_que_get(ata_queid,&cmd);
    }
    if(rc<0) {
      display_puts("cmd receive error=");
      int2dec(-rc,s);
      display_puts(s);
      display_puts("\n");
      return 255;
    }

//display_puts("*msg(");
//word2hex(cmd.h.service,s);
//display_puts(s);
//display_puts(":");
//word2hex(cmd.h.command,s);
//display_puts(s);
//display_puts(":");
//word2hex(cmd.h.arg,s);
//display_puts(s);
//display_puts(")*");

    switch(cmd.h.service) {
    case MSG_SRV_KERNEL:
      rc=ata_intr_handler(&cmd);
      break;
    case MSG_SRV_ALARM:
      rc=ata_alarm_recvmsg(&cmd);
      break;
    case ATA_SRV_ATA:
      rc=ata_cmd_handler(&cmd);
      break;
    default:
      display_puts("cmd number error srv=");
      word2hex(cmd.h.service,s);
      display_puts(s);
      display_puts(" sz=");
      word2hex(cmd.h.size,s);
      display_puts(s);
      display_puts(" cmd=");
      long2hex(cmd.h.command,s);
      display_puts(s);
      display_puts(" arg=");
      long2hex(cmd.h.arg,s);
      display_puts(s);
      display_puts("\n");
      rc=-1;
      break;
    }
    //if(rc>=0 && ata_has_request!=0)
    //  rc=ata_response();

    if(rc<0) {
      display_puts("*** ata driver terminate ***\n");
      return 255;
    }

  }
  return 0;
}
