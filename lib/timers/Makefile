all: libtimers.so 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

OBJS =  timers.o 

libtimers.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libtimers.so $(OBJS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)


clean:
	rm libtimers.so *.o

.PHONY: all clean 
.SECONDARY:

