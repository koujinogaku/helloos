/*
** dma.h --- DMA controller
*/
#ifndef DMA_H
#define DMA_H
#include "stdint.h"

/* transfer type */
enum {
  DMA_MODE_VERIFY  = 0x00,
  DMA_MODE_WRITE   = 0x04,
  DMA_MODE_READ    = 0x08,

  DMA_MODE_AUTOINIT= 0x10,

  DMA_MODE_INCL    = 0x00,
  DMA_MODE_DECL    = 0x20,

  DMA_MODE_DEMAND  = 0xc0,
  DMA_MODE_SINGLE  = 0x40,
  DMA_MODE_BLOCK   = 0x80,
  DMA_MODE_CASCADE = 0xc0,

  DMA_CHANNEL_FDD      = 0x02,
  DMA_CHANNEL_XTDISK   = 0x03,
  DMA_CHANNEL_CASCADE  = 0x04,

  DMA_ENABLE       = 1,
  DMA_DISABLE      = 0,
};

void dma_init(void);
int dma_set_mode( uint8_t channel, uint8_t mode );
int dma_enable( uint8_t channel, uint8_t sw );
int dma_set_area( uint8_t channel, void *dma_addr, unsigned int dma_count );
int dma_alloc_buffer(uint8_t channel, unsigned long size);
int dma_free_buffer(uint8_t channel);
int dma_set_buffer(uint8_t channel);
int dma_push_buffer(uint8_t channel, void* buf);
int dma_pull_buffer(uint8_t channel, void* buf);

#endif /* DMA_H */
