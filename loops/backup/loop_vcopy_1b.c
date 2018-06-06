#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

#include "./kernel/minix/proxy.h"
#include "./kernel/minix/molerrno.h"

char buffer[MAXBUFSIZE*3];
   
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
	int dcid, pid1, pid2, p_nr1,p_nr2, ret, ep1, ep2, i, loops, len;
	message m, m1,m2;
	double t_start, t_stop, t_total;

    if ( argc != 5) {
 	        printf( "Usage: %s <dcid> <p_nr1> <p_nr2> <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_VMS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_VMS-1 );
 	        exit(1);
	    }

	p_nr1 = atoi(argv[2]);
	p_nr2  = atoi(argv[3]);
	loops  = atoi(argv[4]);

	pid1 = getpid();

     ep1 = mnx_bind(dcid, p_nr1);
	if( ep1 < 0 ) {
		printf("BIND ERROR %d\n",ep1);
		exit(ep1);
	}
    printf("Binding LOCAL process %d to VM%d with p_nr=%d\n",pid1,dcid,p_nr1);

	if( (pid2 = fork()) != 0)	{		/* PARENT "SYSTASK" */

	ret = mnx_receive(ANY, (long) &m1);		/* first receive */
   	if( ret != OK)
		printf("RECEIVE ERROR %d\n",ret);
	printf("FIRST message received from %d\n",m1.m_source);


	ret = mnx_receive(ANY, (long) &m2);		/* second receive */
   	if( ret != OK)
		printf("RECEIVE ERROR %d\n",ret);
	printf("SECOND message received from %d\n",m2.m_source);

	printf("VCOPY FROM source=%d TO dest=%d len=%d \n", m1.m_source,m2.m_source, m1.m1_i2);

	t_start = dwalltime();
   	for (i = 0 ; i < loops; i++) {		
		ret = mnx_vcopy(m1.m_source, m1.m1_i1, m2.m_source, m2.m1_i1, m1.m1_i2);	
		if( ret != 0 )
		    	printf("VCOPY ret=%d\n",ret);
	}
 	t_stop  = dwalltime();

	ret = mnx_send(m1.m_source, (long) &m1);	/* first replay	*/
	if( ret != 0 )
	   	printf("SEND ret=%d\n",ret);
	printf("FIRST REPLAY to %d\n",m1.m_source);


	ret = mnx_send(m2.m_source, (long) &m2);	/* second replay 	*/
	if( ret != 0 )
   	printf("SEND ret=%d\n",ret);
	printf("SECOND REPLAY to %d\n",m1.m_source);
	
	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of COPYIN= %f[ms]\n", 1000*t_total/(double)loops);
 	printf("Throuhput = %f [COPYIN/s]\n", (double)loops/t_total);

	sleep(20);

    	printf("Unbinding process %d from VM%d with src_nr=%d\n",pid1,dcid,p_nr1);
   	mnx_unbind(dcid,ep1);
	
	
	}else {
	   	ep2 = mnx_bind(dcid, p_nr2);
		if( ep2 < 0 ) {
			printf("BIND ERROR %d\n",ep2);
			exit(ep2);
			}

		pid2 = getpid();

//strcpy(buffer, "HOLA QUE TAL REMOTO, SOY EL HIJO DE LOCAL");
//len = strlen(buffer);
		for ( i = 0 ; i < (MAXBUFSIZE*3) ; i++) 
			buffer[i] = 'A' + (i%10);
		len = (MAXBUFSIZE*2)+100;

		m.m_type= 0x01;
		m.m1_i1 = (int) &buffer;
		m.m1_i2 = len;
		m.m1_i3 = 0x03;
   		ret = mnx_sendrec(ep1, (long) &m);
		if( ret < 0 ) {
			printf("SENDREC %d\n",ret);
			exit(ret);
			}

	    	printf("Unbinding process %d from VM%d with src_nr=%d\n",pid2,dcid,p_nr2);
   		mnx_unbind(dcid,ep2);
	}
	
 }



