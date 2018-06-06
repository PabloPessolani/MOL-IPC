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

//EXTERN fproc_t *fproc;		/* FS process table			*/

EXTERN int m_lpid;		
EXTERN int m_ep;		
//EXTERN int fs_nr;	

EXTERN drvs_usr_t drvs, *drvs_ptr;
EXTERN VM_usr_t  vmu, *vm_ptr;

EXTERN int err_code;		/* temporary storage for error number */
//EXTERN char user_path[PATH_MAX];/* storage for user path name */

/*
Ver que pasa con esto porque originalmente es un Dev_t con mayusculas
que es un int y aca lo estoy definiendo con minusculas que es un short
 */

EXTERN mnx_dev_t root_dev;		/* device number of the root device */ 

EXTERN char *img_name;		/* name of the ram disk image file*/
EXTERN int local_nodeid;

struct super_block *sb_ptr; /*Super block pointer*/

// EXTERN mnx_time_t boottime;		/* time in seconds at system boot */

/* The following variables are used for returning results to the caller. */
EXTERN int rdwt_err;		/* status of last disk i/o request */

extern _PROTOTYPE (int (*call_vec[]), (void) ); /* sys call table */



