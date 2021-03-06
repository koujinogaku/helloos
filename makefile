TESTS=$(TESTDIR)/tstatac.out $(TESTDIR)/tstata.out $(TESTDIR)/tstiso.out $(TESTDIR)/tstcdfs.out #$(TESTDIR)/tst.out $(TESTDIR)/tst2.out #$(TESTDIR)/tstdemox.out ##$(TESTDIR)/tstnanox.out #$(TESTDIR)/tstmouse.out #$(TESTDIR)/testmwin.out

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
SERVERS=$(SERVERDIR)/displayd.out $(SERVERDIR)/keyboard.out $(SERVERDIR)/command.out $(SERVERDIR)/moused.out $(SERVERDIR)/atad.out $(SERVERDIR)/cdfsd.out
COMMANDS=$(COMMANDDIR)/dir.out $(COMMANDDIR)/type.out $(COMMANDDIR)/free.out $(COMMANDDIR)/ps.out $(COMMANDDIR)/qs.out $(COMMANDDIR)/cls.out $(COMMANDDIR)/date.out
MICROWINLIB=$(MICROWINDIR)/lib/libmwdrivers.a $(MICROWINDIR)/lib/libmwengine.a $(MICROWINDIR)/lib/libmwfonts.a $(MICROWINDIR)/lib/libmwnanox.a
NANOXDEMOS=$(MICROWINDIR)/demos/nanox/nxeyes.out $(MICROWINDIR)/demos/nanox/nterm.out $(MICROWINDIR)/demos/nanox/dashdemo.out $(MICROWINDIR)/demos/nanox/world.out $(MICROWINDIR)/demos/nanox/world.xmp $(MICROWINDIR)/demos/nanox/tux.out $(MICROWINDIR)/demos/nanox/tux.gif
NANOXEXECS=$(MICROWINDIR)/nanox/nanox.out $(MICROWINDIR)/demos/nanowm/nanowm.out $(MICROWINDIR)/demos/nanox/npanel.out $(MICROWINDIR)/demos/nanox/nxclock.out $(MICROWINDIR)/demos/nanox/nxterm.out $(NANOXDEMOS)
EXES=$(LOADERS) $(KERNEL) $(SERVERS) $(COMMANDS) $(NANOXEXECS) $(TESTS)
#../linux-1.1.95/linux/drivers/block/tstlx.out


all: hello.img 

hello.img:  $(EXES) $(LOADERDIR)/fdimage.exe doc/readme.txt test/win.bat #test.bat
	rm -f hello.img
	$(LOADERDIR)/fdimage hello.img $(EXES) doc/readme.txt test/win.bat #test.bat
	rm -f hello.iso
	mkisofs -o hello.iso $(EXES) doc/readme.txt test/win.bat

$(LIBDIR)/libappl.a:
	cd $(LIBDIR) && make

$(LOADERDIR)/ipl.sys:
	cd $(LOADERDIR) && make

$(KERNELDIR)/kernel.sys:
	cd $(KERNELDIR) && make

$(SERVERDIR)/displayd.out: $(STDLIB)
	cd $(SERVERDIR) && make

$(SERVERDIR)/keyboard.out: $(STDLIB)
	cd  $(SERVERDIR) && make

$(SERVERDIR)/command.out: $(STDLIB)
	cd $(SERVERDIR) && make

$(SERVERDIR)/moused.out: $(STDLIB)
	cd $(SERVERDIR) && make

$(COMMANDDIR)/dir.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(COMMANDDIR)/type.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(COMMANDDIR)/free.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(COMMANDDIR)/ps.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(COMMANDDIR)/date.out: $(STDLIB)
	cd $(COMMANDDIR) && make

$(MICROWINDIR)/nanox/nanox.out: $(STDLIB)
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanowm/nanowm.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanox/npanel.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanox/nxeyes.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanox/nxclock.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanox/nterm.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanox/dashdemo.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(MICROWINDIR)/demos/nanox/nxterm.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(MICROWINDIR) && make

$(TESTDIR)/tstdemox.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(TESTDIR) && make tstdemox.out

$(TESTDIR)/tstnanox.out: $(STDLIB) $(MICROWINDIR)/make.touch
	cd $(TESTDIR) && make tstnanox.out

$(TESTDIR)/tstmouse.out: $(STDLIB)
	cd $(TESTDIR) && make tstmouse.out

$(TESTDIR)/tst.out: $(STDLIB)
	cd $(TESTDIR) && make tst.out

$(TESTDIR)/tst2.out: $(STDLIB)
	cd $(TESTDIR) && make tst2.out

$(TESTDIR)/tstata.out: $(STDLIB)
	cd $(TESTDIR) && make tstata.out

$(TESTDIR)/tstatac.out: $(STDLIB)
	cd $(TESTDIR) && make tstatac.out

$(TESTDIR)/tstiso.out: $(STDLIB)
	cd $(TESTDIR) && make tstiso.out

$(TESTDIR)/tstcdfs.out: $(STDLIB)
	cd $(TESTDIR) && make tstcdfs.out

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
	rm -f hello.iso

