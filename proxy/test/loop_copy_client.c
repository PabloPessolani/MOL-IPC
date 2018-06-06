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

#include "../debug.h"
//#define SVRDEBUG(x, args ...)

#define 	RMTNODE	0
#define 	DCID	0
#define 	CLT_NR	1
#define 	SVR_NR	(CLT_NR - 1)
#define 	MAXCHILDREN	100

int dcid, children;
message *m_ptr;
char *buffer;

int child_function(int child) {
	int i , ret;
	pid_t child_pid;
	int child_nr, server_nr, child_ep;
	char rmt_name[16];
	
	child_nr = CLT_NR + ((child+1) * 2);
	printf("CHILD child_nr=%d: child=%d\n", child_nr, child);

	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "CHILD child_nr=%d: message posix_memalign\n", child_nr);
   		exit(1);
	}
	printf("CHILD child_nr=%d: m_ptr=%p\n",child_nr, m_ptr);	
	
	/*---------------- Allocate memory for DATA BUFFER ---------------*/
	posix_memalign( (void**) &buffer, getpagesize(), MAXCOPYLEN);
	if (buffer== NULL) {
   		fprintf(stderr, "CHILD child_nr=%d: buffer posix_memalign\n", child_nr);
   		exit(1);
  	}
	
	/*---------------- Fill with characters the DATA BUFFER ---------------*/
	for(i = 0; i < MAXCOPYLEN-1; i++)
		buffer[i] = ((i%25) + 'A');	
	buffer[MAXCOPYLEN] = 0;
	
	buffer[60] = 0;	
	SVRDEBUG("CHILD child_nr=%d: buffer before = %s\n", child_nr, buffer);

	/* binding remote server */
	server_nr = child_nr-1;
	sprintf(rmt_name,"server%d",server_nr);
	ret = mnx_rmtbind(dcid, rmt_name, server_nr, RMTNODE);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: mnx_rmtbind %d process %s on node %d \n", 
			child_nr, server_nr , rmt_name, RMTNODE);
		exit(1);		
	}
	
	/* binding local client */
  	child_nr = CLT_NR + ((child+1) * 2);
	child_ep = mnx_bind(dcid, child_nr);
	child_pid = getpid();
	SVRDEBUG("CHILD child_nr=%d: child_ep=%d child_pid=%d\n", 
		child_nr, child_ep, child_pid);

	/* START synchronization with MAIN CLIENT */
	ret = mnx_sendrec(CLT_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: mnx_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	
	/* M3-IPC TRANSFER LOOP  */
	m_ptr->m1_i1 = 1;
	m_ptr->m1_i2 = 2;
	m_ptr->m1_i3 = 3;
	m_ptr->m1_p1 = buffer;
 	SVRDEBUG("CHILD child_nr=%d:Sending message to start loop. buffer=%X\n", child_nr, buffer);
	ret = mnx_sendrec(server_nr, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: mnx_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	buffer[30] = 0;	
	SVRDEBUG("CHILD child_nr=%d: buffer after = %s\n", child_nr, buffer);
	
	/* STOP synchronization with MAIN CLIENT */
	ret = mnx_sendrec(CLT_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr=%d: mnx_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	
	SVRDEBUG("CHILD child_nr=%d: unbinding %d\n",child_nr, server_nr);
	mnx_unbind(dcid,server_nr);
	SVRDEBUG("CHILD child_nr=%d:exiting\n", child_nr);
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

  	if (argc != 2) {
    	fprintf(stderr,"usage: %s <children> \n", argv[0]);
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

	if( children < 0 || children > (dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks) ){
   		fprintf(stderr, "children must be > 0 and < %d\n", 
			(dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks));
   		exit(1);
  	}
	
	/*---------------- MAIN CLIENT bind --------------*/
	clt_pid = getpid();
    clt_ep =	mnx_bind(dcid, CLT_NR);
	if( clt_ep < 0 ) {
		fprintf(stderr, "BIND ERROR clt_ep=%d\n",clt_ep);
	}
   	SVRDEBUG("BIND MAIN CLIENT dcid=%d clt_pid=%d CLT_NR=%d clt_ep=%d\n",
		dcid, clt_pid, CLT_NR, 	clt_ep);

	/*--------------- binding remote server ---------*/
	sprintf(rmt_name,"server%d",SVR_NR);
	ret = mnx_rmtbind(dcid, rmt_name, SVR_NR, RMTNODE);
	if(ret < 0) {
    	fprintf(stderr,"ERROR MAIN CLIENT mnx_rmtbind %d: process %s on node %d \n", 
			SVR_NR, rmt_name, RMTNODE);
    	exit(1);		
	}
   	SVRDEBUG("MAIN CLIENT mnx_rmtbind %d: process %s on node %d \n", 
			SVR_NR, rmt_name, RMTNODE);
		
	/*---------------- Creat children ---------------*/
	for( i = 0; i < children; i++){	
		SVRDEBUG("child fork %d\n",i);
		if( (child_pid[i] = fork()) == 0 ){/* Child */
			ret = child_function( i );
		}
		/* parent */
		SVRDEBUG("MAIN CLIENT child_pid[%d]=%d\n",i, child_pid[i]);
	}
			
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "MAIN CLIENT: posix_memalign\n");
   		exit(1);
	}
	printf("MAIN CLIENT: m_ptr=%p\n", m_ptr);			
			
	/*--------- Waiting for children START synchronization: REQUEST ---------*/
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
 	SVRDEBUG("MAIN CLIENT: Sending START message to remote SERVER\n");
	ret = mnx_sendrec(SVR_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"ERROR MAIN CLIENT: mnx_sendrec ret=%d\n", ret);
		exit(1);		
	}
	
	/*--------- Sending children START synchronization: REPLY ---------*/
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
 	SVRDEBUG("MAIN CLIENT: Sending STOP message to remote SERVER\n");
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
	
	/*--------- Unbinding remote MAIN SERVER ----*/
	mnx_unbind(dcid,SVR_NR);

	/*--------- Waiting for children EXIT ---------*/
	for( i = 0; i < children; i++){	
		wait(&ret);
	}
	
	printf("MAIN CLIENT END\n");

}



