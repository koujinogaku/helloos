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
#include "keyboard.h"

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

  ATA_DEVC_INTMASK	= 0x02, // nIEN bit: The enable bit for the device assertion of INTRQ to the host.
  ATA_DEVC_RESET	= 0x04, // SRST bit: The host software reset bit
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

#define ATA_TYPE_UNKOWN      0
#define ATA_TYPE_ATA         1
#define ATA_TYPE_ATAPI       2

struct ata_device {
  int  id;
  int  port;
  int  dev;
  int  sectorsize;
  int  phase;
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
  return ERRNO_TIMEOUT;
}

static void ata_io_read_data(int port, int readsize, void *bufp, int bytesize)
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

static void ata_io_write_data(int port, void *bufp, int bytesize)
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
  ata_io_delay_400ns(port);

  rc=ata_io_wait_stat(port,ATA_STAT_BUSY|ATA_STAT_DRQ,0);
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

//
// ****************** ATA Protocol *********************
//
static int ata_wait_intr(struct ata_device *device,int timeout)
{
  int rc;
  int alarm;
  struct msg_head msg;

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
    display_puts("Intr(svc=");
    word2hex(msg.service,s);
    display_puts(s);
    display_puts(",cmd=");
    word2hex(msg.command,s);
    display_puts(s);
    display_puts(",arg=");
    word2hex(msg.arg,s);
    display_puts(s);
    display_puts(")");
    msg.size=sizeof(msg);
  return 1;
}

// ******************** Clear the queue for initrupt message *****************
static void clear_msg(char *str)
{
  struct msg_head msg;
  msg.size=sizeof(msg);
  while(syscall_que_tryget(ata_queid,&msg)>=0) {
    display_puts("(@");
    display_puts(str);
    display_puts("=@");
    word2hex(msg.service,s);
    display_puts(s);
    word2hex(msg.command,s);
    display_puts(s);
    word2hex(msg.arg,s);
    display_puts(s);
    display_puts("@)");
    msg.size=sizeof(msg);
  }
}

int ata_read_pio(struct ata_device *device, int command, unsigned long long *lba, int *feature, int *count, void *bufp, int bytesize)
{
  unsigned char res8;
  int rc;

// ******************** Select Drive *****************
  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0) {
display_puts("*selectdev*");
    return rc;
  }

// ******************** Clear the queue for initrupt message *****************
  clear_msg("readpio");
// ******************** Send ATA Command of PACKET *****************
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

// ******************** Awaiting ATA command *****************
    rc = ata_wait_intr(device, 100);
    if(rc<0) {
display_puts("*wait_intr1*");
      res8 = cpu_in8(ATA_ADR_STATUS | device->port);
display_puts("*STAT=");
byte2hex(res8,s);
display_puts(s);
      res8 = cpu_in8(ATA_ADR_ERROR | device->port);
display_puts(" ERR=");
byte2hex(res8,s);
display_puts(s);
display_puts("*");
      return rc;
    }

// ******************** Check command status *****************
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
    return ERRNO_NOTEXIST;
  }

// ******************** data transfer *****************
  ata_io_read_data(device->port, bytesize, bufp, bytesize);
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

//  if(command==ATA_ICMD_IDENTIFY_DEVICE) {
//    rc = ata_wait_intr(device, 500);
//    if(rc<0)
//      return rc;
//  }

// ******************** Check transfer status *****************
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
display_puts("[done=");
byte2hex(res8,s);
display_puts(s);
display_puts("]");
//}
/*
if(lba) {
  int n=0;
  for(;;) {
    res8 = cpu_in16(ATA_ADR_DATA | device->port);
    res8 = cpu_in8(ATA_ADR_ALTSTATUS|device->port);
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
void ata_error_dsp_packet(struct ata_device *device, unsigned char res8)
{
  int rc;
  int readsize;
  unsigned int readsize_h,readsize_l;

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
display_puts(":");
display_puts(atapi_sense_key_message[res8>>4]);
if(res8&ATA_ERR_ILI)
  display_puts(" ILI");
if(res8&ATA_ERR_EOM)
  display_puts(" EOM");
if(res8&ATA_ERR_ABRT)
  display_puts(" ABRT");
display_puts("*");

  rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
display_puts("*ireason-cmd=");
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
}

int ata_packet_pio(struct ata_device *device, int feature, void *cmd, int cmdsize, void *bufp, int bytesize, int *sensekey)
{
  unsigned char res8;
  int rc;
  int readsize;
  unsigned int readsize_h,readsize_l;
  struct msg_head msg;
  *sensekey = 0;

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
// ******************** Select Drive *****************
  rc = ata_io_select_device(device->port, device->dev);
  if(rc<0)
    return rc;

// ******************** Clear the queue for initrupt message *****************
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

// ******************** Send ATA Command of PACKET *****************

display_puts("*alloc=");
int2dec(bytesize,s);
display_puts(s);
display_puts("*");

  // Features = PIO
  cpu_out8(ATA_ADR_FEATURES  | device->port, feature);    // OVL=No DMA=No

  cpu_out8(ATA_ADR_SECTORCNT    | device->port, 0xa8); // Tag
  cpu_out8(ATA_ADR_SECTORNUM    | device->port, 0); // N/A

  // buffer size for PIO mode
  cpu_out8(ATA_ADR_CYLINDERLOW  | device->port, bytesize&0x00ff);
  cpu_out8(ATA_ADR_CYLINDERHIGH | device->port, (bytesize>>8)&0x00ff);

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_ICMD_PACKET);
  ata_io_delay_400ns(device->port);

// ******************** Awaiting ATA command *****************
  rc=ata_io_wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
    return rc;
  }

//display_puts("*pktsnd*");

// ******************** Check ATA command status *****************
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


  display_puts("*scsi(");
  byte2hex(((char*)cmd)[0],s);
  display_puts(s);
  display_puts(")*");

//display_puts("*sndcmd[sz=");
//int2dec(cmdsize,s);
//display_puts(s);
//display_puts("]*");

// ******************** Awaiting PACKET command *****************
  rc = ata_wait_intr(device, 500);
  if(rc<0) {
int2dec(-rc,s);
display_puts("intr rc=");
display_puts(s);
display_puts(" error");
    return rc;
  }

    rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
display_puts("*ireason-cmd0=");
byte2hex(rc,s);
display_puts(s);
display_puts("*");
//display_puts("*recvintr*");

// ******************** Check command status *****************
  // Status
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
display_puts("*Status=");
byte2hex(res8,s);
display_puts(s);
  if(res8&ATA_STAT_ERR) {
ata_error_dsp_packet(device, res8);
    *sensekey = (res8>>4);
    return ERRNO_DEVICE;
  }

  rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
display_puts("*ireason-cmd=");
byte2hex(rc,s);
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

// ******************** data transfer *****************
  // Receive data size
  readsize_h = cpu_in8(ATA_ADR_CYLINDERHIGH | device->port);
  readsize_l = cpu_in8(ATA_ADR_CYLINDERLOW  | device->port);
  readsize = (readsize_h << 8)|readsize_l;

display_puts("*recv=");
int2dec(readsize,s);
display_puts(s);
display_puts("*");
  ata_io_read_data(device->port, readsize, bufp, bytesize);

// ******************** Awaiting Data transmission *****************
  if(bytesize>0) {
    rc = ata_wait_intr(device, 100);
    if(rc<0) {
display_puts("*wait-trn=");
int2dec(-rc,s);
display_puts(s);
display_puts("*");
    }
    else {
      rc = cpu_in8(ATA_ADR_SECTORCNT | device->port);
display_puts("*ireason-trn=");
byte2hex(rc,s);
display_puts(s);
display_puts("*");
    }
  }

// ******************** Check transfer status *****************
  // Status
  res8 = cpu_in8(ATA_ADR_STATUS | device->port);
display_puts("*Status-trn=");
byte2hex(res8,s);
display_puts(s);
  if(res8&ATA_STAT_ERR) {
display_puts("*ATA_STAT_ERRP=");
res8 = cpu_in8(ATA_ADR_ERROR | device->port);
byte2hex(res8,s);
display_puts(s);
display_puts("*");
    return ERRNO_NOTEXIST;
  }

  return readsize;
}

//
// ****************** ATAPI Protocol as SCSI Command *********************
//
int atapi_test_unit_ready(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_TEST_UNIT_READY, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int feature = 0;
  int sensekey;

  int rc;

display_puts("\n*test_unit_ready*");
  rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), 0, 0, &sensekey);
  if(rc<0) {
display_puts("*test unit error*");
    return rc;
  }

  return 1;
}

int atapi_request_sense(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_REQUEST_SENSE, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int feature = 0;
  int sensekey;

  unsigned char res[30];
  int rc;
  int rescode;

display_puts("\n*request_sense*");
  memset(&res,0,sizeof(res));

  cmd[4] = sizeof(res);

  rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), &res, sizeof(res), &sensekey);
  if(rc<0) {
display_puts("*request sense error*");
    return rc;
  }

  rescode=(res[0])&0x7f;
  display_puts(" Res=");
  byte2hex(rescode,s);
  display_puts(s);
  if(rescode==0x70 || rescode==0x71) {
    display_puts(" Segment=");
    int2dec(res[1],s);
    display_puts(s);
    if(res[0]&0x80)
      display_puts(" Valid");
    if((res[2])&0x80)
      display_puts(" Filemark");
    if((res[2])&0x40)
      display_puts(" EOM");
    if((res[2])&0x20)
      display_puts(" ILI");
    sensekey = (res[2])&0x0f;
    display_puts(" SenseKey=");
    display_puts(atapi_sense_key_message[sensekey]);
    unsigned long info;

    info = (res[3]<<(8*3))+(res[4]<<(8*2))+(res[5]<<(8*1))+(res[6]<<(8*0));
    display_puts(" Info=");
    long2hex(info,s);
    display_puts(s);

    display_puts(" AddLen=");
    int2dec(res[7],s);
    display_puts(s);

    info = (res[8]<<(8*3))+(res[9]<<(8*2))+(res[10]<<(8*1))+(res[11]<<(8*0));
    display_puts(" CmdInfo=");
    long2hex(info,s);
    display_puts(s);

    display_puts(" AddSenseCode=");
    int2dec(res[12],s);
    display_puts(s);

    display_puts(" AddSenseCodeQualfier=");
    int2dec(res[13],s);
    display_puts(s);

    display_puts(" FieldRep=");
    int2dec(res[14],s);
    display_puts(s);

  }
  else {
    sensekey = (res[1])&0x0f;
    display_puts(" SenseKey=");
    display_puts(atapi_sense_key_message[sensekey]);
  }


  return 1;
}

int atapi_start_stop(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_START_STOP_UNIT, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int feature = 0;
  int sensekey;

  int rc;

  cmd[1] = 0; // Immed=0
  cmd[4] = 0x2 | 0x1;  // LoEj=1, Start=1 => Load the Disc(Close Tray)

  rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), 0, 0, &sensekey);
  if(rc<0) {
display_puts("*start stop error*");
    return rc;
  }

  return 1;
}

int atapi_read_capacity_10(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_READ_CAPACITY_10, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int feature = 0;
  int sensekey;

  unsigned char res[18];
  int rc;
  unsigned long lba;
  unsigned long bsz;

display_puts("\n*read_capacity_10*");
  rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), &res, sizeof(res), &sensekey);
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
  int feature = 0;
  int sensekey;

  int rc;

  cmd[4] = 0x01; // Prevent=1 => Lock

  rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), 0, 0, &sensekey);
  if(rc<0) {
display_puts("*allow_medium_removal error*");
    return rc;
  }

  return 1;
}

int atapi_read_10(struct ata_device *device, unsigned long lba)
{
  static char cmd[12] = { ATAPI_CMD_READ_10, 0, 0, 0, 0, 0,  0,0,0,0,0,0};
  int feature = 0;//ATA_FEATURE_OVL;
  int sensekey;

  int allocsize = 32768; //ATAPI_SECTOR_SIZE;
  char *scsibuff;
  int readcount;
  int i;

display_puts("\n*read_10*");
  scsibuff=malloc(allocsize);
  memset(scsibuff,0,allocsize);

  cmd[2] = (lba >> 24) & 0xFF;   // most sig. byte of LBA
  cmd[3] = (lba >> 16) & 0xFF;
  cmd[4] = (lba >>  8) & 0xFF;
  cmd[5] = (lba >>  0) & 0xFF;   // least sig. byte of LBA
  cmd[7] = 0;                    // sector count high
  cmd[8] = 1;                    // sector count low

  readcount = ata_packet_pio(device, feature, cmd, sizeof(cmd), scsibuff, allocsize, &sensekey);
  if(readcount<0) {
display_puts("*read_10 error*");
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

int atapi_inquiry(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_INQUIRY, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int feature = 0;
  int sensekey;

  int allocsize = 32768; //ATAPI_SECTOR_SIZE;
  char *scsibuff;
  int readcount;
  int i;

display_puts("\n*inquiry*");
  scsibuff=malloc(allocsize);
  memset(scsibuff,0,allocsize);

  cmd[3] = allocsize&0xff;
  cmd[4] = allocsize>>8;

  readcount = ata_packet_pio(device, feature, cmd, sizeof(cmd), scsibuff, allocsize, &sensekey);
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

int atapi_get_event_status_notification(struct ata_device *device)
{
  static char cmd[12] = { ATAPI_CMD_GET_EVENT_STATUS_NOTIFICATION, 0, 0, 0, 0, 0, 0,0,0,0,0,0 };
  int feature = 0;
  int sensekey;

  int allocsize = 32768; //ATAPI_SECTOR_SIZE;
  char *scsibuff;
  int readcount;
  int i;

display_puts("\n*get event*");
  scsibuff=malloc(allocsize);
  memset(scsibuff,0,allocsize);

  cmd[1] = 0x01; // Immed
  cmd[4] = 1<<4; // RequestEvent = Media Status Class Events
  cmd[7] = allocsize&0xff;
  cmd[8] = allocsize>>8;

  readcount = ata_packet_pio(device, feature, cmd, sizeof(cmd), scsibuff, allocsize, &sensekey);
  if(readcount<0) {
display_puts("*event error*");
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
  display_puts("\n*Byte=");
  i=scsibuff[0]*256+scsibuff[1];
  int2dec(i,s);
  display_puts(s);
  if(scsibuff[2]&0x80)
    display_puts(",NEA");
  display_puts(",Class=");
  int2dec(scsibuff[2]&0x07,s);
  display_puts(s);
  display_puts(",SupportClass=");
  byte2hex(scsibuff[3],s);
  display_puts(s);
  if((scsibuff[2]&0x07) == 0x02) {
    display_puts(",PowerManEvent=");
    switch(scsibuff[4]&0x0f) {
    case 0x00:
      display_puts("NoChg");
      break;
    case 0x01:
      display_puts("PwrChg-Succ");
      break;
    case 0x02:
      display_puts("PwrChg-Fail");
      break;
    }
    switch(scsibuff[5]) {
    case 0x01:
      display_puts(",Active");
      break;
    case 0x02:
      display_puts(",Idle");
      break;
    case 0x03:
      display_puts(",Standby");
      break;
    }
  }
  else if((scsibuff[2]&0x07) == 0x04) {
    display_puts(",MediaStatusEvent=");
    switch(scsibuff[4]&0x0f) {
    case 0x00:
      display_puts("NoEvent");
      break;
    case 0x01:
      display_puts("EjectRequest");
      break;
    case 0x02:
      display_puts("NewMedia");
      break;
    case 0x03:
      display_puts("MediaRemoval");
      break;
    }
    if(scsibuff[5]&0x01)
      display_puts(",DoorOpen");
    if(scsibuff[5]&0x02)
      display_puts(",MediaPresent");
    display_puts(",StartSlot=");
    int2dec(scsibuff[6],s);
    display_puts(s);
    display_puts(",EndSlot=");
    int2dec(scsibuff[7],s);
    display_puts(s);
  }
  else if((scsibuff[2]&0x07) == 0x06) {
    display_puts(",BusyStatusEvent=");
    switch(scsibuff[4]&0x0f) {
    case 0x00:
      display_puts("NoEvent");
      break;
    case 0x01:
      display_puts("BusyEvent");
      break;
    }
    switch(scsibuff[5]) {
    case 0x00:
      display_puts("NoEvent");
      break;
    case 0x01:
      display_puts(",Power");
      break;
    case 0x02:
      display_puts(",Immediate");
      break;
    case 0x03:
      display_puts(",Defered");
      break;
    }
    display_puts(",Time=");
    int2dec(scsibuff[6]*256+scsibuff[7],s);
    display_puts(s);
  }
  display_puts("*\n");
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
  cpu_out8(ATA_ADR_HEAD_DEV | device->port, device->dev);
  ata_io_delay_400ns(device->port);

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_ICMD_EXECUTE_DEVICE_DIAGNOSTIC);
  ata_io_delay_400ns(device->port);

  rc=ata_io_wait_stat(device->port,ATA_STAT_BUSY,0);
  if(rc<0) {
    display_puts(" *DIAG ERROR*");
    return rc;
  }
  return 1;
}

int ata_device_reset(struct ata_device *device)
{
  int rc;
  // select device
  cpu_out8(ATA_ADR_HEAD_DEV | device->port, device->dev);
  ata_io_delay_400ns(device->port);

  // Command
  cpu_out8(ATA_ADR_COMMAND | device->port, ATA_ICMD_DEVICE_RESET);
  ata_io_delay_400ns(device->port);

  rc=ata_io_wait_stat(device->port,ATA_STAT_BUSY,0);
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

  rc = ata_io_select_device(device->port, device->dev);
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
  char infostr[80];
  int rc;

clear_msg("probe");

display_puts("\n******id=");
int2dec(id,s);
display_puts(s);
display_puts("******\n");

  memset(device,0,sizeof(struct ata_device));
  device->id   = id;
  device->port = port;
  device->dev  = dev|ATA_DEV_LBA;
  device->phase = 0;
  ata_check_reset(device);
clear_msg("aftchkrst1");
  ata_device_reset(device);
clear_msg("aftrst");
  ata_check_reset(device);
clear_msg("aftchkrst2");

display_puts("(Try ATA)");
  rc = ata_read_pio(device, ATA_ICMD_IDENTIFY_DEVICE, 0, 0, 0, &databuff, sizeof(databuff));

  if(rc<0) {
display_puts("(Try ATAPI)");
display_puts("\n");

//  rc = ata_read_pio(port, dev, ATA_ICMD_DEVICE_RESET, 0, 0, 0, 0, 0);
//  if(rc<0) {
//    display_puts(" *Reset Error*");
//    return rc;
//  }

    device->dev  = dev|ATA_DEV_ATAPI;
    ata_device_reset(device);
    ata_check_reset(device);
    rc = ata_read_pio(device, ATA_ICMD_IDENTIFY_PACKET_DEVICE, 0, 0, 0, &databuff, sizeof(databuff));
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
    device->sectorsize=ATAPI_SECTOR_SIZE;
  }
  else if(device->p_type==1) {
    display_puts("Tape");
    device->sectorsize=0;
  }
  else {
    display_puts("HardDisk");
    device->sectorsize=ATA_SECTOR_SIZE;
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


  display_puts("\n");
  return 1;
}


int ata_init(void)
{
  int rc;
  struct msg_head msg;

display_puts("*init*");

  ata_queid = environment_getqueid();
  if(ata_queid==0) {
    syscall_puts("mou_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }

// ******************** Clear the queue for initrupt message *****************
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

display_puts("*intrON*");
// ******************** Clear the queue for initrupt message *****************
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
  // probe devices
  ata_device_num=0;
  if(ata_probe_device(&ata_device[ata_device_num], ata_device_num, ATA_ADR_PRIMARY,   ATA_DEV_MASTER)>0)
    ata_device_num++;
  if(ata_probe_device(&ata_device[ata_device_num], ata_device_num, ATA_ADR_PRIMARY,   ATA_DEV_SLAVE)>0)
    ata_device_num++;
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


static int test_ata_access(struct ata_device *device)
{
    unsigned long long lba=0x00;
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
int n;
for(n=0;n<80;n++) {
  if(diskbuff[n]>0x20)
    display_putc(diskbuff[n]);
  else
    display_putc('.');
}
    display_puts("\n");
    return 1;

}

static int test_atapi_access(struct ata_device *device)
{
  int rc;

  for(;;) {

    clear_msg("after test");

    char key;
    display_puts("\nhit key(q=End)>");
    key = keyboard_getcode();
    if(key=='q')
      break;

//  ata_device_reset(device);

    rc = atapi_test_unit_ready(device);
    if(rc<0) {
display_puts("*error exit*");
      return rc;
    }

//    rc = atapi_get_event_status_notification(device);
//    if(rc<0) {
//display_puts("*error exit*");
//      return rc;
//    }

//  rc = atapi_start_stop(device);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }

//  rc = atapi_read_capacity_10(device);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }

//  rc = atapi_allow_medium_removal(device);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }

//  rc = atapi_inquiry(device);
//  if(rc<0) {
//display_puts("*error exit*");
//    return rc;
//  }

    unsigned long lba;
    lba = 16; // primary volume descriptor
    rc = atapi_read_10(device,lba);
//    if(rc<0) {
//  display_puts("*error exit*");
//      return rc;
//    }

    rc = atapi_request_sense(device);
    if(rc<0)
      return rc;


  } // end of for()

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

/*
  if(allocsize) {
    scsibuff=malloc(allocsize);
    memset(scsibuff,0,allocsize);
  }
  else {
    scsibuff=0;
  }
  rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), scsibuff, allocsize, &sensekey);
  //rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), scsibuff, 12, &sensekey);
  //rc = ata_packet_pio(device, feature, cmd, sizeof(cmd), 0, 0, &sensekey);
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
*/

  return 1;
}


int start(int ac, char *av[])
{

  ata_init();

  display_puts("***************************\n");

  int i;
  for(i=0;i<ata_device_num;i++) {
    if(ata_device[i].i_type == ATA_TYPE_ATAPI)
      test_atapi_access(&ata_device[i]);
    else
      test_ata_access(&ata_device[i]);

    display_puts("***************************\n");
  }

  return 0;
}
