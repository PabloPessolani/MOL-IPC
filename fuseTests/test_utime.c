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
#include <stdarg.h>
#include <time.h>	
#include <utime.h>

#include "../stub_syscall.h"
#include "../kernel/minix/const.h"
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


#define v7ent(p)	((struct _v7_direct *) (p))
#define V7_EXTENT	(sizeof(struct _v7_direct) / sizeof(struct _fl_direct) - 1)

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
char buffTime[20]; 
struct tm * timeinfo;

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

int MOL_utime(const char *name, struct utimbuf *timp)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_utime\n");
	m_ptr =&m;
	m.m_type = MOLUTIME;

	if (timp == NULL) {
		m.m2_i1 = 0;		/* name size 0 means NULL `timp' */
		m.m2_i2 = strlen(name) + 1;	 /* actual size here */
	} else {
		m.m2_l1 = timp->actime;
		m.m2_l2 = timp->modtime;
		m.m2_i1 = strlen(name) + 1;
	}
	m.m2_p1 = (char *) name;

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));

  	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_utime rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	return( m.m_type);
}

ssize_t MOL_read(int fd, void *buffer, size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_read\n");	
	m_ptr = &m;
  	m.m_type = MOLREAD;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("MOL_read rcode=%d\n", rcode);
  	return(m_ptr->m_type);
}

ssize_t MOL_write(int fd, void *buffer, size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;
	
	TESTDEBUG("INICIO MOL_write\n");	
	m_ptr = &m;
  	m.m_type = MOLWRITE;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("MOL_write rcode=%d\n", rcode);
  	return(m_ptr->m_type);
}  

int MOL_close(int fd)
{
  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_close\n");
	m_ptr = &m;
  	m.m_type = MOLCLOSE;
  	m.m1_i1 = fd;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

  	return(rcode);
}

int MOL_stat(char *name, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_stat\n");
	m_ptr =&m;
	m.m_type = MOLSTAT;

	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_stat rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	return(rcode);
}


int MOL_fstat(int fd, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_fstat\n");
	INFODEBUG("MOL fd is=%d\n",fd);
	m_ptr =&m;
	m.m_type = MOLFSTAT;

	m.m1_i1 = fd;
	m.m1_p1 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_fstat rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	return(rcode);
}


int MOL_fcntl(int fd, int cmd, ...)
{
	va_list argp;
	message m;

	TESTDEBUG("INICIO MOL_fcntl\n");
  	va_start(argp, cmd);

	/* Set up for the sensible case where there is no variable parameter.  This
	* covers F_GETFD, F_GETFL and invalid commands.
	*/
	m.m1_i3 = 0;
	m.m1_p1 = NIL_PTR;

	/* Adjust for the stupid cases. */
	switch(cmd) {
		case F_DUPFD:
		case F_SETFD:
		case F_SETFL:
			m.m1_i3 = va_arg(argp, int);
			break;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
		case F_FREESP:
			m.m1_p1 = (char *) va_arg(argp, struct flock *);
	break;
	}

  	/* Clean up and make the system call. */
  	va_end(argp);

	message *m_ptr;
	int rcode;

	m_ptr =&m;
	m.m_type = MOLFCNTL;

	m.m1_i1 = fd;
	m.m1_i2 = cmd;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_fcntl rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	return( m.m_type);  
}


DIR *MOL_opendir(const char *name)
/* Open a directory for reading. */
{
	int d, f;
	DIR *dp;
	struct stat st;
	TESTDEBUG("INICIO MOL_opendir\n");
	/* Only read directories. */
	if (MOL_stat((char*) name, &st) < 0) return nil;
	if (!S_ISDIR(st.st_mode)) { errno= ENOTDIR; return nil; }

	if ((d= MOL_open((char*) name, O_RDONLY | O_NONBLOCK)) < 0) return nil;

INFODEBUG("MOL d is=%d\n",d);
	/* Check the type again, mark close-on-exec, get a buffer. */
	if (MOL_fstat(d, &st) < 0
		|| (errno= ENOTDIR, !S_ISDIR(st.st_mode))
		|| (f= MOL_fcntl(d, F_GETFD)) < 0
		|| MOL_fcntl(d, F_SETFD, f | FD_CLOEXEC) < 0
		|| (dp= (DIR *) malloc(sizeof(*dp))) == nil
	) {
		TESTDEBUG("errno=%d\n",errno);
		TESTDEBUG("S_ISDIR(st.st_mode)=%d\n",S_ISDIR(st.st_mode));
		TESTDEBUG("f=%d\n",f);
		int err= errno;
		(void) MOL_close(d);
		errno= err;
		return nil;
	}

	dp->_fd= d;
	dp->_v7= -1;
	dp->_count= 0;
	dp->_pos= 0;

	return dp;
}

struct dirent *MOL_readdir(DIR *dp)
/* Return the next entry in a directory.  Handle V7 and FLEX format dirs. */
{
	struct dirent *e;

	TESTDEBUG("INICIO MOL_readdir\n");
	if (dp == nil) { errno= EBADF; return nil; }

	do {
		if (dp->_count <= 0) {
			/* Read the next directory block. */
			dp->_count= MOL_read(dp->_fd, dp->_buf, sizeof(dp->_buf));
			TESTDEBUG("MOL_READ dp->_count=%d\n",dp->_count);
			TESTDEBUG("sizeof(dp->_buf)=%d\n",sizeof(dp->_buf));
			if (dp->_count <= 0) return nil;

			TESTDEBUG("sizeof(dp->_buf[0])=%d\n",sizeof(dp->_buf[0]));
			dp->_count/= sizeof(dp->_buf[0]);

			TESTDEBUG("MOL_READ dp->_count=%d\n",dp->_count);
			dp->_ptr= dp->_buf;
			
			TESTDEBUG("dp->_buf : " FLDIR_FORMAT, FLDIR_FIELDS(dp->_buf));//TODO: Ver esto!!!!!

			/* Extent is zero of the first flex entry. */
			if (dp->_v7 == (char)-1) dp->_v7= dp->_buf[0].d_extent;
		}

		if (!dp->_v7) {
			/* FLEX. */
			e= (struct dirent *) dp->_ptr;
			TESTDEBUG("!dp->_v7=%d\n",dp->_v7);
			// TESTDEBUG("dirent e: " DIRENT_FORMAT, DIRENT_FIELDS(e));
		} else {
			/* V7: transform to FLEX. */
			e= (struct dirent *) dp->_v7f;
			e->d_ino= v7ent(dp->_ptr)->d_ino;
			e->d_extent= V7_EXTENT;
			// TESTDEBUG("V7_EXTENT=%d\n",V7_EXTENT);
			memcpy(e->d_name, v7ent(dp->_ptr)->d_name, DIRSIZ);
			e->d_name[DIRSIZ]= 0;
			TESTDEBUG("e->d_name=%s\n",e->d_name);
			// TESTDEBUG("v7ent(dp->_ptr)->d_name=%s\n",v7ent(dp->_ptr)->d_name);
			TESTDEBUG("dp->_v7=%d\n",dp->_v7);
			// TESTDEBUG("dirent e: " DIRENT_FORMAT, DIRENT_FIELDS(e));
			TESTDEBUG("dp->_pos ANTES =%d\n",dp->_pos);
		}

		dp->_ptr+= 1 + e->d_extent;
		dp->_count-= 1 + e->d_extent;
		dp->_pos+= (1 + e->d_extent) * sizeof(*dp->_ptr);
		TESTDEBUG("dp->_pos DESPUES =%d\n",dp->_pos);

	} while (e->d_ino == 0);
	TESTDEBUG("dirent e: " DIRENT_FORMAT, DIRENT_FIELDS(e));
	return e;
}


int MOL_closedir(DIR *dp)
/* Finish reading a directory. */
{
	int d;

	if (dp == nil) { errno= EBADF; return -1; }

	d= dp->_fd;
	free((void *) dp);
	return MOL_close(d);
}

void printStats (const char* fileName, struct stat bufferStat)
{

// 0 dev      device number of filesystem
// 1 ino      inode number
// 2 mode     file mode  (type and permissions)
// 3 nlink    number of (hard) links to the file
// 4 uid      numeric user ID of file's owner
// 5 gid      numeric group ID of file's owner
// 6 rdev     the device identifier (special files only)
// 7 size     total size of file, in bytes
// 8 atime    last access time in seconds since the epoch
// 9 mtime    last modify time in seconds since the epoch
// 10 ctime    inode change time (NOT creation time!) in seconds
//         since the epoch
// 11 blksize  preferred block size for file system I/O
// 12 blocks   actual number of blocks allocated
// 
 // struct stat {
 //     dev_t     st_dev;     /* device inode resides on */
 //     ino_t     st_ino;     /* this inode's number */
 //     mode_t    st_mode;    /* file mode, protection bits, etc. */
 //     nlink_t   st_nlink;   /* number or hard links to the file */
 //     uid_t     st_uid;     /* user-id of the file's owner */
 //     gid_t     st_gid;     /* group-id of the file's owner */ 
 //     dev_t     st_rdev;    /* the device type, for inode that is device */
 //     off_t     st_size;    /* total size of file */
 //     time_t    st_atime;   /* time of last access */
 //     time_t    st_mtime;   /* time of last data modification */
 //     time_t    st_ctime;   /* time of last file status change */
 // };	
 	printf("\n"); 
	printf("Information for %s\n", fileName);
    printf("---------------------------\n");
    printf("Device: \t\t%d\n",bufferStat.st_dev);
    printf("File inode: \t\t%d\n",bufferStat.st_ino);
    printf("uid: \t%d\n",bufferStat.st_uid);
    printf("guid: \t%d\n",bufferStat.st_gid);
    printf("rdev: \t%d\n",bufferStat.st_rdev);
    printf("Number of Links: \t%d\n",bufferStat.st_nlink);
    printf("File Size: \t\t%d bytes\n",bufferStat.st_size);
 
    printf("File Permissions: \t");
    printf( (S_ISDIR(bufferStat.st_mode)) ? "d" : "-");
    printf( (bufferStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (bufferStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (bufferStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (bufferStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (bufferStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (bufferStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (bufferStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (bufferStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (bufferStat.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n\n");

	timeinfo = gmtime (&(bufferStat.st_atime)); 
	// timeinfo = localtime (&(bufferStat.st_atime)); 
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo); 
	printf("A time \t%s\n",buffTime);
	timeinfo = gmtime (&(bufferStat.st_mtime)); 
	// timeinfo = localtime (&(bufferStat.st_mtime)); 
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo); 
	printf("M time \t%s\n",buffTime);
	timeinfo = gmtime (&(bufferStat.st_ctime)); 
	// timeinfo = localtime (&(bufferStat.st_ctime)); 
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo); 
	printf("C time \t%s\n",buffTime);

    printf("The file %s a symbolic link\n", (S_ISLNK(bufferStat.st_mode)) ? "IS" : "IS NOT");	
	printf("\n");    
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
	//Para STAT
	struct stat bufferStat;
	int retStat;
	//Para FCNTL
	int accmode, retFcntl;
	//Para OPENDIR
	DIR *dp;
	//Para READDIR
	struct dirent * de;
	//Para CLOSEDIR
	int resp_cd;
	// Para UTIME
	struct utimbuf ub;
	int retUtime;

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

	INFODEBUG("CLIENT buf_size=%d\n", buf_size);

	/*hago el binding del proceso cliente, o sea este*/
	clt_pid = getpid();
	clt_ep = mnx_bind(vmid, clt_nr);
	
	if ( clt_ep < 0 ) {
		fprintf(stderr,"BIND ERROR clt_ep=%d\n", clt_ep);
		exit(1);
	}

	INFODEBUG("BIND CLIENT vmid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
   		vmid,
  		clt_pid,
   		clt_nr,
   		clt_ep);
		   
	INFODEBUG("CLIENT pause before SENDREC\n");
	sleep(2); 


	rcode = posix_memalign( (void**) &buffer, getpagesize(), buf_size);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("CLIENT buffer address=%X point=%X\n", buffer, *buffer);
	
// #ifdef ANULADO

	/*----------------------FILE_OPEN-------------------*/
	// fd = MOL_open(argv[4], O_APPEND);
	fd = MOL_open(argv[4], O_RDONLY | O_NONBLOCK);	//Para test de opendir
		
	if( fd < 0 ) {
		fprintf(stderr,"MOL_open fd=%d\n",fd);
		exit(1);
	}
	INFODEBUG("MOL fd is=%d\n",fd);

	/*----------------------FILE STATS-------------------*/
	retStat = MOL_fstat(fd, &bufferStat);
		
	printStats(argv[4], bufferStat);

	if( retStat < 0 ) {
		fprintf(stderr,"MOL_fstat fd=%d\n",retStat);
		exit(1);
	}
	TESTDEBUG("MOL fstats results are=%d\n", retStat);
 
 	/*-----------UPDATE TIMES wiht MOL_utime ---------*/
    ub.actime = bufferStat.st_atime - 120; /*le resto a cada tiempo 2 minutos*/
    ub.modtime = bufferStat.st_mtime - 120;

	retUtime = MOL_utime(argv[4], &ub);
		
	if( retUtime < 0 ) {
		fprintf(stderr,"MOL_utime fd=%d\n",retUtime);
		exit(1);
	}
	TESTDEBUG("MOL_utime results are=%d\n", retUtime); 

	/*----------------------FILE STATS-------------------*/
	retStat = MOL_fstat(fd, &bufferStat);
		
	printStats(argv[4], bufferStat);

	if( retStat < 0 ) {
		fprintf(stderr,"MOL_fstat fd=%d\n",retStat);
		exit(1);
	}
	TESTDEBUG("MOL fstats results are=%d\n", retStat);   

#ifdef ANULADO

	/*----------------------FILE_FCNTL-------------------*/
	retFcntl = MOL_fcntl(fd, F_GETFL, 0);
		
	if( retFcntl < 0 ) {
		fprintf(stderr,"MOL_fcntl fd=%d\n", retFcntl);
		exit(1);
	}    

    accmode = retFcntl & O_ACCMODE;
    if (accmode == O_RDONLY) printf("O_RDONLY(%d): Read Only \n", O_RDONLY);
    else if (accmode == O_WRONLY) printf("O_WRONLY(%d): Write Only \n", O_WRONLY);
    else if (accmode == O_RDWR)   printf("O_RDWR(%d): Read Write \n", O_RDWR);
    else printf("unknown access mode \n");

    if (retFcntl & O_APPEND) printf(", O_APPEND(%d): Append \n", O_APPEND);
    if (retFcntl & O_NONBLOCK) printf(", O_NONBLOCK(%d): NonBlocking \n", O_NONBLOCK); 
    // if (retFcntl & O_SYNC)           printf(", O_SYNC: Synchronous Writes");   

#endif /* ANULADO */

	/*----------------------FILE_CLOSE-------------------*/
	rcode = MOL_close(fd);
	if( rcode != 0 ) {
		fprintf(stderr,"FILE_CLOSE rcode=%d\n",rcode);
		exit(1);
	}

	TESTDEBUG("MOL_close resuts=%d\n",rcode);

// #endif /* ANULADO */

#ifdef ANULADO
	/*----------------------OPEN DIR-------------------*/
	dp = MOL_opendir(argv[4]);
	TESTDEBUG("MOL_opendir reply: " DIR_FORMAT, DIR_FIELDS(dp));
	
	/*----------------------READ DIR-------------------*/	
	de = MOL_readdir(dp);
	TESTDEBUG("MOL_readdir reply: " DIRENT_FORMAT, DIRENT_FIELDS(de));

	resp_cd = MOL_closedir(dp);			
	if( resp_cd < 0 ) {
		fprintf(stderr,"MOL_closedir fd=%d\n",resp_cd);
		exit(1);
	}
#endif /* ANULADO */	

	exit(0);
 }

