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
#include "/usr/src/linux/kernel/minix/ipc.h"

 
#define 	QUEUEBASE	7000
#define  	MAXBUF (4*1024)

typedef struct {
		int		mtype;
		char mtext[MAXBUF];
		} msgq_buf_t;

msgq_buf_t buffer;

struct msqid_ds mq_ds;
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
	int mq, mq1, mq2, bytes, loops, i, status;
	double t_start, t_stop, t_total,loopbysec, tput;
	int maxbuf;
  	if (argc != 2) {
    		printf ("usage: %s <loops> \n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);

	msgctl ( QUEUEBASE, IPC_RMID, 0);
	msgctl ( QUEUEBASE+1, IPC_RMID, 0);

	mq = msgget(QUEUEBASE, IPC_CREAT | 0x660);
	if ( mq < 0) {
		if ( errno != EEXIST) {
			printf("rerror1 %d\n",mq);
			exit(1);
		}
		printf( "The queue with key=%d already exists\n",QUEUEBASE);
		mq = msgget( (QUEUEBASE), 0);
		if(mq < 0) {
			printf("rerror %d\n",mq);
			exit(1);
		}
		printf("msgget OK\n");
	} 

msgctl(mq , IPC_STAT, &mq_ds);
printf("before mq msg_qbytes =%d\n",mq_ds.msg_qbytes);
mq_ds.msg_qbytes = MAXBUF;
msgctl(mq , IPC_SET, &mq_ds);
msgctl(mq , IPC_STAT, &mq_ds);
printf("after mq msg_qbytes =%d\n",mq_ds.msg_qbytes);

		mq1 = msgget(QUEUEBASE+1, IPC_CREAT | 0x660);
		if ( mq1 < 0) {
			if ( errno != EEXIST) {
				printf("rerror2 %d\n",mq1);
				exit(1);
			}
			printf( "The queue with key=%d already exists\n",QUEUEBASE+1);
			mq1 = msgget( (QUEUEBASE+1), 0);
			if(mq1 < 0) {
				printf("rerror2 %d\n",mq1);
				exit(1);
			}
			printf("msgget OK\n");
		}

msgctl(mq1 , IPC_STAT, &mq1_ds);
printf("before mq2 msg_qbytes =%d\n",mq1_ds.msg_qbytes);
mq1_ds.msg_qbytes = MAXBUF;
msgctl(mq1 , IPC_SET, &mq1_ds);
msgctl(mq1 , IPC_STAT, &mq1_ds);
printf("after mq1 msg_qbytes =%d\n",mq1_ds.msg_qbytes);

	
		mq2 = msgget(QUEUEBASE+2, IPC_CREAT | 0x660);
		if ( mq2 < 0) {
			if ( errno != EEXIST) {
				printf("rerror2 %d\n",mq2);
				exit(1);
			}
			printf( "The queue with key=%d already exists\n",QUEUEBASE+2);
			mq2 = msgget( (QUEUEBASE+2), 0);
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

	maxbuf = MAXBUF; 
	
	for(i = 0; i < MAXBUF; i++)
		buffer.mtext[i] = ((i%10) + '0');
	buffer.mtext[40]= 0;
	
	if( fork() != 0 )	{		/* PARENT = SOURCE */
		buffer.mtype = 0x0001;
		bytes = msgrcv(mq, &buffer, MAXBUF, 0 , 0 );
		bytes = msgsnd(mq2, &buffer, MAXBUF, 0); 
		printf("SERVER starting the loop\n");
		t_start = dwalltime();
		for( i = 0; i < loops; i++) {
			bytes = msgrcv(mq, &buffer, MAXBUF, 0 , 0 );
//printf("SERVER receive1:%s\n",&buffer.mtext);
			bytes = msgsnd(mq1, &buffer, MAXBUF, 0); 
			bytes = msgrcv(mq, &buffer, MAXBUF, 0 , 0 );
//printf("SERVER receive2:%s\n",&buffer.mtext);
			bytes = msgsnd(mq2, &buffer, MAXBUF, 0);
		}
		t_stop  = dwalltime();
		t_total = (t_stop-t_start);
		loopbysec = (double)(loops)/t_total;
		tput = loopbysec * (double)maxbuf *2;	
		
		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf, loops, loopbysec);
 		printf("Throuhput = %f [bytes/s]\n", tput);
		wait(&status);
		wait(&status);
	}else{				
		if( fork() != 0) {
			sleep(2); /* PAUSE before RECEIVE*/
			buffer.mtype = 0x0001;
			bytes = msgsnd(mq, &buffer, MAXBUF, 0); 
			bytes = msgrcv(mq2, &buffer, MAXBUF, 0 , 0 );
			printf("CLIENT2 starting the loop\n");
			for( i = 0; i < loops; i++) {
				bytes = msgsnd(mq, &buffer, MAXBUF, 0); 
				bytes = msgrcv(mq2, &buffer, MAXBUF, 0 , 0 );
//printf("CLIENT2 receive1:%s\n",&buffer.mtext);
			}
		}else {
			sleep(2); /* PAUSE before RECEIVE*/
			buffer.mtype = 0x0001;
			printf("CLIENT1 starting the loop\n");
			for( i = 0; i < loops; i++) {
				bytes = msgrcv(mq1, &buffer, MAXBUF, 0 , 0 );
//printf("CLIENT1 receive1:%s\n",&buffer.mtext);
				bytes = msgsnd(mq, &buffer, MAXBUF, 0); 
			}		
		}
	}	

 printf("\n");

 exit(0);
}



