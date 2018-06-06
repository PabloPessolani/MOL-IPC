BINS= udp_proxy tcp_proxy tipc_proxy svr_proxy clt_proxy tcp_th_proxy \
		socat_sproxy socat_rproxy socat_up udt_proxy \
		lz4tcp_th_proxy raw_proxy lz4tipc_proxy  stub4cpp.o 
		
# Which compiler
CC = gcc
C++ = g++

# Where are include files kept
INCLUDE = ..
MODDIR= /home/MoL_Module
MOLDIR= /home/MoL_Module/mol-ipc
LIBS =  $(MOLDIR)/lib/syslib/libsyslib.so $(MOLDIR)/lib/mollib/libmollib.so $(MOLDIR)/stub_syscall.o
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic 
LZ4FLAGS = $(CFLAGS)  
LZ4FLAGS += -std=c99 -Wall -Wextra -Wundef -Wshadow -Wcast-align -Wstrict-prototypes

LZ4LIBS = $(LIBS) 
LZ4LIBS += 	 $(MODDIR)/lz4-r131/lib/lz4frame.o \
			 $(MODDIR)/lz4-r131/lib/lz4.o \
			 $(MODDIR)/lz4-r131/lib/lz4hc.o \
			 $(MODDIR)/lz4-r131/lib/xxhash.o		
	
UDTLIBS = $(LIBS)
UDTLIBS += 	/home/udt/udt4/src/libudt.so ./stub4cpp.o
UDTFLAGS = $(CFLAGS)  
UDTFLAGS += -lstdc++ -finline-functions -O3 -Wall -Wextra -Wundef -Wshadow -Wcast-align 

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

all: $(BINS)

udt_proxy: udt_proxy.cpp stub4cpp.o ./stub4cpp.o
	$(C++)  $(UDTFLAGS) -o udt_proxy udt_proxy.cpp ./stub4cpp.o $(UDTLIBS) -I$(INCLUDE)	

stub4cpp.o: stub4cpp.c stub4cpp.h 
	$(CC)  $(CFLAGS) -shared -o stub4cpp.o stub4cpp.c $(MOLDIR)/stub_syscall.o
	
socat_up: socat_up.c 
	$(CC)  $(CFLAGS)  -o socat_up socat_up.c $(LIBS) -I$(INCLUDE)
	
socat_rproxy: socat_rproxy.c 
	$(CC)  $(CFLAGS)  -o socat_rproxy socat_rproxy.c $(LIBS) -I$(INCLUDE)

socat_sproxy: socat_sproxy.c 
	$(CC)  $(CFLAGS)  -o socat_sproxy socat_sproxy.c $(LIBS) -I$(INCLUDE)
	
lz4tcp_th_proxy: lz4tcp_th_proxy.c 
	$(CC)  $(LZ4FLAGS)  -o lz4tcp_th_proxy lz4tcp_th_proxy.c $(LZ4LIBS) -I$(INCLUDE) 

tcp_th_proxy: tcp_th_proxy.c 
	$(CC)  $(CFLAGS)  -o tcp_th_proxy tcp_th_proxy.c $(LIBS) -I$(INCLUDE)	

tcp_proxy: tcp_proxy.c 
	$(CC)  $(CFLAGS)  -o tcp_proxy tcp_proxy.c $(LIBS) -I$(INCLUDE)	

raw_proxy: raw_proxy.c 
	$(CC)  $(CFLAGS)  -o raw_proxy raw_proxy.c $(LIBS) -I$(INCLUDE)	
	
udp_proxy: udp_proxy.c 
	$(CC)  $(CFLAGS)  -o udp_proxy udp_proxy.c $(LIBS) -I$(INCLUDE)	

tipc_proxy: tipc_proxy.c 
	$(CC)  $(CFLAGS)  -o tipc_proxy tipc_proxy.c $(LIBS) -I$(INCLUDE)	
	
lz4tipc_proxy: lz4tipc_proxy.c 
	$(CC)  $(LZ4FLAGS)  -o lz4tipc_proxy lz4tipc_proxy.c $(LZ4LIBS) -I$(INCLUDE) 	

svr_proxy: svr_proxy.c 
	$(CC)  $(CFLAGS)  -o svr_proxy svr_proxy.c $(LIBS) -I$(INCLUDE)	

clt_proxy: clt_proxy.c 
	$(CC)  $(CFLAGS)  -o clt_proxy clt_proxy.c $(LIBS) -I$(INCLUDE)	

	
clean:
	rm $(BINS) *.o
