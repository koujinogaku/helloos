LIBDIR=lib

all:  $(LIBDIR)/libmwdrivers.a $(LIBDIR)/libmwengine.a $(LIBDIR)/libmwfonts.a $(LIBDIR)/libmwnanox.a

$(LIBDIR)/libmwengine.a:
	cd engine && make && touch ../make.touch

$(LIBDIR)/libmwdrivers.a:
	cd drivers && make && touch ../make.touch

$(LIBDIR)/libmwfonts.a:
	cd fonts && make && touch ../make.touch

$(LIBDIR)/libmwnanox.a:
	cd nanox && make && touch ../make.touch

clean:
	cd engine && make clean
	cd drivers && make clean
	cd fonts && make clean
	cd nanox && make clean
	rm -f make.touch
