#ifndef PAGE_H
#define PAGE_H

#define PAGE_TYPE_PRESENT 0x01
#define PAGE_TYPE_RDONLY  0x00
#define PAGE_TYPE_RDWR    0x02
#define PAGE_TYPE_SUPER   0x00
#define PAGE_TYPE_USER    0x04

#define page_frame_size(sz)   (((sz)&0xfffff000)+(((sz)&0x00000fff)?0x1000:0))
#define page_frame_addr(padr) ((unsigned int*)((unsigned int)(padr)&0xfffff000))
#define PAGE_PAGESIZE (4096)

int page_map_vpage(unsigned int *pgd, void *vpage, void *ppage, int type );
void *page_unmap_vpage( unsigned int *pgd, void *vpage );
void *page_get_ppage(unsigned int *pgd, void *vpage);

void page_free(void *vpageaddr);
void *page_alloc(void);
unsigned int page_get_totalfree(void);

int page_init(void);
void *page_get_system_pgd(void);
void *page_get_current_pgd(void);
void *page_get_faultvaddr(void);
void *page_create_pgd(void);
void page_delete_pgd(void *pgd);
void page_dump_pgd(void *pgd,unsigned int startaddr,unsigned int endaddr);

int page_map_vmem(void *pgd, void *vmem, void *pmem, unsigned int size, int type);
int page_alloc_vmem(void *pgd, void *vmem, unsigned int size, int type);
void *page_memcpy(void *pgd, void *dest, void *src, unsigned int size);
int page_map_vga(void *pgd);
#endif
