CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

OBJS = ConnectionOperation.o csapp.o cache.o proxy.o dlist.o queue.o
TARGET = proxy
all: proxy

%.o: %.c
	@echo "CC $@"
	$(CC) $(CFLAGS) -c  $<
.cpp.o:
	@echo "GCC $@"
	$(CC) $(CFLAGS) -c  $@ 
proxy: $(OBJS)
	@echo "LD $@"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o core proxy
