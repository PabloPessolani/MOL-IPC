CC=gcc
BINS= 	test_sysfork test_sysvcopy1 test_sysvcopy2 test_sysexit test_systimes \
	test_sysnice test_sysprivctl test_sysgetinfo test_syssetalarm test_sysmemset
	
OPT=
#OPT= "--param" "l1-cache-line-size=64" "--param" "l1-cache-size=32" "--param" "l2-cache-size=2048" 

LIBS = ../../lib/syslib/libsyslib.so ../../lib/timers/libtimers.so ../../stub_syscall.o
 
all: $(BINS)

test_sysfork: test_sysfork.c 
	$(CC) $(OPT) -o test_sysfork test_sysfork.c  $(LIBS) -I../..

test_sysvcopy1: test_sysvcopy1.c 
	$(CC) $(OPT) -o test_sysvcopy1 test_sysvcopy1.c  $(LIBS) -I../..

test_sysvcopy2: test_sysvcopy2.c 
	$(CC) $(OPT) -o test_sysvcopy2 test_sysvcopy2.c  $(LIBS) -I../..

test_sysexit: test_sysexit.c 
	$(CC) $(OPT) -o test_sysexit test_sysexit.c  $(LIBS) -I../..

test_systimes: test_systimes.c 
	$(CC) $(OPT) -o test_systimes test_systimes.c  $(LIBS) -I../..

test_sysnice: test_sysnice.c 
	$(CC) $(OPT) -o test_sysnice test_sysnice.c  $(LIBS) -I../..

test_sysprivctl: test_sysprivctl.c 
	$(CC) $(OPT) -o test_sysprivctl test_sysprivctl.c  $(LIBS) -I../..

test_sysgetinfo: test_sysgetinfo.c 
	$(CC) $(OPT) -o test_sysgetinfo test_sysgetinfo.c  $(LIBS) -I../..

test_syssetalarm: test_syssetalarm.c 
	$(CC) $(OPT) -o test_syssetalarm test_syssetalarm.c  $(LIBS) -I../..

test_sysmemset: test_sysmemset.c 
	$(CC) $(OPT) -o test_sysmemset test_sysmemset.c  $(LIBS) -I../..
	
clean:
	rm -f $(BINS)
