 /*This file contains a few general purpose utility routines.
 *
 * The entry points into this file are
 *   clock_time:  ask the clock task for the real time
 *   copy:	  copy a block of data
 *   fetch_name:  go get a path name from user space
 *   no_sys:      reject a system call that FS does not handle
 *   panic:       something awful has occurred;  MINIX cannot continue
 *   conv2:	  do byte swapping on a 16-bit int
 *   conv4:	  do byte swapping on a 32-bit long
 */

// #define SVRDBG    	1
// #define INFODBG 	1
#define UTILITY_C   1
#include "fs.h"
#include <sys/stat.h>
#include <sys/statfs.h>
int panicking;

/*===========================================================================*
 *				no_sys					     *
 *===========================================================================*/
int no_sys(void)
{

SVRDEBUG("A system call number not implemented by FS has been requested from %d\n", who_e);

  ERROR_RETURN(EMOLNOSYS);
}

/*===========================================================================*
 *				fproc_init			     *
 *===========================================================================*/
void fproc_init(int p_nr)
{
	fproc_t *fp;
	mproc_t *mp;	

	fp = &fproc[p_nr];
	mp = &mproc[p_nr];
	fp->fp_realuid		    = (mnx_uid_t) SU_UID;
	fp->fp_effuid		    = (mnx_uid_t) SU_UID; //Defino esto para que el userid sea ROOT para todos los casos
	fp->fp_realgid		    = (mnx_gid_t) SYS_GID;
	fp->fp_effgid		    = (mnx_gid_t) SYS_GID;
	fp->fp_umask 	        = ~0;
	fp->fp_tty		        = -1;
	fp->fp_fd		        = -1;
	fp->fp_buffer		    = NULL;
	fp->fp_nbytes		    = 0;
	fp->fp_cum_io_partial	= 0;
	fp->fp_suspended	    = 0;
	fp->fp_revived		    = 0;
	fp->fp_task		        = HARDWARE;
	fp->fp_sesldr		    = 0;
	fp->fp_execced		    = 0;
	fp->fp_pid		        = PROC_NO_PID;
	fp->fp_cloexec		    = 0;
	fp->fp_endpoint		    = NONE;
	
	if( mp->mp_flags != 0) {
		SVRDEBUG(PM_PROC_FORMAT,PM_PROC_FIELDS(mp));
		fp->fp_endpoint	= mp->mp_endpoint;
		fp->fp_pid		= mp->mp_pid;
	}
}

// /*===========================================================================*
//  *				load_image				     *
//  *===========================================================================*/
// int load_image(char *img_name)
// {
// 	int rcode, img_fd, bytes, blocks, total;
// 	struct stat img_stat;
// 	char *ptr;

// // SVRDEBUG("image name=%s\n", img_name);


// 	/* get the image file size */
// 	rcode = stat(img_name,  &img_stat);
// 	if(rcode) ERROR_EXIT(errno);

// // SVRDEBUG("image size=%d[bytes]\n", img_stat.st_size);
// // SVRDEBUG("block size=%d[bytes]\n", img_stat.st_blksize);
// 	img_size = img_stat.st_size;

// 	/* alloc dynamic memory for image file size */
// // SVRDEBUG("Alloc dynamic memory for disk image file bytes=%d\n", img_size);
// 	posix_memalign( (void**) &img_ptr, getpagesize(), (img_size+getpagesize()));
// 	if(img_ptr == NULL) ERROR_EXIT(errno);

// 	/* Try to open the disk image */
// 	img_fd = open(img_name, O_RDONLY);
// 	if(img_fd < 0) ERROR_EXIT(errno);
// // SVRDEBUG("FD de archivo imagen (Memoria - Disco RAM): %d \n", img_fd);

// 	/* dump the image file into the allocated memory */
// 	ptr = img_ptr;
// 	blocks = 0;
// 	total = 0;
// 	while( (bytes = read(img_fd, ptr, img_stat.st_blksize)) > 0 ) {
// 		blocks++;
// 		total += bytes;
// 		ptr += bytes;
// 	}
// // SVRDEBUG("blocks read=%d bytes read=%d\n", blocks, total);

// 	/* close the disk image */
// // SVRDEBUG("Cerrando FD (Memoria - Disco RAM): %d \n", img_fd);
// 	rcode = close(img_fd);
// 	if(rcode) ERROR_EXIT(errno);

// #define BLOCK_SIZE	1024

// 	sb_ptr = (struct super_block *) (img_ptr + BLOCK_SIZE);

// // SVRDEBUG(SUPER_BLOCK_FORMAT1, SUPER_BLOCK_FIELDS1(sb_ptr));

//   return(OK);
// }



/*===========================================================================*
 *				fetch_name				     *
 *===========================================================================*/
int fetch_name(char *path, int len, int flag)
{
/* Go get path and put it in 'user_path'.
 * If 'flag' = M3 and 'len' <= M3_STRING, the path is present in 'message'.
 * If it is not, go copy it from user space.
 */
  register char *rpu, *rpm;
  int r;

//   SVRDEBUG("Entro a fetchNAME\n");
//   SVRDEBUG("path=%s len=%d flag=%d\n",  path, len, flag);

  /* Check name length for validity. */
  if (len <= 0) {
	err_code = EMOLINVAL;
	ERROR_RETURN(EMOLGENERIC);
  }
  if (len > MNX_PATH_MAX) {
	err_code = EMOLNAMETOOLONG;
	ERROR_RETURN(EMOLGENERIC);
  }

	// SVRDEBUG("M3=%d\n", M3);
	// SVRDEBUG("M3_STRING=%d\n", M3_STRING);
	if (flag == M3 && len <= M3_STRING) {
		/* Just copy the path from the message to 'user_path'. */
		rpu = &user_path[0];
		rpm = m_in.pathname;		/* contained in input message */
		do { *rpu++ = *rpm++; } while (--len);
		r = OK;
	} else {
		/* String is not contained in the message.  Get it from user space. */
		//r = mnx_vcopy(who_e, path, FS_PROC_NR, user_path, (len+1)); Ver si se hace del lado de OPEN.
		r = mnx_vcopy(who_e, path, SELF, user_path, (phys_bytes) len);
	}
	// SVRDEBUG("user_path=%s\n", user_path);
	// SVRDEBUG("r =%d\n", r);
  return(r);
}

/*===========================================================================*
 *				conv2					     *
 *===========================================================================*/
unsigned conv2(int norm, int w)
// int norm;			/* TRUE if no swap, FALSE for byte swap */
// int w;				/* promotion of 16-bit word to be swapped */
{
/* Possibly swap a 16-bit word between 8086 and 68000 byte order. */
  if (norm) return( (unsigned) w & 0xFFFF);
  return( ((w&BYTE) << 8) | ( (w>>8) & BYTE));
}

/*===========================================================================*
 *				conv4					     *
 *===========================================================================*/
long conv4(int norm, long x)
//int norm;			/* TRUE if no swap, FALSE for byte swap */
//long x;				/* 32-bit long to be byte swapped */
{
/* Possibly swap a 32-bit long between 8086 and 68000 byte order. */
  unsigned lo, hi;
  long l;

  if (norm) return(x);			/* byte order was already ok */
  lo = conv2(FALSE, (int) x & 0xFFFF);	/* low-order half, byte swapped */
  hi = conv2(FALSE, (int) (x>>16) & 0xFFFF);	/* high-order half, swapped */
  l = ( (long) lo <<16) | hi;
  return(l);
}

/*===========================================================================*
 *				panic					     *
 *===========================================================================*/
void panic(char *who, char *mess, int num)
//char *who;			/* who caused the panic */
//char *mess;			/* panic message string */
//int num;			/* number to go with it */
{
/* Something awful has happened.  Panics are caused when an internal
 * inconsistency is detected, e.g., a programming error or illegal value of a
 * defined constant.
 */
  if (panicking) return;	/* do not panic during a sync */
  panicking = TRUE;		/* prevent another panic during the sync */

  INFODEBUG("FS panic (%s): %s ", who, mess);
  if (num != NO_NUM) INFODEBUG("%d",num);
  (void) do_sync();		/* flush everything to the disk */
//   sys_exit(SELF);
	ERROR_EXIT(SELF);
}

/*===========================================================================*
 *				clock_time				     *
 *===========================================================================*/
mnx_time_t clock_time()
{
/* This routine returns the time in seconds since 1.1.1970.  MINIX is an
 * astrophysically naive system that assumes the earth rotates at a constant
 * rate and that such things as leap seconds do not exist.
 */
//   register int k;
  clock_t uptime;

  //if ( (k=getuptime(&uptime)) != OK) panic(__FILE__,"clock_time err", k);
  //if ( (k=time(&uptime)) != OK) {panic(__FILE__,"clock_time err", k); SVRDEBUG("\n");}
  //return( (mnx_time_t) (boottime + (uptime/HZ)));
  time(&uptime);
  // return (mnx_time_t) uptime;
  return( (mnx_time_t) (boottime + (uptime/MINIX_HZ)));
}

/*===========================================================================*
 *				isokendpt_f				     *
 *===========================================================================*/
int isokendpt_f(char *file, int line, int endpoint, int *proc, int fatal)
{
	int failed = 0;
	*proc = _ENDPOINT_P(endpoint);

 SVRDEBUG("endpoint %d\n", endpoint);
 SVRDEBUG("*proc %d\n", *proc);

	if(*proc < 0 || *proc >= dc_ptr->dc_nr_procs) {
		INFODEBUG("FS:%s:%d: proc (%d) from endpoint (%d) out of range\n",
			file, line, *proc, endpoint);
		failed = 1;
	} else if(fproc[*proc].fp_endpoint != endpoint) {
		INFODEBUG("FS:%s:%d: proc (%d) from endpoint (%d) doesn't match "
			"known endpoint (%d)\n",
			file, line, *proc, endpoint, fproc[*proc].fp_endpoint);
		failed = 1;
	}

	if(failed && fatal)
		panic(__FILE__, "isokendpt_f failed", NO_NUM);

	return failed ? EMOLDEADSRCDST : OK;
}
