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

#define 	RMTNODE	1
#define 	DCID	0
#define 	SVR_NR	0
#define 	CLT_NR	(SVR_NR + 1)
#define 	MAXCHILDREN	100

int dcid, loops, children;
message *m_ptr;

int child_function(int child) {
	int i , ret;
	pid_t child_pid;
	int child_nr,client_nr, child_ep;
	char rmt_name[16];
	
  	child_nr = SVR_NR + ((child+1) * 2);
	SVRDEBUG("child %d: loops=%d child_nr=%d\n", child, loops, child_nr);
		
	/* binding local server */
	child_ep = mnx_bind(dcid, child_nr);
	child_pid = getpid();
	SVRDEBUG("CHILD child=%d child_nr=%d child_ep=%d child_pid=%d\n", 
	child, child_nr, child_ep, child_pid);
	
	/* binding remote client */
	client_nr = child_nr + 1;
	sprintf(rmt_name,"client%d",client_nr);
	ret = mnx_rmtbind(dcid, rmt_name, client_nr, RMTNODE);
	if(ret < 0) {
		fprintf(stderr,"mnx_rmtbind %d: process %s on node %d \n", 
			client_nr , rmt_name, RMTNODE);
		exit(1);		
	}
	
	/* START synchronization with M3FTP SERVER */
	do {
		ret = mnx_sendrec(SVR_NR, (long) m_ptr);
		if (ret == EMOLDEADSRCDST)sleep(1);
	} while (ret == EMOLDEADSRCDST);
	if(ret < 0) {
		fprintf(stderr,"CHILD %d: mnx_sendrec ret=%d\n", child, ret);
		exit(1);		
	}
		
	/* M3-IPC TRANSFER LOOP  */
 	SVRDEBUG("CHILD %d: Starting loop\n", child);
	for( i = 0; i < loops; i++) {
    	ret = mnx_receive(client_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"CHILD %d: mnx_receive ret=%d\n", child, ret);
			exit(1);		
		}		
		m_ptr->m1_i1= i;
   		ret = mnx_send(client_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"CHILD %d: mnx_send ret=%d\n", child, ret);
			exit(1);		
		}	
	}
	SVRDEBUG("CHILD %d:" MSG1_FORMAT, child, MSG1_FIELDS(m_ptr));

	/* STOP synchronization with M3FTP SERVER */
	do {
		ret = mnx_sendrec(SVR_NR, (long) m_ptr);
		if (ret == EMOLDEADSRCDST)sleep(1);
	} while (ret == EMOLDEADSRCDST);
	if(ret < 0) {
		fprintf(stderr,"CHILD %d: mnx_sendrec ret=%d\n", child, ret);
		exit(1);		
	}
		
	mnx_unbind(dcid,client_nr);

	SVRDEBUG("CHILD %d: exiting\n", child);
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
	int  svr_pid, ret, i, svr_ep, pid, child_nr;
	double t_start, t_stop, t_total, child_ep;
	pid_t child_pid[MAXCHILDREN];
	struct dc_usr dc, *dc_usr_ptr;
	char rmt_name[16];

  	if (argc != 1) {
    	fprintf(stderr,"usage: %s\n", argv[0]);
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
	SVRDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_usr_ptr));
	
	/*---------------- Check svr endpoint  ---------------*/
  	svr_ep  = SVR_EP;
	if( svr_ep < 0 || svr_ep > (dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks) ){
   		fprintf(stderr, "svr_ep must be > 0 and < %d\n", 
			(dc_usr_ptr->dc_nr_sysprocs - dc_usr_ptr->dc_nr_tasks));
   		exit(1);
  	}
		
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "posix_memalign m_ptr\n");
   		exit(1);
	}
	
	/*---------------- Allocate memory for data   ---------------*/
	posix_memalign( (void **) &data_ptr, getpagesize(),  MAXCOPYBUF );
	if (data_ptr== NULL) {
   		fprintf(stderr, "posix_memalign data_ptr\n");
   		exit(1);
	}

	/*---------------- Allocate memory for filename   ---------------*/
	posix_memalign( (void **) &path_ptr, getpagesize(),  MNX_PATH_MAX );
	if (path_ptr== NULL) {
   		fprintf(stderr, "posix_memalign path_ptr\n");
   		exit(1);
	}
	
	SVRDEBUG("M3FTP SERVER m_ptr=%p\n",m_ptr);	
	
	/*---------------- M3FTP SERVER binding ---------------*/
	svr_pid = getpid();
    rcode  =	mnx_bind(dcid, svr_ep);
	if( svr_ep < 0 || svr_ep != rcode   ) {
		fprintf(stderr, "BIND ERROR rcode=%d\n",rcode);
	}
	
   	SVRDEBUG("BIND M3FTP SERVER dcid=%d svr_pid=%d svr_ep=%d\n",
		dcid, svr_pid, svr_ep);
	
	
	/*---------------- children creation ---------------*/
	for( i = 0; i < children; i++){	
		SVRDEBUG("child fork %d\n",i);
		if( (child_pid[i] = fork()) == 0 ){/* Child */
			ret = child_function( i );
		}
		/* parent */
		SVRDEBUG("M3FTP SERVER child_pid[%d]=%d\n",i, child_pid[i]);
	}
								
	/*--------- Waiting for START synchronization from children ---------*/
	SVRDEBUG("M3FTP SERVER: START synchronization from %d children: REQUEST\n", children);
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
		do {
			ret = mnx_receive(child_nr, (long) m_ptr);
			if (ret == EMOLSRCDIED)sleep(1);
		} while (ret == EMOLSRCDIED);
		if(ret < 0) {
			fprintf(stderr,"ERROR M3FTP SERVER: mnx_receive ret=%d\n", ret);
			exit(1);		
		}
	}
	/*--------------- Reply to  children -------------------------------*/
	SVRDEBUG("M3FTP SERVER: START synchronization from %d children: REPLY \n", children);
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
   		ret = mnx_send(child_nr,(long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR M3FTP SERVER: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}	
	
	/*--- Waiting START message from remote CLIENT----*/
 	SVRDEBUG("M3FTP SERVER: Waiting START message from remote CLIENT\n");
   	ret = mnx_receive(ANY, (long) m_ptr);
	if( m_ptr->m_source != CLT_NR){
		fprintf(stderr,"ERROR M3FTP SERVER: m_source(%d) != %d\n", m_ptr->m_source, CLT_NR);
		exit(1);		
	}
	ret = mnx_send(CLT_NR, (long) m_ptr);
	t_start = dwalltime();

	/*--- Waiting STOP message from remote M3FTP CLIENT----*/
 	SVRDEBUG("M3FTP SERVER: Waiting STOP message from remote M3FTP CLIENT\n");
   	ret = mnx_receive(CLT_NR, (long) m_ptr);
	t_stop  = dwalltime();
	/* reply to remote M3FTP CLIENT */
	ret = mnx_send(CLT_NR, (long) m_ptr);
	
	/*--------- Report statistics  ---------*/
	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of SENDREC/RECEIVE-SEND= %f[ms]\n", 1000*t_total/(double)(loops*children));
 	printf("Throuhput = %f [SENDREC/RECEIVE-SEND/s]\n", (double)(loops*children)/t_total);
	
	/*--------- STOP synchronization from children: REQUEST ---------*/
	SVRDEBUG("M3FTP SERVER: STOP synchronization from %d children: REQUEST\n", children);
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
		do {
			ret = mnx_receive(child_nr, (long) m_ptr);
			if (ret == EMOLSRCDIED)sleep(1);
		} while (ret == EMOLSRCDIED);
		if(ret < 0) {
			fprintf(stderr,"ERROR M3FTP SERVER: mnx_receive ret=%d\n", ret);
			exit(1);		
		}
	}
	/*--------- STOP synchronization from children: REPLY ---------*/
	SVRDEBUG("M3FTP SERVER: STOP synchronization from %d children: REPLY\n", children);
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
   		ret = mnx_send(child_nr,(long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR M3FTP SERVER: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}
	
	
	/*--------- Waiting for children EXIT ---------*/
 	SVRDEBUG("M3FTP SERVER: Waiting for children exit\n");
	for( i = 0; i < children; i++){	
		wait(&ret);
	}

	/*--------- Unbinding remote M3FTP client ----*/
	mnx_unbind(dcid,CLT_NR);
	SVRDEBUG("M3FTP SERVER END\n");

}



