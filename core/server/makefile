TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I../include
LSFILE=../library/appl.ls

STDLIBDIR=../library
STARTOBJ=$(STDLIBDIR)/startappl.o
APPLIB=$(STDLIBDIR)/libappl.a

DISPLAYOBJ= displaysrv.o
KEYBOARDOBJ= keyboardsrv.o
COMMANDOBJ= command.o
MOUSEOBJ= mousesrv.o
ATAOBJ= atasrv.o
CDFSOBJ= cdfssrv.o

all: displayd.out keyboard.out command.out moused.out atad.out cdfsd.out

displayd.out: $(DISPLAYOBJ) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(DISPLAYOBJ) $(APPLIB) -o $@

keyboard.out: $(KEYBOARDOBJ) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(KEYBOARDOBJ) $(APPLIB) -o $@

command.out: $(COMMANDOBJ) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(COMMANDOBJ) $(APPLIB) -o $@

moused.out: $(MOUSEOBJ) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(MOUSEOBJ) $(APPLIB) -o $@

atad.out: $(ATAOBJ) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(ATAOBJ) $(APPLIB) -o $@

cdfsd.out: $(CDFSOBJ) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(CDFSOBJ) $(APPLIB) -o $@


#%.a: %.o
#	$(AR) r $@ $?

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

dump:
	objdump -b binary -m i8086 -D hello.img

write:
	dd if=$(TARGET).img of=/dev/fd0

