all: m3ftpd m3ftp 
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

OBJ1 =  m3ftpd.o 
OBJ2 =  m3ftp.o 

LIBS =  ../../lib/syslib/libsyslib.so \
		../../lib/timers/libtimers.so \
		../../lib/mollib/libmollib.so \
		../../stub_syscall.o

m3ftpd: $(OBJ1)  m3ftp.h 
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl  -pthread -rdynamic -o m3ftpd $(OBJ1) $(LIBS) -I$(INCLUDE)	

m3ftp: $(OBJ2)  m3ftp.h 
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl  -pthread -rdynamic -o m3ftp $(OBJ2) $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE) -Wall

clean:
	rm m3ftpd m3ftp *.o
