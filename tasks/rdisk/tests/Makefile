CC=gcc
BINS= cliente_devcread \
			
INCLUDE = ..
LIBS = /home/MoL_Module/mol-module/mol-ipc/stub_syscall.o
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic

LZ4LIBS = $(LIBS) 
LZ4LIBS += 	../../../../lz4-r131/lib/lz4frame.o \
			../../../../lz4-r131/lib/lz4.o \
			../../../../lz4-r131/lib/lz4hc.o \
			../../../../lz4-r131/lib/xxhash.o		

all: $(BINS)

		
cliente_devcread: Tlz4_data_cd.c cliente_devcread.c 
	$(CC) $(CFLAGS) -o cliente_devcread Tlz4_data_cd.c cliente_devcread.c $(LZ4LIBS) -I$(INCLUDE)


clean:
	rm -f $(BINS)

