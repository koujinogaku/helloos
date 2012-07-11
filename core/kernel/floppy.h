/*
** floppy.h --- floppy disk driver
*/
#ifndef FLOPPY_H
#define FLOPPY_H

#define FLOPPY_NUM_DRIVE  4
#define FLOPPY_BOOTSECSZ  512

int floppy_init( void );
int floppy_open( byte drive );
int floppy_close( byte drive );
int floppy_read_sector( byte drive, void *buf, unsigned int sector, unsigned int count );
int floppy_write_sector( byte drive, void *buf, unsigned int sector, unsigned int count );

#endif /* FLOPPY_H */
