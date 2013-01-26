TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I../include #-I../loader

TARGET=hello2
OBJS= kernel.o desc.o console.o string.o kmemory.o task.o pic.o intr.o queue.o alarm.o timer.o dma.o floppy.o fat.o sysentry.o exception.o mutex.o page.o shrmem.o kprogram.o fpu.o excptdmp.o

EXES=ipl.sys setup.sys kernel.sys display.out keyboard.out command.out dir.out type.out mouse.out $(TESTEXES) #window.out winmgr.out

kernel.sys: kernelstart.o $(OBJS) kernel.ls
	$(LD) -T kernel.ls -Map kernel.map --cref kernelstart.o $(OBJS) -o $@

%.sys: %.o %.ls
	$(LD) -T $*.ls -Map $*.map --cref $< -o $@

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

