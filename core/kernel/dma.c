/*
** dma.c --- DMA controller
*/

#include "config.h"
#include "cpu.h"
#include "console.h"
#include "string.h"
#include "dma.h"

/* DMA controller registers */
enum {
  DMA_SLV_BASE	= 0x00,
  DMA_SLV_STAT	= 0x08,
  DMA_SLV_CMD	= 0x08,
  DMA_SLV_REQ	= 0x09,
  DMA_SLV_MASK	= 0x0a,
  DMA_SLV_MODE	= 0x0b,
  DMA_SLV_CLRFF	= 0x0c,
  DMA_SLV_RESET	= 0x0d,

  DMA_MST_BASE	= 0xc0,
  DMA_MST_STAT	= 0xd0,
  DMA_MST_CMD	= 0xd0,
  DMA_MST_REQ	= 0xd2,
  DMA_MST_MASK	= 0xd4,
  DMA_MST_MODE	= 0xd6,
  DMA_MST_CLRFF	= 0xd8,
  DMA_MST_RESET	= 0xda,

  DMA_PAGE0	= 0x87,
  DMA_PAGE1	= 0x83,
  DMA_PAGE2	= 0x81,
  DMA_PAGE3	= 0x82,
  DMA_PAGE4	= 0x8f,
  DMA_PAGE5	= 0x8b,
  DMA_PAGE6	= 0x89,
  DMA_PAGE7	= 0x8a,
};

static inline void
dma_iodelay(int n)
{
  int i;
  for(i=0;i<n;i++)
    cpu_in8(0x80);
}

inline void
dma_clear_flipflop( byte channel )
{
  byte reg_addr;
  if(channel < 4) {
    reg_addr = DMA_SLV_CLRFF;
  }
  else {
    reg_addr = DMA_MST_CLRFF;
    channel -= 4;
  }

  cpu_out8( reg_addr, 0 );
  dma_iodelay(1);
}

void
dma_enable( byte channel )
{
  byte reg_addr;
  if(channel < 4) {
    reg_addr = DMA_SLV_MASK;
  }
  else {
    reg_addr = DMA_MST_MASK;
    channel -= 4;
  }

  cpu_out8( reg_addr, channel );
  dma_iodelay(1);
}

void
dma_disable( byte channel )
{
  byte reg_addr, reg_data;
  if(channel < 4) {
    reg_addr = DMA_SLV_MASK;
  }
  else {
    reg_addr = DMA_MST_MASK;
    channel -= 4;
  }

  reg_data = channel | 0x04;
  cpu_out8( reg_addr, reg_data );
  dma_iodelay(1);
}

void
dma_set_mode( byte channel, byte mode )
{
  byte reg_addr, reg_data;
  if(channel < 4) {
    reg_addr = DMA_SLV_MODE;
  }
  else {
    reg_addr = DMA_MST_MODE;
    channel -= 4;
  }

  reg_data = channel | mode;
  cpu_out8( reg_addr, reg_data);
  dma_iodelay(1);
}

void
dma_init(void)
{
  /* reset DMA controllers */
  cpu_out8( DMA_MST_RESET, 0 );
  dma_iodelay(1);
  cpu_out8( DMA_SLV_RESET, 0 );
  dma_iodelay(1);

  cpu_out8( DMA_MST_CMD, 0 );
  dma_iodelay(1);
  cpu_out8( DMA_SLV_CMD, 0 );
  dma_iodelay(1);

  dma_set_mode( 4, DMA_MODE_CASCADE); // Cascade the channel 4
  dma_enable( 4 );
}

/* dma memory area must be on 128KB boundary physical memory address */
void
dma_set_area( byte channel, unsigned int dma_addr, unsigned int dma_count )
{
  static const byte reg_page[8] = {
    DMA_PAGE0, DMA_PAGE1, DMA_PAGE2, DMA_PAGE3,
    DMA_PAGE4, DMA_PAGE5, DMA_PAGE6, DMA_PAGE7,
  };
  byte reg_addr, reg_count;

  if( channel < 4 ){
    reg_addr = DMA_SLV_BASE + channel * 2;
    reg_count = reg_addr + 1;
  }
  else {
    reg_addr = DMA_MST_BASE + channel * 4;
    reg_count = reg_addr + 2;
  }

  /* set base address */
  dma_clear_flipflop(channel);
  cpu_out8( reg_addr, (dma_addr & 0x0000ff) );
  dma_iodelay(1);
  cpu_out8( reg_addr, (dma_addr & 0x00ff00) >> 8 );
  dma_iodelay(1);

  /* set base count */
  dma_clear_flipflop(channel);
  cpu_out8( reg_count, (dma_count & 0x00ff) );
  dma_iodelay(1);
  cpu_out8( reg_count, (dma_count & 0xff00) >> 8 );
  dma_iodelay(1);

  /* set dma page */
  cpu_out8( reg_page[channel], (dma_addr & 0xff0000) >> 16 );
  dma_iodelay(1);
}

