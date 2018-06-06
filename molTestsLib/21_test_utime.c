/*
	This program test the Virtual File System connected to the 
	Storage Task (tasks/memory) that serves 
	using a disk image file (i.e. image_file.img) 
	It reads one file from and store it into a local file equally named
	To check correctness and integrity you must do	cmp command
	
*/

#define  MOL_USERSPACE	1
#define nil 0

#include "testlib.h"


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

// VARIABLES GLOBALES - NUEVO BINDING - START

#define WAIT4BIND_MS 1000
int dcid, clt_ep, clt_nr, clt_lpid;
int local_nodeid;
dvs_usr_t dvs, *dvs_ptr;
proc_usr_t fs, *fs_ptr;
proc_usr_t clt, *clt_ptr;
DC_usr_t  vmu, *dc_ptr;

// VARIABLES GLOBALES - NUEVO BINDING - END

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

/*===========================================================================*
 *				init_m3ipc					     *
 *===========================================================================*/
void init_m3ipc(void)
{
	int rcode;

	clt_lpid = getpid();
	do {
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("CLIENT mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("CLIENT mnx_wait4bind_T TIMEOUT\n");
			continue ;
		} else if ( rcode < 0)
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);

	SVRDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&dvs);
	if (rcode) ERROR_EXIT(rcode);
	dvs_ptr = &dvs;
	SVRDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));

	SVRDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if (rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	SVRDEBUG(DC_USR_FORMAT, DC_USR_FIELDS(dc_ptr));

	SVRDEBUG("Get FS info from SYSTASK\n");
	rcode = sys_getproc(&fs, FS_PROC_NR);
	if (rcode) ERROR_EXIT(rcode);
	fs_ptr = &fs;
	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(fs_ptr));
	if ( TEST_BIT(fs_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr, "FS not started\n");
		fflush(stderr);
		ERROR_EXIT(EMOLNOTBIND);
	}

	SVRDEBUG("Get Client info from SYSTASK\n");
	rcode = sys_getproc(&clt, SELF);
	if (rcode) ERROR_EXIT(rcode);
	clt_ptr = &clt;
	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(clt_ptr));

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
	//Para STAT
	struct mnx_stat bufferStat;
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

//****************************** MODIFICADO PAP :  START ****************************
	if ( argc != 2 && argc != 3)	{
		fprintf(stderr, "Usage: %s <MolFilename> [buf_size] \n", argv[0] );
		exit(1);
	}

	if ( argc == 3)	{
		buf_size = atoi(argv[2]);
		if ( buf_size < 0 || buf_size >  MAXCOPYLEN) {
			fprintf(stderr, "Invalid 0 < buf_size=%d < %d\n", buf_size, MAXCOPYLEN + 1);
			exit(1);
		}
	}
	else {
		buf_size = MAXCOPYBUF;
	}

	init_m3ipc();

	SVRDEBUG("(%d)Starting %s in dcid=%d endpoint=%d MolFilename=%s buf_size=%d\n",
	         clt_lpid, argv[0], dc_ptr->dc_dcid, clt_ep, argv[1], buf_size);

//****************************** MODIFICADO PAP :  END  ****************************

	INFODEBUG("CLIENT pause before SENDREC\n");
	sleep(2); 


	/*---------------------- BUFFER ALIGNMENT-------------------*/
	rcode = posix_memalign( (void**) &buffer, getpagesize(), buf_size);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}
	TESTDEBUG("CLIENT buffer address=%X point=%X\n", buffer, *buffer);
	
	/*----------------------FILE_OPEN-------------------*/
	// fd = mol_open(argv[4], O_APPEND);
	fd = mol_open(argv[1], O_RDONLY | O_NONBLOCK);	//Para test de opendir
		
	if( fd < 0 ) {
		fprintf(stderr,"mol_open fd=%d\n",fd);
		exit(1);
	}
	INFODEBUG("MOL fd is=%d\n",fd);

	/*----------------------FILE STATS-------------------*/
	retStat = mol_fstat(fd, &bufferStat);
		
	printStats(argv[1], bufferStat);

	if( retStat < 0 ) {
		fprintf(stderr,"mol_fstat fd=%d\n",retStat);
		exit(1);
	}
	TESTDEBUG("MOL fstats results are=%d\n", retStat);
 
 	/*-----------UPDATE TIMES wiht mol_utime ---------*/
    ub.actime = bufferStat.st_atime - 120; /*le resto a cada tiempo 2 minutos*/
    ub.modtime = bufferStat.st_mtime - 120;

	retUtime = mol_utime(argv[1], &ub);
		
	if( retUtime < 0 ) {
		fprintf(stderr,"mol_utime fd=%d\n",retUtime);
		exit(1);
	}
	TESTDEBUG("mol_utime results are=%d\n", retUtime); 

	/*----------------------FILE STATS-------------------*/
	retStat = mol_fstat(fd, &bufferStat);
		
	printStats(argv[1], bufferStat);

	if( retStat < 0 ) {
		fprintf(stderr,"mol_fstat fd=%d\n",retStat);
		exit(1);
	}
	TESTDEBUG("MOL fstats results are=%d\n", retStat);   

	/*----------------------FILE_CLOSE-------------------*/
	rcode = mol_close(fd);
	if( rcode != 0 ) {
		fprintf(stderr,"FILE_CLOSE rcode=%d\n",rcode);
		exit(1);
	}

	TESTDEBUG("mol_close resuts=%d\n",rcode);

	exit(0);
 }

