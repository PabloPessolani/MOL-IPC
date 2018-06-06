/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusehmf.c `pkg-config fuse --cflags --libs` -o fusehmf
*/

#define FUSE_USE_VERSION 26

#define MOL_USERSPACE 1
// #define _MNX_STAT_H 1


#define nil 0
#include "fuselib.h"

#define gettid() (pid_t) syscall(SYS_gettid)
#define v7ent(p) ((struct _v7_direct *)(p))
#define V7_EXTENT (sizeof(struct _v7_direct) / sizeof(struct _fl_direct) - 1)

//FUSE declarations
struct molfuse_state
{
	int vmid;
	int fuse_ep;
	int clt_nr;
	int fuse_pid;
	pid_t fuse_tid;
	char *rootdir;
};

/*ATENCION: Esta estructura es exactamente la misma que esta en "../kernel/minix/mnx_stat.h", 
Se incluye de forma local porque con FUSE está dando errores de compilacion con 
los nombres de los campos:
  mnx_time_t st_atim;		
  mnx_time_t st_mtim;		
  mnx_time_t st_ctim;
  
Se trabajará entonces por ahora con esta estructura para simplificar la compilacion.
*/
struct fuse_mnx_stat {
  mnx_dev_t st_dev;			/* major/minor device number */
  mnx_ino_t st_ino;			/* i-node number */
  mnx_mode_t st_mode;		/* file mode, protection bits, etc. */
  short int st_nlink;		/* # links; TEMPORARY HACK: should be nlink_t*/
  mnx_uid_t st_uid;			/* uid of the file's owner */
  short int st_gid;		/* gid; TEMPORARY HACK: should be gid_t */
  mnx_dev_t st_rdev;
  mnx_off_t st_size;		/* file size */
  mnx_time_t st_atim;		/* time of last access */
  mnx_time_t st_mtim;		/* time of last data modification */
  mnx_time_t st_ctim;		/* time of last file status change */
};

#define MFS_DATA ((struct molfuse_state *)fuse_get_context()->private_data)

//MOLFS declarationss
#define MNX_POSIX_PATH_MAX 256

#define USER_ID 0
#define GROUP_ID 0
#define ALL_RWXB (S_IRWXU | S_IRWXG | S_IRWXO)
#define ALL_SETB (S_ISUID | S_ISGID)
#define ALL_BITS (ALL_RWXB | ALL_SETB)

#define WAIT4BIND_MS 1000
int vmid, clt_ep, clt_nr, clt_lpid;
int local_nodeid;
dvs_usr_t dvs, *dvs_ptr;
proc_usr_t fs, *fs_ptr;
proc_usr_t clt, *clt_ptr;
VM_usr_t vmu, *dc_ptr;

int fuse_ep, svr_ep;
int clt_nr, rcode;
int fuse_pid;
pid_t fuse_tid;
char buffTime[20];
struct tm *timeinfo;

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
void printStats(const char *fileName, struct fuse_mnx_stat bufferStat)
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
	// struct mnx_stat {
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
	printf((S_ISDIR(bufferStat.st_mode)) ? "d" : "-");
	printf((bufferStat.st_mode & S_IRUSR) ? "r" : "-");
	printf((bufferStat.st_mode & S_IWUSR) ? "w" : "-");
	printf((bufferStat.st_mode & S_IXUSR) ? "x" : "-");
	printf((bufferStat.st_mode & S_IRGRP) ? "r" : "-");
	printf((bufferStat.st_mode & S_IWGRP) ? "w" : "-");
	printf((bufferStat.st_mode & S_IXGRP) ? "x" : "-");
	printf((bufferStat.st_mode & S_IROTH) ? "r" : "-");
	printf((bufferStat.st_mode & S_IWOTH) ? "w" : "-");
	printf((bufferStat.st_mode & S_IXOTH) ? "x" : "-");
	printf("\n\n");

	timeinfo = gmtime(&(bufferStat.st_atim));
	// timeinfo = localtime (&(bufferStat.st_atime));
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo);
	printf("A time \t%s\n", buffTime);
	timeinfo = gmtime(&(bufferStat.st_mtim));
	// timeinfo = localtime (&(bufferStat.st_mtime));
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo);
	printf("M time \t%s\n", buffTime);
	timeinfo = gmtime(&(bufferStat.st_ctim));
	// timeinfo = localtime (&(bufferStat.st_ctime));
	strftime(buffTime, 20, "%d/%m/%Y %H:%M", timeinfo);
	printf("C time \t%s\n", buffTime);

	// TESTDEBUG("S_ISLNK(bufferStat.st_mode)=%d\n", S_ISLNK(bufferStat.st_mode));
	// TESTDEBUG("bufferStat.st_mode=%d\n", bufferStat.st_mode);
	printf("The file %s a symbolic link\n", (S_ISLNK(bufferStat.st_mode)) ? "IS" : "IS NOT");
	printf("The file %s a DIRECTORY \n", (S_ISDIR(bufferStat.st_mode)) ? "IS" : "IS NOT");
	printf("\n");
}

_mnx_Mode_t parseMode(const char *mode)
{

	_mnx_Mode_t nuevoModo;
	// INFODEBUG("MODE OWN=%c\n", mode[0]);
	// INFODEBUG("MODE GRP=%c\n", mode[1]);
	// INFODEBUG("MODE OTH=%c\n", mode[2]);

	switch (mode[0])
	{
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

	switch (mode[1])
	{
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

	switch (mode[2])
	{
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

char *fixPath(const char *path, char *fixedPath)
{
	char val[MNX_POSIX_PATH_MAX];
	strcpy(val, path);
	int index = 0;
	while (val[index])
	{
		if (val[index] == '/')
			val[index] = '\\';
		index++;
	}
	strcpy(fixedPath, val);

	return fixedPath;
}

char *getAllButFirst(const char *input)
{
	char *fixed;

	// TESTDEBUG("(input=\"%s\")\n", input);
	int len = strlen(input);
	// TESTDEBUG("(len=\"%d\")\n", len);
	if (len > 0)
		input++; //Go past the first char

	len = strlen(input);
	// TESTDEBUG("(len=\"%d\")\n", len);
	// TESTDEBUG("(input=\"%s\")\n", input);
	fixed = (char *) malloc(len);
	strcpy(fixed, input);
	// TESTDEBUG("(fixed=\"%s\")\n", fixed);
	return fixed;
}

char *getAllButFirstAndLast(char *input)
{
	int len = strlen(input);
	if (len > 0)
		input++; //Go past the first char
	if (len > 1)
		input[len - 2] = '\0'; //Replace the last char with a null termination
	return input;
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
	msgptr->m3_p1 = (char *)name;
	if (k <= sizeof msgptr->m3_ca1)
		strcpy(msgptr->m3_ca1, name);
}

int MOL_creat(const char *name, mnx_mode_t mode)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLCREAT;

	m.m3_i2 = mode;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

int MOL_open(char path[], int flags)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLOPEN;
	TESTDEBUG("%s %X\n", path, flags);

	// TESTDEBUG("strlen(path) =%d\n", strlen(path));
	if (flags & O_CREAT)
	{
		m.m1_i1 = strlen(path) + 1;
		m.m1_i2 = flags;
		// m.m1_i3 = va_arg(argp, _mnx_Mode_t); //Ver esto de los arg variables de open
		m.m1_p1 = (char *)path;
		INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	}
	else
	{
		MOL_loadname(path, &m);
		m.m3_i2 = flags;
		INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	}

	rcode = mnx_sendrec(FS_PROC_NR, (long)&m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (m.m_type);
}

int MOL_close(int fd)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	TESTDEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLCLOSE;
	m.m1_i1 = fd;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

ssize_t MOL_read(int fd, void *buffer, mnx_size_t nbytes)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	TESTDEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLREAD;
	m.m1_i1 = fd;
	m.m1_i2 = nbytes;
	m.m1_p1 = (char *)buffer;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (m_ptr->m_type);
}

ssize_t MOL_write(int fd, const void *buffer, mnx_size_t nbytes)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	INFODEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLWRITE;
	m.m1_i1 = fd;
	m.m1_i2 = nbytes;
	m.m1_p1 = (char *)buffer;
	TESTDEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	TESTDEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	INFODEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (m_ptr->m_type);
}

int MOL_sync()
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLSYNC;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

int MOL_fsync(int fd)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	TESTDEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLSYNC;
	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

int MOL_stat(const char *name, struct fuse_mnx_stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	// INFODEBUG("filename%s\n", name);
	m_ptr = &m;
	m.m_type = MOLSTAT;
	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *)name;
	m.m1_p2 = (char *)buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, &m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

int MOL_fstat(int fd, struct fuse_mnx_stat *buffer)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO %s\n");
	INFODEBUG("MOL fd is=%d\n", fd);
	m_ptr = &m;
	m.m_type = MOLFSTAT;
	m.m1_i1 = fd;
	m.m1_p1 = (char *)buffer;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long)&m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

int MOL_closedir(DIR *dp)
/* Finish reading a directory. */
{
	int d;
	int rcode;

	INFODEBUG("INICIO\n");
	if (dp == nil)
	{
		errno = EBADF;
		return -1;
	}

	d = dp->_fd;
	free((void *)dp);
	rcode = MOL_close(d);
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (rcode);
}

int MOL_mkdir(const char *name, mnx_mode_t mode)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLMKDIR;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = mode;
	m.m1_p1 = (char *)name;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long)&m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (m_ptr->m_type);
}

int MOL_rmdir(const char *name)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLRMDIR;
	MOL_loadname(name, &m);

	INFODEBUG("Request: " MSG3_FORMAT, MSG3_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long)&m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (m_ptr->m_type);
}

int MOL_rename(const char *name, const char *name2)
{
	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLRENAME;
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *)name;
	m.m1_p2 = (char *)name2;

	INFODEBUG("Request: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long)&m);
	if (rcode != OK)
	{
		ERROR_EXIT(rcode);
		exit(1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return (m_ptr->m_type);
}

mnx_off_t MOL_lseek(int fd, mnx_off_t offset, int whence)
{

	message m, *m_ptr;
	int rcode;

	INFODEBUG("INICIO \n");
	m_ptr = &m;
	m.m_type = MOLLSEEK;
	m.m2_i1 = fd;
	m.m2_l1 = offset;
	m.m2_i2 = whence;

	INFODEBUG("Request: " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(FS_PROC_NR, (long)&m);
	if (rcode < 0)
	{
		ERROR_EXIT(rcode);
		return ((mnx_off_t)-1);
	}
	INFODEBUG("Reply: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	TESTDEBUG("rcode=%d\n", rcode);
	INFODEBUG("FIN \n");
	return ((mnx_off_t)m.m2_l1);
}

/*===========================================================================*
 *				init_m3ipc					     *
 *===========================================================================*/
// void MOL_Bind(void)
// {
// 	int rcode;

// 	clt_lpid = getpid();
// 	SVRDEBUG("CLIENT PID =%d\n", clt_lpid);
// 	do {
// 		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
// 		SVRDEBUG("CLIENT mnx_wait4bind_T  rcode=%d\n", rcode);
// 		if (rcode == EMOLTIMEDOUT) {
// 			SVRDEBUG("CLIENT mnx_wait4bind_T TIMEOUT\n");
// 			continue ;
// 		} else if ( rcode < 0)
// 			ERROR_EXIT(EXIT_FAILURE);
// 	} while	(rcode < OK);

// 	fuse_ep = mnx_getep(clt_lpid);

// 	SVRDEBUG("Get the DRVS info from SYSTASK\n");
// 	rcode = sys_getkinfo(&dvs);
// 	if (rcode) ERROR_EXIT(rcode);
// 	dvs_ptr = &dvs;
// 	SVRDEBUG(DRVS_USR_FORMAT, DRVS_USR_FIELDS(dvs_ptr));

// 	SVRDEBUG("Get the VM info from SYSTASK\n");
// 	rcode = sys_getmachine(&vmu);
// 	if (rcode) ERROR_EXIT(rcode);
// 	dc_ptr = &vmu;
// 	SVRDEBUG(VM_USR_FORMAT, VM_USR_FIELDS(dc_ptr));

// 	SVRDEBUG("Get FS info from SYSTASK\n");
// 	rcode = sys_getproc(&fs, FS_PROC_NR);
// 	if (rcode) ERROR_EXIT(rcode);
// 	fs_ptr = &fs;
// 	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(fs_ptr));
// 	if ( TEST_BIT(fs_ptr->p_rts_flags, BIT_SLOT_FREE)) {
// 		fprintf(stderr, "FS not started\n");
// 		fflush(stderr);
// 		ERROR_EXIT(EMOLNOTBIND);
// 	}

// 	SVRDEBUG("Get Client info from SYSTASK\n");
// 	rcode = sys_getproc(&clt, SELF);
// 	if (rcode) ERROR_EXIT(rcode);
// 	clt_ptr = &clt;
// 	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(clt_ptr));

// }

int MOL_Bind()
{
	fuse_pid = getpid();
	// fuse_tid = gettid();
	INFODEBUG("INICIO \n");

	vmid = 0;
	if (vmid < 0 || vmid >= NR_VMS)
	{
		INFODEBUG("Invalid vmid [0-%d]\n", NR_VMS - 1);
		exit(1);
	}

	clt_nr = 21; //Defino arbitrario para probar
	/*Si no esta hecho, hago el binding del proceso cliente, o sea este*/
	// sleep(5);
	// INFODEBUG("ANTES fuse_ep=%d\n", fuse_ep);
	// INFODEBUG("ANTES fuse_pid=%d\n", fuse_pid);
	// INFODEBUG("ANTES fuse_tid =%d\n", fuse_tid);
	if (fuse_ep = mnx_getep(fuse_pid) < 0)
		fuse_ep = mnx_bind(vmid, clt_nr);
	// fuse_ep = mnx_lclbind(vmid,fuse_pid,clt_nr);
	// INFODEBUG("fuse_ep=%d\n", fuse_ep);
	// INFODEBUG("fuse_pid=%d\n", fuse_pid);
	// INFODEBUG("fuse_tid =%d\n", fuse_tid);

	if (fuse_ep < 0)
	{
		INFODEBUG("BIND ERROR fuse_ep=%d\n", fuse_ep);
		exit(1);
	}

	INFODEBUG("BIND CLIENT vmid=%d fuse_pid=%d clt_nr=%d fuse_ep=%d\n",
			  vmid,
			  fuse_pid,
			  clt_nr,
			  fuse_ep);
	INFODEBUG("FIN \n");
}

int MOL_UnBind()
{
	// vmid = 0;

	INFODEBUG("INICIO \n");

	// clt_nr = 2;
	SVRDEBUG("Unbinding FUSE\n");
	mnx_unbind(dc_ptr->dc_vmid, clt_ptr->p_endpoint);

	SVRDEBUG("Get the VM info from SYSTASK\n");
	SVRDEBUG(VM_USR_FORMAT, VM_USR_FIELDS(dc_ptr));

	SVRDEBUG("Get Client info from SYSTASK\n");
	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(clt_ptr));

	INFODEBUG("FIN \n");
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
static void mfs_fullpath(char fpath[MNX_POSIX_PATH_MAX], const char *path)
{
	strcpy(fpath, MFS_DATA->rootdir);
	strncat(fpath, path, MNX_POSIX_PATH_MAX); // ridiculously long paths will
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

	int fd;

	INFODEBUG("INICIO \n");
	TESTDEBUG("(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_creat(path, mode);
	if (fd < 0)
		retstat = mfs_error("mfs_create -> MOL_creat");

	fi->fh = fd;

	pthread_mutex_unlock(&m3_IPC);

	INFODEBUG("FIN \n");
	return 0;
}

static int mfs_access(const char *path, int mask)
{
	//#define R_OK   4   /* test for read permission */
	//#define W_OK   2   /* test for write permission */
	//#define X_OK   1   /* test for execute (search) permission */
	//#define F_OK   0   /* test for presence of file */
	
	int retstat = R_OK | W_OK | F_OK;

	//Ver si con esto alcanza. Ver notas en bbfs.c para esta funcion. Flush no equivale a sync al parecer

	return retstat;
}


static int mfs_open(const char *path, struct fuse_file_info *fi)
{
	int fd;
	char *fixedPath;

	INFODEBUG("INICIO \n");
	// TESTDEBUG("(path=\"%s\", fi=0x%08x)\n", path, (int) fi);

	fixedPath = getAllButFirst(path);
	// TESTDEBUG("(fixedPath=\"%s\")\n", fixedPath);	

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_open(fixedPath, fi->flags);
	if (fd < 0)
		fd = mfs_error("mfs_open -> MOL_open");

	fi->fh = fd;

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return 0;
}

static int mfs_getattr(const char *path, struct stat *stbuf)
{
	int retstat;

	char *fixedPath;
	static struct fuse_mnx_stat st;

	INFODEBUG("INICIO \n");
	TESTDEBUG("(path=\"%s\", stbuf=0x%08x)\n", path, (int)stbuf);

	// fixPath(path, fixedPath);
	// // if (fixedPath[0] == '\\')

	// TESTDEBUG("(fixedPath=\"%s\")\n", fixedPath);

	fixedPath = getAllButFirst(path);
	// TESTDEBUG("(fixedPath=\"%s\")\n", fixedPath);
	//TESTDEBUG("(tamaño struct stat =\"%d\")\n", sizeof(struct stat));
	// TESTDEBUG("(MAXCOPYBUF =\"%d\")\n", MAXCOPYBUF);
	/*---------------------- BUFFER ALIGNMENT-------------------*/
	retstat = posix_memalign((void **)&st, getpagesize(), MAXCOPYBUF); //OJO VER ESTO
	if (retstat)
	{
		ERROR_EXIT(retstat);
		exit(1);
	}
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_stat(fixedPath, &st);
	stbuf->st_dev = st.st_dev;
	stbuf->st_ino = st.st_ino;
	stbuf->st_mode = st.st_mode;
	stbuf->st_nlink = st.st_nlink;
	stbuf->st_uid = st.st_uid;
	stbuf->st_gid = st.st_gid;
	stbuf->st_rdev = st.st_rdev;
	stbuf->st_size = st.st_size;
	stbuf->st_atime = st.st_atim;
	stbuf->st_mtime = st.st_mtim;
	stbuf->st_ctime = st.st_ctim;

	// printStats(path, st);
	if (retstat != 0)
		retstat = mfs_error("mfs_getattr -> MOL_stat");

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return retstat;
}

int mfs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	char *fixedPath;
	static struct fuse_mnx_stat st;

	INFODEBUG("INICIO\n");
	TESTDEBUG("(path=\"%s\", stbuf=0x%08x)\n", path, (int)stbuf);

	fixedPath = getAllButFirst(path);
	// TESTDEBUG("(fixedPath=\"%s\")\n", fixedPath);
	//TESTDEBUG("(tamaño struct stat =\"%d\")\n", sizeof(struct stat));
	// TESTDEBUG("(MAXCOPYBUF =\"%d\")\n", MAXCOPYBUF);
	/*---------------------- BUFFER ALIGNMENT-------------------*/
	retstat = posix_memalign((void **)&st, getpagesize(), MAXCOPYBUF); //OJO VER ESTO
	if (retstat)
	{
		ERROR_EXIT(retstat);
		exit(1);
	}

	pthread_mutex_lock(&m3_IPC);

	// On FreeBSD, trying to do anything with the mountpoint ends up
	// opening it, and then using the FD for an fgetattr.  So in the
	// special case of a path of "/", I need to do a getattr on the
	// underlying root directory instead of doing the fgetattr().
	// TESTDEBUG("(strcmp(path, '/') = \"%d\")\n", strcmp(path, "/"));
	if (!strcmp(path, "/"))
		return mfs_getattr(fixedPath, stbuf);
	MOL_Bind();
	TESTDEBUG("ANTES DE FSTAT\n");
	retstat = MOL_fstat(fi->fh, &st);
	if (retstat < 0)
		retstat = mfs_error("mfs_fgetattr -> MOL_fstat");

	stbuf->st_dev = st.st_dev;
	stbuf->st_ino = st.st_ino;
	stbuf->st_mode = st.st_mode;
	stbuf->st_nlink = st.st_nlink;
	stbuf->st_uid = st.st_uid;
	stbuf->st_gid = st.st_gid;
	stbuf->st_rdev = st.st_rdev;
	stbuf->st_size = st.st_size;
	stbuf->st_atime = st.st_atim;
	stbuf->st_mtime = st.st_mtim;
	stbuf->st_ctime = st.st_ctim;

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return retstat;
}

int mfs_mkdir(const char *path, mode_t mode)
{
	int retstat = 0;

	INFODEBUG("INICIO\n");
	TESTDEBUG("(path=\"%s\", mode=0x%lld)\n", path, (int)mode);

	//pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_mkdir(path, mode);
	if (retstat < 0)
		retstat = mfs_error("mfs_mkdir -> MOL_mkdir");

	//pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return 0;
}

int mfs_rename(const char *path, const char *newpath)
{
	int retstat = 0;

	// char fnewpath[PATH_MAX];

	TESTDEBUG("\n(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);

	// mfs_fullpath(fnewpath, newpath);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	retstat = MOL_rename(path, newpath);
	if (retstat < 0)
		retstat = mfs_error("mfs_rename -> MOL_rename");

	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return 0;
}

static int mfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char *fixedPath;

	INFODEBUG("INICIO \n");
	TESTDEBUG("(path=\"%s\", buf=0x%08x, size=0x%08x, offset=%lld, fi=0x%08x)\n", path, (int)buf, (int)size, offset, (int)fi);

	fixedPath = getAllButFirst(path);
	// TESTDEBUG("(fixedPath=\"%s\")\n", fixedPath);

	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_open(fixedPath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = MOL_lseek(fd, offset, SEEK_SET);
	if (res == -1)
		res = -errno;

	res = MOL_read(fd, buf, size);
	if (res == -1)
		res = -errno;

	TESTDEBUG("(res=\"%d\")\n", res);
	TESTDEBUG("(buf=\"%s\")\n", buf);
	MOL_close(fd);
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return res;
}

static int mfs_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char *fixedPath;

	INFODEBUG("INICIO %s\n");
	TESTDEBUG("%s(path=\"%s\", buf=0x%08x, size=0x%08x, offset=%lld, fi=0x%08x)\n", path, (int)buf, (int)size, offset, (int)fi);
	
	fixedPath = getAllButFirst(path);
	// TESTDEBUG("(fixedPath=\"%s\")\n", fixedPath);
	
	pthread_mutex_lock(&m3_IPC);

	MOL_Bind();
	fd = MOL_open(fixedPath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = MOL_lseek(fd, offset, SEEK_SET);
	if (res == -1)
		res = -errno;

	res = MOL_write(fd, buf, size);
	if (res == -1)
		res = -errno;

	MOL_close(fd);
	pthread_mutex_unlock(&m3_IPC);
	INFODEBUG("FIN \n");
	return res;
}

static int mfs_truncate(const char *path, off_t length)
{
	int retstat = 0;

	//Ver si con esto alcanza. Ver notas en bbfs.c para esta funcion. Flush no equivale a sync al parecer

	return retstat;
}

static int mfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	int retstat = 0;
	(void)fi;

	//Ver si con esto alcanza. Ver notas en bbfs.c para esta funcion. Flush no equivale a sync al parecer

	return retstat;
}

static int mfs_flush(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	(void)fi;

	//Ver si con esto alcanza. Ver notas en bbfs.c para esta funcion. Flush no equivale a sync al parecer

	return retstat;
}

static int mfs_release(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	(void)fi;

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
	(void)fi;

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
	MOL_Bind();
	fuse_get_context();
	return MFS_DATA;
}

static struct fuse_operations mfs_oper = {
	.create = mfs_create,
	.open = mfs_open,
	.read = mfs_read,
	.write = mfs_write,
	.access	= mfs_access,
	.getattr = mfs_getattr,
	.fgetattr	= mfs_fgetattr,
	//.opendir 	= mfs_opendir,
	// .readdir	= mfs_readdir,
	.mkdir = mfs_mkdir,
	.rename = mfs_rename,
	.truncate 	= mfs_truncate,
    .ftruncate  = mfs_ftruncate,	
	.flush = mfs_flush,
	.release = mfs_release,
	.releasedir = mfs_releasedir,
	.init = mfs_init
};

int main(int argc, char *argv[])
{
	int fuse_stat;
	struct molfuse_state *mfs_data;

	//umask(0);
	// return fuse_main(argc, argv, &mfs_oper, NULL);

	TESTDEBUG("argc=%d\n", argc);
	TESTDEBUG("argv[0]=%s\n", argv[0]);
	TESTDEBUG("argv[1]=%s\n", argv[1]);
	TESTDEBUG("argv[2]=%s\n", argv[2]);
	TESTDEBUG("argv[3]=%s\n", argv[3]);
	// TESTDEBUG("argv[argc-2][0]=%c\n",argv[argc-2][0]);
	// TESTDEBUG("argv[argc-1][0]=%c\n",argv[argc-1][0]);
	// TESTDEBUG("argv[argc-2]=%s\n",argv[argc-2]);
	// TESTDEBUG("argv[argc-1]=%s\n",argv[argc-1]);

	// if ((argc < 2) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	// mfs_usage();

	//mfs_data = malloc(sizeof(struct molfuse_state));
	//if (mfs_data == NULL) {
	// 	perror("main malloc");
	// 	abort();
	//}

	// Pull the rootdir out of the argument list and save it in my
	// internal data
	// mfs_data->rootdir = "/"; //realpath(argv[argc-2], NULL);
	// argv[argc-2] = argv[argc-1];
	// argv[argc-1] = NULL;
	// argc--;

	fprintf(stderr, "Before calling fuse_main\n");
	// fuse_stat = fuse_main(argc, argv, &mfs_oper, mfs_data);
	fuse_stat = fuse_main(argc, argv, &mfs_oper, NULL);
	fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

	return fuse_stat;
}
