/* EXTERN should be extern except for the table file */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

/* The parameters of the call are kept here. */
EXTERN message m_in;		/* the input message itself */
EXTERN message m_out;		/* the output message used for reply */
EXTERN message *m_ptr;		/* pointer to message */

EXTERN int who_p, who_e;	/* caller's proc number, endpoint */
EXTERN int call_nr;		/* system call number */

EXTERN int img_size;		/* testing image file size */
EXTERN char *img_ptr;		/* pointer to the first byte of the ram disk image */

//EXTERN FILE *img_fd_disk;		/* file descriptor for disk image */
//EXTERN int fileDescDiskImage;	/* file descriptor for disk image */

EXTERN FF_BYTE *localbuff;		/* pointer to the first byte of the local buffer FATFS datatype*/	
// EXTERN unsigned localbuff[_MAX_BLOCK_SIZE];

EXTERN fproc_t *fproc;		/* FS process table			*/
EXTERN mproc_t	*mproc; 	/* PM process table			*/

EXTERN dmap_t dmap_tab[NR_DEVICES]; /* dev_nr -> major/task */
EXTERN int	dmap_rev[NR_DEVICES];	/* major/task -> dev_nr */

EXTERN int	cfg_dev_nr;			/* # of devices in config.file */
EXTERN unsigned int mandatory;


/*Declarations for FatFS*/
EXTERN FATFS fsWork;		/* FatFs work area needed for each volume */
EXTERN char *img_name;		/* name of the ram disk image file*/
EXTERN int fdisk_sts;
EXTERN int rdisk_sts;

EXTERN int root_major;
EXTERN dconf_t *root_ptr;
EXTERN mnx_dev_t root_dev;

//Ver la diferencia entre el codigo original donde fproc es un arreglo de estructuras y aca es solo una estructura
EXTERN struct fproc *fp;	/* pointer to caller's fproc struct */
EXTERN int super_user;		/* 1 if caller is super_user, else 0 */

EXTERN int fs_lpid;		
EXTERN int fs_ep;		
EXTERN int fs_nr;	

EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN dc_usr_t  dcu, *dc_ptr;
EXTERN int local_nodeid;
EXTERN proc_usr_t proc_fs, *fs_ptr;	
EXTERN int err_code;		/* temporary storage for error number */
//EXTERN char user_path[MNX_PATH_MAX];/* storage for user path name */
EXTERN char *user_path;
 
/*
Ver que pasa con esto porque originalmente es un Dev_t con mayusculas
que es un int y aca lo estoy definiendo con minusculas que es un short
 */

EXTERN mnx_dev_t root_dev;		/* device number of the root device */ 

struct super_block *sb_ptr; /*Super block pointer*/

EXTERN mnx_time_t boottime;		/* time in seconds at system boot */

/* The following variables are used for returning results to the caller. */
EXTERN int rdwt_err;		/* status of last disk i/o request */

EXTERN struct inode *rdahed_inode;  /* pointer to inode to read ahead */
EXTERN mnx_off_t rdahedpos;             /* position to read ahead */

EXTERN int susp_count;              /* number of procs suspended on pipe */

EXTERN int nr_locks;		/* number of locks currently in place */

EXTERN int reviving;		/* number of pipe processes to be revived */

extern _PROTOTYPE (int (*call_vec[]), (void) ); /* sys call table */
extern char dot1[2];   /* dot1 (&dot1[0]) and dot2 (&dot2[0]) have a special */
extern char dot2[3];   /* meaning to search_dir: no access permission check. */


#ifdef ANULADO
/* File System global variables */
EXTERN struct fproc *fp;	/* pointer to caller's fproc struct */
EXTERN int super_user;		/* 1 if caller is super_user, else 0 */
EXTERN int susp_count;		/* number of procs suspended on pipe */
EXTERN int nr_locks;		/* number of locks currently in place */
EXTERN int reviving;		/* number of pipe processes to be revived */
EXTERN off_t rdahedpos;		/* position to read ahead */
EXTERN struct inode *rdahed_inode;	/* pointer to inode to read ahead */
EXTERN mnx_dev_t root_dev;		/* device number of the root device */
EXTERN time_t boottime;		/* time in seconds at system boot */



/* The following variables are used for returning results to the caller. */
EXTERN int rdwt_err;		/* status of last disk i/o request */

/* Data initialized elsewhere. */
extern char dot1[2];   /* dot1 (&dot1[0]) and dot2 (&dot2[0]) have a special */
extern char dot2[3];   /* meaning to search_dir: no access permission check. */
#endif /* ANULADO */

