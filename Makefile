
CC=gcc
CFLAGS=  -p -g   -lm -lpthread -lsqlite3 -lmrss -lGeoIP -lircclient -lpthread -llcfg
LDFLAGS=
SOURCES=./rss.c ./geo_ip.c ./run.c  ./delayed_send.c  ./utilities.c ./cve.c ./Gigabit.c 
EXECUTABLE=Gigabit

all: $(SOURCES) 
		$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SOURCES)

