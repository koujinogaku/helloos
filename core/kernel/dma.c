/*
** dma.c --- DMA controller
*/

#include "cpu.h"
#include "string.h"
#include "mutex.h"
#include "dma.h"
#include "errno.h"
#include "kmemory.h"

/* DMA controller registers */
enum {
  DMA_SLV_BASE	= 0x000,
  DMA_SLV_STAT	= 0x008,
  DMA_SLV_CMD	= 0x008,
  DMA_SLV_REQ	= 0x009,
  DMA_SLV_MASK	= 0x00a,
  DMA_SLV_MODE	= 0x00b,
  DMA_SLV_CLRFF	= 0x00c,
  DMA_SLV_RESET	= 0x00d,

  DMA_MST_BASE	= 0x0c0,
  DMA_MST_STAT	= 0x0d0,
  DMA_MST_CMD	= 0x0d0,
  DMA_MST_REQ	= 0x0d2,
  DMA_MST_MASK	= 0x0d4,
  DMA_MST_MODE	= 0x0d6,
  DMA_MST_CLRFF	= 0x0d8,
  DMA_MST_RESET	= 0x0da,

  DMA_PAGE0	= 0x087,
  DMA_PAGE1	= 0x083,
  DMA_PAGE2	= 0x081,
  DMA_PAGE3	= 0x082,
  DMA_PAGE4	= 0x08f,
  DMA_PAGE5	= 0x08b,
  DMA_PAGE6	= 0x089,
  DMA_PAGE7	= 0x08a,

  DMA_SINGLE_MASK = 0x04,
};

struct dma_desc {
  void *buf;
  unsigned long bufsize;
};

static int dma_mutex=0;
#define DMA_CHANNEL_MAX 8
static struct dma_desc dma_desc[DMA_CHANNEL_MAX];

static inline void
dma_iodelay(int n)
{
  int i;
  for(i=0;i<n;i++)
    cpu_in8(0x80);
}

inline void
dma_clear_flipflop( uint8_t channel )
{
  uint8_t reg_addr;
  if(channel < DMA_CHANNEL_CASCADE) {
    reg_addr = DMA_SLV_CLRFF;
  }
  else {
    reg_addr = DMA_MST_CLRFF;
    channel -= DMA_CHANNEL_CASCADE;
  }

  cpu_out8( reg_addr, 0 );
  dma_iodelay(1);
}

int dma_enable( uint8_t channel, uint8_t sw )
{
  unsigned int reg_addr;
  uint8_t reg_data;

  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;

  mutex_lock(&dma_mutex);
  if(channel < DMA_CHANNEL_CASCADE) {
    reg_addr = DMA_SLV_MASK;
  }
  else {
    reg_addr = DMA_MST_MASK;
    channel -= DMA_CHANNEL_CASCADE;
  }

  if(sw==DMA_DISABLE)
    reg_data = channel | DMA_SINGLE_MASK;
  else
    reg_data = channel;

  cpu_out8( reg_addr, reg_data );
  dma_iodelay(1);
  mutex_unlock(&dma_mutex);

  return 0;
}

int dma_set_mode( uint8_t channel, uint8_t mode )
{
  unsigned int reg_addr;
  uint8_t reg_data;

  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;

  mutex_lock(&dma_mutex);
  if(channel < DMA_CHANNEL_CASCADE) {
    reg_addr = DMA_SLV_MODE;
  }
  else {
    reg_addr = DMA_MST_MODE;
    channel -= DMA_CHANNEL_CASCADE;
  }

  reg_data = channel | mode;
  cpu_out8( reg_addr, reg_data);
  dma_iodelay(1);
  mutex_unlock(&dma_mutex);

  return 0;
}

/* dma memory area must be on 128KB boundary physical memory address */
int dma_set_area( uint8_t channel, void *dma_addr, unsigned int dma_count )
{
  static const unsigned int reg_page[DMA_CHANNEL_MAX] = {
    DMA_PAGE0, DMA_PAGE1, DMA_PAGE2, DMA_PAGE3,
    DMA_PAGE4, DMA_PAGE5, DMA_PAGE6, DMA_PAGE7,
  };
  unsigned int reg_addr;
  unsigned int reg_count;

  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;

  mutex_lock(&dma_mutex);
  if( channel < DMA_CHANNEL_CASCADE ){
    reg_addr = DMA_SLV_BASE + channel * 2;
    reg_count = reg_addr + 1;
  }
  else {
    reg_addr = DMA_MST_BASE + channel * 4;
    reg_count = reg_addr + 2;
  }

  /* set base address */
  dma_clear_flipflop(channel);
  cpu_out8( reg_addr, ((unsigned long)dma_addr & 0x0000ff) );
  dma_iodelay(1);
  cpu_out8( reg_addr, ((unsigned long)dma_addr & 0x00ff00) >> 8 );
  dma_iodelay(1);

  /* set base count */
  dma_clear_flipflop(channel);
  cpu_out8( reg_count, (dma_count & 0x00ff) );
  dma_iodelay(1);
  cpu_out8( reg_count, (dma_count & 0xff00) >> 8 );
  dma_iodelay(1);

  /* set dma page */
  cpu_out8( reg_page[channel], ((unsigned long)dma_addr & 0xff0000) >> 16 );
  dma_iodelay(1);

  mutex_unlock(&dma_mutex);

  return 0;
}

int dma_alloc_buffer(uint8_t channel, unsigned long size)
{
  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;

  mutex_lock(&dma_mutex);
  if(dma_desc[channel].buf) {
    mutex_unlock(&dma_mutex);
    return ERRNO_INUSE;
  }
  dma_desc[channel].buf=mem_alloc(size);
  if(dma_desc[channel].buf==0) {
    mutex_unlock(&dma_mutex);
    return ERRNO_RESOURCE;
  }
  dma_desc[channel].bufsize=size;
  mutex_unlock(&dma_mutex);
  return channel;
}

int dma_free_buffer(uint8_t channel)
{
  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;

  mutex_lock(&dma_mutex);
  if(dma_desc[channel].buf==0) {
    mutex_unlock(&dma_mutex);
    return ERRNO_NOTEXIST;
  }
  mem_free(dma_desc[channel].buf,dma_desc[channel].bufsize);
  dma_desc[channel].buf=0;
  dma_desc[channel].bufsize=0;
  mutex_unlock(&dma_mutex);
  return channel;
}

int dma_set_buffer(uint8_t channel)
{
  int rc;
  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;

  if(dma_desc[channel].buf==0) {
    return ERRNO_NOTEXIST;
  }
  rc = dma_set_area(channel,dma_desc[channel].buf,dma_desc[channel].bufsize);
  return rc;
}
int dma_push_buffer(uint8_t channel, void* buf)
{
  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;
  mutex_lock(&dma_mutex);
  if(dma_desc[channel].buf==0) {
    mutex_unlock(&dma_mutex);
    return ERRNO_NOTEXIST;
  }
  memcpy(dma_desc[channel].buf,buf,dma_desc[channel].bufsize);
  mutex_unlock(&dma_mutex);
  return 0;
}
int dma_pull_buffer(uint8_t channel, void* buf)
{
  if(channel>=DMA_CHANNEL_MAX)
    return ERRNO_NOTEXIST;
  mutex_lock(&dma_mutex);
  if(dma_desc[channel].buf==0) {
    mutex_unlock(&dma_mutex);
    return ERRNO_NOTEXIST;
  }
  memcpy(buf,dma_desc[channel].buf,dma_desc[channel].bufsize);
  mutex_unlock(&dma_mutex);
  return 0;
}

void
dma_init(void)
{
  memset(&dma_desc,0,sizeof(dma_desc));

  /* reset DMA controllers */
  cpu_out8( DMA_MST_RESET, 0 );
  dma_iodelay(1);
  cpu_out8( DMA_SLV_RESET, 0 );
  dma_iodelay(1);

  cpu_out8( DMA_MST_CMD, 0 );
  dma_iodelay(1);
  cpu_out8( DMA_SLV_CMD, 0 );
  dma_iodelay(1);

  dma_set_mode( DMA_CHANNEL_CASCADE, DMA_MODE_CASCADE); // Cascade the channel 4
  dma_enable( DMA_CHANNEL_CASCADE , DMA_ENABLE);

}

