#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 


#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/molerrno.h"


#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	SRC_NR	1
#define 	DST_NR	2

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

   
void  main ( int argc, char *argv[] )
{
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, maxbuf;
	message m;
	long int loops, total_bytes = 0 ;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer;
	
  	if (argc != 3) {
    		printf ("usage: %s <loops> <bufsize> \n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);
  	maxbuf = atoi(argv[2]);

	posix_memalign( &buffer, getpagesize(), maxbuf );
	if (buffer== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
	printf("buffer %p\n",buffer);
	
	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	dst_pid = getpid();
    	dst_ep =	mnx_bind(dcid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		dcid,
		dst_pid,
		dst_nr,
		dst_ep);

	for(i = 0; i < maxbuf-1; i++)
		buffer[i] = ((i%25) + 'a');	
	buffer[maxbuf] = 0;
	
	if( (src_pid = fork()) != 0 )	{		/* PARENT */
		printf("RECEIVER pause before RECEIVE\n");

if( maxbuf < 1025)
		printf("RECEIVER buffer before = %s\n", buffer);

		sleep(2); /* PAUSE before RECEIVE*/
		do {
			src_ep = mnx_getep(src_pid);
			sleep(1);
			} while(src_ep < 0);
/*			
		printf("Dump procs of DC%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}
*/	
	    ret = mnx_receive(ANY, (long) &m);
		t_start = dwalltime();
		for( i = 0; i < (loops+10); i++) {
			if( i == 10) t_start = dwalltime();
			ret = mnx_vcopy( src_ep, buffer, dst_ep, buffer, maxbuf);	
		}
	    t_stop  = dwalltime();
  		ret = mnx_send(src_ep, (long) &m);

if( maxbuf < 1025)
		printf("RECEIVER buffer after = %s\n", buffer);

		t_total = (t_stop-t_start);
		loopbysec = (double)(loops)/t_total;
		tput = loopbysec * (double)maxbuf;
		
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf, loops, loopbysec);
 		printf("Throuhput = %f [bytes/s]\n", tput);
		wait(&ret);

	}else{						/* SON = 		*/
		for(i = 0; i < maxbuf-1; i++)
			buffer[i] = ((i%25) + 'A');	
		buffer[maxbuf] = 0;

if( maxbuf < 1025)
		printf("SENDER buffer before = %s\n", buffer);

		src_pid = getpid();
    		src_ep =mnx_bind(dcid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);

   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);

	  	m.m_type= 0x0001;
		m.m1_i1 = 0x00;
		m.m1_i2 = 0x02;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

    		ret = mnx_sendrec(dst_ep, (long) &m);
//		printf("SENDER buffer after = %s\n", buffer);

	}
 printf("\n");
 

 
 exit(0);
}



