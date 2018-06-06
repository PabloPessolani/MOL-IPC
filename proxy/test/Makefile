CC=gcc
BINS= 	loop_client1 loop_client2  loop_client3 \
	loop_server1 loop_server2  loop_server3 loop_server4 \
	loop_r-s_server loop_r-r_server loop_r-n_server \
	loop_s-r_client loop_sr_client loop_s-r_client loop_n-r_client\
	loop_copy_server loop_copy_client loop_ring \
	loop_svr_vcopy1 loop_svr_vcopy1a loop_svr_vcopy2 \
	loop_svr_vcopy3 loop_svr_vcopy3a loop_svr_vcopy4 \
	loop_clt_vcopy1 loop_clt_vcopy1a loop_clt_vcopy2 

# loop_clt_vcopy3 \
#		loop_clt_vcopy3a loop_clt_vcopy3b loop_clt_vcopy4 \
#		loop_svr_vcopy1 loop_svr_vcopy2 loop_svr_vcopy3 \
#		loop_svr_vcopy3a loop_svr_vcopy3b loop_svr_vcopy4		
	
LIBS = /home/MoL_Module/mol-module/mol-ipc/stub_syscall.o
INCLUDE = /home/MoL_Module/mol-module/mol-ipc
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic
		
all: $(BINS)


loop_client1: loop_client1.c 
	$(CC)  $(CFLAGS)  -o loop_client1 loop_client1.c $(LIBS) -I$(INCLUDE)

loop_client2: loop_client2.c 
	$(CC)  $(CFLAGS)  -o loop_client2 loop_client2.c $(LIBS) -I$(INCLUDE)

loop_client3: loop_client3.c 
	$(CC)  $(CFLAGS)  -o loop_client3 loop_client3.c $(LIBS) -I$(INCLUDE)

loop_server1: loop_server1.c 
	$(CC)  $(CFLAGS)  -o loop_server1 loop_server1.c $(LIBS) -I$(INCLUDE)

loop_server2: loop_server2.c 
	$(CC)  $(CFLAGS)  -o loop_server2 loop_server2.c $(LIBS) -I$(INCLUDE)

loop_server3: loop_server3.c 
	$(CC)  $(CFLAGS)  -o loop_server3 loop_server3.c $(LIBS) -I$(INCLUDE)

loop_server4: loop_server4.c 
	$(CC)  $(CFLAGS)  -o loop_server4 loop_server4.c $(LIBS) -I$(INCLUDE)
	
loop_r-s_server: loop_r-s_server.c 
	$(CC)  $(CFLAGS)  -o loop_r-s_server loop_r-s_server.c $(LIBS) -I$(INCLUDE)

loop_r-r_server: loop_r-r_server.c 
	$(CC)  $(CFLAGS)  -o loop_r-r_server loop_r-r_server.c $(LIBS) -I$(INCLUDE)

loop_r-n_server: loop_r-n_server.c 
	$(CC)  $(CFLAGS)  -o loop_r-n_server loop_r-n_server.c $(LIBS) -I$(INCLUDE)
	
loop_s-r_client: loop_s-r_client.c 
	$(CC)  $(CFLAGS)  -o loop_s-r_client loop_s-r_client.c $(LIBS) -I$(INCLUDE)

loop_sr_client: loop_sr_client.c 
	$(CC)  $(CFLAGS)  -o loop_sr_client loop_sr_client.c $(LIBS) -I$(INCLUDE)

loop_n-r_client: loop_n-r_client.c 
	$(CC)  $(CFLAGS)  -o loop_n-r_client loop_n-r_client.c $(LIBS) -I$(INCLUDE)	

loop_copy_client: loop_copy_client.c 
	$(CC)  $(CFLAGS)  -o loop_copy_client loop_copy_client.c $(LIBS) -I$(INCLUDE)	

loop_copy_server: loop_copy_server.c 
	$(CC)  $(CFLAGS)  -o loop_copy_server loop_copy_server.c $(LIBS) -I$(INCLUDE)	
	
loop_ring: loop_ring.c 
	$(CC)  $(CFLAGS)  -o loop_ring loop_ring.c $(LIBS) -I$(INCLUDE)
	
loop_svr_vcopy1: loop_svr_vcopy1.c 
	$(CC)  $(CFLAGS)  -o loop_svr_vcopy1 loop_svr_vcopy1.c $(LIBS) -I$(INCLUDE)

loop_clt_vcopy1: loop_clt_vcopy1.c 
	$(CC)  $(CFLAGS)  -o loop_clt_vcopy1 loop_clt_vcopy1.c $(LIBS) -I$(INCLUDE)

loop_svr_vcopy1a: loop_svr_vcopy1a.c 
	$(CC)  $(CFLAGS)  -o loop_svr_vcopy1a loop_svr_vcopy1a.c $(LIBS) -I$(INCLUDE)

loop_clt_vcopy1a: loop_clt_vcopy1a.c 
	$(CC)  $(CFLAGS)  -o loop_clt_vcopy1a loop_clt_vcopy1a.c $(LIBS) -I$(INCLUDE)

loop_svr_vcopy2: loop_svr_vcopy2.c 
	$(CC)  $(CFLAGS)  -o loop_svr_vcopy2 loop_svr_vcopy2.c $(LIBS) -I$(INCLUDE)

loop_clt_vcopy2: loop_clt_vcopy2.c 
	$(CC)  $(CFLAGS)  -o loop_clt_vcopy2 loop_clt_vcopy2.c $(LIBS) -I$(INCLUDE)
	
loop_svr_vcopy3: loop_svr_vcopy3.c 
	$(CC)  $(CFLAGS)  -o loop_svr_vcopy3 loop_svr_vcopy3.c $(LIBS) -I$(INCLUDE)

loop_svr_vcopy4: loop_svr_vcopy4.c 
	$(CC)  $(CFLAGS)  -o loop_svr_vcopy4 loop_svr_vcopy4.c $(LIBS) -I$(INCLUDE)
	
loop_svr_vcopy3a: loop_svr_vcopy3a.c 
	$(CC)  $(CFLAGS)  -o loop_svr_vcopy3a loop_svr_vcopy3a.c $(LIBS) -I$(INCLUDE)

clean:
	rm -f $(BINS)
