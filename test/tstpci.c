#include "config.h"
#include "string.h"
#include "display.h"
#include "syscall.h"
#include "errno.h"
#include "print.h"

// --- copy from kernel/desc.h
enum {
  DESC_KERNEL_DATA	= 1,
  DESC_KERNEL_CODE	= 2,
  DESC_KERNEL_STACK	= 3,
  DESC_KERNEL_TASK	= 4,
  DESC_KERNEL_TASKBAK	= 5,
  DESC_USER_DATA	= 6,
  DESC_USER_CODE	= 7,
  DESC_USER_STACK	= 8,
};
#define DESC_SELECTOR(x) ((x)*8)
// ---

struct pci_bios {
  unsigned long signature;	/* _32_ */
  unsigned long entry;		/* 32 bit physical address */
  unsigned char revision;		/* Revision level, 0 */
  unsigned char length;		/* Length in paragraphs should be 01 */
  unsigned char checksum;		/* All bytes must add up to zero */
  unsigned char reserved[5]; 	/* Must be zero */
};

//static unsigned long bios32_entry = 0;
struct pci_far_addr {
  unsigned long address;
  unsigned short segment;
};
struct pci_far_addr pci_bios_entry = { 0, DESC_SELECTOR(DESC_USER_CODE) };

static char s[80];

#define PCI_SERVICE		(('$' << 0) + ('P' << 8) + ('C' << 16) + ('I' << 24))


static void dump(char *addr)
{
  int i,j;
  for(j=0;j<2;j++) {
    long2hex((unsigned long)addr,s);
    display_puts(s);
    for(i=0;i<16;i++,addr++) {
      byte2hex(*addr,s);
      display_puts(" ");
      display_puts(s);
    }
    display_puts("\n");
    syscall_wait(50);
  }

}



static void *pci_scan_bios(void)
{
  unsigned char *bios_addr;
  for(bios_addr = (void*)CFG_MEM_BIOSSTART;
      (unsigned long)bios_addr < CFG_MEM_BIOSMAX-sizeof(struct pci_bios) ;
      bios_addr++  )
  {
    if(memcmp(bios_addr,"_32_",4)!=0)
      continue;
    struct pci_bios *info=(void*)bios_addr;
    if(info->revision!=0)
      continue;
    if(info->length!=(unsigned char)1)
      continue;
    unsigned char i,checksum=0;
    for(i=0;i<sizeof(struct pci_bios);i++) {
      checksum += bios_addr[i];
    }
    if(checksum!=info->checksum)
      continue;

display_puts("entry=");
long2hex(info->entry,s);
display_puts(s);
display_puts(",rev=");
int2dec(info->revision,s);
display_puts(s);
display_puts(",len=");
int2dec(info->length,s);
display_puts(s);
display_puts(",chk=");
byte2hex(info->checksum,s);
display_puts(s);
display_puts("\n");

    return (void*)info->entry;
    //display_puts(".");
    //syscall_wait(100);
  }
  display_puts("PCI BIOS is not found\n");
  dump((void*)CFG_MEM_BIOSSTART);
  return 0;
}

#pragma GCC push_options
#pragma GCC optimize("O0")

static unsigned long pci_bios32_service_directory(unsigned long service)
{
  unsigned char return_code; /* %al */
  unsigned long address;     /* %ebx */
  unsigned long length;      /* %ecx */
  unsigned long entry;       /* %edx */
  unsigned long call_addr = (unsigned long)&pci_bios_entry;

  asm volatile(
    "lcall (%%edi)"
   :"=a" (return_code),
    "=b" (address),
    "=c" (length),
    "=d" (entry)
   :"0" (service),
    "1" (0),
    "D" (call_addr)
  );

  switch (return_code) {
  case 0:
    return address + entry;
  case 0x80: /* Not present */
    print_format("bios32_service(%d) : not present\n", service);
    return 0;
  default: /* Shouldn't happen */
    print_format("bios32_service(%d) : returned 0x%x\n",
    service, return_code);
    return 0;
  }
}

#pragma GCC pop_options

int pci_init(void)
{
  pci_bios_entry.address=(unsigned long)pci_scan_bios();
  if(pci_bios_entry.address==0) {
    return ERRNO_NOTEXIST;
  }

  unsigned long addr = pci_bios32_service_directory(PCI_SERVICE);
  if(addr)
    display_puts("In service");
  else
    display_puts("Out of service");
  return 0;
}


int start(int ac,char *av[])
{
  pci_init();
  return 0;
}
