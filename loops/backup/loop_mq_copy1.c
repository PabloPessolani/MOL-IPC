#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>      // for time()
#include <string.h>
#include <syslog.h> 

#include <sys/types.h>
#include <sys/wait.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include "./kernel/minix/ipc.h"

 
#define		MAXBUF	(4*1024)
#define 	QUEUEBASE	7000
#define		LOOPS		500000

typedef struct {
		int		mtype;
		char mtext[MAXBUF];
		} msgq_buf_t;

msgq_buf_t buffer;

struct msqid_ds mq1_ds;
struct msqid_ds mq2_ds;


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
	int mq1, mq2, bytes, fbytes, i, ntransf;
	long int total_bytes = 0 ;
	double t_start, t_stop, t_total;

		printf("PROGRAM: %s\n",argv[0]);

		msgctl ( QUEUEBASE, IPC_RMID, 0);
		msgctl ( QUEUEBASE+1, IPC_RMID, 0);

		mq1 = msgget(QUEUEBASE, IPC_CREAT | 0x660);
		if ( mq1 < 0) {
			if ( errno != EEXIST) {
				printf("rerror1 %d\n",mq1);
				exit(1);
			}
			printf( "The queue with key=%d already exists\n",QUEUEBASE);
			mq1 = msgget( (QUEUEBASE), 0);
			if(mq1 < 0) {
				printf("rerror1 %d\n",mq1);
				exit(1);
			}
			printf("msgget OK\n");
		} 

msgctl(mq1 , IPC_STAT, &mq1_ds);
printf("before mq1 msg_qbytes =%d\n",mq1_ds.msg_qbytes);
mq1_ds.msg_qbytes = MAXBUF;
msgctl(mq1 , IPC_SET, &mq1_ds);
msgctl(mq1 , IPC_STAT, &mq1_ds);
printf("after mq1 msg_qbytes =%d\n",mq1_ds.msg_qbytes);

		mq2 = msgget(QUEUEBASE+1, IPC_CREAT | 0x660);
		if ( mq2 < 0) {
			if ( errno != EEXIST) {
				printf("rerror2 %d\n",mq2);
				exit(1);
			}
			printf( "The queue with key=%d already exists\n",QUEUEBASE+1);
			mq2 = msgget( (QUEUEBASE+1), 0);
			if(mq2 < 0) {
				printf("rerror2 %d\n",mq2);
				exit(1);
			}
			printf("msgget OK\n");
		}

msgctl(mq2 , IPC_STAT, &mq2_ds);
printf("before mq2 msg_qbytes =%d\n",mq2_ds.msg_qbytes);
mq2_ds.msg_qbytes = MAXBUF;
msgctl(mq2 , IPC_SET, &mq2_ds);
msgctl(mq2 , IPC_STAT, &mq2_ds);
printf("after mq2 msg_qbytes =%d\n",mq2_ds.msg_qbytes);

	for(i = 0; i < MAXBUF; i++)
		buffer.mtext[i] = ((i%10) + '0');

	if( fork() != 0 )	{		/* PARENT = SOURCE */
		printf("SENDER starting the loop\n");

		buffer.mtype = 0x0001;
		for( i = 0; i < LOOPS; i++) {
//		while( (fbytes = fread(&buffer.mtext, sizeof(char), MAXBUF, stdin)) > 0) {
//printf("SENDER: %s\n", &buffer.mtext);
			bytes = msgsnd(mq1, &buffer, MAXBUF, 0); 
//printf("S\n");

		}
		sleep(10);

	}else{						/* SON = DESTINATION		*/

		printf("RECEIVER pause before RECEIVE\n");
		sleep(5); /* PAUSE before RECEIVE*/

		ntransf = 0;
		t_start = dwalltime();
		for( i = 0; i < LOOPS; i++) {
			bytes = msgrcv(mq1, &buffer, MAXBUF, 0 , 0 );
//printf("RECEIVER: %s\n", &buffer.mtext);
//printf("R\n");
		}
		t_stop  = dwalltime();

		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Total Bytes=%d transfer size=%d #transfers=%d\n", (MAXBUF*LOOPS) , MAXBUF, LOOPS);
 		printf("Throuhput = %.0f [bytes/s]\n", (double)(MAXBUF*LOOPS)/t_total);
		
	}

 printf("\n");

 exit(0);
}



