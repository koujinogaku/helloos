#ifndef FAT_H
#define FAT_H

int fat_init(void);
int fat_open(byte drive);
int fat_open_file(char *filename, int mode);
int fat_close_file(int fp);
int fat_get_filesize(int fp, unsigned int *size);
int fat_read_file(int fp, void* buf, unsigned int size);
int fat_open_dir(unsigned long *vdirdesc, char *dirname);
int fat_read_dir(unsigned long *vdirdesc, char *dirent);
int fat_close_dir(unsigned long *vdirdesc);


/* Specifiy one of these flags to define the access mode. */
#define FAT_O_RDONLY 0x0001
#define FAT_O_WRONLY 0x0002
#define FAT_O_RDWR   0x0003

/* Mask for access mode bits in the _open flags. */
#define	FAT_O_APPEND 0x0008 /* Writes will add to the end of the file. */

#define	FAT_O_CREAT  0x0100 /* Create the file if it does not exist. */
#define	FAT_O_TRUNC  0x0200 /* Truncate the file if it does exist. */
#define	FAT_O_EXCL   0x0400 /* Open only if the file does not exist. */

#endif
