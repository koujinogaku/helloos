TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I../include

APPLIB=libappl.a
APPLOBJ= syscall.o string.o keyboard.o display.o pic.o environm.o memory.o message.o mouse.o qsort.o print.o rand.o dbgmemory.o

all: $(APPLIB) startappl.o

$(APPLIB): $(APPLOBJ)
	rm -f $@
	$(AR) r $@ $?

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

