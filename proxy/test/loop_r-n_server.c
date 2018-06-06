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
	
	/* synchronization with MAIN SERVER */
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
   		ret = mnx_notify(client_nr);
		if(ret < 0) {
			fprintf(stderr,"CHILD %d: mnx_notify ret=%d\n", child, ret);
			exit(1);		
		}	
	}
	SVRDEBUG("CHILD %d:" MSG1_FORMAT, child, MSG1_FIELDS(m_ptr));

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
	SVRDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_usr_ptr));
	
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
	SVRDEBUG("MAIN SERVER m_ptr=%p\n",m_ptr);	
	
	/*---------------- MAIN SERVER bind ---------------*/
	svr_pid = getpid();
    svr_ep =	mnx_bind(dcid, SVR_NR);
	if( svr_ep < 0 ) {
		fprintf(stderr, "BIND ERROR svr_ep=%d\n",svr_ep);
	}
   	SVRDEBUG("BIND MAIN SERVER dcid=%d svr_pid=%d SVR_NR=%d svr_ep=%d\n",
		dcid, svr_pid, SVR_NR, 	svr_ep);
	
	/*--------------- binding remote client ---------*/
	sprintf(rmt_name,"client%d",CLT_NR);
	ret = mnx_rmtbind(dcid, rmt_name, CLT_NR, RMTNODE);
	if(ret < 0) {
    	fprintf(stderr,"ERROR MAIN SERVER mnx_rmtbind %d: process %s on node %d ret=%d\n", 
			CLT_NR, rmt_name, RMTNODE, ret);
    	exit(1);		
	}
   	SVRDEBUG("MAIN SERVER mnx_rmtbind %d: process %s on node %d \n", 
			CLT_NR, rmt_name, RMTNODE);	
			
	/*---------------- Creat children ---------------*/
	for( i = 0; i < children; i++){	
		SVRDEBUG("child fork %d\n",i);
		if( (child_pid[i] = fork()) == 0 ){/* Child */
			ret = child_function( i );
		}
		/* parent */
		SVRDEBUG("MAIN SERVER child_pid[%d]=%d\n",i, child_pid[i]);
	}
				
	/*--------- Waiting for children START synchronization ---------*/
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
		do {
			ret = mnx_receive(child_nr, (long) m_ptr);
			if (ret == EMOLSRCDIED)sleep(1);
		} while (ret == EMOLSRCDIED);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN SERVER: mnx_receive ret=%d\n", ret);
			exit(1);		
		}		
   		ret = mnx_send(child_nr,(long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN SERVER: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}	
	
	/*--- Waiting START message from remote CLIENT----*/
 	SVRDEBUG("MAIN SERVER: Waiting START message from remote CLIENT\n");
   	ret = mnx_receive(ANY, (long) m_ptr);
	if( m_ptr->m_source != CLT_NR){
		fprintf(stderr,"ERROR MAIN SERVER: m_source(%d) != %d\n", m_ptr->m_source, CLT_NR);
		exit(1);		
	}
	ret = mnx_send(CLT_NR, (long) m_ptr);
	t_start = dwalltime();

	/*--- Waiting STOP message from remote CLIENT----*/
 	SVRDEBUG("MAIN SERVER: Waiting STOP message from remote CLIENT\n");
   	ret = mnx_receive(CLT_NR, (long) m_ptr);
	t_stop  = dwalltime();
	
	/*--------- Report statistics  ---------*/
	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of SENDREC/RECEIVE-NOTIFY= %f[ms]\n", 1000*t_total/(double)(loops*children));
 	printf("Throuhput = %f [SENDREC/RECEIVE-NOTIFY/s]\n", (double)(loops*children)/t_total);
	
	/*--------- Waiting for children EXIT ---------*/
 	SVRDEBUG("MAIN SERVER: Waiting for children exit\n");
	for( i = 0; i < children; i++){	
		wait(&ret);
	}
	
	/* reply to remote MAIN CLIENT */
	ret = mnx_send(CLT_NR, (long) m_ptr);
	
	/*--------- Unbinding remote MAIN client ----*/
	mnx_unbind(dcid,CLT_NR);
	SVRDEBUG("MAIN SERVER END\n");

}



