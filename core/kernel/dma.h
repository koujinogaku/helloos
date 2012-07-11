/*
** dma.h --- DMA controller
*/
#ifndef DMA_H
#define DMA_H
#include "config.h"

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
};

void dma_init(void);
void dma_set_mode( byte channel, byte mode );
void dma_enable( byte channel );
void dma_disable( byte channel );
void dma_set_area( byte channel, unsigned int dma_addr, unsigned int dma_count );

#endif /* DMA_H */
