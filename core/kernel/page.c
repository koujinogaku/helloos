/* memory page manager */

#include "config.h"
#include "string.h"
#include "console.h"
#include "page.h"
#include "exception.h"
#include "intr.h"
#include "cpu.h"
#include "kmemory.h"
#include "errno.h"
#include "kernel.h"

#define page_pgd_index(padr)	((padr)>>22)
#define page_pte_index(padr)	(((padr)&0x003ff000)>>12)
#define page_index2vaddr(pgdidx,pteidx) (((pgdidx)<<22)|((pteidx)<<12))

struct pagelist {
  struct pagelist *next;
};
static char s[10];

struct PTE *page_system_pgd;
struct pagelist page_freelist;
static unsigned int page_total_free_page;

//void
static inline void
page_flush_cache(void)
{
  asm volatile(
    "mov %%cr3, %%eax\n"
    "mov %%eax, %%cr3\n"
    :::"eax"  );
}
//void
static inline void
page_paging_on(void)
{
  asm volatile(
    "mov %%cr0, %%eax\n"
    "or  $0x80000000, %%eax\n"
    "mov %%eax, %%cr0"
    :::"eax" );
}
//void
static inline void
page_paging_off(void)
{
  asm volatile(
    "mov %%cr0, %%eax\n"
    "and $0x7fffffff, %%eax\n"
    "mov %%eax, %%cr0"
    :::"eax" );
}
//int
static inline int
page_paging_check(void)
{
  int enabled;
  asm volatile("mov %%cr0, %0":"=r"(enabled));
  return enabled & 0x80000000;
}

//void
static inline void
page_set_pgd(void *pgd)
{
  asm volatile("mov %0, %%cr3"::"r"(pgd));
}

//void *
static inline void *
page_get_pgd(void)
{
  void *vaddr;
  asm volatile(
    "movl %%cr3, %%eax \n"
    "movl %%eax, %0 \n"
   :"=m"(vaddr)
   ::"eax"
  );
  return vaddr;
}

//void *
static inline void *
page_get_faultaddr(void)
{
  void *vaddr;
  asm volatile(
    "movl %%cr2, %%eax \n"
    "movl %%eax, %0 \n"
   :"=m"(vaddr)
   ::"eax"
  );
  return vaddr;
}


void page_free(void *vpageaddr)
{
  struct pagelist *pageaddr=vpageaddr,*pagewindow=(void*)CFG_MEM_PAGEWINDOW;
  void *pgd;

/*
  BEGIN_PAGINGOFF;
  asm("nop");
  list_add_tail(&page_freelist,pageaddr);
  END_PAGINGOFF;
*/
  BEGIN_CPULOCK();
  pgd=page_get_pgd();
  page_flush_cache();
  page_map_vpage(pgd,pagewindow,pageaddr,(PAGE_TYPE_RDWR|PAGE_TYPE_USER));
  pagewindow->next = page_freelist.next;
  page_freelist.next = pageaddr;
  page_unmap_vpage(pgd,pagewindow);
  page_flush_cache();
  END_CPULOCK();
/*
  console_puts("[pfree=");
  long2hex((unsigned int)pageaddr,s);
  console_puts(s);
  console_puts("]");
*/
  page_total_free_page++;

}
void *page_alloc(void)
{
  struct pagelist *pageaddr,*pagewindow=(void*)CFG_MEM_PAGEWINDOW;
  void *pgd;
//  int r;

/*
  BEGIN_PAGINGOFF;
  asm("nop");
  pageaddr = page_freelist.next;
  list_del(pageaddr);
  END_PAGINGOFF;
*/
  BEGIN_CPULOCK();
  pgd=page_get_pgd();
  pageaddr = page_freelist.next;
  page_map_vpage(pgd,pagewindow,pageaddr,(PAGE_TYPE_RDWR|PAGE_TYPE_USER));
  page_flush_cache();
  page_freelist.next=pagewindow->next;
  page_unmap_vpage(pgd,pagewindow);
  page_flush_cache();
  END_CPULOCK();
/*
  console_puts("[palloc=");
  long2hex((unsigned int)pageaddr,s);
  console_puts(s);
  console_puts("]");
*/
  page_total_free_page--;

  return pageaddr;
}

unsigned int
page_get_totalfree(void)
{
  return page_total_free_page;
}

int
page_map_vpage(unsigned int *pgd, void *vpage, void *ppage, int type )
{
  unsigned int *pte, pgd_item, pte_item;
  unsigned int dir_idx, tbl_idx;

  dir_idx = page_pgd_index((unsigned int)vpage);
  tbl_idx = page_pte_index((unsigned int)vpage);

  pgd_item = pgd[dir_idx];
  if( (pgd_item & PAGE_TYPE_PRESENT) != 0 )
  {
    pte = page_frame_addr(pgd_item);
  }
  else
  {
//console_puts("[PTEalloc]");
    pte = mem_alloc(PAGE_PAGESIZE);
    if(pte == 0) {
      console_puts("**** cannot allocates new page for PTE ****\n");
      return ERRNO_RESOURCE;
    }
    memset(pte,0,PAGE_PAGESIZE);
    pgd[dir_idx] = (unsigned int)page_frame_addr(pte);
    pgd[dir_idx] |= (PAGE_TYPE_PRESENT | PAGE_TYPE_USER | PAGE_TYPE_RDWR );
//    pgd[dir_idx] |= (PAGE_TYPE_PRESENT | type );
  }

  pte_item = pte[tbl_idx];

  if(pte_item & PAGE_TYPE_PRESENT) {
    console_puts("**** already mapped page entry ****\n");
/*
    console_puts(" vaddr=");
    long2hex((unsigned int)vpage,s);
    console_puts(s);
    console_puts(" paddr=");
    long2hex((unsigned int)ppage,s);
    console_puts(s);
*/

    console_puts(" pgd=");
    long2hex((unsigned int)pgd,s);
    console_puts(s);

    console_puts(" dir=");
    long2hex((unsigned int)dir_idx,s);
    console_puts(s);
    console_puts(" tbl=");
    long2hex((unsigned int)tbl_idx,s);
    console_puts(s);
/*
    console_puts(" pgd_item=");
    long2hex((unsigned int)pgd_item,s);
    console_puts(s);
*/
    console_puts(" pte=");
    long2hex((unsigned int)pte,s);
    console_puts(s);
/*
    console_puts(" pte_item=");
    long2hex((unsigned int)pte_item,s);
    console_puts(s);
*/
    console_puts("\n");


    return ERRNO_INUSE;
  }
  /* page table */
  pte[tbl_idx] = (unsigned int)page_frame_addr((unsigned int)ppage);
  pte[tbl_idx] |= (PAGE_TYPE_PRESENT | type);
/*
  console_puts("[pgd=");
  int2dec(dir_idx,s);
  console_puts(s);
  console_puts(",tbl=");
  int2dec(tbl_idx,s);
  console_puts(s);
  console_puts("]");
*/

  return 0;
}

void *
page_unmap_vpage( unsigned int *pgd, void *vpage)
{
  unsigned int *pte, pgd_item, pte_item;
  unsigned int dir_idx, tbl_idx;
  void *ppage;

  dir_idx = page_pgd_index((unsigned int)vpage);
  tbl_idx = page_pte_index((unsigned int)vpage);
  
  pgd_item = pgd[dir_idx];
  if((pgd_item & PAGE_TYPE_PRESENT)==0) {
    //console_puts("vpage not avail on pgd\n");
    return 0;
  }
  pte = page_frame_addr(pgd_item);

  pte_item = pte[tbl_idx];
  if((pte_item & PAGE_TYPE_PRESENT)==0) {
    //console_puts("vpage not avail on pte\n");
    return 0;
  }
  pte[tbl_idx] &= (~PAGE_TYPE_PRESENT);

  ppage = (void*) page_frame_addr(pte_item);
  return ppage;
}


void *
page_get_ppage(unsigned int *pgd, void *vpage)
{
  unsigned int *pte, pgd_item, pte_item;
  unsigned int dir_idx, tbl_idx;
  void *ppage;

  dir_idx = page_pgd_index((unsigned int)vpage);
  tbl_idx = page_pte_index((unsigned int)vpage);
/*
console_puts("[");
long2hex((int)dir_idx,s);
console_puts(s);
console_puts(",");
long2hex((int)tbl_idx,s);
console_puts(s);
console_puts("]");
*/

  pgd_item = pgd[dir_idx];
  if((pgd_item & PAGE_TYPE_PRESENT)==0) {
    return 0;
  }
  pte = page_frame_addr(pgd_item);

  pte_item = pte[tbl_idx];
  if((pte_item & PAGE_TYPE_PRESENT)==0) {
    return 0;
  }
  ppage = (void*) page_frame_addr(pte_item);
  return ppage;
}

int page_fault_proc( Excinfo *info )
{

  unsigned int vpage;
  void *ppage;
  void *pgd;
  char s[16];

  if( info->errcode&PAGE_TYPE_PRESENT ) {
    console_puts("*** page is not present ***\n");
    return -1;
  }
  //if( (info->errcode&PAGE_TYPE_USER)==PAGE_TYPE_SUPER )  {
  //  console_puts("*** page is supper user ***\n");
  //  console_puts("pgd(cr3)=");
  //  pgd=page_get_pgd();
  //  long2hex((unsigned long)pgd,s);
  //  console_puts(s);
  //  console_puts("\n");
  //  return -1;
  //}

  vpage = (unsigned int)page_get_faultaddr();

  if(   !(CFG_MEM_USER <= vpage && vpage < CFG_MEM_USERDATAMAX)  ) {
    console_puts("*** vpage is out of range. address=");
    long2hex(vpage,s);
    console_puts(s);
    console_puts("\n");
    return -1;
  }

  ppage=page_alloc();
  if(ppage==0) {
    console_puts("*** no memory ***\n");
    return -1;
  }

  pgd=page_get_pgd();

  if(page_map_vpage(pgd,(void*)vpage,ppage,PAGE_TYPE_USER|PAGE_TYPE_RDWR)<0) {
    page_free(ppage);
    return -1;
  }

//  console_putc('*');
  page_flush_cache();

  return 0;
}

EXCP_EXCEPTION_EC(page_fault_handler); 
void page_fault_handler( Excinfo info )
{
  void *addr;
  addr = page_get_pgd();
  if((int)addr==(int)page_system_pgd) { // Kernel Process
    console_puts("[cr3=");
    long2hex((unsigned int)addr,s);
    console_puts(s);
    console_puts(",syspgd=");
    long2hex((unsigned int)page_system_pgd,s);
    console_puts(s);
    addr = page_get_faultaddr();
    console_puts(",cr2=");
    long2hex((unsigned int)addr,s);
    console_puts(s);
    console_puts("]");
    for(;;) {
      cpu_halt(); // stop
    }
  }

  if(page_fault_proc(&info)<0) {
    excp_abort(&info, EXCP_NO_PGF, "Page Fault");
  }
}

int
page_init(void)
{
  unsigned int pageaddr, memsize;
  struct pagelist *p;

  page_total_free_page=0;
  memsize = mem_get_totalsize();

  if(mem_get_status()==0)
    return ERRNO_NOTINIT;
  if(mem_get_status()==1) // kernel heap is in low address
    pageaddr=CFG_MEM_KERNELHEAP; 
  else                    // kernel heap is in high address
    pageaddr=CFG_MEM_PAGEPOOL;

  /* setup page pool */
  page_freelist.next=0;
  p=&page_freelist;
  for(;
      pageaddr<memsize;
      pageaddr+=PAGE_PAGESIZE)
  {
    p->next=(struct pagelist*)pageaddr;
    p=(struct pagelist*)pageaddr;
    p->next=0;
    page_total_free_page++;
  }

  /* setup physical memory for kernel 0-8MB */
  page_system_pgd = page_create_pgd();
  if(page_system_pgd==0)
    return ERRNO_RESOURCE;

  page_map_vga(page_system_pgd);

  /* begin the paging mode */
  intr_define( EXCP_NO_PGF, EXCP_ENTRY_EC(page_fault_handler), INTR_DPL_USER );	// Page Fault

  page_set_pgd(page_system_pgd);
  page_paging_on();

  return 0;
}

void *
page_get_system_pgd(void)
{
  return page_system_pgd;
}
void *
page_get_current_pgd(void)
{
  return page_get_pgd();
}
void *page_get_faultvaddr(void)
{
  return page_get_faultaddr();
}
int
page_map_vmem(void *pgd, void *vmem, void *pmem, unsigned int size, int type)
{
  unsigned int ppage,vpage,pagesz,i;
  int r;

  ppage=(unsigned int)page_frame_addr(pmem);
  vpage=(unsigned int)page_frame_addr(vmem);
  pagesz=page_frame_size(size);

  for(i=0;i<pagesz;i+=PAGE_PAGESIZE) {

    r=page_map_vpage( pgd, (void*)vpage, (void*)ppage, type );
    if(r<0) {
      console_puts("vmem mapping error\n");

  console_puts("pgd=");
  long2hex((unsigned int)pgd,s);
  console_puts(s);
  console_puts(" ppage=");
  long2hex((unsigned int)ppage,s);
  console_puts(s);
  console_puts(" vpage=");
  long2hex((unsigned int)vpage,s);
  console_puts(s);
  console_puts(" size=");
  long2hex((unsigned int)size,s);
  console_puts(s);
  console_puts("\n");

      return r;
    }
    ppage += PAGE_PAGESIZE;
    vpage += PAGE_PAGESIZE;
  }
  return 0;
}

void *
page_create_pgd(void)
{
  void *pgd;
  int r;
  unsigned int kernelmax;

  pgd=mem_alloc(PAGE_PAGESIZE);
  if(pgd==NULL)
     return NULL;
  memset(pgd,0,PAGE_PAGESIZE);
  if(mem_get_status()>=2)
    kernelmax=CFG_MEM_KERNELMAX;
  else
    kernelmax=CFG_MEM_KERNELHEAP;

  r=page_map_vmem(pgd, 0, 0, kernelmax, (PAGE_TYPE_RDWR|PAGE_TYPE_SUPER));
  if(r<0) {
     return NULL;
  }
  if(page_unmap_vpage(pgd, (void*)CFG_MEM_PAGEWINDOW)==NULL)
    return NULL;

  return pgd;
}

void
page_delete_pgd(void *ppgd)
{
  unsigned int dir_idx, tbl_idx;
  unsigned int pgd_item,pte_item;
  unsigned int *pgd=ppgd,*pte;
  unsigned int kernelmax;
  unsigned int page_addr;
  struct bios_info *binfo;
  unsigned int vram_start,vram_end;

  if(mem_get_status()>=2)
    kernelmax=CFG_MEM_KERNELMAX;
  else
    kernelmax=CFG_MEM_KERNELHEAP;

  binfo = kernel_get_bios_info_addr();
  if(binfo->vmode < 0x100) { // TEXT or VGA MODE
    vram_start = CFG_MEM_VGAGRAPHICS;
    vram_end   = CFG_MEM_VGAGRAPHICS+CFG_MEM_VGASIZE;
  }
  else {                     // VESA MODE
    vram_start = CFG_MEM_VESAWINDOWSTART;
    vram_end   = vram_start + (binfo->scrnx * binfo->scrny);
  }

  for(dir_idx=0;dir_idx<1024;dir_idx++)
  {
    pgd_item = pgd[dir_idx];
    if( (pgd_item & PAGE_TYPE_PRESENT) != 0 )
    {
      pte = page_frame_addr(pgd_item);
      for(tbl_idx=0;tbl_idx<1024;tbl_idx++) {
        pte_item = pte[tbl_idx];
        if((pte_item & PAGE_TYPE_PRESENT) != 0 ) {
          /* free user memory */
          page_addr=(unsigned long)page_index2vaddr(dir_idx,tbl_idx);
          if(page_addr>=(unsigned long)CFG_MEM_USER &&
             (page_addr<vram_start || page_addr>=vram_end) ) {
/*
  console_puts("[free=");
  long2hex((unsigned int)pgd,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)pte,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)dir_idx,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)tbl_idx,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)page_index2vaddr(dir_idx,tbl_idx),s);
  console_puts(s);
  console_puts(",");
  long2hex(pte_item,s);
  console_puts(s);
  console_puts("]");
*/
            page_free(page_frame_addr(pte_item));
          }
        }
      }
      /* free pte */
      mem_free(pte,PAGE_PAGESIZE);
    }
  }
  mem_free(pgd,PAGE_PAGESIZE);
}

int
page_alloc_vmem(void *pgd, void *vmem, unsigned int size, int type)
{
  unsigned int ppage,vpage,pagesz,i;
  int r;

  vpage=(unsigned int)page_frame_addr(vmem);
  pagesz=page_frame_size(size);

  for(i=0;i<pagesz;i+=PAGE_PAGESIZE) {
    ppage=(unsigned int)page_alloc();
    if(ppage==0) {
      return ERRNO_RESOURCE;
    }
    r=page_map_vpage( pgd, (void*)vpage, (void*)ppage, type );
    if(r<0) {
      console_puts("vmem mapping error in alloc_vmem\n");

  console_puts("pgd=");
  long2hex((unsigned int)pgd,s);
  console_puts(s);
  console_puts(" ppage=");
  long2hex((unsigned int)ppage,s);
  console_puts(s);
  console_puts(" vpage=");
  long2hex((unsigned int)vpage,s);
  console_puts(s);
  console_puts(" size=");
  long2hex((unsigned int)size,s);
  console_puts(s);
  console_puts("\n");

      return r;
    }
    vpage += PAGE_PAGESIZE;
  }

  return 0;
}


void *
page_memcpy(void *pgd, void *dest, void *src, unsigned int size)
{
  void *orgpgd;

  BEGIN_CPULOCK();
  orgpgd = (void*)page_get_pgd();
  page_set_pgd(pgd);
  memcpy(dest,src,size);
  page_set_pgd(orgpgd);
  END_CPULOCK();

  return dest;
}


void
page_dump_pgd(void *ppgd,unsigned int startaddr,unsigned int endaddr)
{
  unsigned int dir_idx, tbl_idx;
  unsigned int pgd_item,pte_item;
  unsigned int *pgd=ppgd,*pte;

  for(dir_idx=0;dir_idx<1024;dir_idx++)
  {
    pgd_item = pgd[dir_idx];
    if( (pgd_item & PAGE_TYPE_PRESENT) != 0 )
    {
      console_puts("pgd[");
      int2dec((unsigned int)dir_idx,s);
      console_puts(s);
      console_puts("]=");
      long2hex((unsigned int)pgd_item,s);
      console_puts(s);
      console_puts(" ");
      pte = page_frame_addr(pgd_item);
      for(tbl_idx=0;tbl_idx<1024;tbl_idx++) {
        pte_item = pte[tbl_idx];
        if((pte_item & PAGE_TYPE_PRESENT) != 0 ) {

          if(page_index2vaddr(dir_idx,tbl_idx)>=startaddr && page_index2vaddr(dir_idx,tbl_idx)<=endaddr) {
            console_puts("pte[");
            int2dec((unsigned int)tbl_idx,s);
            console_puts(s);
            console_puts("]=");
            long2hex((unsigned int)pte_item,s);
            console_puts(s);
            console_puts(" ");
          }

/*
          if(page_index2vaddr(dir_idx,tbl_idx)>=kernelmax) {
  console_puts("[");
  long2hex((unsigned int)pgd,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)pte,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)dir_idx,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)tbl_idx,s);
  console_puts(s);
  console_puts(",");
  long2hex((unsigned int)page_index2vaddr(dir_idx,tbl_idx),s);
  console_puts(s);
  console_puts(",");
  long2hex(pte_item,s);
  console_puts(s);
  console_puts("]");
          }
*/
        }
      }
    }
  }
  console_puts("\n");
}

int page_map_vga(void *pgd)
{
  unsigned int vpage;
  int r;
  struct bios_info *binfo;
  unsigned int vram_start, vram_size, vram_window_start, vram_window_end;

  binfo = kernel_get_bios_info_addr();
  if(binfo->vmode < 0x100) {  // VGA TEXT of VGA GRAPHICS MODE
    vram_start = CFG_MEM_VGAGRAPHICS;
    vram_size  = CFG_MEM_VGASIZE;
    vram_window_start = vram_start;
  }
  else {                      // VESA MODE
    vram_start = binfo->vram;
    vram_size  = binfo->scrnx * binfo->scrny;
    vram_window_start = CFG_MEM_VESAWINDOWSTART;
  }
  vram_window_end   = vram_window_start+vram_size;

  for(vpage=vram_window_start;vpage<vram_window_end;vpage+=PAGE_PAGESIZE)
    if(page_unmap_vpage(pgd, (void*)vpage)==0)
      ;//console_puts("vga unmapp error\n");

  r = page_map_vmem( pgd, (void*)vram_window_start, (void*)vram_start, vram_size, PAGE_TYPE_USER|PAGE_TYPE_RDWR );

  return r;
}
