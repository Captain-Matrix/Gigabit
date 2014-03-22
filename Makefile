
CC=gcc
CFLAGS=  -p -g   -lm -lpthread -lsqlite3 -lmrss -lGeoIP -lircclient -lpthread
LDFLAGS=
SOURCES=./md5.c ./rss.c ./geo_ip.c ./run.c  ./delayed_send.c  ./utilities.c ./Gigabit.c 
EXECUTABLE=Gigabit

all: $(SOURCES) 
		$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SOURCES)

