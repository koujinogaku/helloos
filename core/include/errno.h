#ifndef ERRNO_H
#define ERRNO_H

#define ERRNO_NOTINIT     -1
#define ERRNO_RESOURCE    -2
#define ERRNO_TIMEOUT     -3
#define ERRNO_INUSE       -4
#define ERRNO_NOTEXIST    -5
#define ERRNO_SEEK        -6
#define ERRNO_RECALIBRATE -7
#define ERRNO_WRITE       -8
#define ERRNO_READ        -9
#define ERRNO_MODE       -10
#define ERRNO_OVER       -11
#define ERRNO_CTRLBLOCK  -12
#define ERRNO_DEVICE     -13

#define ERRNO_SCSI_NoSense        -14 // 0h
#define ERRNO_SCSI_RecoveredError -15 // 1h
#define ERRNO_SCSI_NotReady       -16 // 2h
#define ERRNO_SCSI_MediumError    -17 // 3h
#define ERRNO_SCSI_HardwareError  -18 // 4h
#define ERRNO_SCSI_IllegalRequest -19 // 5h
#define ERRNO_SCSI_UnitAttention  -20 // 6h
#define ERRNO_SCSI_DataProtect    -21 // 7h
#define ERRNO_SCSI_BlankCheck     -22 // 8h
#define ERRNO_SCSI_VendorSpecific -23 // 9h
#define ERRNO_SCSI_CopyAborted    -24 // Ah
#define ERRNO_SCSI_AbortedCommand -25 // Bh
#define ERRNO_SCSI_Equal          -26 // Ch
#define ERRNO_SCSI_VolumeOverflow -27 // Dh
#define ERRNO_SCSI_Miscompare     -28 // Eh
#define ERRNO_SCSI_Reserved       -29 // Fh


#endif
