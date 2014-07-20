
CC=gcc
CFLAGS= -g -Wall
LIBS=-lresolv
DEFS=
INC= -I ./

all: dnsperf

dnsperf: dnsperf.o events.o sock.o
	$(CC) $(CFLAGS) $(DEFS) -o $@ $^ $(LIBS) $(INC)

dnsperf.o: dnsperf.c
	$(CC) $(CFLAGS) -c $^ $(INC)

events.o: events.c
	$(CC) $(CFLAGS) -c $^ $(INC)

sock.o: sock.c
	$(CC) $(CFLAGS) -c $^ $(INC)

clean:
	rm -f *.o dnsperf
