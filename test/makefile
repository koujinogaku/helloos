MICROWINDIR=../app/microwindows
STDINC=../core/include
WININC=$(MICROWINDIR)/include

TS=i386-elf
LD=$(TS)-ld
AR=$(TS)-ar
OBJDUMP=$(TS)-objdump
CC=$(TS)-gcc
CFLAGS=-O2 -Wall -fcall-used-ebx -fno-builtin -nostdlib -static -I$(STDINC) -I$(WININC) 
LSFILE=../core/library/appl.ls

STDLIBDIR=../core/library
STARTOBJ=$(STDLIBDIR)/startappl.o
APPLIB=$(STDLIBDIR)/libappl.a $(STDLIBDIR)/math/libmath.a
MICROWINLIB=$(MICROWINDIR)/lib/libmwnanox.a # $(MICROWINDIR)/lib/libmwdrivers.a $(MICROWINDIR)/lib/libmwengine.a $(MICROWINDIR)/lib/libmwfonts.a

TST  =tst.o
TST2 =tst2.o
TESTMWINOBJ= testmwin.o
TSTNANOX= tstnanox.o
TSTMOUSE= tstmouse.o
TSTDEMOX= tstdemox.o
TSTATA= tstata.o
TSTATAC= tstatac.o
TSTISO= tstiso.o
TSTCDFS= tstcdfs.o

all: tstata.out tstatac.out tstiso.out tstcdfs.out #tst.out tst2.out #tstdemox.out #tstnanox.out #tstmouse.out testmwin.out 

tst.out: $(TST) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TST) $(APPLIB) -o $@

tst2.out: $(TST2) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TST2) $(APPLIB) -o $@

tstmouse.out: $(TSTMOUSE) $(APPLIB) $(MICROWINLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTMOUSE) $(APPLIB) -o $@

testmwin.out: $(TESTMWINOBJ) $(APPLIB) $(MICROWINLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TESTMWINOBJ) $(MICROWINLIB) $(APPLIB) -o $@

tstnanox.out: $(TSTNANOX) $(APPLIB) $(MICROWINLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTNANOX) $(MICROWINLIB) $(APPLIB) -o $@

tstdemox.out: $(TSTDEMOX) $(APPLIB) $(MICROWINLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTDEMOX) $(MICROWINLIB) $(APPLIB) -o $@

tstata.out: $(TSTATA) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTATA) $(APPLIB) -o $@

tstatac.out: $(TSTATAC) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTATAC) $(APPLIB) -o $@

tstiso.out: $(TSTISO) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTISO) $(APPLIB) -o $@

tstcdfs.out: $(TSTCDFS) $(APPLIB)
	$(LD) -T $(LSFILE) -Map $*.map --cref $(STARTOBJ) $(TSTCDFS) $(APPLIB) -o $@

#%.a: %.o
#	$(AR) r $@ $?

atapi.a: $(OBJS)
	rm -f $@
	$(AR) rcs $@ $(OBJS)

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

