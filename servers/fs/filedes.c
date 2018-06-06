/* This file contains the procedures that manipulate file descriptors.
 *
 * The entry points into this file are
 *   get_fd:	 look for free file descriptor and free filp slots
 *   get_filp:	 look up the filp entry for a given file descriptor
 *   find_filp:	 find a filp slot that points to a given inode
 *   inval_filp: invalidate a filp and associated fd's, only let close()
 *               happen on it
 */

// #define SVRDBG    1

#include "fs.h"

 /*===========================================================================*
 *				get_fd					     *
 *===========================================================================*/
int get_fd(int start, mnx_mode_t bits, int *k, struct filp **fpt)
{
/* Look for a free file descriptor and a free filp slot.  Fill in the mode word
 * in the latter, but don't claim either one yet, since the open() or creat()
 * may yet fail.
 */

	register struct filp *f;
	register int i;

	*k = -1;			/* we need a way to tell if file desc found */
	SVRDEBUG("start=%d mode=%X\n", start,bits);

	/* Search the fproc fp_filp table for a free file descriptor. */
	for (i = start; i < MNX_OPEN_MAX; i++) {
		if (fp->fp_filp[i] == NIL_FILP && !FD_ISSET(i, &fp->fp_filp_inuse)) {
			/* A file descriptor has been located. */
			SVRDEBUG("FD located is=%d\n", i);
			*k = i;
			break;
		}
	}

  /* Check to see if a file descriptor has been found. */
  if (*k < 0) return(EMOLMFILE);	/* this is why we initialized k to -1 */

  /* Now that a file descriptor has been found, look for a free filp slot. */
  for (f = &filp[0]; f < &filp[NR_FILPS]; f++) {
	if (f->filp_count == 0) {
		f->filp_mode = bits;
		f->filp_pos = 0L;
		f->filp_selectors = 0;
		f->filp_select_ops = 0;
		f->filp_pipe_select_ops = 0;
		f->filp_flags = 0;
		*fpt = f;
		return(OK);
	}
  }

  /* If control passes here, the filp table must be full.  Report that back. */
  return(EMOLNFILE);
}

/*===========================================================================*
 *				get_filp				     *
 *===========================================================================*/
struct filp *get_filp(int fild)
//int fild;			/* file descriptor */
{
/* See if 'fild' refers to a valid file descr.  If so, return its filp ptr. */

  err_code = EMOLBADF;
  SVRDEBUG("fild is=%d\n",fild);
  if (fild < 0 || fild >= MNX_OPEN_MAX ) return(NIL_FILP);
  return(fp->fp_filp[fild]);	/* may also be NIL_FILP */
}

/*===========================================================================*
 *				find_filp				     *
 *===========================================================================*/
struct filp *find_filp(register struct inode *rip, mnx_mode_t bits)
{
/* Find a filp slot that refers to the inode 'rip' in a way as described
 * by the mode bit 'bits'. Used for determining whether somebody is still
 * interested in either end of a pipe.  Also used when opening a FIFO to
 * find partners to share a filp field with (to shared the file position).
 * Like 'get_fd' it performs its job by linear search through the filp table.
 */

  register struct filp *f;

  for (f = &filp[0]; f < &filp[NR_FILPS]; f++) {
	if (f->filp_count != 0 && f->filp_ino == rip && (f->filp_mode & bits)){
		return(f);
	}
  }

  /* If control passes here, the filp wasn't there.  Report that back. */
  return(NIL_FILP);
}

/*===========================================================================*
 *        inval_filp             *
 *===========================================================================*/
int inval_filp(struct filp *fp)
{
  int f, fid, n = 0;
  for(f = 0; f < dc_ptr->dc_nr_procs; f++) {
    if(fproc[f].fp_pid == PID_FREE) continue;
    for(fid = 0; fid < MNX_OPEN_MAX; fid++) {
      if(fproc[f].fp_filp[fid] && fproc[f].fp_filp[fid] == fp) {
        fproc[f].fp_filp[fid] = NIL_FILP;
        n++;
      }
    }
  }

  return n;
}