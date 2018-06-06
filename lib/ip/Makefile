all: libip.so 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

OBJS =  inet_addr.o inet_ntoa.o 

libip.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libip.so $(OBJS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)


clean:
	rm libip.so *.o

.PHONY: all clean 
.SECONDARY:

