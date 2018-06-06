CC=gcc
BINS= loop_mq_ipc1 loop_mq_ipc2 loop_mq_ipc3 loop_mq_copy1 loop_mq_copy2 \
	loop_mol_ipc1 loop_mol_ipc2 loop_mol_ipc3 loop_mol_ipc4 loop_mol_copy1 loop_mol_copy2 \
	loop_mol_zcopy1 loop_mol_zcopy2 loop_rs_mperf loop_sr_mperf loop_rr_mperf \
	loop_zcopy_mperf loop_sr_mpeer loop_rr_mpeer loop_sr_mthread loop_rr_mthread loop_sr_thread \
	loop_mol_bind loop_mol_getep loop_mol_rqrp loop_readimg loop_writeimg \
	loop_read_fs 


#OPT= "--param" "l1-cache-line-size=64" "--param" "l1-cache-size=32" "--param" "l2-cache-size=2048" 
 
all: $(BINS)

		
INCLUDE = ..
LIBS = /home/MoL_Module/mol-module/mol-ipc/stub_syscall.o
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic

loop_mq_ipc1: loop_mq_ipc1.c 
	$(CC) $(FLAGS) -o loop_mq_ipc1 loop_mq_ipc1.c $(LIBS) -I$(INCLUDE)

loop_mq_ipc2: loop_mq_ipc2.c 
	$(CC) $(FLAGS) -o loop_mq_ipc2 loop_mq_ipc2.c $(LIBS) -I$(INCLUDE)

loop_mq_ipc3: loop_mq_ipc3.c 
	$(CC) $(FLAGS) -o loop_mq_ipc3 loop_mq_ipc3.c $(LIBS) -I$(INCLUDE)
	
loop_mq_copy1: loop_mq_copy1.c 
	$(CC) $(FLAGS) -o loop_mq_copy1 loop_mq_copy1.c $(LIBS) -I$(INCLUDE)

loop_mq_copy2: loop_mq_copy2.c 
	$(CC) $(FLAGS) -o loop_mq_copy2 loop_mq_copy2.c $(LIBS) -I$(INCLUDE)
	
loop_mol_ipc1: loop_mol_ipc1.c 
	$(CC) $(FLAGS) -o loop_mol_ipc1 loop_mol_ipc1.c  $(LIBS) -I$(INCLUDE)

loop_mol_ipc2: loop_mol_ipc2.c 
	$(CC) $(FLAGS) -o loop_mol_ipc2 loop_mol_ipc2.c  $(LIBS) -I$(INCLUDE)

loop_mol_ipc3: loop_mol_ipc3.c 
	$(CC) $(FLAGS) -o loop_mol_ipc3 loop_mol_ipc3.c  $(LIBS) -I$(INCLUDE)

loop_mol_ipc4: loop_mol_ipc4.c 
	$(CC) $(FLAGS) -o loop_mol_ipc4 loop_mol_ipc4.c  $(LIBS) -I$(INCLUDE)

loop_mol_copy1: loop_mol_copy1.c 
	$(CC) $(FLAGS) -o loop_mol_copy1 loop_mol_copy1.c  $(LIBS) -I$(INCLUDE)

loop_mol_copy2: loop_mol_copy2.c 
	$(CC) $(FLAGS) -o loop_mol_copy2 loop_mol_copy2.c  $(LIBS) -I$(INCLUDE)

loop_mol_zcopy1: loop_mol_zcopy1.c 
	$(CC) $(FLAGS) -o loop_mol_zcopy1 loop_mol_zcopy1.c  $(LIBS) -I$(INCLUDE)
	
loop_mol_zcopy2: loop_mol_zcopy2.c 
	$(CC) $(FLAGS) -o loop_mol_zcopy2 loop_mol_zcopy2.c  $(LIBS) -I$(INCLUDE)

loop_rs_mperf: loop_rs_mperf.c 
	$(CC) $(FLAGS) -o loop_rs_mperf loop_rs_mperf.c  $(LIBS) -I$(INCLUDE)

loop_sr_mperf: loop_sr_mperf.c 
	$(CC) $(FLAGS) -o loop_sr_mperf loop_sr_mperf.c  $(LIBS) -I$(INCLUDE)

loop_rr_mperf: loop_rr_mperf.c 
	$(CC) $(FLAGS) -o loop_rr_mperf loop_rr_mperf.c  $(LIBS) -I$(INCLUDE)
	
loop_zcopy_mperf: loop_zcopy_mperf.c 
	$(CC) $(FLAGS) -o loop_zcopy_mperf loop_zcopy_mperf.c  $(LIBS) -I$(INCLUDE)

loop_sr_mpeer: loop_sr_mpeer.c 
	$(CC) $(FLAGS) -o loop_sr_mpeer loop_sr_mpeer.c  $(LIBS) -I$(INCLUDE)

loop_rr_mpeer: loop_rr_mpeer.c 
	$(CC) $(FLAGS) -o loop_rr_mpeer loop_rr_mpeer.c  $(LIBS) -I$(INCLUDE)
	
loop_mol_bind: loop_mol_bind.c 
	$(CC) $(FLAGS) -o loop_mol_bind loop_mol_bind.c  $(LIBS) -I$(INCLUDE)

loop_mol_getep: loop_mol_getep.c 
	$(CC) $(FLAGS) -o loop_mol_getep loop_mol_getep.c  $(LIBS) -I$(INCLUDE)
	
loop_sr_mthread: loop_sr_mthread.c 
	$(CC) $(FLAGS)  -o loop_sr_mthread loop_sr_mthread.c  $(LIBS) -I$(INCLUDE)

loop_rr_mthread: loop_rr_mthread.c 
	$(CC) $(FLAGS)  -o loop_rr_mthread loop_rr_mthread.c  $(LIBS) -I$(INCLUDE)
	
loop_sr_thread: loop_sr_thread.c 
	$(CC) $(FLAGS) -o loop_sr_thread loop_sr_thread.c  $(LIBS) -I$(INCLUDE)
	
loop_mol_rqrp: loop_mol_rqrp.c 
	$(CC) $(FLAGS) -o loop_mol_rqrp loop_mol_rqrp.c  $(LIBS) -I$(INCLUDE)

loop_readimg: loop_readimg.c 
	$(CC) $(FLAGS) -o loop_readimg loop_readimg.c  $(LIBS) -I$(INCLUDE)

loop_writeimg: loop_writeimg.c 
	$(CC) $(FLAGS) -o loop_writeimg loop_writeimg.c  $(LIBS) -I$(INCLUDE)

loop_read_fs: loop_read_fs.c 
	$(CC) $(FLAGS) -o loop_read_fs loop_read_fs.c  $(LIBS) -I$(INCLUDE)
	
clean:
	rm -f $(BINS)
