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
#include "../kernel/minix/types.h"	
#include "../kernel/minix/fcntl.h"
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

char *rand_string(char *str, size_t size)
{
	size_t n;
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    if (size) {
        --size;
        for (n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
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

ssize_t MOL_write(int fd, void *buffer, size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;
	
	m_ptr = &m;
  	m.m_type = MOLWRITE;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("MOL_write rcode=%d\n", rcode);
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

	
	fd = MOL_open(argv[4], O_RDWR);
		
	if( fd < 0 ) {
		fprintf(stderr,"MOL_open fd=%d\n",fd);
		exit(1);
	}
	TESTDEBUG("fd=%d\n",fd);

	TESTDEBUG("Opening local file =%s\n",argv[3]);
	/* open the local image file to write remote copy */
	fp_lcl = fopen(argv[3], "r+");
	if( fp_lcl == NULL) {
		fprintf(stderr,"fopen ERROR for file=%s errno=%d\n", argv[3], errno);
		exit(1);
	}

	TESTDEBUG("fp_lcl=%d\n",fp_lcl);

	//strcpy(buffer, argv[5]);
	// rand_string((void *) buffer, buf_size);

	// TESTDEBUG("buffer=%s\n", buffer);
	// TESTDEBUG("buffer=%c\n", buffer[buf_size-2]);
	// TESTDEBUG("TAMAÑO buffer=%d\n", sizeof(buffer) / sizeof(*buffer));
	// TESTDEBUG("TAMAÑO buffer=%d\n", strlen(buffer));

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
		bytes = fread( (void *) buffer, 1, buf_size, fp_lcl);
		if( bytes < 0) {
			fprintf(stderr,"fwread ERROR rcode=%d\n",bytes);
			exit(1);
		}		
		TESTDEBUG("bytes read =%d\n",bytes);

		loops++;
		TESTDEBUG("loops =%d\n",loops);
		if( bytes == 0) { /* EOF */ 
			printf("EOF\n");
			break;
		}
		
		bytes = MOL_write(fd, (void *) buffer, buf_size); 
		if( bytes < 0) {
			fprintf(stderr," MOL_write ERROR rcode=%d\n",bytes);
			exit(1);
		}
	// 	position += bytes;

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

