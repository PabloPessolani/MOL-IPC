/*
	This program test the Virtual File System connected to the 
	Storage Task (tasks/memory) that serves 
	using a disk image file (i.e. image_file.img) 
	It reads one file from and store it into a local file equally named
	To check correctness and integrity you must do	cmp command
	
*/

#include "tests.h"


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

// VARIABLES GLOBALES - PAP - START
#define WAIT4BIND_MS 1000
int dcid, clt_ep, clt_nr, clt_lpid;
int local_nodeid;
dvs_usr_t dvs, *dvs_ptr;
proc_usr_t fs, *fs_ptr;	
proc_usr_t clt, *clt_ptr;	
VM_usr_t  vmu, *vm_ptr;
// VARIABLES GLOBALES - PAP - END

	/* POSIX masks for st_mode. */
// #define S_IRWXU   00700		/* owner:  rwx------ */
// #define S_IRUSR   00400		/* owner:  r-------- */
// #define S_IWUSR   00200		/* owner:  -w------- */
// #define S_IXUSR   00100		/* owner:  --x------ */

// #define S_IRWXG   00070		/* group:  ---rwx--- */
// #define S_IRGRP   00040		/* group:  ---r----- */
// #define S_IWGRP   00020		/* group:  ----w---- */
void printStats (const char* fileName, struct mnx_stat bufferStat)
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

// #define ALL_RWXB	(S_IRWXU | S_IRWXG | S_IRWXO)
// #define S_IXGRP   00010		/* group:  -----x--- */

// #define S_IRWXO   00007		/* others: ------rwx */
// #define S_IROTH   00004		/* others: ------r-- */
// #define S_IWOTH   00002		/* others: -------w- */
// #define S_IXOTH   00001		/* others: --------x */

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
  register size_t k;

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

MOL_creat(char *name, mnx_mode_t mode)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr =&m;
	m.m_type = MOLCREAT;

	m.m3_i2 = mode;
	MOL_loadname(name, &m);
	
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	

  	return(rcode);		
}


int MOL_open(char path[], int flags) 
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr =&m;
	m.m_type = MOLOPEN;
	TESTDEBUG("%s file: %s, flags: %X\n" , __func__, path, flags);

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

	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
	return( m.m_type);
}

int MOL_close(int fd)
{
  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	INFODEBUG("MOL fd is=%d\n", fd);		
	m_ptr = &m;
  	m.m_type = MOLCLOSE;
  	m.m1_i1 = fd;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	INFODEBUG("%s rcode=%d\n", __func__, rcode);	

  	return(rcode);
}

ssize_t MOL_read(int fd, void *buffer, size_t nbytes)
{
  	message m , *m_ptr;
	int rcode;
	
	TESTDEBUG("INICIO %s\n", __func__);
	INFODEBUG("MOL fd is=%d\n", fd);		
	m_ptr = &m;
  	m.m_type = MOLREAD;
  	m.m1_i1 = fd;
  	m.m1_i2 = nbytes;
  	m.m1_p1 = (char *) buffer;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
  	return(m_ptr->m_type);
}

ssize_t MOL_write(int fd, void *buffer, size_t nbytes)
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
	if(rcode < 0) exit(1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
  	return(m_ptr->m_type);
}  

int MOL_sync()
{
  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLSYNC;

	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	INFODEBUG("%s rcode=%d\n", __func__, rcode);		

  	return(rcode);
}


int MOL_fsync(int fd)
{
	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	INFODEBUG("MOL fd is=%d\n", fd);	
	m_ptr = &m;
	m.m_type = MOLSYNC;

	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	INFODEBUG("%s rcode=%d\n", __func__, rcode);	

	return(rcode);
}

int MOL_utime(const char *name, struct utimbuf *timp)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
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
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
	return( m.m_type);
}

mnx_mode_t MOL_umask(mnx_mode_t complmode)
{
	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
	m.m_type = MOLUMASK;

	m.m1_i1 = complmode;

	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) return( (mnx_mode_t) -1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);	
	
  	return((mnx_mode_t) rcode);   
}

int MOL_stat(char *name, struct mnx_stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
	m.m_type = MOLSTAT;

	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr, "MOL_stat rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	INFODEBUG("%s rcode=%d\n", __func__, rcode);

	return (rcode);
}


int MOL_fstat(int fd, struct mnx_stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	INFODEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLFSTAT;

	m.m1_i1 = fd;
	m.m1_p1 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	rcode = mnx_sendrec(FS_PROC_NR, (long) &m);
	if ( rcode != OK ) {
		fprintf(stderr, "MOL_fstat rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	INFODEBUG("%s rcode=%d\n", __func__, rcode);

	return (rcode);
}

int MOL_fcntl(int fd, int cmd, ...)
{
	va_list argp;
	message m;

	TESTDEBUG("INICIO %s\n", __func__);
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
		fprintf(stderr, "MOL_fcntl rcode=%d\n", rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		

	return ( m.m_type);
}

DIR *MOL_opendir(const char *name)
/* Open a directory for reading. */
{
	int d, f;
	DIR *dp;
	struct mnx_stat st;
	TESTDEBUG("INICIO %s\n", __func__);
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
	return e;
}


int MOL_closedir(DIR *dp)
/* Finish reading a directory. */
{
	int d;
	int rcode;
	TESTDEBUG("INICIO %s\n", __func__);

	if (dp == nil) { errno= EBADF; return -1; }

	d= dp->_fd;
	free((void *) dp);
	rcode = MOL_close(d);
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
	return rcode;
}

int MOL_mkdir(char *name, mnx_mode_t mode)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLMKDIR;

	m.m1_i1 = strlen(name) + 1;
  	m.m1_i2 = mode;
	m.m1_p1 = (char *) name;  
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
  	return(m_ptr->m_type);  
}

int MOL_rmdir(char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLRMDIR;

	MOL_loadname(name, &m);
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
  	return(m_ptr->m_type);  
}


int MOL_chdir(char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLCHDIR;

	MOL_loadname(name, &m);
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}


int MOL_fchdir(int fd)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLCHDIR;

  	m.m1_i1 = fd;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

int MOL_rename(char *name, char *name2)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLRENAME;

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
  	return(m_ptr->m_type);  
}

int MOL_link(char *name, char *name2)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLLINK;

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}


int MOL_symlink(char *name, char *name2)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLSYMLINK;

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

int MOL_readlink(char *name, char *buffer, mnx_size_t bufsiz)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLRDLNK;

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = bufsiz;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

int MOL_unlink(char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLUNLINK;

	MOL_loadname(name, &m);
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

mnx_off_t MOL_lseek(int fd, mnx_off_t offset, int whence)
{

  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLLSEEK;

	m.m2_i1 = fd;
	m.m2_l1 = offset;
	m.m2_i2 = whence;

	TESTDEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) return( (mnx_off_t) -1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		
	
  	return((mnx_off_t) m.m2_l1);  
}

int MOL_truncate(char *path, mnx_off_t length)
{  

  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLTRUNCATE;

	m.m2_p1 = (char *) path;
	m.m2_i1 = strlen(path)+1;
	m.m2_l1 = length;	

	TESTDEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		

  	return(m_ptr->m_type); 
}

int MOL_ftruncate(int fd, mnx_off_t length)
{

  	message m , *m_ptr;
	int rcode;

	TESTDEBUG("INICIO %s\n", __func__);
	m_ptr = &m;
  	m.m_type = MOLFTRUNCATE;

	m.m2_i1 = fd;
	m.m2_l1 = length;	

	TESTDEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);		

  	return(m_ptr->m_type); 
}

int MOL_access(char *name, int mode)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLACCESS;

	m.m3_i2 = mode;
	
	MOL_loadname(name, &m);
	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

int MOL_mount(char *special, char *name, int flag)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLMOUNT;

	m.m1_i1 = strlen(special) + 1;
	m.m1_i2 = strlen(name) + 1;
	m.m1_i3 = flag;
	m.m1_p1 = special;
	m.m1_p2 = name;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

int MOL_umount(char *name)
{
  	message m , *m_ptr;
	int rcode;
  
	TESTDEBUG("INICIO %s\n", __func__);	
	m_ptr = &m;
  	m.m_type = MOLUMOUNT;

  	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
  	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if(rcode < 0) exit(1);
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
  	return(m_ptr->m_type);  
}

int MOL_dup(int fd)
{
	int rcode;
	
	TESTDEBUG("INICIO %s\n", __func__);	
	rcode = MOL_fcntl(fd, F_DUPFD, 0);
	INFODEBUG("%s rcode=%d\n", __func__, rcode);
	return (rcode);
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
		fprintf(stderr, "Usage: %s <dcid> <clt_nr> <MolFilename> [buf_size] \n", argv[0] );
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

	/*---------------------- ALINEAR BUFFER-------------------*/
	rcode = posix_memalign( (void**) &buffer, getpagesize(), buf_size);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}
	TESTDEBUG("CLIENT buffer address=%X point=%X\n", buffer, *buffer);

	/*----------------------FILE_OPEN-------------------*/
	fd = MOL_creat(argv[3], O_CREAT);
		
	if( fd < 0 ) {
		fprintf(stderr,"MOL_creat fd=%d\n",fd);
		exit(1);
	}
	TESTDEBUG("MOL creat reply is=%d\n",fd);

	
	/*----------------------FILE_OPEN-------------------*/
	fd = MOL_open(argv[3], O_RDWR);
		
	if( fd < 0 ) {
		fprintf(stderr,"MOL_open fd=%d\n",fd);
		exit(1);
	}
	TESTDEBUG("MOL fd is=%d\n",fd);

	/*----------------------FILE_READ-------------------*/
	bytes = MOL_read(fd, (void *) buffer, buf_size); 
	if( bytes < 0) {
		fprintf(stderr," MOL_read ERROR rcode=%d\n",bytes);
		exit(1);
	}

	TESTDEBUG("Buffer read is=%s\n", buffer);

	/*----------------------FILE_WRITE-------------------*/
	strcpy(buffer, "TESTING NEW!!!!\n");
	buf_size = strlen(buffer);
	bytes = MOL_write(fd, (void *) buffer, buf_size); 
	if( bytes < 0) {
		fprintf(stderr," MOL_write ERROR rcode=%d\n",bytes);
		exit(1);
	}

	TESTDEBUG("Buffer write is=%s\n", buffer);	

	TESTDEBUG("bytes written=%d\n", bytes);	

	/*----------------------FILE_CLOSE-------------------*/
	rcode = MOL_close(fd);
	if( rcode != 0 ) {
		fprintf(stderr,"FILE_CLOSE rcode=%d\n",rcode);
		exit(1);
	}

	TESTDEBUG("MOL_close resuts=%d\n",rcode);

	/*----------------------MOL_SYNC-------------------*/
	rcode = MOL_sync();
	if( rcode != 0 ) {
		fprintf(stderr,"MOL_SYNC rcode=%d\n",rcode);
		exit(1);
	}

	TESTDEBUG("MOL_SYNC resuts=%d\n",rcode);	

	exit(0);
 }

