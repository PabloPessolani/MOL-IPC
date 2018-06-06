#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>    // for srand(), rand()
#include <time.h>      // for time()
#include <errno.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h> /* For portability */
#include <sys/msg.h>

struct msgbuf {
    long mtype;       /* message type, must be > 0 */
    char mtext[1024];    /* message data */
} mbuf;

main(int argc, char *argv[])
{
	int mq, bytes;

	mq = msgget( 0x1234, IPC_CREAT | 0x660);
	if (  mq < 0) {
		if ( errno != EEXIST) {
			printf("msgget errno=%d\n",errno);
			exit(1);
		}
		mq = msgget( 0x1234, 0);
		if(  mq < 0) {
			printf("msgget errno=%d\n",errno);
			exit(1);
		}
	} 

	printf("msgget OK\n");
	
	mbuf.mtype = 0x7890;
	sprintf(mbuf.mtext, "MENSAJE DESDE PROCESO %d", getpid());

	bytes = msgsnd(mq, &mbuf, sizeof(mbuf),0);
	if ( bytes < 0) {
		printf("msgrcv msgsnd=%d\n",errno);
		exit(1);
	} 

	printf("SENT bytes=%d type=%X text=%s \n", bytes, mbuf.mtype,mbuf.mtext);
	exit(0);
}


