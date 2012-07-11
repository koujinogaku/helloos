STDINC=../../../core/include
WININC=../include

TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I$(STDINC) -I$(WININC)
VGAOBJS=vgaplan4.o vgainit.o scr_bios.o memplan4.o
FBOBJS=scr_fb.o fb.o fblin8.o fbportrait_down.o fbportrait_left.o fbportrait_right.o
VIDEOOBJS=$(FBOBJS)
OBJS=$(VIDEOOBJS) mou_hello.o kbd_hello.o genmem.o genfont.o g_8x16_font.o
TARGET=../lib/libmwdrivers.a

all: $(TARGET)


$(TARGET): $(OBJS)
	$(AR) r $@ $?
	ranlib $(TARGET)

%.sys: %.o %.ls
	$(LD) -T $*.ls -Map $*.map --cref $< -o $@

%.a: %.o
	$(AR) r $@ $?

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

