
CC      = gcc
CFLAGS  = -g -Wall
LIBS    = -lresolv
SHELL   = /bin/sh
DEFINS    =
INC     = -I ./

ifeq ($(SHELL test -f /usr/include/sys/epoll.h && echo yes), yes)
DEFINES     += -DHAVE_EPOLL
endif

ifeq ($(SHELL test -f /usr/include/sys/event.h && echo yes), yes)
DEFINES     += -DHAVE_KQUEUE
endif

all: dnsperf

dnsperf: dnsperf.o events.o sock.o
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $^ $(LIBS) $(INC)

dnsperf.o: dnsperf.c
	$(CC) $(CFLAGS) -c $^ $(INC)

events.o: events.c
	$(CC) $(CFLAGS) $(DEFINES) -c $^ $(INC)

sock.o: sock.c
	$(CC) $(CFLAGS) -c $^ $(INC)

clean:
	rm -f *.o dnsperf
