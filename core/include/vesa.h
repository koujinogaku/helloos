/*
** vesa.h --- vesa information blocks
*/
#ifndef VESA_H
#define VESA_H
/* VESA structure used by INT 10h AX=4F00h */
#pragma pack(1)
typedef struct
{
	char sig[4];
	char ver_minor;
	char ver_major;
/*	char far *oem_name; */
	word16 oem_name_off, oem_name_seg;
	unsigned long capabilities;	 /* b1=1 for non-VGA board */
/*	uint16_t far *mode_list; */
	word16 mode_list_off, mode_list_seg;
	word16 vid_mem_size; /* in units of 64K */
	char reserved1[492];
} vbe_info_t;

/* VESA structure used by INT 10h AX=4F01h */
#pragma pack(1)
typedef struct
{
	word16 mode_attrib;	 /* b5=1 for non-VGA mode */
	char win_a_attrib;
	char win_b_attrib;
	word16 k_per_gran;
	word16 win_size;
	word16 win_a_seg;
	word16 win_b_seg;
	char reserved1[4];
/* this is not always the expected value;
rounded up to the next power of 2 for some video boards: */
	word16 bytes_per_row;
/* OEM modes and VBE 1.2 only: */
	word16 wd;
	word16 ht;
	char char_wd;
	char char_ht;
	char planes;
	char depth;
	char banks;
	char memory_model;
	char k_per_bank;
	char num_pages;	 /* ? */
	char reserved2;
/* VBE 1.2 only */
	char red_width;
	char red_shift;
	char green_width;
	char green_shift;
	char blue_width;
	char blue_shift;
	char reserved3[3];
	unsigned long lfb_adr;
	char reserved4[212];
} mode_info_t;

#endif /* VESA_H */

