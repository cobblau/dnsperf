
CC      = gcc
CFLAGS  = -g -Wall
LIBS    = -lresolv
shell   = /bin/sh
ECHO    = /bin/echo
DEFINES    = -DHAVE_EPOLL
INC     = -I ./

ifeq ($(shell test -f /usr/include/sys/event.h && echo yes), yes)
DEFINES     = -DHAVE_KQUEUE
@echo "Use Kqueue"
endif

all: dnsperf

dnsperf: dnsperf.o events.o sock.o
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $^ $(LIBS) $(INC)

dnsperf.o: dnsperf.c
	$(CC) $(CFLAGS) $(DEFINES) -c $^ $(INC)

events.o: events.c
	$(CC) $(CFLAGS) $(DEFINES) -c $^ $(INC)

sock.o: sock.c
	$(CC) $(CFLAGS) $(DEFINES) -c $^ $(INC)

clean:
	rm -f *.o dnsperf
