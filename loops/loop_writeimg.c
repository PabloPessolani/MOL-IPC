/*
	This program test the Virtual Storage Task (tasks/memory) that serves 
	using a disk image file (i.e. image_file.img) 
	It reads the local image file (loop_readimg.img) and 
	writes it throuth the task to a image_file.img
	To check correctness and integrity you must do
		cmp loop_readimg.img image_file.img
		
*/

#define  MOL_USERSPACE	1

#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "../tasks/memory/memory.h"
#include "../tasks/debug.h"

#define TESTDEBUG 	TASKDEBUG


/* The drivers support the following operations (using message format m2):
 *
 *    m_type      DEVICE    IO_ENDPT    COUNT    POSITION  ADRRESS
 * ----------------------------------------------------------------
 * |  DEV_OPEN  | device  | proc nr |         |         |         |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_CLOSE | device  | proc nr |         |         |         |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_READ  | device  | proc nr |  bytes  |  offset | buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_WRITE | device  | proc nr |  bytes  |  offset | buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * | DEV_GATHER | device  | proc nr | iov len |  offset | iov ptr |
 * |------------+---------+---------+---------+---------+---------|
 * | DEV_SCATTER| device  | proc nr | iov len |  offset | iov ptr |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_IOCTL | device  | proc nr |func code|         | buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * |  CANCEL    | device  | proc nr | r/w     |         |         |
 * |------------+---------+---------+---------+---------+---------|
 * |  HARD_STOP |         |         |         |         |         |
 * ----------------------------------------------------------------
*/

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}


char *buffer;

void  main( int argc, char *argv[])
{
	int dcid;
	int clt_pid, svr_pid;
	int clt_ep, svr_ep;
	int clt_nr, svr_nr, rcode;
	off_t pos;
	unsigned position;
	message m , m1, *m_ptr;
	int i;
	FILE *fp_img;
	double t_start, t_stop, t_total;
	int buf_size, loops, bytes;

	if ( argc != 4 && argc != 5)	{
		fprintf(stderr, "Usage: %s <dcid> <clt_nr> <svr_nr> [buf_size] \n", argv[0] );
		exit(1);
	}

	/*verifico q el nro de vm sea válido*/
	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
		fprintf(stderr,"Invalid dcid [0-%d]\n", NR_DCS - 1 );
		exit(1);
	}

	/*entiendo nro cliente "cualquiera" - sería este proceso el q va a ser el cliente*/
	clt_nr = atoi(argv[2]);
	
	/*server endpoint es el q ya tengo de memory*/
	svr_ep = svr_nr = atoi(argv[3]);

	if ( argc == 5)	{
		buf_size = atoi(argv[4]);
		if( buf_size < 0 || buf_size >  MAXCOPYBUF) {
			fprintf(stderr,"Invalid 0 < buf_size=%d < %d\n", buf_size, MAXCOPYBUF+1);
			exit(1);
		}
	}else{
		buf_size = MAXCOPYBUF;
	}
	TESTDEBUG("CLIENT buf_size=%d\n", buf_size);

	/*hago el binding del proceso cliente, o sea este*/
	clt_pid = getpid();
	clt_ep = mnx_bind(dcid, clt_nr);
	
	if ( clt_ep < 0 ) {
		fprintf(stderr,"BIND ERROR clt_ep=%d\n", clt_ep);
		exit(1);
	}


	TESTDEBUG("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
   		dcid,
  		clt_pid,
   		clt_nr,
   		clt_ep);
		   
	TESTDEBUG("CLIENT pause before SENDREC\n");
	sleep(2); 

	m_ptr = &m;

	/*----------------------DEV_OPEN-------------------*/
	m.m_type 	= DEV_OPEN;
	m.DEVICE 	= RAM_DEV;
	m.IO_ENDPT 	= clt_ep;
	TESTDEBUG("DEV_OPEN REQUEST: " MSG2_FORMAT,MSG2_FIELDS(m_ptr) );
	rcode = mnx_sendrec(svr_ep, (long) &m); 
	
	if( rcode != 0 ) {
		fprintf(stderr,"DEV_OPEN mnx_sendrec rcode=%d\n",rcode);
		exit(1);
	}
	TESTDEBUG("DEV_OPEN REPLY: " MSG2_FORMAT,MSG2_FIELDS(m_ptr) );

	if( m.REP_STATUS != OK) {
		fprintf(stderr,"DEV_OPEN REPLY ERROR REP_STATUS=%d\n",m.REP_STATUS);
		exit(1);
	}

	sleep(2);

	rcode = posix_memalign( (void**) &buffer, getpagesize(), buf_size);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}

	/* open/create the local image file to copy */
	fp_img = fopen("image.img", "r");
	if( fp_img == NULL) {
		fprintf(stderr,"DEV_OPEN fopen ERROR for file image.img errno=%d\n", errno);
		exit(1);
	}

	/*----------------------DEV_WRITE-------------------*/
	position 	= 0;
	loops		= 1;
	m1.m_type 	= DEV_WRITE;
	m1.DEVICE 	= RAM_DEV; 
	m1.IO_ENDPT 	= clt_ep;
	m1.POSITION 	= position;
	m1.COUNT 	= buf_size; /*cantidad de bytes a escribir*/
	m1.ADDRESS	= buffer;
	t_start = dwalltime();
	do { 
		bytes= fread( (void *) buffer, 1, buf_size, fp_img);
		if( bytes < 0) {
			fprintf(stderr,"DEV_WRITE fread ERROR rcode=%d\n",bytes);
			exit(1);
		}
		if( bytes == 0) break; /* EOF */

		m =  m1; 
		m.POSITION = position;
		m1.COUNT   = bytes;

		TESTDEBUG("DEV_WRITE REQUEST: loops=%d " MSG2_FORMAT, loops, MSG2_FIELDS(m_ptr) );
		rcode = mnx_sendrec(svr_ep, (long) &m); 
		if( rcode != 0 ){
			fprintf(stderr,"DEV_WRITE mnx_sendrec rcode=%d\n",rcode);
			exit(1);
		}
		TESTDEBUG("DEV_WRITE REPLY: " MSG2_FORMAT,MSG2_FIELDS(m_ptr) );
		if( m.REP_STATUS < 0) {
			fprintf(stderr,"DEV_WRITE REPLY ERROR REP_STATUS=%d\n",m.REP_STATUS);
			exit(1);
		}

		position += bytes;
		loops++;
	}while( m.REP_STATUS != 0); /* means EOF */
     	t_stop  = dwalltime();
	TESTDEBUG("DEV_WRITE total bytes read=%d", position);
	t_total = (t_stop-t_start);
	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
	printf("Loops = %d\n", loops);
	printf("Time for a pair of REQUEST-REPLY= %f[ms]\n", 1000*t_total/2/(double)loops);
	printf("Message Throuhput = %f [REQUEST-REPLY/s]\n", (double)loops*2/t_total);
	printf("Trasfered bytes= %f [Bytes]\n", (double)position);
	printf("Data Throuhput = %f [MBytes/s]\n", (double)position/1024/1024/t_total);

	/*----------------------DEV_CLOSE-------------------*/
	
    	m.m_type = DEV_CLOSE;
	m.DEVICE = RAM_DEV; 
	m.IO_ENDPT = clt_ep;
	TESTDEBUG("DEV_CLOSE REQUEST: " MSG2_FORMAT,MSG2_FIELDS(m_ptr) );
	rcode = mnx_sendrec(svr_ep, (long) &m); 
	
	if( rcode != 0 ) {
		fprintf(stderr,"DEV_CLOSE mnx_sendrec rcode=%d\n",rcode);
		exit(1);
	}
	TESTDEBUG("DEV_CLOSE REPLY: " MSG2_FORMAT,MSG2_FIELDS(m_ptr) );

	if( m.REP_STATUS != OK) {
		fprintf(stderr,"DEV_CLOSE REPLY ERROR REP_STATUS=%d\n",m.REP_STATUS);
		exit(1);
	}

	rcode = fclose( fp_img );
	if (rcode)	{
		fprintf(stderr,"DEV_CLOSE fclose rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	exit(0);

 }

