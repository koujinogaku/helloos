#ifndef KPROGRAM_H
#define KPROGRAM_H
#include "kmessage.h"

#define PGM_TYPE_IO  0x00000001
#define PGM_TYPE_VGA 0x00000002

int program_init(void);
int program_load(char *filename, int type);
int program_set_args(int taskid, char *args, int argsize);
int program_start(int taskid, int exitque);
int program_delete(int taskid);
int program_get_exitque(int taskid);
int program_exitevent(struct msg_head *msg);

#endif
