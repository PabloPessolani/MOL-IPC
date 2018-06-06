all: libconfig.so 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

OBJS =  configfile.o 

libconfig.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libconfig.so $(OBJS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)


clean:
	rm libconfig.so *.o

.PHONY: all clean 
.SECONDARY:

