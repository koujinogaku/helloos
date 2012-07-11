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

all: display.out keyboard.out command.out mouse.out

display.out: $(DISPLAYOBJ)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(DISPLAYOBJ) $(APPLIB) -o $@

keyboard.out: $(KEYBOARDOBJ)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(KEYBOARDOBJ) $(APPLIB) -o $@

command.out: $(COMMANDOBJ)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(COMMANDOBJ) $(APPLIB) -o $@

mouse.out: $(MOUSEOBJ)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(MOUSEOBJ) $(APPLIB) -o $@


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

dump:
	objdump -b binary -m i8086 -D hello.img

write:
	dd if=$(TARGET).img of=/dev/fd0

