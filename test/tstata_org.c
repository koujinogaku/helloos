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

  ATA_DEVC_INTMASK	= 0x02,
  ATA_DEVC_RESET	= 0x04,
  ATA_DEVC_BIT3		= 0x08,
  ATA_DEVC_RST_DR0	= 0x10,
  ATA_DEVC_RST_DR1	= 0x20,

  // ATA Status
  ATA_STAT_ERR		= 0x01, // Error
  ATA_STAT_INDEX	= 0x02,
  ATA_STAT_COR		= 0x04, // Corrected Error
  ATA_STAT_DRQ		= 0x08, // Data Transfer Request
  ATA_STAT_DSC		= 0x10, // Device Seek Complete
  ATA_STAT_DF		= 0x20, // Device Fault
  ATA_STAT_DRDY		= 0x40, // Drive Ready
  ATA_STAT_BUSY		= 0x80, // 
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

  // ATA command for HDD
  ATA_CMD_IDENTIFY_DEVICE	= 0xec,
  ATA_CMD_READ_SECTORS		= 0x20,
  // ATA command for ATAPI device
  ATA_CMD_DEVICE_RESET		= 0x08,
  ATA_CMD_PACKET		= 0xa0,
  ATA_CMD_IDENTIFY_PACKET_DEVICE= 0xa1,
  ATA_CMD_SERVICE		= 0xa2,

  ATA_CMD_INITIALIZE_DEVICE_PARAMETERS = 0x91, // Legacy command; set device sector size for ATAPI

  // SCSI/ATAPI command
  ATAPI_CMD_TEST_UNIT_READY	= 0x00,
  ATAPI_CMD_REQUEST_SENSE	= 0x03,
  ATAPI_CMD_START_STOP_UNIT	= 0x1b,
  ATAPI_CMD_ALLOW_MEDIUM_REMOVAL= 0x1e,
  ATAPI_CMD_READ_CAPACITY_10	= 0x25,
  ATAPI_CMD_MODE_SENSE_10	= 0x5a,
  ATAPI_CMD_MODE_SELECT_10	= 0x55,

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

#define ATA_TYPE_UNKOWN      0
#define ATA_TYPE_ATA         1
#define ATA_TYPE_ATAPI       2

struct ata_device {
  int  id;
  int  port;
  int  dev;
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

static struct ata_device ata_device[4];
static int ata_device_num=0;
static char s[16];
static int ata_queid=0;

static void delay_400ns(int port)
{
   //cpu_in8(ATA_ADR_ALTSTATUS|port);
   //cpu_in8(ATA_ADR_ALTSTATUS|port);
   //cpu_in8(ATA_ADR_ALTSTATUS|port);
   //cpu_in8(ATA_ADR_ALTSTATUS|port);
   syscall_wait(1);
}

static int wait_stat(int port,int mask,int stat)
{
  int i;
  char status;
  for(i=0; i<ATA_TIMEOUT_COUNT ; i++) {
    status = cpu_in8(ATA_ADR_ALTSTATUS|port);
    if((status&(mask)) == stat)
      return 0;
  }
  return ERRNO_TIMEOUT;
}

static int wait_intr(int port,int timeout)
{
  int rc;
  int alarm;
  struct msg_head msg;

  alarm=syscall_alarm_set(timeout,ata_queid,port);
  msg.size=sizeof(msg);
  rc=syscall_que_get(ata_queid,&msg);
  syscall_alarm_unset(alarm,ata_queid);
  if(rc<0) {
    return ERRNO_CTRLBLOCK;
  }
  if(msg.service==MSG_SRV_ALARM) {
    return ERRNO_TIMEOUT;
  }
    display_puts("+");
    word2hex(msg.service,s);
    display_puts(s);
    word2hex(msg.command,s);
    display_puts(s);
    word2hex(msg.arg,s);
    display_puts(s);
    msg.size=sizeof(msg);
  return 1;
}

static void read_data_port(int port, int readsize, void *bufp, int bytesize)
{
  int size,buffsize;
  int i;
  unsigned short *p;

  size=readsize/2+(readsize&1);
  buffsize=bytesize/2;
  p=(unsigned short*)(bufp);
  for(i=0;i<size;i++) {
    short tmp;
    tmp = cpu_in16(ATA_ADR_DATA | port);
    if(i<buffsize) {
      *p=tmp;
      p++;
    }
  }
}

static void write_data_port(int port, void *bufp, int bytesize)
{
  int buffsize;
  int i;
  unsigned short *p;

  buffsize=bytesize/2;
  p=(unsigned short*)(bufp);
  for(i=0;i<buffsize;i++,p++) {
    cpu_out16(ATA_ADR_DATA | port, *p);
  }
}

static int select_device(int port, int dev)
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
  //  delay_400ns(port);
  //  return 1;
  //}
  //lastdev[portp]=dev;


  rc=wait_stat(port,ATA_STAT_BUSY|ATA_STAT_DRQ,0);
  if(rc<0) {
    res8 = cpu_in8(ATA_ADR_ALTSTATUS|port);
    if(res8 != 0xff) {
display_puts("*wait_stat1inselect=");
byte2hex(res8,s);
display_puts(s);
display_puts("*");
      return rc;
    }
  }

  // select device
  cpu_out8(ATA_ADR_HEAD_DEV | port, dev);
  delay_400ns(port);

  rc=wait_stat(port,ATA_STAT_BUSY|ATA_STAT_DRQ,0);
  if(rc<0) {
display_puts("*wait_stat2inselect");
res8 = cpu_in8(ATA_ADR_ALTSTATUS|port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return rc;
  }

  rc = cpu_in8(ATA_ADR_STATUS | port);
  if((rc&0xff) == 0xff) {
display_puts("*ATA_ADR_STATUSinselect*");
    return ERRNO_NOTEXIST;
  }

  return 1;
}

int ata_read_pio(int port,int dev, int command, unsigned long long *lba, int *feature, int *count, void *bufp, int bytesize)
{
  unsigned char res8;
  int rc;
  struct msg_head msg;

  rc = select_device(port, dev);
  if(rc<0) {
display_puts("*selectdev*");
    return rc;
  }

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
  // Feature
  if(feature)
    cpu_out8(ATA_ADR_FEATURES | port, *feature);

  // Count
  if(count)
    cpu_out8(ATA_ADR_SECTORCNT | port, *count);

  if(lba) {
    cpu_out8(ATA_ADR_LBALOW   | port, (*lba)&0x00ff);
    cpu_out8(ATA_ADR_LBAMID   | port, ((*lba)>>8)&0x00ff);
    cpu_out8(ATA_ADR_LBAHIGH  | port, ((*lba)>>16)&0x00ff);
    cpu_out8(ATA_ADR_HEAD_DEV | port, dev|(((*lba)>>24)&0x000f) );
  }

  // Command
  cpu_out8(ATA_ADR_COMMAND | port, command);

  rc = wait_intr(port, 100);
  if(rc<0) {
display_puts("*wait_intr1*");
  res8 = cpu_in8(ATA_ADR_STATUS | port);
display_puts("*STAT=");
byte2hex(res8,s);
display_puts(s);
res8 = cpu_in8(ATA_ADR_ERROR | port);
display_puts(" ERR=");
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return rc;
  }

  res8 = cpu_in8(ATA_ADR_STATUS | port);
  if(res8&ATA_STAT_ERR) {
display_puts("*Status=");
byte2hex(res8,s);
display_puts(s);
display_puts("*");
display_puts("*ATA_STAT_ERR=");
res8 = cpu_in8(ATA_ADR_ERROR | port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
display_puts("*ireason=");
byte2hex(rc,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }

  read_data_port(port, bytesize, bufp, bytesize);
if(lba) {
display_puts("[read=");
int2dec(bytesize,s);
display_puts(s);
display_puts("]");
display_puts("[count=");
int2dec(*count,s);
display_puts(s);
display_puts("]");
}


  res8 = cpu_in8(ATA_ADR_ALTSTATUS|port);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERR2=");
res8 = cpu_in8(ATA_ADR_ERROR | port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }
  res8 = cpu_in8(ATA_ADR_STATUS | port);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERR3=");
res8 = cpu_in8(ATA_ADR_ERROR | port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }
//if(lba) {
display_puts("[done=");
byte2hex(res8,s);
display_puts(s);
display_puts("]");
//}
/*
if(lba) {
  int n=0;
  for(;;) {
    res8 = cpu_in16(ATA_ADR_DATA | port);
    res8 = cpu_in8(ATA_ADR_ALTSTATUS|port);
    n++;
    if((res8&ATA_STAT_DRQ)==0)
      break;
  }
  display_puts("[n=");
  int2dec(n,s);
  display_puts(s);
  display_puts("]");
}
*/

  return 0;
}

int ata_packet_pio(struct ata_device *device, void *cmd, int cmdsize, void *bufp, int bytesize)
{
  unsigned char res8;
  int rc;
  int readsize;
  unsigned int readsize_h,readsize_l;
  struct msg_head msg;

  if(device->i_type != ATA_TYPE_ATAPI)
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
  rc = select_device(device->port, device->dev|ATA_DEV_ATAPI);
  if(rc<0)
    return rc;

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

display_puts("*alloc=");
int2dec(bytesize,s);
display_puts(s);
display_puts("*");

  // Features = PIO
  cpu_out8(ATA_ADR_FEATURES  | device->port, 0);    // OVL=No DMA=No

  cpu_out8(ATA_ADR_SECTORCNT    | device->port, 0); // Tag
  cpu_out8(ATA_ADR_SECTORNUM    | device->port, 0); // N/A

  // buffer size for PIO mode
  cpu_out8(ATA_ADR_CYLINDERLOW  | device->port, bytesize&0x00ff);
  cpu_out8(ATA_ADR_CYLINDERHIGH | device->port, (bytesize>>8)&0x00ff);

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_CMD_PACKET);
  delay_400ns(device->port);

  rc=wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
    return rc;
  }

//display_puts("*pktsnd*");

  res8 = cpu_in8(ATA_ADR_ALTSTATUS | device->port);
  if(res8&ATA_STAT_ERR) {
    // clear interrupt message in error
    res8 = cpu_in8(ATA_ADR_STATUS | device->port);
    wait_intr(device->port,100);
    return ERRNO_DEVICE;
  }

//display_puts("*noerror*");

  // data request status
  if((res8&ATA_STAT_DRQ)==0) {
    return ERRNO_DEVICE;
  }

//display_puts("*datareq*");

  // send scsi/atapi command
  write_data_port(device->port, cmd, cmdsize);


    display_puts("*scsi");
    byte2hex(((char*)cmd)[0],s);
    display_puts(s);
    display_puts("*");

//display_puts("*sndcmd[sz=");
//int2dec(cmdsize,s);
//display_puts(s);
//display_puts("]*");

  rc = wait_intr(device->port, 500);
  if(rc<0) {
int2dec(-rc,s);
display_puts("intr rc=");
display_puts(s);
display_puts(" error");
    return rc;
  }

//display_puts("*recvintr*");

  // Status
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
  if(res8&ATA_STAT_ERR) {
display_puts("*Status=");
byte2hex(res8,s);
display_puts(s);
if(res8&ATA_STAT_ERR)
  display_puts(" ERR");
if(res8&ATA_STAT_DRQ)
  display_puts(" DRQ");
if(res8&ATA_STAT_DSC)
  display_puts(" SERV");
if(res8&ATA_STAT_DF)
  display_puts(" DF");
if(res8&ATA_STAT_DRDY)
  display_puts(" DRDY");
if(res8&ATA_STAT_BUSY)
  display_puts(" BUSY");
display_puts("*");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
display_puts("*ERR=");
byte2hex(res8,s);
display_puts(s);
display_puts(" SenseKey=");
int2dec(res8>>4,s);
display_puts(s);
if(res8&ATA_ERR_ILI)
  display_puts(" ILI");
if(res8&ATA_ERR_EOM)
  display_puts(" EOM");
if(res8&ATA_ERR_ABRT)
  display_puts(" ABRT");
display_puts("*");
  rc = cpu_in8(ATA_ADR_SECTORNUM | device->port);
display_puts("*ireason=");
byte2hex(rc,s);
display_puts(s);
display_puts("*");

  // Receive data size
  readsize_h = cpu_in8(ATA_ADR_CYLINDERHIGH | device->port);
  readsize_l = cpu_in8(ATA_ADR_CYLINDERLOW  | device->port);
  readsize = (readsize_h << 8)|readsize_l;
display_puts("*resbyte=");
int2dec(readsize,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }

  rc = cpu_in8(ATA_ADR_SECTORNUM | device->port);
display_puts("*ireason=");
int2dec(rc,s);
display_puts(s);
display_puts("*");

  if((res8&ATA_STAT_DRQ)==0 && bytesize!=0) {
display_puts("*NO DATA");
display_puts(" ATA_STAT_ERRP=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_DEVICE;
  }
//display_puts("*noerror*");

  // Receive data size
  readsize_h = cpu_in8(ATA_ADR_CYLINDERHIGH | device->port);
  readsize_l = cpu_in8(ATA_ADR_CYLINDERLOW  | device->port);
  readsize = (readsize_h << 8)|readsize_l;

display_puts("*recv=");
int2dec(readsize,s);
display_puts(s);
display_puts("*");
  read_data_port(device->port, readsize, bufp, bytesize);
  if(bytesize>0)
    rc = wait_intr(device->port, 100);

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

  return 0;
}

int atapi_test_unit_ready(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_TEST_UNIT_READY, 0, 0, 0, 18, 0, 0,0,0,0,0,0 };
  int rc;

  rc = ata_packet_pio(device, cmd, sizeof(cmd), 0, 0);
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

int ata_device_reset(struct ata_device *device)
{
  int rc;
  // select device
  cpu_out8(ATA_ADR_HEAD_DEV | device->port, device->dev);
  delay_400ns(device->port);

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_CMD_DEVICE_RESET);
  delay_400ns(device->port);

  rc=wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
    display_puts(" *RESET ERROR*");
    return rc;
  }
  return 1;
}

int ata_check_reset(struct ata_device *device)
{
  unsigned char res8;
  int rc;

  rc = select_device(device->port, device->dev);
  if(rc<0) {
    display_puts(" DEVSELECTERROR");
    return rc;
  }

  display_puts(" DEV=");
  res8 = cpu_in8(ATA_ADR_SECTORCNT | device->port);
  byte2hex(res8,s);
  display_puts(s);
  res8 = cpu_in8(ATA_ADR_LBAHIGH | device->port);
  byte2hex(res8,s);
  display_puts(s);
  res8 = cpu_in8(ATA_ADR_LBAMID  | device->port);
  byte2hex(res8,s);
  display_puts(s);
  res8 = cpu_in8(ATA_ADR_LBALOW  | device->port);
  byte2hex(res8,s);
  display_puts(s);

  return 1;
}

int ata_probe_device(struct ata_device *device, int id, int port, int dev)
{
  unsigned short databuff[256];
  char *scsibuff;
  char infostr[80];
  int rc;

display_puts("\n******id=");
int2dec(id,s);
display_puts(s);
display_puts("******\n");

  memset(device,0,sizeof(struct ata_device));
  device->id   = id;
  device->port = port;
  device->dev  = dev;
  ata_check_reset(device);
  ata_device_reset(device);
  ata_check_reset(device);

  if(port==ATA_ADR_PRIMARY)
    rc = ata_read_pio(port, dev, ATA_CMD_IDENTIFY_DEVICE, 0, 0, 0, &databuff, sizeof(databuff));
  else
    rc = -1;

  if(rc<0) {
display_puts("\n");

//  rc = ata_read_pio(port, dev, ATA_CMD_DEVICE_RESET, 0, 0, 0, 0, 0);
//  if(rc<0) {
//    display_puts(" *Reset Error*");
//    return rc;
//  }

    rc = ata_read_pio(port, dev, ATA_CMD_IDENTIFY_PACKET_DEVICE, 0, 0, 0, &databuff, sizeof(databuff));
  }
  if(rc<0)
    return rc;

  if((databuff[0]&0x8000)==0) {
    device->i_type = ATA_TYPE_ATA;
    display_puts(" ATA");
  }
  else if((databuff[0]&0xc000)==0x8000) {
    device->i_type = ATA_TYPE_ATAPI;
    display_puts(" ATAPI");
  }
  else if((databuff[0]&0xc000)==0xc000) {
    device->i_type = ATA_TYPE_UNKOWN;
    display_puts(" Reserved");
  }

  device->p_type = (databuff[0]>>8)&0x001f;
  display_puts(" Type=");
  if(device->p_type==0||device->p_type==5) {
    display_puts("Cdrom");
  }
  if(device->p_type==1) {
    display_puts("Tape");
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

  device->info = malloc(sizeof(databuff));
  memcpy(device->info,databuff,sizeof(databuff));

  if(device->i_type!=ATA_TYPE_ATAPI) {
    unsigned long long lba=0;
    int count=1;
    unsigned char *diskbuff;

    diskbuff=malloc(ATA_SECTOR_SIZE);
    memset(diskbuff,0,ATA_SECTOR_SIZE);

    rc = wait_stat(port,ATA_STAT_DRDY|ATA_STAT_DRDY,ATA_STAT_DRDY);
    if(rc<0) {
      display_puts("*drive not ready*");
      return ERRNO_DEVICE;
    }
    rc = ata_read_pio(port, dev, ATA_CMD_READ_SECTORS, &lba, 0, &count, diskbuff, ATA_SECTOR_SIZE);
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

  ///////////////////////////////////////////////////////////////////////
  //    Access to ATAPI device
  ///////////////////////////////////////////////////////////////////////
  int count = ATAPI_SECTOR_SIZE;
  rc = ata_read_pio(port, dev|ATA_DEV_ATAPI, ATA_CMD_INITIALIZE_DEVICE_PARAMETERS, 0, 0, &count, 0, 0);
  if(rc<0) {
    display_puts("*NotSupINITDEVPARM*");
  }

//  rc = atapi_request_sense(device);
//  if(rc<0)
//    return rc;

//  ata_device_reset(device);

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



  // Inquiry The Peripheral device type from ATAPI Device
  static char cmd[12] = { 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0};

//  static char cmd[] = { ATAPI_CMD_READ12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//  long long lba;
//  static char cmd[6] = { ATAPI_CMD_INQUIRY, 0, 0, ATAPI_SECTOR_SIZE>>8, ATAPI_SECTOR_SIZE||0xff, 0 };
//  static char cmd[6] = { 0, 0, 0, 0, 200, 0 };
//  static char cmd[12] = { ATAPI_CMD_INQUIRY, ATAPI_CMD_INQUIRY, 0, ATAPI_SECTOR_SIZE>>8, ATAPI_SECTOR_SIZE||0xff, 0, 0,0,0,0,0,0 };
//  static char cmd[6] = { ATAPI_CMD_INQUIRY, ATAPI_CMD_INQUIRY, 0, ATAPI_SECTOR_SIZE>>8, ATAPI_SECTOR_SIZE||0xff, 0 };
//  static char cmd[12] = { ATAPI_CMD_INQUIRY, ATAPI_CMD_INQUIRY, 0, ATAPI_SECTOR_SIZE>>8, ATAPI_SECTOR_SIZE||0xff, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_INQUIRY, 0, 0, 0, 96, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_REPORT_LUNS, 0, 0, 0, 0, 0, 0,0,ATAPI_SECTOR_SIZE>>8,ATAPI_SECTOR_SIZE&0xff,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READ_12, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READ_10, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READ_CD, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READTOC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//
// Correct Command Set
//  static char cmd[12] = { ATAPI_CMD_TEST_UNIT_READY, 0, 0, 0, 0, 0,  0,0,0,0,0,0};
//  static char cmd[12] = { ATAPI_CMD_REQUEST_SENSE, 0, 0, 0, 18, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_START_STOP_UNIT, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_READ_CAPACITY_10, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_ALLOW_MEDIUM_REMOVAL, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
//  static char cmd[12] = { ATAPI_CMD_MODE_SENSE_10, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };

  //int allocsize = ATAPI_SECTOR_SIZE;

  unsigned long lba;
  lba = 0x10;

// INQUIRY
//  int allocsize = 96;
//  cmd[0] = ATAPI_CMD_INQUIRY;
//  cmd[4] = allocsize;

// READ12
//  int allocsize = ATAPI_SECTOR_SIZE;
//  cmd[0] = ATAPI_CMD_READ_12;
//  cmd[2] = (lba >> 0x18) & 0xFF;   // most sig. byte of LBA
//  cmd[3] = (lba >> 0x10) & 0xFF;
//  cmd[4] = (lba >> 0x08) & 0xFF;
//  cmd[5] = (lba >> 0x00) & 0xFF;   // least sig. byte of LBA
//  cmd[9] = 1;                      // sector count

// READ10
  int allocsize = 32768; //ATAPI_SECTOR_SIZE;
  cmd[0] = ATAPI_CMD_READ_10;
  cmd[1] = 0;                    // N/A
  cmd[2] = (lba >> 24) & 0xFF;   // most sig. byte of LBA
  cmd[3] = (lba >> 16) & 0xFF;
  cmd[4] = (lba >>  8) & 0xFF;
  cmd[5] = (lba >>  0) & 0xFF;   // least sig. byte of LBA
  cmd[6] = 0;                    // N/A
  cmd[7] = 0;                    // sector count high
  cmd[8] = 1;                    // sector count low

// START STOP
//  int allocsize = 0;
//  cmd[0] = ATAPI_CMD_START_STOP_UNIT;
//  cmd[1] = 0; // Immed=0
//  cmd[4] = 0x2 | 0x1;  // LoEj=1, Start=1 => Load the Disc(Close Tray)

// READ_CAPACITY_10

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

// ALLOW_MEDIUM_REMOVAL
//  int allocsize = 0;
//  cmd[0] = ATAPI_CMD_ALLOW_MEDIUM_REMOVAL;
//  cmd[4] = 0x01;                   // Prevent=1 => Lock

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
    ata_check_reset(device);
    ata_device_reset(device);
    ata_check_reset(device);
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

  display_puts("\n");
  return 1;
}

int ata_init(void)
{
  int rc;

  ata_queid = environment_getqueid();
  if(ata_queid==0) {
    syscall_puts("mou_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }

  // Reset Primary & Secondary
  cpu_out8(ATA_ADR_DEVCONTROL|ATA_ADR_PRIMARY, ATA_DEVC_INTMASK|ATA_DEVC_RESET|ATA_DEVC_BIT3);
  syscall_wait(1);
  cpu_out8(ATA_ADR_DEVCONTROL|ATA_ADR_PRIMARY, ATA_DEVC_INTMASK|ATA_DEVC_BIT3);
  syscall_wait(5);

  rc=syscall_intr_regist(PIC_IRQ_IDE0, ata_queid);
  if(rc<0) {
    ata_queid = 0;
    display_puts("mou_init intrreg=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  rc=syscall_intr_regist(PIC_IRQ_IDE1, ata_queid);
  if(rc<0) {
    ata_queid = 0;
    display_puts("mou_init intrreg=");
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
  ata_device_num=0;
//  if(ata_probe_device(&ata_device[ata_device_num], ata_device_num, ATA_ADR_PRIMARY,   ATA_DEV_MASTER)>0)
//    ata_device_num++;
//  if(ata_probe_device(&ata_device[ata_device_num], ata_device_num, ATA_ADR_PRIMARY,   ATA_DEV_SLAVE)>0)
//    ata_device_num++;
  if(ata_probe_device(&ata_device[ata_device_num], ata_device_num, ATA_ADR_SECONDARY, ATA_DEV_MASTER)>0)
    ata_device_num++;
  if(ata_probe_device(&ata_device[ata_device_num], ata_device_num, ATA_ADR_SECONDARY, ATA_DEV_SLAVE)>0)
    ata_device_num++;

  return 0;
}

int ata_read_sector(int devno, int head, int cyl, int sec)
{
  return 0;
}

int start(int ac, char *av[])
{
  long long tmp;

  ata_init();

  tmp=0;

  int2dec(sizeof(tmp),s);

  display_puts("longlong size");
  display_puts(s);

  return 0;
}
