LIBDIR=lib

all:  $(LIBDIR)/libmwdrivers.a $(LIBDIR)/libmwengine.a $(LIBDIR)/libmwfonts.a $(LIBDIR)/libmwnanox.a demos/nanowm/nanowm.out demos/nanox/npanel.out demos/nanox/nweyes.out

$(LIBDIR)/libmwengine.a:
	cd engine && make && touch ../make.touch

$(LIBDIR)/libmwdrivers.a:
	cd drivers && make && touch ../make.touch

$(LIBDIR)/libmwfonts.a:
	cd fonts && make && touch ../make.touch

$(LIBDIR)/libmwnanox.a:
	cd nanox && make && touch ../make.touch

demos/nanowm/nanowm.out:
	cd demos/nanowm && make && touch ../make.touch

demos/nanox/npanel.out:
	cd demos/nanox && make && touch ../make.touch

demos/nanox/nweyes.out:
	cd demos/nanox && make && touch ../make.touch

clean:
	cd engine && make clean
	cd drivers && make clean
	cd fonts && make clean
	cd nanox && make clean
	cd demos/nanowm && make clean
	cd demos/nanox && make clean
	rm -f make.touch
