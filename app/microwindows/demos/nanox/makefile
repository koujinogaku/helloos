MICROWINDIR=../..
COREDIR=../../../../core
STDINC=$(COREDIR)/include
WININC=$(MICROWINDIR)/include

TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I$(STDINC) -I$(WININC)
LSFILE=$(COREDIR)/library/appl.ls

STDLIBDIR=$(COREDIR)/library
STARTOBJ=$(STDLIBDIR)/startappl.o
APPLIB=$(STDLIBDIR)/libappl.a $(STDLIBDIR)/math/libmath.a
MICROWINLIB=$(MICROWINDIR)/lib/libmwnanox.a

OBJS= 

all: npanel.out nxeyes.out

npanel.out: npanel.o $(MICROWINLIB) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) npanel.o $(MICROWINLIB) $(APPLIB) -o $@

nxcal.out: nxcal.o $(MICROWINLIB) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) nxcal.o $(MICROWINLIB) $(APPLIB) -o $@

nxeyes.out: nxeyes.o $(MICROWINLIB) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) nxeyes.o $(MICROWINLIB) $(APPLIB) -o $@

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
	rm -f make.touch

dump:
	objdump -b binary -m i8086 -D hello.img

write:
	dd if=$(TARGET).img of=/dev/fd0

