/*
	This program test the Virtual File System connected to the 
	Storage Task (tasks/memory) that serves 
	using a disk image file (i.e. image_file.img) 
	It reads one file from and store it into a local file equally named
	To check correctness and integrity you must do	cmp command
	
*/

#define  MOL_USERSPACE	1

#define nil 0
#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "../stub_syscall.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/callnr.h"
#include "../kernel/minix/types.h"	
#include "../kernel/minix/fcntl.h"
#include "../kernel/minix/dir.h"
#include "../kernel/minix/dirent.h"
#include "../servers/debug.h"
#include "limits.h"


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

void MOL_loadname(const char *name, message *msgptr)
{
/* This function is used to load a string into a type m3 message. If the
 * string fits in the message, it is copied there.  If not, a pointer to
 * it is passed.
 */
  register size_t k;

  k = strlen(name) + 1;
  msgptr->m3_i1 = k;
  msgptr->m3_p1 = (char *) name;
  if (k <= sizeof msgptr->m3_ca1) strcpy(msgptr->m3_ca1, name);
}

int MOL_open(char path[], int flags) 
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_open\n");
	m_ptr =&m;
	m.m_type = MOLOPEN;
	TESTDEBUG("MOL_open: %s %X\n" , path, flags);

	// TESTDEBUG("strlen(path) =%d\n", strlen(path));
	if (flags & O_CREAT) {
		m.m1_i1 = strlen(path) + 1;
		m.m1_i2 = flags; 
		// m.m1_i3 = va_arg(argp, _mnx_Mode_t); //Ver esto de los arg variables de open
		m.m1_p1 = (char *) path;
		INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	} else {
		MOL_loadname(path, &m);
		m.m3_i2 = flags;
		INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	}

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_open rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_open\n");	
	return( m.m_type);
}

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


void  main( int argc, char *argv[])
{
	int vmid;
	int clt_pid, svr_pid;
	int clt_nr, rcode;
	off_t pos;
	unsigned position;
	int i, fd;
	double t_start, t_stop, t_total;
	int buf_size, loops, bytes;
	FILE *fp_lcl;

	if ( argc != 5 && argc != 6)	{
		fprintf(stderr, "Usage: %s <vmid> <clt_nr> <localFilename> <MolFilename> [buf_size] \n", argv[0] );
		exit(1);
	}

	/*verifico q el nro de vm sea válido*/
	vmid = atoi(argv[1]);
	if ( vmid < 0 || vmid >= NR_VMS) {
		fprintf(stderr,"Invalid vmid [0-%d]\n", NR_VMS - 1 );
		exit(1);
	}

	/*entiendo nro cliente "cualquiera" - sería este proceso el q va a ser el cliente*/
	clt_nr = atoi(argv[2]);
	
	if ( argc == 6)	{
		buf_size = atoi(argv[5]);
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
	clt_ep = mnx_bind(vmid, clt_nr);
	
	if ( clt_ep < 0 ) {
		fprintf(stderr,"BIND ERROR clt_ep=%d\n", clt_ep);
		exit(1);
	}

	TESTDEBUG("BIND CLIENT vmid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
   		vmid,
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
	
	/*----------------------FILE_OPEN-------------------*/
	fd = MOL_open(argv[4], O_RDWR);
		
	if( fd < 0 ) {
		fprintf(stderr,"MOL_open fd=%d\n",fd);
		exit(1);
	}
	TESTDEBUG("MOL fd is=%d\n",fd);


	/*----------------------FILE_CLOSE-------------------*/
	rcode = MOL_close(fd);
	if( rcode != 0 ) {
		fprintf(stderr,"FILE_CLOSE rcode=%d\n",rcode);
		exit(1);
	}

	TESTDEBUG("MOL_close resuts=%d\n",rcode);

	exit(0);
 }

