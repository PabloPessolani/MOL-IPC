/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusehmf.c `pkg-config fuse --cflags --libs` -o fusehmf
*/

#define FUSE_USE_VERSION 26

#define  MOL_USERSPACE	1

#define nil 0
#include <asm/ptrace.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <time.h>
#include <utime.h>

#include "../stub_syscall.h"
#include "../kernel/minix/com.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/callnr.h"
#include "../kernel/minix/limits.h"	
#include "../kernel/minix/const.h"
#include "../kernel/minix/types.h"
#include "../kernel/minix/fcntl.h"
#include "../kernel/minix/dir.h"
#include "../kernel/minix/dirent.h"
#include "../servers/debug.h"
#include "../kernel/minix/fuse.h"
#include "../kernel/minix/timers.h"
#include "../kernel/minix/ansi.h"
#include "../kernel/minix/type.h"
#include "../kernel/minix/ioctl.h"
#include "limits.h"  

#define gettid()    (pid_t) syscall (SYS_gettid)
#define v7ent(p)	((struct _v7_direct *) (p))
#define V7_EXTENT	(sizeof(struct _v7_direct) / sizeof(struct _fl_direct) - 1)

//FUSE declarations
struct molfuse_state {
	int dcid;
	int fuse_ep;
	int clt_nr;
	int fuse_pid;
	pid_t fuse_tid;
	char *rootdir;
};

#define MFS_DATA ((struct molfuse_state *) fuse_get_context()->private_data)

//MOLFS declarations

#define MNX_POSIX_PATH_MAX	256
#define USER_ID   0
#define GROUP_ID   0
#define ALL_RWXB	(S_IRWXU | S_IRWXG | S_IRWXO)
#define ALL_SETB	(S_ISUID | S_ISGID)
#define ALL_BITS	(ALL_RWXB | ALL_SETB)

int dcid;
int fuse_ep, svr_ep;
int clt_nr, rcode;
int fuse_pid;
pid_t fuse_tid;
char img_path[MNX_POSIX_PATH_MAX + 1];
char *buffer;
char buffTime[20];
struct tm * timeinfo;

//For blocking  M3_IPC operations
pthread_mutex_t m3_IPC;

/************************/
/*    MOLFS functions   */
/************************/
/* POSIX masks for st_mode. */
// #define S_IRWXU   00700		/* owner:  rwx------ */
// #define S_IRUSR   00400		/* owner:  r-------- */
// #define S_IWUSR   00200		/* owner:  -w------- */
// #define S_IXUSR   00100		/* owner:  --x------ */

// #define S_IRWXG   00070		/* group:  ---rwx--- */
// #define S_IRGRP   00040		/* group:  ---r----- */
// #define S_IWGRP   00020		/* group:  ----w---- */
// #define S_IXGRP   00010		/* group:  -----x--- */

// #define S_IRWXO   00007		/* others: ------rwx */
// #define S_IROTH   00004		/* others: ------r-- */
// #define S_IWOTH   00002		/* others: -------w- */
// #define S_IXOTH   00001		/* others: --------x */
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
	printf("Device: \t\t%d\n", bufferStat.st_dev);
	printf("File inode: \t\t%d\n", bufferStat.st_ino);
	printf("uid: \t%d\n", bufferStat.st_uid);
	printf("guid: \t%d\n", bufferStat.st_gid);
	printf("rdev: \t%d\n", bufferStat.st_rdev);
	printf("Number of Links: \t%d\n", bufferStat.st_nlink);
	printf("File Size: \t\t%d bytes\n", bufferStat.st_size);

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
	printf("A time \t%s\n", buffTime);
	timeinfo = gmtime (&(bufferStat.st_mtime));
	// timeinfo = localtime (&(bufferStat.st_mtime));
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo);
	printf("M time \t%s\n", buffTime);
	timeinfo = gmtime (&(bufferStat.st_ctime));
	// timeinfo = localtime (&(bufferStat.st_ctime));
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo);
	printf("C time \t%s\n", buffTime);

	printf("The file %s a symbolic link\n", (S_ISLNK(bufferStat.st_mode)) ? "IS" : "IS NOT");
	printf("\n");
}


_mnx_Mode_t parseMode(const char* mode)
{

	_mnx_Mode_t nuevoModo;
	// INFODEBUG("MODE OWN=%c\n", mode[0]);
	// INFODEBUG("MODE GRP=%c\n", mode[1]);
	// INFODEBUG("MODE OTH=%c\n", mode[2]);

	switch (mode[0]) {
	case '0':
		nuevoModo = 00000;
		break;
	case '1':
		nuevoModo = S_IXUSR;	
		break;
	case '2':
		nuevoModo = S_IWUSR;	
		break;
	case '3':
		nuevoModo = S_IWUSR | S_IXUSR;	
		break;
	case '4':
		nuevoModo = S_IRUSR;	
		break;
	case '5':
		nuevoModo = S_IRUSR | S_IXUSR;
		break;
	case '6':
		nuevoModo = S_IRUSR | S_IWUSR;
		break;
	case '7':
		nuevoModo = S_IRWXU;
		break;
	}

	switch (mode[1]) {
	case '0':
		nuevoModo |= 00000;
		break;
	case '1':
		nuevoModo |= S_IXGRP;	
		break;
	case '2':
		nuevoModo |= S_IWGRP;	
		break;
	case '3':
		nuevoModo |= S_IWGRP | S_IXGRP;	
		break;
	case '4':
		nuevoModo |= S_IRGRP;	
		break;
	case '5':
		nuevoModo |= S_IRGRP | S_IXGRP;
		break;
	case '6':
		nuevoModo |= S_IRGRP | S_IWGRP;
		break;
	case '7':
		nuevoModo |= S_IRWXG;
		break;
	}

	switch (mode[2]) {
	case '0':
		nuevoModo |= 00000;
		break;
	case '1':
		nuevoModo |= S_IXOTH;	
		break;
	case '2':
		nuevoModo |= S_IWOTH;	
		break;
	case '3':
		nuevoModo |= S_IWOTH | S_IXOTH;	
		break;
	case '4':
		nuevoModo |= S_IROTH;	
		break;
	case '5':
		nuevoModo |= S_IROTH | S_IXOTH;
		break;
	case '6':
		nuevoModo |= S_IROTH | S_IWOTH;
		break;
	case '7':
		nuevoModo |= S_IRWXO;
		break;
	}	

	return nuevoModo;
}

void MOL_loadname(const char *name, message *msgptr)
{
/* This function is used to load a string into a type m3 message. If the
 * string fits in the message, it is copied there.  If not, a pointer to
 * it is passed.
 */
  register mnx_size_t k;

  k = strlen(name) + 1;
  msgptr->m3_i1 = k;
  msgptr->m3_p1 = (char *) name;
  if (k <= sizeof msgptr->m3_ca1) strcpy(msgptr->m3_ca1, name);
}

int MOL_chown(const char *name, _mnx_Uid_t owner, _mnx_Gid_t grp)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
	m.m_type = MOLCHOWN;

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = owner;
	m.m1_i3 = grp;
	m.m1_p1 = (char *) name;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr, "MOL_chown rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
	return ( m.m_type);
}

int MOL_chmod(const char *name, _mnx_Mode_t mode)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
	m.m_type = MOLCHMOD;

	m.m3_i2 = mode;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr, "MOL_chmod rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
	return ( m.m_type);
}

int MOL_chroot(char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLCHROOT;

	MOL_loadname(name, &m);
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
  	return(m_ptr->m_type);  
}

int MOL_creat(const char *name, mnx_mode_t mode)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr =&m;
	m.m_type = MOLCREAT;

	m.m3_i2 = mode;
	MOL_loadname(name, &m);
	
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	  	
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);
	return(rcode);
}


int MOL_open(char path[], int flags) 
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr =&m;
	m.m_type = MOLOPEN;
	TESTDEBUG("%s %s %X\n" , __func__, path, flags);

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
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	  	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);
	return( m.m_type);
}

int MOL_close(int fd)
{
  	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("MOL fd is=%d\n", fd);		
	m_ptr = &m;
  	m.m_type = MOLCLOSE;
  	m.m1_i1 = fd;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	  	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);	
  	return(rcode);
}

ssize_t MOL_read(int fd, void *buffer, mnx_size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;
	
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("MOL fd is=%d\n", fd);		
	m_ptr = &m;
  	m.m_type = MOLREAD;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	  	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);	
  	return(m_ptr->m_type);
}

ssize_t MOL_write(int fd, void *buffer, mnx_size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;
	
	TESTDEBUG("INICIO %s\n", __func__);
	INFODEBUG("MOL fd is=%d\n", fd);		
	m_ptr = &m;
  	m.m_type = MOLWRITE;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	  	
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);			
  	return(m_ptr->m_type);
}  

int MOL_sync()
{
  	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLSYNC;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	  	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);		
  	return(rcode);
}


int MOL_fsync(int fd)
{
	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("MOL fd is=%d\n", fd);	
	m_ptr = &m;
	m.m_type = MOLSYNC;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);	
	return(rcode);
}

int MOL_utime(const char *name, struct utimbuf *timp)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
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
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);	
	return( m.m_type);
}

mnx_mode_t MOL_umask(mnx_mode_t complmode)
{
	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
	m.m_type = MOLUMASK;
	m.m1_i1 = complmode;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		return( (mnx_mode_t) -1);
	}  	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);	
  	return((mnx_mode_t) rcode);   
}

int MOL_stat(const char *name, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
	m.m_type = MOLSTAT;
	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);	
	return (rcode);
}

int MOL_lstat(const char *name, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO MOL_lstat\n");
	m_ptr =&m;
	m.m_type = MOLLSTAT;
	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);	
	return(rcode);
}


int MOL_fstat(int fd, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	INFODEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLFSTAT;
	m.m1_i1 = fd;
	m.m1_p1 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
	return (rcode);
}

int MOL_fcntl(int fd, int cmd, ...)
{
	va_list argp;
	message m;

	INFODEBUG("INICIO %s\n", __func__);
	va_start(argp, cmd);

	/* Set up for the sensible case where there is no variable parameter.  This
	* covers F_GETFD, F_GETFL and invalid commands.
	*/
	m.m1_i3 = 0;
	m.m1_p1 = NIL_PTR;

	/* Adjust for the stupid cases. */
	switch (cmd) {
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

	m_ptr = &m;
	m.m_type = MOLFCNTL;

	m.m1_i1 = fd;
	m.m1_i2 = cmd;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);			
	return ( m.m_type);
}

DIR *MOL_opendir(const char *name)
/* Open a directory for reading. */
{
	int d, f;
	DIR *dp;
	struct stat st;
	INFODEBUG("INICIO %s\n", __func__);
	/* Only read directories. */
	if (MOL_stat((char*) name, &st) < 0) return nil;
	if (!S_ISDIR(st.st_mode)) { errno= ENOTDIR; return nil; }

	if ((d= MOL_open((char*) name, O_RDONLY | O_NONBLOCK)) < 0) return nil;

	TESTDEBUG("MOL d is=%d\n",d);
	/* Check the type again, mark close-on-exec, get a buffer. */
	if (MOL_fstat(d, &st) < 0
		|| (errno= ENOTDIR, !S_ISDIR(st.st_mode))
		|| (f= MOL_fcntl(d, F_GETFD)) < 0
		|| MOL_fcntl(d, F_SETFD, f | FD_CLOEXEC) < 0
		|| (dp= (DIR *) malloc(sizeof(*dp))) == nil) 
		{
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

	INFODEBUG("FIN %s\n", __func__);	
	return dp;
}

struct dirent *MOL_readdir(DIR *dp)
/* Return the next entry in a directory.  Handle V7 and FLEX format dirs. */
{
	struct dirent *e;

	TESTDEBUG("INICIO %s\n", __func__);
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
	INFODEBUG("FIN %s\n", __func__);		
	return e;
}


int MOL_closedir(DIR *dp)
/* Finish reading a directory. */
{
	int d;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	if (dp == nil) { errno= EBADF; return -1; }

	d= dp->_fd;
	free((void *) dp);
	rcode = MOL_close(d);
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);			
	return (rcode);
}

int MOL_mkdir(const char *name, mnx_mode_t mode)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLMKDIR;
	m.m1_i1 = strlen(name) + 1;
  	m.m1_i2 = mode;
	m.m1_p1 = (char *) name;  

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);		
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_rmdir(const char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLRMDIR;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);			
  	return(m_ptr->m_type);  
}

int MOL_chdir(const char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLCHDIR;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}


int MOL_fchdir(int fd)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLCHDIR;
  	m.m1_i1 = fd;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_rename(const char *name, const char *name2)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLRENAME;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);			
  	return(m_ptr->m_type);  
}

int MOL_link(const char *name, const char *name2)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLLINK;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}


int MOL_symlink(const char *name, const char *name2)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLSYMLINK;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_readlink(const char *name, char *buffer, mnx_size_t bufsiz)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLRDLNK;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = bufsiz;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_unlink(const char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLUNLINK;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

mnx_off_t MOL_lseek(int fd, mnx_off_t offset, int whence)
{

  	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLLSEEK;
	m.m2_i1 = fd;
	m.m2_l1 = offset;
	m.m2_i2 = whence;

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if (rcode < 0) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		return( (mnx_off_t) -1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);				
  	return((mnx_off_t) m.m2_l1);  
}

int MOL_truncate(char *path, mnx_off_t length)
{  

  	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLTRUNCATE;
	m.m2_p1 = (char *) path;
	m.m2_i1 = strlen(path)+1;
	m.m2_l1 = length;	

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);	
	INFODEBUG("FIN %s\n", __func__);			
  	return(m_ptr->m_type); 
}

int MOL_ftruncate(int fd, mnx_off_t length)
{

  	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLFTRUNCATE;
	m.m2_i1 = fd;
	m.m2_l1 = length;	

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);		
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type); 
}

int MOL_access(const char *name, int mode)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLACCESS;
	m.m3_i2 = mode;	
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_mount(char *special, char *name, int flag)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLMOUNT;
	m.m1_i1 = strlen(special) + 1;
	m.m1_i2 = strlen(name) + 1;
	m.m1_i3 = flag;
	m.m1_p1 = special;
	m.m1_p2 = name;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_umount(const char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	INFODEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLUMOUNT;
  	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
  	return(m_ptr->m_type);  
}

int MOL_dup(int fd)
{
	int rcode;	
	INFODEBUG("INICIO %s\n", __func__);	

	rcode = MOL_fcntl(fd, F_DUPFD, 0);
	if ( rcode < 0 ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}		
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
	return (rcode);
}


int MOL_dup2(int fd, int fd2)
{
	int rcode;
	INFODEBUG("INICIO %s\n", __func__);		
/* The behavior of dup2 is defined by POSIX in 6.2.1.2 as almost, but not
 * quite the same as fcntl.
 */
  if (fd2 < 0 || fd2 > MNX_OPEN_MAX) {
	errno = EBADF;
	return(-1);
  }

  /* Check to see if fildes is valid. */
  if (MOL_fcntl(fd, F_GETFL) < 0) {
	/* 'fd' is not valid. */
	return(-1);
  } else {
	/* 'fd' is valid. */
	if (fd == fd2) return(fd2);
	MOL_close(fd2);
	rcode = MOL_fcntl(fd, F_DUPFD, fd2);
	if ( rcode < 0 ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}		
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);
	INFODEBUG("FIN %s\n", __func__);		
	return(rcode);
  }
}

int MOL_mknod(const char *name, mnx_mode_t mode, mnx_dev_t dev)
{
  	message m , *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLMKNOD;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = mode;
	m.m1_i3 = dev;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) ((int) 0);		/* obsolete size field */

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"%s rcode=%d\n", __func__, rcode);
		exit(1);
	}	
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("%s rcode=%d\n", __func__, rcode);		
	INFODEBUG("FIN %s\n", __func__);		
  	return(rcode);   
}

int MOL_Bind()
{
	fuse_pid = getpid();
	fuse_tid = gettid();
	INFODEBUG("INICIO %s\n", __func__);	

	dcid = 0;
	if ( dcid < 0 || dcid >= NR_VMS) {
		INFODEBUG("Invalid dcid [0-%d]\n", NR_VMS - 1 );
		exit(1);
	}

	clt_nr = 2;
	/*Si no esta hecho, hago el binding del proceso cliente, o sea este*/
	// sleep(5);
	// INFODEBUG("ANTES fuse_ep=%d\n", fuse_ep);
	// INFODEBUG("ANTES fuse_pid=%d\n", fuse_pid);
	// INFODEBUG("ANTES fuse_tid =%d\n", fuse_tid);
	if( fuse_ep = mnx_getep(fuse_pid) < 0)
		fuse_ep = mnx_bind(dcid, clt_nr);
		// fuse_ep = mnx_lclbind(dcid,fuse_pid,clt_nr);
	// INFODEBUG("fuse_ep=%d\n", fuse_ep);
	// INFODEBUG("fuse_pid=%d\n", fuse_pid);
	// INFODEBUG("fuse_tid =%d\n", fuse_tid);

	if ( fuse_ep < 0 ) {
		INFODEBUG("BIND ERROR fuse_ep=%d\n", fuse_ep);
		exit(1);
	}

	INFODEBUG("BIND CLIENT dcid=%d fuse_pid=%d fuse_tid=%d clt_nr=%d fuse_ep=%d\n",
		dcid,
		fuse_pid,
		fuse_tid,
		clt_nr,
		fuse_ep);
	INFODEBUG("FIN %s\n", __func__);
}

int MOL_UnBind()
{
	dcid = 0;

	INFODEBUG("INICIO %s\n", __func__);	
	if ( dcid < 0 || dcid >= NR_VMS) {
		INFODEBUG("Invalid dcid [0-%d]\n", NR_VMS - 1 );
		exit(1);
	}

	clt_nr = 2;
	mnx_unbind(dcid,clt_nr);

	INFODEBUG("UNBIND CLIENT dcid=%d fuse_pid=%d fuse_tid=%d clt_nr=%d fuse_ep=%d\n",
		dcid,
		getpid(),
		fuse_tid,
		clt_nr,
		fuse_ep);

	INFODEBUG("FIN %s\n", __func__);	
}

/************************************************************************************************************************/
/*************************************************FUSE Wrapper functions*************************************************/
/************************************************************************************************************************/

// Report errors to logfile and give -errno to caller
static int mfs_error(char *str)
{
    int ret = -errno;
    
    INFODEBUG("    ERROR %s: %s\n", str, strerror(errno));
    
    return ret;
}

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void mfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, MFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here
}

void mfs_usage()
{
    fprintf(stderr, "usage:  test_hello_molfuse [FUSE and mount options] rootDir mountPoint\n");
    abort();
}


int mfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
    int fd;
    
    INFODEBUG("INICIO %s\n", __func__);
    TESTDEBUG("%s(path=\"%s\", mode=0%03o, fi=0x%08x)\n", __func__, path, mode, fi);   	
   	// mfs_fullpath(fpath, path);

   	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();    
    fd = MOL_creat(path, mode);
    if (fd < 0)
		retstat = mfs_error("mfs_create -> MOL_creat");
    
    fi->fh = fd;
    
	pthread_mutex_unlock(&m3_IPC);

	INFODEBUG("FIN %s\n", __func__);   
    return 0;
}

static int mfs_open(const char *path, struct fuse_file_info *fi)
{
    int fd;
    // char fpath[PATH_MAX];	

	INFODEBUG("INICIO %s\n", __func__);	
	TESTDEBUG("%s(path=\"%s\", fi=0x%08x)\n", __func__, path, (int) fi);
   	// mfs_fullpath(fpath, path);	

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_open((char*) path, fi->flags);
    if (fd < 0)
		fd = mfs_error("mfs_open -> MOL_open");
    
    fi->fh = fd;

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return 0;
}

static int mfs_getattr(const char *path, struct stat *stbuf)
{
	int res;
    // char fpath[PATH_MAX];

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", stbuf=0x%08x)\n", __func__, path, (int) stbuf);
   	// mfs_fullpath(fpath, path);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	res = MOL_lstat((char*) path, stbuf);
	TESTDEBUG("st_size=%d", stbuf->st_size);
    if (res != 0)
		res = mfs_error("mfs_getattr -> MOL_lstat");	

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return res;
}

int mfs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    // char fpath[PATH_MAX];

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", stbuf=0x%08x)\n", __func__, path, (int) stbuf);
   	// mfs_fullpath(fpath, path);

	pthread_mutex_lock(&m3_IPC);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (!strcmp(path, "/"))
		return mfs_getattr(path, stbuf);
    MOL_Bind();
    retstat = MOL_fstat(fi->fh, stbuf);
    if (retstat < 0)
		retstat = mfs_error("mfs_fgetattr -> MOL_fstat");
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
    return 0;
}


static int mfs_access(const char *path, int mask)
{
	int res;
    // char fpath[PATH_MAX];	

	INFODEBUG("INICIO %s\n", __func__);	
	TESTDEBUG("%s(path=\"%s\", mask=0x%08x)\n", __func__, path, (int) mask);
   	// mfs_fullpath(fpath, path);
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();	
	res = MOL_access(path, mask);  
    if (res < 0)
		res = mfs_error("mfs_access -> MOL_access");	

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return res;
}


static int mfs_opendir(const char *path, struct fuse_file_info *fi)
{
	DIR *dp;
	int retstat = 0;
    // char fpath[PATH_MAX];	

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", fi=0x%08x)\n", __func__, path, (int) fi);
	// mfs_fullpath(fpath, path);
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	dp = MOL_opendir((char*) path);
    if (dp == NULL)
		retstat = mfs_error("mfs_opendir -> MOL_opendir");
    
    fi->fh = (intptr_t) dp;       
	
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
    return 0;	
}

static int mfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	int retstat = 0;
	// char fpath[PATH_MAX];
	struct stat st;	

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
            __func__, path, (int) buf, (int) filler,  offset, (int) fi);
	// mfs_fullpath(fpath, path);
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	dp = MOL_opendir((char*) path);
	if (dp == NULL)
		retstat = mfs_error("mfs_opendir -> MOL_opendir");

	while ((de = MOL_readdir(dp)) != NULL) {
		// TESTDEBUG("de->d_name=\"%s\"\n", de->d_name);
		// struct stat st;
		// memset(&st, 0, sizeof(st));
		// st.st_ino = de->d_ino;
		// st.st_mode = de->d_extent;
		if (MOL_stat((char*) path, &st) < 0) return nil;
		// if (!S_ISDIR(st.st_mode)) { errno= ENOTDIR; return nil; }
		if (filler(buf, de->d_name, &st, 0) != 0) {
		    	TESTDEBUG("Error");
		    return -ENOMEM;
		}
	}

	MOL_closedir(dp);
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return 0;
}

int mfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
    
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", mode=0x%lld)\n", __func__, path, (int) mode);	
	// mfs_fullpath(fpath, path);        	
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
    retstat = MOL_mkdir(path, mode);
    if (retstat < 0)
		retstat = mfs_error("mfs_mkdir -> MOL_mkdir");
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);    
    return 0;
}


int mfs_rmdir(const char *path)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
    
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\")\n", __func__, path);	
	// mfs_fullpath(fpath, path);        	
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();    
    retstat = rmdir(path);
    if (retstat < 0)
		retstat = mfs_error("mfs_rmdir -> MOL_rmdir");
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);      
    return 0;
}

int mfs_link(const char *path, const char *newpath)
{
    int retstat = 0;
    // char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("\n%s(path=\"%s\", newpath=\"%s\")\n",  __func__, path, newpath);	
    // mfs_fullpath(fpath, path);
    // mfs_fullpath(fnewpath, newpath);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();        
    retstat = MOL_link(path, newpath);
    if (retstat < 0)
		retstat = mfs_error("mfs_link -> MOL_link");
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);          
    return 0;
}

int mfs_rename(const char *path, const char *newpath)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
    // char fnewpath[PATH_MAX];
    
    TESTDEBUG("\n%s(fpath=\"%s\", newpath=\"%s\")\n", __func__, path, newpath);
    // mfs_fullpath(fpath, path);
    // mfs_fullpath(fnewpath, newpath);
    
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();      
    retstat = MOL_rename(path, newpath);
    if (retstat < 0)
		retstat = mfs_error("mfs_rename -> MOL_rename");
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);     
    return 0;
}

int mfs_unlink(const char *path)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
	
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\")\n", __func__, path);	
	// mfs_fullpath(fpath, path);        	
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();   
    retstat = MOL_unlink(path);
    if (retstat < 0)
		retstat = mfs_error("mfs_unlink -> MOL_unlink");
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);     
    return 0;
}

int mfs_readlink(const char *path, char *link, size_t size)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
    
	INFODEBUG("INICIO %s\n", __func__);    
    TESTDEBUG("%s(path=\"%s\", link=\"%s\", size=%d)\n",  __func__, path, link, size);
    // mfs_fullpath(fpath, path);
    
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();      
    retstat = MOL_readlink(path, link, size - 1);
    if (retstat < 0)
		retstat = mfs_error("mfs_readlink -> MOL_readlink");
    else  
    {
		link[retstat] = '\0';
		retstat = 0;
    }
        
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);     
    return 0;
}

int mfs_symlink(const char *path, const char *link)
{
    int retstat = 0;
    // char flink[PATH_MAX];
    
    TESTDEBUG("\n%s(path=\"%s\", link=\"%s\")\n",  __func__, path, link);
    // mfs_fullpath(flink, link);
    
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();          
    retstat = MOL_symlink(path, link);
    if (retstat < 0)
		retstat = mfs_error("mfs_symlink -> MOL_symlink");
        
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);     
    return 0;
}

static int mfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	// char fpath[PATH_MAX];		

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", buf=0x%08x, size=0x%08x, offset=%lld, fi=0x%08x)\n",
        __func__, path, (int) buf, (int) size,  offset, (int) fi);
	// mfs_fullpath(fpath, path);        	
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_open((char*) path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = MOL_lseek(fd, offset, SEEK_SET);
	if (res == -1)
		res = -errno;

	res = MOL_read(fd, buf, size);
	if (res == -1)
		res = -errno;

	MOL_close(fd);
	pthread_mutex_unlock(&m3_IPC);	
	INFODEBUG("FIN %s\n", __func__);
	return res;
}

static int mfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	// char fpath[PATH_MAX];

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", buf=0x%08x, size=0x%08x, offset=%lld, fi=0x%08x)\n",
        __func__, path, (int) buf, (int) size,  offset, (int) fi);	
	// mfs_fullpath(fpath, path);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_open((char*) path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = MOL_lseek(fd, offset, SEEK_SET);
	if (res == -1)
		res = -errno;	

	res = MOL_write(fd, (char*) buf, size);
	if (res == -1)
		res = -errno;

	MOL_close(fd);
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return res;
}

static int mfs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    // char fpath[PATH_MAX];    
    
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", ubuf=0x%08x)\n",  __func__, path, (int) ubuf);
	// mfs_fullpath(fpath, path);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_utime((char*) path, ubuf);
    if (retstat < 0)
		retstat = mfs_error("mfs_utime -> MOL_utime");
	
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return 0;
}

static int mfs_chown(const char *path, uid_t uid, gid_t gid)
{
    int retstat = 0;
    // char fpath[PATH_MAX];    
    
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", uid=0x%08x, gid=0x%08x)\n", __func__, path, (int) uid, (int) gid);	
	// mfs_fullpath(fpath, path);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_chown((char*) path, uid, gid);
    if (retstat < 0)
		retstat = mfs_error("mfs_chown -> MOL_chown");

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return 0;
}

static int mfs_chmod(const char *path, mode_t mode)
{
    int retstat = 0;
    // char fpath[PATH_MAX];    
    
	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", mode=0x%08x)\n", __func__, path, (int) mode);	
	// mfs_fullpath(fpath, path);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_chmod((char*) path, mode);
    if (retstat < 0)
		retstat = mfs_error("mfs_chmod -> MOL_chmod");
	
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);
	return 0;
}

static int mfs_truncate(const char *path, off_t length)
{
    int retstat = 0;
    // char fpath[PATH_MAX];    

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", length=0x%08x)\n", __func__, path, (int) length);
	// mfs_fullpath(fpath, path);	
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_truncate((char*) path, length);
	if (retstat < 0)
		return -errno;
	
	pthread_mutex_unlock(&m3_IPC);

	INFODEBUG("FIN %s\n", __func__);
	return 0;
}

static int mfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    // char fpath[PATH_MAX];    

	INFODEBUG("INICIO %s\n", __func__);
	TESTDEBUG("%s(path=\"%s\", offset=0x%08x)\n", __func__, path, (int) offset);
	// mfs_fullpath(fpath, path);	    
    
    pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
    retstat = MOL_ftruncate(fi->fh, offset);
    if (retstat < 0)
		retstat = mfs_error("mfs_ftruncate -> MOL_ftruncate");

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);    
    return 0;
}

int mfs_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = 0;
    // char fpath[PATH_MAX];
    
    TESTDEBUG("\n%s(path=\"%s\", mode=0%3o, dev=%lld)\n", __func__, path, mode, dev);
    // mfs_fullpath(fpath, path);
    
    pthread_mutex_lock(&m3_IPC);

	MOL_Bind();    
    // On Linux this could just be 'mknod(path, mode, rdev)' but this
    //  is more portable
    if (S_ISREG(mode)) {
        retstat = MOL_open((char*) path, O_CREAT | O_EXCL | O_WRONLY | O_NONBLOCK | 0666);
	if (retstat < 0)
	    retstat = mfs_error("mfs_mknod -> MOL_open");
        else {
            retstat = MOL_close(retstat);
	    if (retstat < 0)
		retstat = mfs_error("mfs_mknod -> MOL_close");
	}
    } else
	if (S_ISFIFO(mode)) {
	    retstat = MOL_mknod((char*) path, mode | S_IFIFO, (dev_t) 0);
	    if (retstat < 0)
		retstat = mfs_error("mfs_mknod -> mkfifo");
	} else {
	    retstat = MOL_mknod((char*) path, mode, dev);
	    if (retstat < 0)
		retstat = mfs_error("mfs_mknod -> MOL_mknod");
	}
    
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN %s\n", __func__);      
    return 0;
}


static int mfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    (void) fi;
   	
   	//Ver si con esto alcanza. Ver notas en bbfs.c para esta funcion. Flush no equivale a sync al parecer
	
    return retstat;
}

static int mfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    (void) fi;

   	//Ver si con esto alcanza. Ver tambien notas en bbfs.c para esta funcion. Minix no tiene release
   	//
    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    //retstat = close(fi->fh);

    return retstat;
}

static int mfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
	(void) fi;
    
    // log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
	   //  path, fi);
    // log_fi(fi);
    
    // closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

void *mfs_init(struct fuse_conn_info *conn)
{
    // log_msg("\nbb_init()\n");

    // log_conn(conn);
    // log_fuse_context(fuse_get_context());
    // MOL_Bind();
    fuse_get_context();
    return MFS_DATA;
}

static struct fuse_operations mfs_oper = {
    .create     = mfs_create,	
	.open		= mfs_open,
	.read		= mfs_read,
	.write		= mfs_write,
	.access		= mfs_access,
	.getattr	= mfs_getattr,
	// .fgetattr	= mfs_fgetattr,	
  	.opendir 	= mfs_opendir,
	.readdir	= mfs_readdir,
	.mkdir      = mfs_mkdir,
	.rmdir      = mfs_rmdir,
	.link       = mfs_link,	
	.readlink   = mfs_readlink,
	.unlink     = mfs_unlink,
	.symlink    = mfs_symlink,
	.rename     = mfs_rename,
	.mknod      = mfs_mknod,
  	.utime 		= mfs_utime,
	.chmod 		= mfs_chmod,
	.chown 		= mfs_chown,
	.truncate 	= mfs_truncate,
    .ftruncate  = mfs_ftruncate,
	.flush 		= mfs_flush, 
	.release 	= mfs_release, 
	.releasedir	= mfs_releasedir,
	.init 		= mfs_init
};

// struct fuse_operations bb_oper = {
//   .readlink = bb_readlink,
//   .fsync = bb_fsync, //Este es el sync comun

//   /** Just a placeholder, don't set */ // huh???
//   .statfs = bb_statfs, //PH
//   .flush = bb_flush, //PH 
//   .release = bb_release, //PH + Close???
//   .releasedir = bb_releasedir, // PH + Closedir
//   .fsyncdir = bb_fsyncdir, // PH
//   .destroy = bb_destroy, //PH
// };


int main(int argc, char *argv[])
{
	int fuse_stat;
	struct molfuse_state *mfs_data;

	//umask(0);
	// return fuse_main(argc, argv, &mfs_oper, NULL);
	
	TESTDEBUG("argc=%d\n",argc);
	TESTDEBUG("argv[0]=%s\n",argv[0]);
	TESTDEBUG("argv[1]=%s\n",argv[1]);
	TESTDEBUG("argv[2]=%s\n",argv[2]);
	// TESTDEBUG("argv[3]=%s\n",argv[3]);
	// TESTDEBUG("argv[argc-2][0]=%c\n",argv[argc-2][0]);
	// TESTDEBUG("argv[argc-1][0]=%c\n",argv[argc-1][0]);
	// TESTDEBUG("argv[argc-2]=%s\n",argv[argc-2]);
	// TESTDEBUG("argv[argc-1]=%s\n",argv[argc-1]);	

	// if ((argc < 2) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	// mfs_usage();

	// mfs_data = malloc(sizeof(struct molfuse_state));
 //    if (mfs_data == NULL) {
	// 	perror("main malloc");
	// 	abort();
 //    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    // mfs_data->rootdir = realpath(argv[argc-2], NULL);
    // argv[argc-2] = argv[argc-1];
    // argv[argc-1] = NULL;
    // argc--;

    fprintf(stderr, "Before calling fuse_main\n");
    // fuse_stat = fuse_main(argc, argv, &mfs_oper, mfs_data);
    fuse_stat = fuse_main(argc, argv, &mfs_oper, mfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

    return fuse_stat;
}
