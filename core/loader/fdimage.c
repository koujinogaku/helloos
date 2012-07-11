/*
** fdimage : Floppy Disk Image Maker
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#include "config.h"
#include "iplinfo.h"

static char buff[FDD_SECSIZE*FDD_SECPCLS+1];
static int writecount = 0;
static unsigned short fat16[FDD_FATCNT];
static unsigned char  fat12[FDD_FATCNT*3/2];
static struct fdd_directory rootdir[FDD_DIRCNT];

void
usage(void)
{
  puts("Usage: fdimage OutPutFDImageFile BootSectorFile SecondBootFile KernelFile\n");
}

void fat16tofat12(unsigned short fat16[], unsigned char fat12[])
{
  int i=0,j=0;
  for(;i<FDD_FATCNT;i+=2,j+=3) {
    fat12[j]   = fat16[i] & 0xff;
    fat12[j+1] = (fat16[i] & 0xf00)/256 + (fat16[i+1] & 0xf)*16;
    fat12[j+2] = (fat16[i+1] & 0xff0)/16;
  }
}

int
make_fileentryname(char *filename, char *fileentryname)
{
  int i;

  i=strlen(filename);
  for(;i>0;--i) {
    if(filename[i]=='/') {
      filename = &filename[i+1];
      break;
    }
  }

  memset(fileentryname,' ',11);
  fileentryname[12]=0;

  for(i=0;i<12 && *filename!=0 ;i++,filename++) {
    if(*filename=='.')
      i=7;
    else
      fileentryname[i] = toupper(*filename);
  }

  return 0;
}


int
add_fileentry(char *filename, long filesize, unsigned short attr, unsigned short filedate)
{
  int dir,clst;

  for(dir=0;dir<FDD_DIRCNT;dir++) {
    if(rootdir[dir].filename[0]==0)
      break;
  }
  if(dir>=FDD_DIRCNT)
    return -1;

  memcpy(rootdir[dir].filename,filename,11);

  rootdir[dir].filesize=filesize;
  rootdir[dir].attr=attr;
  rootdir[dir].date=filedate;
  rootdir[dir].accdate=filedate;
  rootdir[dir].updatedate=filedate;

  for(clst=0;clst<FDD_FATCNT;clst++)
    if(fat16[clst]==0)
      break;
  if(clst>=FDD_FATCNT)
    return -1;

  rootdir[dir].clusterLo=clst;

  for( ;clst<FDD_FATCNT;clst++) {
    filesize -= FDD_SECSIZE*FDD_SECPCLS;
    if(filesize<=0) {
      fat16[clst]=0xffff;
      break;
    }
    fat16[clst]=clst+1;
  }
  if(clst>=FDD_FATCNT)
    return -1;

  return 0;
}



int
get_filesize( const char *path )
{
  FILE *file;
  int size;

  file = fopen(path, "rb");
  if( file == NULL )
    {
      perror(path);
      return -1;
    }
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fclose(file);
  return size;
}

int
write_image( FILE *fddfile, const char *source, struct ipl_info *iplinfo)
{
  FILE *ifile;
  int count,i, blocksz;

  if(iplinfo!=NULL || source==NULL) /* bootsector or filler */
    blocksz = FDD_SECSIZE;
  else
    blocksz = FDD_SECSIZE*FDD_SECPCLS;


  if(source!=NULL){ /* if to copy file */
    ifile = fopen(source, "rb");
    if( ifile == NULL ){
      perror(source);
      return -1;
    }
  }
  else {   /* if to fill left */
    ifile = NULL;
  }

  for(;;) {
    if(ifile!=NULL) {
      count = fread(buff, 1, blocksz, ifile);
    }
    else {
      memset(buff, 0, blocksz);
      count = blocksz;
    }
    if(count<=0) /* end of file */
      break;

    for(i=count;i<blocksz;i++) /* zero-clear left cluster or sector */
      buff[i] = 0;
    if(iplinfo != NULL){ /* if to copy iplinfo (if boot sector) */
      memcpy(buff+CFG_MEM_IPLINFO, iplinfo, sizeof(struct ipl_info));
    }
    if((count=fwrite(buff, 1, blocksz, fddfile))<0) {
      perror(source);
      fclose(ifile);
      return -1;
    }
    writecount += blocksz;
    if(writecount >= 1474560)
      break;
  }
  if(ifile!=NULL)
    fclose(ifile);
  return 0;
}

int
write_memory( FILE *fddfile, void *mem, unsigned long buffsz)
{
  int leftsz;
  if(fwrite((char*)mem, 1, buffsz, fddfile)<0) {
    perror("error\n");
    return -1;
  }
  writecount += buffsz;

  leftsz = buffsz % FDD_SECSIZE;
  if(leftsz == 0)
    return 0;
  
  memset(buff,0,FDD_SECSIZE);
  if(fwrite(buff, 1, FDD_SECSIZE-leftsz, fddfile)<0) {
    perror("error\n");
    return -1;
  }
  writecount += FDD_SECSIZE-leftsz;
  
  return 0;
}

int
main( int argc, char *argv[] )
{
  char *image, *boot, *setup, *kernel;
  int setup_size, kernel_size, file_size;
  int i;
  FILE *fddfile;
  struct ipl_info iplinfo;
  char **filenames,fileentryname[16];
  int filecnt;


  if( argc < 5 )
    goto error;

  image  = argv[1];
  boot   = argv[2];
  setup  = argv[3];
  kernel = argv[4];

  filecnt = argc-5;
  filenames = &(argv[5]);

  /* get setup and kernel size */
  setup_size  = get_filesize( setup );
  kernel_size = get_filesize( kernel );
  if( setup_size < 0 || kernel_size < 0 )
    goto error;

  iplinfo.setup_head   = FDD_SETUPHD;
  iplinfo.setup_count  = setup_size/FDD_SECSIZE;
  if( (setup_size%FDD_SECSIZE)!=0 )
    iplinfo.setup_count++;

  if((iplinfo.setup_count%FDD_SECPCLS)==0)
    iplinfo.kernel_head = iplinfo.setup_head + iplinfo.setup_count;
  else
    iplinfo.kernel_head = iplinfo.setup_head + iplinfo.setup_count + (FDD_SECPCLS-(iplinfo.setup_count%FDD_SECPCLS));

  iplinfo.kernel_count = kernel_size/FDD_SECSIZE;
  if( (kernel_size%FDD_SECSIZE)!=0 )
    iplinfo.kernel_count++;

  iplinfo.kernel_size  = kernel_size;

  memset(fat16,0,sizeof(fat16));
  memset(fat12,0,sizeof(fat12));
  memset(rootdir,0,sizeof(rootdir));
  fat16[0]=0xff0;
  fat16[1]=0xfff;

  printf("file=%s ->  %s(%d)\n", setup, "SETUP   SYS", setup_size);
                 /* 12345678123             FDD_ATTR_BOOTFILE*/
  if(add_fileentry("SETUP   SYS",setup_size,0,((2007-1980)<<9)+(1<<5)+(1))) {
    printf("DISKFULL:SETUP.SYS\n");
    goto error;
  }
  printf("file=%s ->  %s(%d)\n", kernel, "KERNEL  SYS", kernel_size);
                  /* 12345678123  */
  if(add_fileentry("KERNEL  SYS",kernel_size,0,((2007-1980)<<9)+(1<<5)+(1))) {
    printf("DISKFULL:KERNEL.SYS\n");
    goto error;
  }

  for(i=0;i<filecnt;i++) {
    make_fileentryname(filenames[i],fileentryname);
    file_size=get_filesize(filenames[i]);
    printf("file=%s ->  %s(%d)\n", filenames[i], fileentryname, file_size);
    if(add_fileentry(fileentryname,file_size,0,((2007-1980)<<9)+(1<<5)+(1))) {
      printf("DISKFULL:%s\n",fileentryname);
      goto error;
    }
  }

  fat16tofat12(fat16,fat12);

/*
  for(i=0;i<FDD_DIRCNT;i++) {
    if(rootdir[i].filename[0]==0)
      break;
    printf("file=%-11s\n",rootdir[i].filename);
    printf("cluster=%d,%d\n",rootdir[i].clusterHi,rootdir[i].clusterLo);
  }
  printf("FAT\n");
  for(i=0;i<16;i++)
    printf("%02x ",fat16[i]);
  printf("\n");

  printf("FAT12\n");
  for(i=0;i<16;i++)
    printf("%02x ",fat12[i]);
  printf("\n");
*/

  /* write boot information */
  if((fddfile = fopen(image, "w+b"))==NULL)
    goto error;

  if( write_image(fddfile, boot, &iplinfo) < 0 )
    goto error;

  if( write_memory(fddfile, &fat12, sizeof(fat12)) < 0 )
    goto error;

  if( write_memory(fddfile, &fat12, sizeof(fat12)) < 0 )
    goto error;

  if( write_memory(fddfile, &rootdir, sizeof(rootdir)) < 0 )
    goto error;

  printf("setup start byte=%d\n",writecount);
  if( write_image(fddfile, setup, 0) < 0 )
    goto error;
  printf("setup size=%d\n",setup_size);

  printf("kernel start byte=%d\n",writecount);
  if( write_image(fddfile, kernel, 0) < 0 )
    goto error;
  printf("kernel size=%d\n",kernel_size);

  for(i=0;i<filecnt;i++) {
    printf("%s start byte=%d\n", filenames[i], writecount);
    if( write_image(fddfile, filenames[i], 0) < 0 )
      goto error;
  }

  if( write_image(fddfile, NULL, 0) < 0 )
    goto error;

  printf("EOF=%d\n    1474560 is right size\n",writecount);

  fclose(fddfile);

  return 0;

 error:
  usage();
  return 1;
}

