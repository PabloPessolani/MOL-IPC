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
#define  	MAXBUF (64*1024)

typedef struct {
		int		mtype;
		char mtext[MAXBUF];
		} msgq_buf_t;

msgq_buf_t buffer;

struct msqid_ds mq_ds;
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
	int mq, mq2, bytes, loops, i, status;
	double t_start, t_stop, t_total,loopbysec, tput;
	int buf_size;
  	if (argc != 3) {
    		printf ("usage: %s <size> <loops> \n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);
  	buf_size  = atoi(argv[2]);

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
mq_ds.msg_qbytes = buf_size;
msgctl(mq , IPC_SET, &mq_ds);
msgctl(mq , IPC_STAT, &mq_ds);
printf("after mq msg_qbytes =%d\n",mq_ds.msg_qbytes);

	
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
mq2_ds.msg_qbytes = buf_size;
msgctl(mq2 , IPC_SET, &mq2_ds);
msgctl(mq2 , IPC_STAT, &mq2_ds);
printf("after mq2 msg_qbytes =%d\n",mq2_ds.msg_qbytes);


	for(i = 0; i < buf_size; i++)
		buffer.mtext[i] = ((i%10) + '0');
	buffer.mtext[40]= 0;

	if( fork() != 0 )	{		/* PARENT = SOURCE */
		printf("SERVER starting the loop\n");

		buffer.mtype = 0x0001;
		bytes = msgrcv(mq, &buffer, buf_size, 0 , 0 );
		bytes = msgsnd(mq2, &buffer, buf_size, 0); 
		t_start = dwalltime();
		for( i = 0; i < loops; i++) {
			bytes = msgrcv(mq, &buffer, buf_size, 0 , 0 );
//printf("SERVER receive:%s\n",&buffer.mtext);
			bytes = msgsnd(mq2, &buffer, buf_size, 0); 
		}
		t_stop  = dwalltime();
		t_total = (t_stop-t_start);
		loopbysec = (double)(loops)/t_total;
		tput = loopbysec * (double)buf_size *2;	
		
		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("transfer size=%d #transfers=%d loopbysec=%f\n", buf_size, loops, loopbysec);
 		printf("Throuhput = %f [bytes/s]\n", tput);
		wait(&status);
		wait(&status);
	}else{						/* SON = DESTINATION		*/

		printf("RECEIVER pause before RECEIVE\n");
		sleep(1); /* PAUSE before RECEIVE*/
		buffer.mtype = 0x0001;
		bytes = msgsnd(mq, &buffer, buf_size, 0); 
		bytes = msgrcv(mq2, &buffer, buf_size, 0 , 0 );
		for( i = 0; i < loops; i++) {
			bytes = msgsnd(mq, &buffer, buf_size, 0); 
			bytes = msgrcv(mq2, &buffer, buf_size, 0 , 0 );
//printf("CLIENT receive:%s\n",&buffer.mtext);
		}
	}

 printf("\n");

 exit(0);
}



