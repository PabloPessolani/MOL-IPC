/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusehmf.c `pkg-config fuse --cflags --libs` -o fusehmf
*/

#define FUSE_USE_VERSION 26

// #include <fcntl.h>
// #include <dirent.h>
// #include <sys/time.h>

#define  MOL_USERSPACE	1

#define nil 0
// #include <fuse.h>
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

#include "../stub_syscall.h"
#include "../kernel/minix/com.h"
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
#include "../kernel/minix/fuse.h"
#include "limits.h"

#define gettid()    (pid_t) syscall (SYS_gettid)
#define v7ent(p)	((struct _v7_direct *) (p))
#define V7_EXTENT	(sizeof(struct _v7_direct) / sizeof(struct _fl_direct) - 1)

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
int dcid;
int fuse_ep, svr_ep;
int clt_nr, rcode;
int fuse_pid;
pid_t fuse_tid;

//Para bloquear operaciones M3_IPC
pthread_mutex_t m3_IPC;

/************************/
/*    MOLFS functions   */
/************************/

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

int MOL_truncate(const char *path, off_t length)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_truncate\n");
	m_ptr =&m;
	m.m_type = MOLTRUNCATE;

	m.m2_p1 = (char *) path;
	m.m2_i1 = strlen(path)+1;
	m.m2_l1 = length;

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_truncate rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_truncate\n");	
	return( m.m_type);
}

int MOL_ftruncate(int fd, off_t length)
{
	message m, *m_ptr;
	int rcode;

 	TESTDEBUG("INICIO MOL_ftruncate\n");
	m_ptr =&m;
	m.m_type = MOLFTRUNCATE;

	m.m2_l1 = length;
	m.m2_i1 = fd;

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_truncate rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_ftruncate\n");		
	return( m.m_type);
}

int MOL_chown(const char *name, _mnx_Uid_t owner, _mnx_Gid_t grp)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_chown\n");
	m_ptr =&m;
	m.m_type = MOLCHOWN;

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = owner;
	m.m1_i3 = grp;
	m.m1_p1 = (char *) name;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_chown rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_chown\n");	
	return( m.m_type);
}


int MOL_chmod(const char *name, _mnx_Mode_t mode)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_chmod\n");
	m_ptr =&m;
	m.m_type = MOLCHMOD;

	m.m3_i2 = mode;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	
	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_chmod rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_chmod\n");	
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

	TESTDEBUG("FIN MOL_utime\n");	
	return( m.m_type);
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


int MOL_access(const char *name, int mode)
{
  	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_access\n");
	m_ptr =&m;
	m.m_type = MOLACCESS;

  	m.m3_i2 = mode;
  	MOL_loadname((char*) name, &m);
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_access rcode=%d\n", rcode);
		exit(1);
	}

	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_access\n");

	return( m.m_type);
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

	TESTDEBUG("FIN MOL_close\n");

  	return(rcode);
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

	TESTDEBUG("FIN MOL_read\n");

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

	TESTDEBUG("FIN MOL_write\n");

  	return(m_ptr->m_type);
}

off_t MOL_lseek(int fd, off_t offset, int whence)
{
  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_lseek\n");

	m_ptr = &m;
  	m.m_type = MOLLSEEK;
  	m.m2_i1 = fd;
  	m.m2_l1 = offset;
  	m.m2_i2 = whence;
	INFODEBUG("Request: " MSG2_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m); 
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG2_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("MOL_lseek rcode=%d\n", rcode);
	
	TESTDEBUG("FIN MOL_lseek\n");

  	return( (off_t) m.m2_l1);
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

	TESTDEBUG("FIN MOL_stat\n");

	return(rcode);
}


int MOL_lstat(char *name, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_lstat\n");
	m_ptr =&m;
	m.m_type = MOLLSTAT;

	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr,"MOL_lstat rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	TESTDEBUG("FIN MOL_lstat\n");

	return(rcode);
}

int MOL_fstat(int fd, struct stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO MOL_fstat\n");
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

	TESTDEBUG("FIN MOL_fstat\n");

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

	TESTDEBUG("FIN MOL_fcntl\n");

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

	TESTDEBUG("FIN MOL_opendir\n");
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
			TESTDEBUG("dp->_count=%d\n",dp->_count);
			if (dp->_count <= 0) return nil;

			TESTDEBUG("sizeof(dp->_buf[0])=%d\n",sizeof(dp->_buf[0]));
			dp->_count/= sizeof(dp->_buf[0]);

			TESTDEBUG("dp->_count=%d\n",dp->_count);
			dp->_ptr= dp->_buf;

			// TESTDEBUG("dp->_buf : " FLDIR_FORMAT, FLDIR_FIELDS(dp->_buf));//TODO: Ver esto!!!!!

			/* Extent is zero of the first flex entry. */
			if (dp->_v7 == (char)-1) dp->_v7= dp->_buf[0].d_extent;
		}

		if (!dp->_v7) {
			/* FLEX. */
			e= (struct dirent *) dp->_ptr;
			// TESTDEBUG("!dp->_v7=%d\n",dp->_v7);
			// TESTDEBUG("dirent e: " DIRENT_FORMAT, DIRENT_FIELDS(e));
		} else {
			/* V7: transform to FLEX. */
			e= (struct dirent *) dp->_v7f;
			e->d_ino= v7ent(dp->_ptr)->d_ino;
			e->d_extent= V7_EXTENT;
			TESTDEBUG("V7_EXTENT=%d\n",V7_EXTENT);
			memcpy(e->d_name, v7ent(dp->_ptr)->d_name, DIRSIZ);
			e->d_name[DIRSIZ]= 0;
			TESTDEBUG("e->d_name=%s\n",e->d_name);
			TESTDEBUG("dp->_v7=%d\n",dp->_v7);
			TESTDEBUG("dirent e: " DIRENT_FORMAT, DIRENT_FIELDS(e));
			TESTDEBUG("dp->_pos ANTES =%d\n",dp->_pos);
		}

		dp->_ptr+= 1 + e->d_extent;
		dp->_count-= 1 + e->d_extent;
		dp->_pos+= (1 + e->d_extent) * sizeof(*dp->_ptr);
		TESTDEBUG("dp->_pos DESPUES =%d\n",dp->_pos);

	} while (e->d_ino == 0);

	TESTDEBUG("FIN MOL_readdir\n");
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

int MOL_Bind()
{
	fuse_pid = getpid();
	fuse_tid = gettid();
	// INFODEBUG("GETTID =%d\n", fuse_tid);

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
	INFODEBUG("fuse_ep=%d\n", fuse_ep);
	INFODEBUG("fuse_pid=%d\n", fuse_pid);
	INFODEBUG("fuse_tid =%d\n", fuse_tid);

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
}

int MOL_UnBind()
{
	dcid = 0;
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
}

/************************/
/*FUSE Wrapper functions*/
/************************/

static int hmf_getattr(const char *path, struct stat *stbuf)
{
	int res;

	TESTDEBUG("INICIO hmf_getattr\n");

	TESTDEBUG("hmf_getattr(path=\"%s\", stbuf=0x%08x)\n", path, (int) stbuf);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	res = MOL_lstat((char*) path, stbuf);
	if (res == -1)
		return -errno;

	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_getattr\n");
	return 0;
}

static int hmf_access(const char *path, int mask)
{
	int res;

	TESTDEBUG("INICIO hmf_access\n");
	
	TESTDEBUG("hmf_access(path=\"%s\", mask=0x%08x)\n", path, (int) mask);
	
	pthread_mutex_lock(&m3_IPC);

	res = MOL_access(path, mask);
	if (res == -1)
		return -errno;

	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_access\n");
	return 0;
}


static int hmf_opendir(const char *path, struct fuse_file_info *fi)
{
	DIR *dp;
	// struct dirent *de;

	(void) fi;
	TESTDEBUG("INICIO hmf_opendir\n");

	TESTDEBUG("hmf_opendir(path=\"%s\", fi=0x%08x)\n", path, (int) fi);
	
	pthread_mutex_lock(&m3_IPC);

	dp = MOL_opendir((char*) path);
	if (dp == NULL)
		return -errno;
	
	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_opendir\n");
	return 0;
}

static int hmf_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	TESTDEBUG("INICIO hmf_readdir\n");

	TESTDEBUG("hmf_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
            path, (int) buf, (int) filler,  offset, (int) fi);
	
	pthread_mutex_lock(&m3_IPC);

	dp = MOL_opendir((char*) path);
	if (dp == NULL)
		return -errno;

	while ((de = MOL_readdir(dp)) != NULL) {
		TESTDEBUG("de->d_name=\"%s\"\n", de->d_name);
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		// st.st_mode = de->d_extent;
		if (MOL_stat((char*) path, &st) < 0) return nil;
		// if (!S_ISDIR(st.st_mode)) { errno= ENOTDIR; return nil; }
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	MOL_closedir(dp);

	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_readdir\n");
	return 0;
}

static int hmf_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	TESTDEBUG("INICIO hmf_open\n");
	

	TESTDEBUG("hmf_open(path=\"%s\", fi=0x%08x)\n",
        path, (int) fi);

	pthread_mutex_lock(&m3_IPC);

	res = MOL_open((char*) path, fi->flags);
	if (res == -1)
		return -errno;

	MOL_close(res);

	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_open\n");
	return 0;
}

static int hmf_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;

	TESTDEBUG("INICIO hmf_read\n");

	TESTDEBUG("hmf_read(path=\"%s\", buf=0x%08x, size=0x%08x, offset=%lld, fi=0x%08x)\n",
        path, (int) buf, (int) size,  offset, (int) fi);	

	pthread_mutex_lock(&m3_IPC);
	
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
	
	TESTDEBUG("FIN hmf_read\n");
	return res;
}

static int hmf_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;

	TESTDEBUG("INICIO hmf_write\n");

	TESTDEBUG("hmf_write(path=\"%s\", buf=0x%08x, size=0x%08x, offset=%lld, fi=0x%08x)\n",
        path, (int) buf, (int) size,  offset, (int) fi);	

	pthread_mutex_lock(&m3_IPC);

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

	TESTDEBUG("FIN hmf_write\n");
	return res;
}

static int hmf_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    
    return retstat;

	TESTDEBUG("INICIO hmf_utime\n");

	TESTDEBUG("hmf_utime(path=\"%s\", ubuf=0x%08x)\n",
            path, (int) ubuf);

	pthread_mutex_lock(&m3_IPC);


	retstat = MOL_utime((char*) path, ubuf);
	if (retstat < 0)
		return -errno;

	
	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_utime\n");
	return retstat;
}

static int hmf_chown(const char *path, uid_t uid, gid_t gid)
{
    int retstat = 0;
    
    return retstat;

	TESTDEBUG("INICIO hmf_chown\n");
	
	TESTDEBUG("hmf_chown(path=\"%s\", uid=0x%08x, gid=0x%08x)\n",
        path, (int) uid, (int) gid);	

	pthread_mutex_lock(&m3_IPC);

	retstat = MOL_chown((char*) path, uid, gid);
	if (retstat < 0)
		return -errno;
	
	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_chown\n");
	return retstat;
}

static int hmf_chmod(const char *path, mode_t mode)
{
    int retstat = 0;
    
    return retstat;

	TESTDEBUG("INICIO hmf_chmod\n");

	TESTDEBUG("hmf_chmod(path=\"%s\", mode=0x%08x)\n",
        path, (int) mode);	

	pthread_mutex_lock(&m3_IPC);

	retstat = MOL_chmod((char*) path, mode);
	if (retstat < 0)
		return -errno;
	
	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_chmod\n");
	return retstat;
}

static int hmf_truncate(const char *path, off_t length)
{
    int retstat = 0;
    
    return retstat;

	TESTDEBUG("INICIO hmf_truncate\n");

	TESTDEBUG("hmf_truncate(path=\"%s\", length=0x%08x)\n",
        path, (int) length);
	
	pthread_mutex_lock(&m3_IPC);

	retstat = MOL_truncate((char*) path, length);
	if (retstat < 0)
		return -errno;
	
	pthread_mutex_unlock(&m3_IPC);

	TESTDEBUG("FIN hmf_truncate\n");
	return retstat;
}

static int hmf_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    (void) fi;
   	
   	//Ver si con esto alcanza. Ver notas en bbfs.c para esta funcion. Flush no equivale a sync al parecer
	
    return retstat;
}

static int hmf_release(const char *path, struct fuse_file_info *fi)
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

int hmf_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
	(void) fi;
    
    // log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
	   //  path, fi);
    // log_fi(fi);
    
    // closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

// static int hmf_ftruncate(const char *path, off_t length, struct fuse_file_info *fi)

void *hmf_init(struct fuse_conn_info *conn)
{
    // log_msg("\nbb_init()\n");

    // log_conn(conn);
    // log_fuse_context(fuse_get_context());
    // MOL_Bind();
    fuse_get_context();
    return MFS_DATA;
}

static struct fuse_operations hmf_oper = {
	.access		= hmf_access,
	.getattr	= hmf_getattr,
  	.opendir 	= hmf_opendir,
	.readdir	= hmf_readdir,
	.releasedir	= hmf_releasedir,
	.open		= hmf_open,
	.read		= hmf_read,
	.write		= hmf_write,
  	.utime 		= hmf_utime,
	.chmod 		= hmf_chmod,
	.chown 		= hmf_chown,
	.truncate 	= hmf_truncate,
	.flush 		= hmf_flush, 
	.release 	= hmf_release, 
	.init 		= hmf_init
};

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void hmf_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, MFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here

    // log_msg("    bb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
	   //  MFS_DATA->rootdir, path, fpath);
}

void hmf_usage()
{
    fprintf(stderr, "usage:  test_hello_molfuse [FUSE and mount options] rootDir mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
	int fuse_stat;
	struct molfuse_state *mfs_data;

	//umask(0);
	// return fuse_main(argc, argv, &hmf_oper, NULL);
	
	if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	hmf_usage();

	mfs_data = malloc(sizeof(struct molfuse_state));
    if (mfs_data == NULL) {
		perror("main malloc");
		abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    mfs_data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    fprintf(stderr, "Antes de llamar a fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &hmf_oper, mfs_data);
    fprintf(stderr, "fuse_main retornó %d\n", fuse_stat);

    return fuse_stat;
}
