#ifndef FILE_H
#define FILE_H

struct file_info {
  unsigned long size;
  unsigned long blksize;
  unsigned char attr;
  unsigned char atime[7];//BCD yyyymmddhhmmss
  unsigned char mtime[7];//BCD yyyymmddhhmmss
  unsigned char namelen;
  char name[0];
};


#endif
