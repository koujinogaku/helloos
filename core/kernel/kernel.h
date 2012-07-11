#ifndef KERNEL_H
#define KERNEL_H

#define KERNEL_MSGOPT_EXIT   0x01

int kernel_get_bios_info(char *binfo);
struct bios_info *kernel_get_bios_info_addr(void);

#endif
