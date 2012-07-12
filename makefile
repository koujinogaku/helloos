TESTS= $(TESTDIR)/tst.out $(TESTDIR)/tst2.out #$(TESTDIR)/tstdemox.out #$(TESTDIR)/tstnanox.out $(TESTDIR)/tstmouse.out #$(TESTDIR)/testmwin.out

LOADERDIR=core/loader
KERNELDIR=core/kernel
SERVERDIR=core/server
COMMANDDIR=core/command
LIBDIR=core/library/
TESTDIR=test
MICROWINDIR=app/microwindows




STDLIB=$(LIBDIR)/libappl.a
LOADERS=$(LOADERDIR)/ipl.sys $(LOADERDIR)/setup.sys
KERNEL=$(KERNELDIR)/kernel.sys
SERVERS=$(SERVERDIR)/display.out $(SERVERDIR)/keyboard.out $(SERVERDIR)/command.out $(SERVERDIR)/mouse.out
COMMANDS=$(COMMANDDIR)/dir.out $(COMMANDDIR)/type.out
MICROWINLIB=$(MICROWINDIR)/lib/libmwdrivers.a $(MICROWINDIR)/lib/libmwengine.a $(MICROWINDIR)/lib/libmwfonts.a $(MICROWINDIR)/lib/libmwnanox.a
EXES=$(LOADERS) $(KERNEL) $(SERVERS) $(COMMANDS) $(TESTS)


all: hello.img 

hello.img:  $(EXES) $(LOADERDIR)/fdimage.exe doc/readme.txt # win.bat test.bat
	$(LOADERDIR)/fdimage hello.img $(EXES) doc/readme.txt # win.bat test.bat

$(LIBDIR)/libappl.a:
	cd $(LIBDIR) && make

$(LOADERDIR)/ipl.sys:
	cd $(LOADERDIR) && make

$(KERNELDIR)/kernel.sys:
	cd $(KERNELDIR) && make

$(SERVERDIR)/display.out: $(STDLIB)
	cd $(SERVERDIR) && make

$(SERVERDIR)/keyboard.out: $(STDLIB)
	cd  $(SERVERDIR) && make

$(SERVERDIR)/command.out: $(STDLIB)
	cd $(SERVERDIR) && make

$(SERVERDIR)/mouse.out: $(STDLIB)
	cd $(SERVERDIR) && make

$(COMMANDDIR)/dir.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(COMMANDDIR)/type.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(TESTDIR)/tstdemox.out: $(STDLIB)
	cd $(TESTDIR) && make

$(TESTDIR)/tst.out: $(STDLIB)
	cd $(TESTDIR) && make

$(TESTDIR)/tst2.out: $(STDLIB)
	cd $(TESTDIR) && make

$(MICROWINDIR)/make.touch: $(STDLIB)
	cd $(MICROWINDIR) && make

clean:
	cd $(LIBDIR) && make clean
	cd $(LOADERDIR) && make clean
	cd $(KERNELDIR) && make clean
	cd $(SERVERDIR) && make clean
	cd $(COMMANDDIR) && make clean
	cd $(MICROWINDIR) && make clean
	cd $(TESTDIR) && make clean
	rm -f hello.img

