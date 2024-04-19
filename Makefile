CFLAGS?=-O2 -g -Wall -W $(shell pkg-config --cflags librtlsdr)
LDLIBS+=$(shell pkg-config --libs librtlsdr) -lpthread -lm
CC?=gcc
PROGNAME=dump1090

all: dump1090

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

OBJS=dump1090.o anet.o geoutils.o ppi-display.o strokefont.o ws2812-rpi.o fb-display.o ppi-datafile.o ppi-neoring.o i2clcd.o
dump1090: ${OBJS}
	$(CXX) -g -o dump1090 ${OBJS} $(LDFLAGS) $(LDLIBS) -lproj -li2c	

lcdmsg: i2clcd.o lcdmsg.o
	$(CXX) -g -o lcdmsg lcdmsg.o i2clcd.o $(LDFLAGS) $(LDLIBS) -li2c

clean:
	rm -f *.o dump1090

zip:
	tar -czvf $(PROGNAME).tgz *.c *.cpp *.h *.hpp *.txt *.md *.html *.sh Makefile
