#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define  MOL_USERSPACE	1

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/dc_usr.h"
#include "./kernel/minix/molerrno.h"

#include <sys/syscall.h>

// #include "../debug.h"
#define SVRDEBUG(x, args ...)


#define 	RMTNODE	0
#define 	DCID	0
#define 	CLT_NR	1
#define 	SVR_NR	(CLT_NR - 1)
#define 	MAXCHILDREN	100

int dcid, loops, children;
message *m_ptr;

int child_function(int child) {
	int i , ret;
	pid_t child_pid;
	int child_nr, server_nr, child_ep;
	char rmt_name[16];
	
	child_nr = CLT_NR + ((child+1) * 2);
	printf("child %d: loops=%d child_nr=%d\n", child, loops, child_nr);

	/* binding remote server */
	server_nr = child_nr-1;
	sprintf(rmt_name,"server%d",server_nr);
	ret = mnx_rmtbind(dcid, rmt_name, server_nr, RMTNODE);
	if(ret < 0) {
		fprintf(stderr,"mnx_rmtbind %d: process %s on node %d \n", 
			server_nr , rmt_name, RMTNODE);
		exit(1);		
	}
	
	/* binding local client */
  	child_nr = CLT_NR + ((child+1) * 2);
	child_ep = mnx_bind(dcid, child_nr);
	child_pid = getpid();
	printf("CHILD child=%d child_nr=%d child_ep=%d child_pid=%d\n", 
	child, child_nr, child_ep, child_pid);

	/* START synchronization with MAIN CLIENT */
	do {
		ret = mnx_sendrec(CLT_NR, (long) m_ptr);
		if (ret == EMOLDEADSRCDST)sleep(1);
	} while (ret == EMOLDEADSRCDST);
	if(ret < 0) {
		fprintf(stderr,"CHILD %d: mnx_sendrec ret=%d\n", child, ret);
		exit(1);		
	}	
	
	/* M3-IPC TRANSFER LOOP  */
 	printf("CHILD %d: Starting loop\n", child);
	for( i = 0; i < loops; i++) {
		m_ptr->m1_i2= i;
   		ret = mnx_send(server_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"CHILD %d: mnx_send ret=%d\n", child, ret);
			exit(1);		
		}		
		ret = mnx_receive(server_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"CHILD %d: mnx_receive ret=%d\n", child, ret);
			exit(1);		
		}	
	}
	printf("CHILD %d:" MSG1_FORMAT, child, MSG1_FIELDS(m_ptr));

	/* STOP synchronization with MAIN CLIENT */
	ret = mnx_sendrec(CLT_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD %d: mnx_sendrec ret=%d\n", child, ret);
		exit(1);		
	}	
	
	mnx_unbind(dcid,server_nr);
	printf("CHILD %d: exiting\n", child);
	exit(0);
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}
   
int  main ( int argc, char *argv[] )
{
	int  clt_pid, ret, i, pid, clt_ep,child_ep, child_nr;
	double t_start, t_stop, t_total;
	pid_t child_pid[MAXCHILDREN];
	struct dc_usr dc, *dc_usr_ptr;
	char rmt_name[16];

  	if (argc != 3) {
    	fprintf(stderr,"usage: %s <children> <loops> \n", argv[0]);
    	exit(1);
  	}

	/*---------------- Get DC info ---------------*/
	dcid 	= DCID;
	dc_usr_ptr = &dc;
	ret = mnx_getdcinfo(dcid, dc_usr_ptr);
	if(ret < 0) {
 	    fprintf(stderr,"mnx_getdcinfo error=%d \n", ret );
 	    exit(1);
	}
	printf(DC_USR_FORMAT,DC_USR_FIELDS(dc_usr_ptr));
	
	/*---------------- Check Arguments ---------------*/
	children = atoi(argv[1]);
  	loops = atoi(argv[2]);
	if( loops <= 0) {
   		fprintf(stderr, "loops must be > 0\n");
   		exit(1);
  	}
	if( children < 0 || children > (dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks) ){
   		fprintf(stderr, "children must be > 0 and < %d\n", 
			(dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks));
   		exit(1);
  	}
	
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "posix_memalign\n");
   		exit(1);
	}
	printf("MAIN CLIENT m_ptr=%p\n",m_ptr);	
	
	/*---------------- Creat children ---------------*/
	for( i = 0; i < children; i++){	
		printf("child fork %d\n",i);
		if( (child_pid[i] = fork()) == 0 ){/* Child */
			ret = child_function( i );
		}
		/* parent */
		printf("MAIN CLIENT child_pid[%d]=%d\n",i, child_pid[i]);
	}

	/*---------------- MAIN CLIENT bind --------------*/
	clt_pid = getpid();
    clt_ep =	mnx_bind(dcid, CLT_NR);
	if( clt_ep < 0 ) {
		fprintf(stderr, "BIND ERROR clt_ep=%d\n",clt_ep);
	}
   	printf("BIND MAIN CLIENT dcid=%d clt_pid=%d CLT_NR=%d clt_ep=%d\n",
		dcid, clt_pid, CLT_NR, 	clt_ep);
		
	/*--------------- binding remote server ---------*/
	sprintf(rmt_name,"server%d",SVR_NR);
	ret = mnx_rmtbind(dcid, rmt_name, SVR_NR, RMTNODE);
	if(ret < 0) {
    	fprintf(stderr,"ERROR MAIN CLIENT mnx_rmtbind %d: process %s on node %d \n", 
			SVR_NR, rmt_name, RMTNODE);
    	exit(1);		
	}
   	printf("MAIN CLIENT mnx_rmtbind %d: process %s on node %d \n", 
			SVR_NR, rmt_name, RMTNODE);
				
	/*--------- Waiting for children  START synchronization: REQUEST ---------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
		do {
			ret = mnx_receive(child_nr, (long) m_ptr);
			if (ret == EMOLSRCDIED)sleep(1);
		} while (ret == EMOLSRCDIED);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: mnx_receive ret=%d\n", ret);
			exit(1);		
		}			
	}		
	
	/*--- Sending START message to remote SERVER ----*/
 	printf("MAIN CLIENT: Sending START message to remote SERVER\n");
	ret = mnx_sendrec(SVR_NR, (long) m_ptr);

	/*--------- Waiting for children  START synchronization: REPLY ---------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
   		ret = mnx_send(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}
	
	/*--------- Waiting for children  STOP synchronization ---------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
    	ret = mnx_receive(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: mnx_receive ret=%d\n", ret);
			exit(1);		
		}		
	}	

	/*--- Sending STOP message to remote SERVER ----*/
 	printf("MAIN CLIENT: Sending STOP message to remote SERVER\n");
	ret = mnx_sendrec(SVR_NR, (long) m_ptr);

	/*--------- Sending replies to children --------*/
	for( i = 0; i < children; i++){
		child_nr = CLT_NR + ((i+1) * 2);
   		ret = mnx_send(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN CLIENT: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}	
	
	sleep(3);
	mnx_unbind(dcid,SVR_NR);

	/*--------- Waiting for children EXIT ---------*/
	for( i = 0; i < children; i++){	
		wait(&ret);
	}
	printf("MAIN CLIENT END\n");

}



