all: test_rexec test_config single_server  

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

LIB1 = ../../lib/mollib/libmollib.so 
LIB2 = ../../lib/config/libconfig.so 
LIB3 = ../../stub_syscall.o

test_rexec: test_rexec.c
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o test_rexec test_rexec.c  $(LIB1) -I$(INCLUDE)

test_config: test_config.c    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o  test_config  test_config.c   $(LIB2) -I$(INCLUDE)

single_server: single_server.c
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o single_server single_server.c  $(LIB1) $(LIB3) -I$(INCLUDE)


.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm  *.o test_rexec test_config single_server 
	
