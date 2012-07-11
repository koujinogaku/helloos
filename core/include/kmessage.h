#ifndef KMESSAGE_H
#define KMESSAGE_H

struct msg_head {
  unsigned short size;
  unsigned short service;
  unsigned short command;
  unsigned short arg;
};

#define msg_sizeof(msg)    ((sizeof(msg)+3)&0xfffc) // Rounded up to 4 byte each (Prevent the truncated in internal)
#define msg_size(sz)       (((sz)+3)&0xfffc)        // Rounded up to 4 byte each

#define MSG_QNM_KERNEL    0x0001
#define MSG_QNM_ALARM     0x0002
#define MSG_SRV_KERNEL    MSG_QNM_KERNEL
#define MSG_SRV_ALARM     MSG_QNM_ALARM

#define MSG_CMD_KRN_INTERRUPT 0x0001
#define MSG_CMD_KRN_EXIT      0x0002


#endif
