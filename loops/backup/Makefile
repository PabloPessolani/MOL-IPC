CC=gcc
BINS= loop_send loop_receive loop_sendrec loop_rs loop_notify \
	loop_vcopy_1 loop_vcopy_2 loop_vcopy_1a loop_vcopy_2a \
	loop_rs_perf loop_rs_mperf loop_vcopy_1b loop_vcopy_2b \
	loop_msend loop_mreceive loop_mq_perf loop_rs2_perf \
	loop_sr2_perf loop_mq2_perf loop_nr2_perf loop_mq_copy \
	loop_sr2_copy loop_sr3_copy loop_mq3_copy loop_mq3_perf \
	loop_sr4_copy

all: $(BINS)

loop_rs_perf: loop_rs_perf.c ../stub_syscall.c
	$(CC) -o loop_rs_perf loop_rs_perf.c ../stub_syscall.c -I..

loop_rs_mperf: loop_rs_mperf.c ../stub_syscall.c
	$(CC) -o loop_rs_mperf loop_rs_mperf.c ../stub_syscall.c -I..


loop_send: loop_send.c ../stub_syscall.c
	$(CC) -o loop_send loop_send.c ../stub_syscall.c -I..

loop_receive: loop_receive.c ../stub_syscall.c
	$(CC) -o loop_receive loop_receive.c ../stub_syscall.c -I..	

loop_sendrec: loop_sendrec.c ../stub_syscall.c
	$(CC) -o loop_sendrec loop_sendrec.c ../stub_syscall.c -I..

loop_rs: loop_rs.c ../stub_syscall.c
	$(CC) -o loop_rs loop_rs.c ../stub_syscall.c -I..

loop_notify: loop_notify.c ../stub_syscall.c
	$(CC) -o loop_notify loop_notify.c ../stub_syscall.c -I..

loop_vcopy_1: loop_vcopy_1.c ../stub_syscall.c
	$(CC) -o loop_vcopy_1 loop_vcopy_1.c ../stub_syscall.c -I..

loop_vcopy_2: loop_vcopy_2.c ../stub_syscall.c
	$(CC) -o loop_vcopy_2 loop_vcopy_2.c ../stub_syscall.c -I..

loop_vcopy_1a: loop_vcopy_1a.c ../stub_syscall.c
	$(CC) -o loop_vcopy_1a loop_vcopy_1a.c ../stub_syscall.c -I..

loop_vcopy_2a: loop_vcopy_2a.c ../stub_syscall.c
	$(CC) -o loop_vcopy_2a loop_vcopy_2a.c ../stub_syscall.c -I..

loop_vcopy_1b: loop_vcopy_1b.c ../stub_syscall.c
	$(CC) -o loop_vcopy_1b loop_vcopy_1b.c ../stub_syscall.c -I..

loop_vcopy_2b: loop_vcopy_2b.c ../stub_syscall.c
	$(CC) -o loop_vcopy_2b loop_vcopy_2b.c ../stub_syscall.c -I..

loop_msend: loop_msend.c ../stub_syscall.c
	$(CC) -o loop_msend loop_msend.c ../stub_syscall.c -I..

loop_mreceive: loop_mreceive.c ../stub_syscall.c
	$(CC) -o loop_mreceive loop_mreceive.c ../stub_syscall.c -I..

loop_mq_perf: loop_mq_perf.c 
	$(CC) -o loop_mq_perf loop_mq_perf.c -I..

loop_mq2_perf: loop_mq2_perf.c 
	$(CC) -o loop_mq2_perf loop_mq2_perf.c -I..

loop_mq3_perf: loop_mq3_perf.c 
	$(CC) -o loop_mq3_perf loop_mq3_perf.c -I..
	
loop_rs2_perf: loop_rs2_perf.c ../stub_syscall.c
	$(CC) -o loop_rs2_perf loop_rs2_perf.c ../stub_syscall.c -I..

loop_sr2_perf: loop_sr2_perf.c ../stub_syscall.c
	$(CC) -o loop_sr2_perf loop_sr2_perf.c ../stub_syscall.c -I..

loop_nr2_perf: loop_nr2_perf.c ../stub_syscall.c
	$(CC) -o loop_nr2_perf loop_nr2_perf.c ../stub_syscall.c -I..

loop_mq_copy: loop_mq_copy.c 
	$(CC) -o loop_mq_copy loop_mq_copy.c -I..

loop_mq3_copy: loop_mq3_copy.c 
	$(CC) -o loop_mq3_copy loop_mq3_copy.c -I..
	
loop_sr2_copy: loop_sr2_copy.c ../stub_syscall.c
	$(CC) -o loop_sr2_copy loop_sr2_copy.c ../stub_syscall.c -I..

loop_sr3_copy: loop_sr3_copy.c ../stub_syscall.c
	$(CC) -o loop_sr3_copy loop_sr3_copy.c ../stub_syscall.c -I..

loop_sr4_copy: loop_sr4_copy.c ../stub_syscall.c
	$(CC) -o loop_sr4_copy loop_sr4_copy.c ../stub_syscall.c -I..
	
clean:
	rm -f $(BINS)
