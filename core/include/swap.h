#ifndef SWAP_H
#define SWAP_H

#if __BYTE_ORDER == __BIG_ENDIAN

/** Convert little-endian 16-bit number to the host CPU format. */
# define wswap(x)	((((x) << 8) & 0xff00) | (((x) >> 8) & 0x00ff))
/** Convert little-endian 32-bit number to the host CPU format. */
# define dwswap(x)	((((x) << 24) & 0xff000000L) | \
			 (((x) <<  8) & 0x00ff0000L) | \
			 (((x) >>  8) & 0x0000ff00L) | \
			 (((x) >> 24) & 0x000000ffL) )
#else
/* little endian - no action required */
# define wswap(x)	(x)
# define dwswap(x)	(x)
#endif


/**
 * Read little endian format 32-bit number from buffer, possibly not
 * aligned, and convert to the host CPU format.
 */
#define dwread(addr)	((((unsigned char *)(addr))[0] | \
			 (((unsigned char *)(addr))[1] << 8) | \
			 (((unsigned char *)(addr))[2] << 16) | \
			 (((unsigned char *)(addr))[3] << 24)))


#endif
