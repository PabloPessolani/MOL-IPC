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

#define		RANDOM_PATERN	0
#define 	RMTNODE	1
#define 	DCID	0
#define 	SVR_NR	0
#define 	CLT_NR	(SVR_NR + 1)
#define 	MAXCHILDREN	100

int dcid, loops, children, maxbuf;
message *m_ptr;
char *buffer;

int child_function(int child) {
	int i , ret;
	pid_t child_pid;
	int child_nr,client_nr, child_ep;
	char rmt_name[16];
	
  	child_nr = SVR_NR + ((child+1) * 2);
	SVRDEBUG("CHILD child_nr %d: loops=%d child=%d\n", child_nr, loops, child);
		
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "CHILD child_nr %d: message posix_memalign\n", child_nr);
   		exit(1);
	}
	SVRDEBUG("CHILD child %d: m_ptr=%p\n",child_nr, m_ptr);
	
	/*---------------- Allocate memory for DATA BUFFER ---------------*/
	posix_memalign( (void**) &buffer, getpagesize(), MAXCOPYLEN);
	if (buffer== NULL) {
   		fprintf(stderr, "CHILD child_nr %d:buffer posix_memalign\n", child_nr);
   		exit(1);
  	}
	
	/*---------------- Fill with characters the DATA BUFFER ---------------*/
	srandom( getpid());
	
	for(i = 0; i < maxbuf-1; i++){
#define MAX_ALPHABET ('z' - '0')
#if RANDOM_PATERN
		buffer[i] =  (random()/(RAND_MAX/MAX_ALPHABET)) + '0';
#else	
		buffer[i] = ((i%25) + 'a');	
#endif
	}
	buffer[maxbuf] = 0;
	
	if( maxbuf > 30) buffer[30] = 0;	
	SVRDEBUG("CHILD child %d: buffer before=%s\n", child, buffer);
		
	/* binding remote client */
	client_nr = child_nr + 1;
	sprintf(rmt_name,"client%d",client_nr);
	ret = mnx_rmtbind(dcid, rmt_name, client_nr, RMTNODE);
	if(ret < 0) {
		fprintf(stderr,"CHILD child %d: mnx_rmtbind process %s on node %d \n", 
			client_nr , rmt_name, RMTNODE);
		exit(1);		
	}

	/* binding local server */
	child_ep = mnx_bind(dcid, child_nr);
	child_pid = getpid();
	SVRDEBUG("CHILD child_nr=%d: child_ep=%d child_pid=%d\n", 
		child_nr, child_ep, child_pid);
	
	/* synchronization with MAIN SERVER */
	ret = mnx_sendrec(SVR_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr %d: mnx_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}
		
	/* wait for message from remote client */
	ret = mnx_receive(client_nr, (long) m_ptr);
 	SVRDEBUG("CHILD %d: " MSG1_FORMAT, child, MSG1_FIELDS(m_ptr));
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr %d: mnx_receive ret=%d\n", child_nr, ret);
		exit(1);		
	}	
		
	/* M3-IPC TRANSFER LOOP  */
 	SVRDEBUG("CHILD child_nr %d:Starting loop\n", child_nr);
	for( i = 0; i < loops; i++) {
		ret = mnx_vcopy(child_ep, buffer, client_nr, m_ptr->m1_p1, maxbuf);	
		if(ret < 0) {
			SVRDEBUG("CHILD child_nr %d: VCOPY error=%d\n", child_nr, ret);
			exit(1);
		}
	}

	/* reply to remote client */
	ret = mnx_send(client_nr, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr %d: mnx_send ret=%d\n", child_nr, ret);
		exit(1);		
	}	

	/* STOP synchronization with MAIN SERVER */
	ret = mnx_sendrec(SVR_NR, (long) m_ptr);
	if(ret < 0) {
		fprintf(stderr,"CHILD child_nr %d: mnx_sendrec ret=%d\n", child_nr, ret);
		exit(1);		
	}	
	
	SVRDEBUG("CHILD child_nr %d: unbinding %d\n", child_nr,client_nr);
	mnx_unbind(dcid,client_nr);
	SVRDEBUG("CHILD child_nr %d:: exiting\n", child_nr);
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
	int  svr_pid, ret, i, svr_ep, pid, child_nr, child_ep;
	double t_start, t_stop, t_total, loopbysec, tput; 
	pid_t child_pid[MAXCHILDREN];
	struct dc_usr dc, *dc_usr_ptr;
	char rmt_name[16];

  	if (argc != 4) {
    	fprintf(stderr,"usage: %s <children> <loops>  <maxbuf> <\n", argv[0]);
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
	
	maxbuf = atoi(argv[3]);
	if( maxbuf <= 0 || maxbuf > MAXCOPYLEN) {
   		fprintf(stderr, " 0 < maxbuf < %d\n", MAXCOPYLEN);
   		exit(1);
	}
		
	/*---------------- MAIN SERVER binding ---------------*/
	svr_pid = getpid();
    svr_ep =	mnx_bind(dcid, SVR_NR);
	if( svr_ep < 0 ) {
		fprintf(stderr, "BIND ERROR svr_ep=%d\n",svr_ep);
	}
   	SVRDEBUG("BIND MAIN SERVER dcid=%d svr_pid=%d SVR_NR=%d svr_ep=%d\n",
		dcid, svr_pid, SVR_NR, 	svr_ep);

	/*---------------  remote client binding ---------*/
	sprintf(rmt_name,"client%d",CLT_NR);
	ret = mnx_rmtbind(dcid, rmt_name, CLT_NR, RMTNODE);
	if(ret < 0) {
    	fprintf(stderr,"ERROR MAIN SERVER mnx_rmtbind %d: process %s on node %d ret=%d\n", 
			CLT_NR, rmt_name, RMTNODE, ret);
    	exit(1);		
	}
   	SVRDEBUG("MAIN SERVER mnx_rmtbind %d: process %s on node %d \n", 
			CLT_NR, rmt_name, RMTNODE);
		
	/*---------------- children CREATION ---------------*/
	for( i = 0; i < children; i++){	
		SVRDEBUG("child fork %d\n",i);
		if( (child_pid[i] = fork()) == 0 ){/* Child */
			ret = child_function( i );
		}
		/* parent */
		SVRDEBUG("MAIN SERVER child_pid[%d]=%d\n",i, child_pid[i]);
	}
		
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "posix_memalign\n");
   		exit(1);
	}
	SVRDEBUG("MAIN SERVER m_ptr=%p\n",m_ptr);
	
	/*--------- Waiting for children START synchronization: REQUEST ---------*/
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
	}	

	/*--- Waiting START message from remote MAIN CLIENT----*/
 	SVRDEBUG("MAIN SERVER: Waiting START message from remote CLIENT\n");
   	ret = mnx_receive(CLT_NR, (long) m_ptr);

	/*--------- Sending children START synchronization REPLY ---------*/
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
   		ret = mnx_send(child_nr,(long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN SERVER: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}	
	ret = mnx_send(CLT_NR, (long) m_ptr);
	t_start = dwalltime();
	
	/*--- Waiting STOP message from remote CLIENT----*/
 	SVRDEBUG("MAIN SERVER: Waiting STOP message from remote CLIENT\n");
   	ret = mnx_receive(CLT_NR, (long) m_ptr);
	t_stop  = dwalltime();
	/* reply to remote MAIN CLIENT */
	ret = mnx_send(CLT_NR, (long) m_ptr);
	
	/*--------- Report statistics  ---------*/
	t_total = (t_stop-t_start);
	loopbysec = (double)(loops)/t_total;
	tput = loopbysec * (double)maxbuf;
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf , loops, loopbysec);
 	printf("Throuhput = %f [bytes/s]\n", tput);
	
	/*--------- Waiting for children STOP synchronization: REQUEST ---------*/
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
		ret = mnx_receive(child_nr, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN SERVER: mnx_receive ret=%d\n", ret);
			exit(1);		
		}		
	}	

	/*--------- Sending children STOP synchronization  REPLY ---------*/
	for( i = 0; i < children; i++){
	  	child_nr = SVR_NR + ((i+1) * 2);
   		ret = mnx_send(child_nr,(long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"ERROR MAIN SERVER: mnx_send ret=%d\n", ret);
			exit(1);		
		}			
	}	
	
	/*--------- Unbinding remote MAIN client ----*/
	mnx_unbind(dcid,CLT_NR);

	/*--------- Waiting for children EXIT ---------*/
 	SVRDEBUG("MAIN SERVER: Waiting for children exit\n");
	for( i = 0; i < children; i++){	
		wait(&ret);
	}
		
	SVRDEBUG("MAIN SERVER END\n");
	exit(0);
}



