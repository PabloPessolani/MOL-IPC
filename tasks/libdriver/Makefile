all: libdriver.so 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

OBJS =  driver.o 

libdriver.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libdriver.so $(OBJS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)


clean:
	rm libdriver.so *.o

.PHONY: all clean 
.SECONDARY:

