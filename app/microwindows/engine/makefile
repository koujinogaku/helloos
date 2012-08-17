STDINC=../../../core/include
WININC=../include

TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I$(STDINC) -I$(WININC)
#CFLAGS0=-Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I../.. -I../include
OBJS= devarc.o devclip.o devdraw.o devfont.o devimage.o devimage_stretch.o devkbd.o devlist.o devmouse.o devopen.o \
devpal1.o devpal2.o devpal4.o devpal8.o devpalgray4.o devpoly.o devrgn2.o devrgn.o devstipple.o devtimer.o \
error.o  \
obsolete.o selfont.o font_dbcs.o image_bmp.o image_gif.o

### image_jpeg.o image_png.o image_pnm.o image_tiff.o image_xpm.o
### font_dbcs.o font_eucjp.o font_fnt.o font_freetype2.o font_freetype.o font_freetype.cache.o font_hzk.o font_pcf.o

TARGET=../lib/libmwengine.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) r $@ $?
	ranlib $(TARGET)

%.sys: %.o %.ls
	$(LD) -T $*.ls -Map $*.map --cref $< -o $@

%.a: %.o
	$(AR) r $@ $?

#error.o: error.c
#	$(CC) $(CFLAGS0) -c $< -o $@



#$(ARC): $(OBJS)
#	$(AR) r $(ARC) $(OBJS)

#$(TARGET).img: $(TARGET).out
#	dd if=/dev/zero of=$(TARGET).img count=2880
#	dd if=$(TARGET).out of=$(TARGET).img conv=notrunc

clean:
	rm -f *.img
	rm -f *.map
	rm -f *.sys
	rm -f *.out
	rm -f *.bin
	rm -f *.o
	rm -f *.a
	rm -f *.exe
	rm -f $(TARGET)

dump:
	objdump -b binary -m i8086 -D hello.img

write:
	dd if=$(TARGET).img of=/dev/fd0

