STDINC=../../../core/include
WININC=../include

TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I$(STDINC) -I$(WININC)
OBJSCLI=# client.o clientfb.o nxproto.o srvnet.o 
OBJS=$(OBJSCLI) nxdraw.o nxtransform.o nxutil.o srvclip.o srvclip.o srvevent.o srvfunc.o srvmain.o srvutil.o

TARGET=../lib/libmwnanox.a

all: $(TARGET)

$(TARGET): $(OBJS) makefile
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

