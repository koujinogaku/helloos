STDINC=../../../core/include
WININC=../include

TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I$(STDINC) -I$(WININC)
LSFILE=../../../core/library/appl.ls
STDLIBDIR=../../../core/library
STARTOBJ=$(STDLIBDIR)/startappl.o
APPLIB=$(STDLIBDIR)/libappl.a

OBJSCLI=client.o nxproto.o error.o nxutil.o #clientfb.o 
OBJS=nxdraw.o nxtransform.o srvclip.o srvclip.o srvevent.o srvfunc.o srvutil.o srvnet.o
LIBCLI=../lib/libmwnanox.a
LIBSRV=../lib/libmwnanoxsrv.a
LIBDRIVERS=../lib/libmwdrivers.a
LIBENGINE=../lib/libmwengine.a
LIBFONTS=../lib/libmwfonts.a
LIBNANOXSRV=$(LIBSRV) $(LIBDRIVERS) $(LIBENGINE) $(LIBFONTS)

TARGET=$(LIBCLI) $(LIBSRV) nanox.out

all: $(TARGET)

$(LIBCLI): $(OBJSCLI) makefile
	$(AR) r $@ $?
	ranlib $@

$(LIBSRV): $(OBJS) makefile
	$(AR) r $@ $?
	ranlib $@

nanox.out: srvmain.o $(LIBSRV)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) srvmain.o $(LIBNANOXSRV) $(APPLIB) -o $@

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

