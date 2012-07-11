
/* start boot record */
asm (
	".text				\n"
	".code16gcc			\n"
	"	jmp	start		\n" /* jumpcode */
	"	nop			\n"
	"	.ascii	\"HELLOWLD\"	\n" /* system */
	"	.word	0x0200		\n" /* byte per sector */
	"	.byte	0x01		\n" /* sector per cluster 04*/
	"	.word	0x0001		\n" /* reserve sector */
	"	.byte	0x02		\n" /* fat copy */
	"	.word	0x00e0		\n" /* directory entry count 6e*/
	"	.word	0x0b40		\n" /* total sector */
	"	.byte	0xf0		\n" /* media descripter */
	"	.word	0x0009		\n" /* sector per fat */
	"	.word	0x0012		\n" /* sector per track */
	"	.word	0x0002		\n" /* head count */
	"	.long	0x00000000	\n" /* hidden sectors */
	"	.long	0x00000000	\n" /* toral sector(32MB) */
	"	.byte	0x00		\n" /* drive num */
	"	.byte	0x00		\n" /* reserve */
	"	.byte	0x29		\n" /* extend boot sig */
	"	.long	0x00000000	\n" /* volume serial */
	"	.ascii	\"NO NAME    \"	\n" /* volume label */
	"	.ascii	\"FAT12   \"	\n" /* fat type */
	"start:				\n"
	"	cli			\n"
	"	jmp	$0,$start2	\n"
	"start2:			\n"
	"	mov	%cs, %ax	\n"
	"	mov	%ax, %ds	\n"
	"	mov	%ax, %es	\n"
	"	mov	%ax, %ss	\n"
	"	mov	$0x7c00, %sp	\n"
	"	sti			\n"
	"	call	ipl_main	\n"
	"loop:	jmp	loop		\n"
);

#include "config.h"
#include "iplinfo.h"

/* display a string using the BIOS */
static void
bios_puts(const char *str)
{
	char c;
	while((c = *str++))
		Asm("int $0x10":: "a"(0x0e00 | (0xff & c)), "b"(7));
}

/* reset a FDD using the BIOS */
static inline word32
bios_reset_drive( byte drive )
{
	int ret;
	Asm("int $0x13": "=a"(ret): "a"(0), "d"(drive));
	return ((ret >> 8) & 0xff);
}

/* read a FDD using the BIOS */
static inline word32
bios_read_drive( byte drive, void* buf, word32 sec )
{
	word32 ret;
	Asm(	"int $0x13": "=a"(ret):
		"a"(0x201),
		"b"(buf),
		"c"( ( (sec/FDD_SECPTRK/2)  << 8) | (sec%FDD_SECPTRK+1) ),
		"d"( (((sec/FDD_SECPTRK)%2) << 8) | drive)			);
	return ((ret >> 8) & 0xff);
}

/* IPL main */
word32
ipl_main(void)
{
  word32 i;
  struct ipl_info *iplinfo = (void*) (CFG_MEM_START+CFG_MEM_IPLINFO);

  bios_puts("boot:");

  /* load setup program */
  if( bios_reset_drive( FDD_BOOTDRV ) ){
    goto load_error;
  }

  for( i = 0 ; i < iplinfo->setup_count ; i++ )
    {
      bios_puts(".");

      if( bios_read_drive(FDD_BOOTDRV,
		     (void*)(CFG_MEM_SETUP + i * FDD_SECSIZE), iplinfo->setup_head + i) ) {
        goto load_error;
      }
    }
  bios_puts("ok\n\r");

  /* activate setup */
  Asm("ljmp $0, %0"::"i"(CFG_MEM_SETUP));

 load_error:
  bios_puts("error");
  return 0;
}
