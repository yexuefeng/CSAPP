CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

OBJS = robust_io.o sbuf.o csapp.o cache.o proxy.o 
TARGET = proxy
all: proxy

%.o: %.c
	@echo "CC $@"
	$(CC) $(CFLAGS) -c  $<
proxy: $(OBJS)
	@echo "LD $@"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o core proxy
