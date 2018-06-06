/*
	This program test the Virtual File System connected to the 
	Storage Task (tasks/memory) that serves 
	using a disk image file (i.e. image_file.img) 
	It reads one file from and store it into a local file equally named
	To check correctness and integrity you must do	cmp command
	
*/

#define  MOL_USERSPACE	1

#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "../stub_syscall.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/callnr.h"
#include "../kernel/minix/fcntl.h"
#include "../servers/debug.h"
#include "limits.h"

#define TESTDBG

#ifdef TESTDBG
 #define TESTDEBUG(text, args ...) \
 do { \
     printf(" %s:%u:" \
             text ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define TESTDEBUG(x, args ...)
#endif 


double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

#define MNX_POSIX_PATH_MAX	256 
char *buffer;
char img_path[MNX_POSIX_PATH_MAX+1];
int clt_ep, svr_ep;


int MOL_close(int fd)
{
  	message m , *m_ptr;
	int rcode;

	m_ptr = &m;
  	m.m_type = MOLCLOSE;
  	m.m1_i1 = fd;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

  	return(rcode);
}

ssize_t MOL_read(int fd, void *buffer, size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;
	
	m_ptr = &m;
  	m.m_type = MOLREAD;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("MOL_read rcode=%d\n", rcode);
  	return(m_ptr->m_type);
}


off_t MOL_lseek(int fd, off_t offset, int whence)
{
  	message m , *m_ptr;
	int rcode;

	m_ptr = &m;
  	m.m_type = MOLLSEEK;
  	m.m2_i1 = fd;
  	m.m2_l1 = offset;
  	m.m2_i2 = whence;
	TESTDEBUG("Request: " MSG2_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m); 
	TESTDEBUG("Reply: " MSG2_FORMAT, MSG1_FIELDS(m_ptr));

  	if( rcode )
		return( (off_t) -1);
	TESTDEBUG("BUFFER [%s]\n", buffer);
  	return( (off_t) m.m2_l1);
}


int MOL_open(char path[], int flags) 
{
	message m, *m_ptr;
	int rcode;

	m_ptr =&m;
	m.m_type = MOLOPEN;
	TESTDEBUG("MOL_open: %s %X\n" , path, flags);

	if( strlen(path) > (M3_STRING-1)) {
		m.m1_i1 = strlen(path) + 1;
		m.m1_i2 = flags; 
		m.m1_p1 = (char *) path;
		TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	} else {
		m.m3_i1 = strlen(path) + 1;	
		m.m3_p1 = path;
		m.m3_i2 = O_RDONLY; //para lectura tipo M3
	 	strcpy(m.m3_ca1, path);	
		TESTDEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	}

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_open rcode=%d\n", rcode);
		exit(1);
	}

	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	return( m.m_type);
}


void  main( int argc, char *argv[])
{
	int dcid;
	int clt_pid, svr_pid;
	int clt_nr, rcode;
	off_t pos;
	unsigned position;
	int i, fd;
	double t_start, t_stop, t_total;
	int buf_size, loops, bytes;
	FILE *fp_lcl;

	if ( argc != 4 && argc != 5)	{
		fprintf(stderr, "Usage: %s <dcid> <clt_nr> <filename> [buf_size] \n", argv[0] );
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
	
	if ( argc == 5)	{
		buf_size = atoi(argv[4]);
		if( buf_size < 0 || buf_size >  MAXCOPYLEN) {
			fprintf(stderr,"Invalid 0 < buf_size=%d < %d\n", buf_size, MAXCOPYLEN+1);
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


	/*---------------------- FILE_OPEN-------------------*/

	rcode = posix_memalign( (void**) &buffer, getpagesize(), buf_size);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}
	TESTDEBUG("CLIENT buffer address=%X point=%X\n", buffer, *buffer);

	
	fd = MOL_open(argv[3], O_RDWR);
		
	if( fd < 0 ) {
		fprintf(stderr,"MOL_open fd=%d\n",fd);
		exit(1);
	}
	TESTDEBUG("fd=%d\n",fd);

	/* open/create the local image file to copy */
	fp_lcl = fopen(argv[3], "w+");
	if( fp_lcl == NULL) {
		fprintf(stderr,"fopen ERROR for file=%s errno=%d\n", argv[3], errno);
		exit(1);
	}

	/*----------------------FILE_READ-------------------*/
	position = 0;
	loops = 0;
	t_start = dwalltime();
	do { 
//		position = MOL_lseek(fd, position SEEK_SET);
//		if( rcode != 0 ){
//			fprintf(stderr,"MOL_lseek rcode=%d\n", position);
//			exit(1);
//		}

		bytes = MOL_read(fd, (void *) buffer, buf_size); 
		if( bytes < 0) {
			fprintf(stderr," MOL_read ERROR rcode=%d\n",bytes);
			exit(1);
		}
		loops++;
		if( bytes == 0) { /* EOF */ 
			printf("EOF\n");
			break;
		}
		position += bytes;

		bytes= fwrite( (void *) buffer, 1, bytes, fp_lcl);
		if( bytes < 0) {
			fprintf(stderr,"fwrite ERROR rcode=%d\n",bytes);
			exit(1);
		}
	}while( bytes > 0);

     	t_stop  = dwalltime();
	TESTDEBUG("FILE_READ total bytes read=%d", position);
	t_total = (t_stop-t_start);
	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
	printf("Loops = %d\n", loops);
	printf("Time for a pair of REQUEST-REPLY= %f[ms]\n", 1000*t_total/2/(double)loops);
	printf("Message Throuhput = %f [REQUEST-REPLY/s]\n", (double)loops*2/t_total);
	printf("Data Throuhput = %f [MBytes/s]\n", (double)position/1024/1024/t_total);

	/*----------------------FILE_CLOSE-------------------*/
	
	rcode = MOL_close(fd);
	if( rcode != 0 ) {
		fprintf(stderr,"FILE_CLOSE rcode=%d\n",rcode);
		exit(1);
	}

	rcode = fclose( fp_lcl );
	if (rcode)	{
		fprintf(stderr,"fclose rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	exit(0);

 }

