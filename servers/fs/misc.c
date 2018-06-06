/* This file contains a collection of miscellaneous procedures.  Some of them
 * perform simple system calls.  Some others do a little part of system calls
 * that are mostly performed by the Memory Manager.
 *
 * The entry points into this file are
 *   do_dup:    perform the DUP system call
 *   do_fcntl:    perform the FCNTL system call
 *   do_sync:   perform the SYNC system call
 *   do_fsync:    perform the FSYNC system call
 *   do_reboot:   sync disks and prepare for shutdown
 *   do_fork:   adjust the tables after MM has performed a FORK system call
 *   do_exec:   handle files with FD_CLOEXEC on after MM has done an EXEC
 *   do_exit:   a process has exited; note that in the tables
 *   do_set:    set uid or gid for some process
 *   do_revive:   revive a process that was waiting for something (e.g. TTY)
 *   do_svrctl:   file system control
 *   do_getsysinfo: request copy of FS data structure
 */

// #define SVRDBG    1

#include "fs.h"

/*===========================================================================*
*        do_getsysinfo            *
*===========================================================================*/
int do_getsysinfo()
{
  struct fproc *proc_addr;
  vir_bytes src_addr, dst_addr;
  mnx_size_t len;
  int s;

  switch (m_in.info_what) {
  case SI_FSPROC_ADDR:
    proc_addr = &fproc[0];
    src_addr = (vir_bytes) &proc_addr;
    len = sizeof(struct fproc *);
    break;
  case SI_FSPROC_TAB:
    src_addr = (vir_bytes) fproc;
    len = sizeof(struct fproc) * dc_ptr->dc_nr_procs;
    break;
  case SI_DMAP_TAB:
    src_addr = (vir_bytes) dmap_tab;
    len = sizeof(dmap_t) * NR_DEVICES;
    break;
  default:
    return (EMOLINVAL);
  }

  dst_addr = m_in.info_where;
  // if (OK != (s=sys_datacopy(SELF, src_addr, who_e, dst_addr, len)))
  if (OK != (s = mnx_vcopy(SELF, src_addr, who_e, dst_addr, len)))
    return (s);
  return (OK);

}

/*===========================================================================*
 *        do_dup               *
 *===========================================================================*/
int do_dup()
{
/* Perform the dup(fd) or dup2(fd,fd2) system call. These system calls are
 * obsolete.  In fact, it is not even possible to invoke them using the
 * current library because the library routines call fcntl().  They are
 * provided to permit old binary programs to continue to run.
 */

  register int rfd;
  register struct filp *f;
  struct filp *dummy;
  int r;

  /* Is the file descriptor valid? */
  rfd = m_in.fd & ~DUP_MASK;    /* kill off dup2 bit, if on */
  if ((f = get_filp(rfd)) == NIL_FILP) return(err_code);

  /* Distinguish between dup and dup2. */
  if (m_in.fd == rfd) {     /* bit not on */
  /* dup(fd) */
  if ( (r = get_fd(0, 0, &m_in.fd2, &dummy)) != OK) return(r);
  } else {
  /* dup2(fd, fd2) */
  if (m_in.fd2 < 0 || m_in.fd2 >= MNX_OPEN_MAX) return(EMOLBADF);
  if (rfd == m_in.fd2) return(m_in.fd2);  /* ignore the call: dup2(x, x) */
  m_in.fd = m_in.fd2;   /* prepare to close fd2 */
  (void) do_close();  /* cannot fail */
  }

  /* Success. Set up new file descriptors. */
  f->filp_count++;
  fp->fp_filp[m_in.fd2] = f;
  FD_SET(m_in.fd2, &fp->fp_filp_inuse);
  return(m_in.fd2);
}

/*===========================================================================*
 *        do_fcntl             *
 *===========================================================================*/
int do_fcntl()
{
  /* Perform the fcntl(fd, request, ...) system call. */

  register struct filp *f;
  int new_fd, r, fl;
  long cloexec_mask;    /* bit map for the FD_CLOEXEC flag */
  long clo_value;   /* FD_CLOEXEC flag in proper position */
  struct filp *dummy;

  SVRDEBUG("m_in.fd is=%d\n", m_in.fd);
  /* Is the file descriptor valid? */
  if ((f = get_filp(m_in.fd)) == NIL_FILP) ERROR_RETURN(err_code);

  switch (m_in.request) {
		case F_DUPFD:
			/* This replaces the old dup() system call. */
			if (m_in.addr < 0 || m_in.addr >= MNX_OPEN_MAX) 
				ERROR_RETURN (EMOLINVAL);
			if ((r = get_fd(m_in.addr, 0, &new_fd, &dummy)) != OK) 
				ERROR_RETURN (r);
			f->filp_count++;
			fp->fp_filp[new_fd] = f;
			return (new_fd);

		case F_GETFD:
			/* Get close-on-exec flag (FD_CLOEXEC in POSIX Table 6-2). */
			return ( ((fp->fp_cloexec >> m_in.fd) & 01) ? FD_CLOEXEC : 0);

		case F_SETFD:
			/* Set close-on-exec flag (FD_CLOEXEC in POSIX Table 6-2). */
			cloexec_mask = 1L << m_in.fd; /* singleton set position ok */
			clo_value = (m_in.addr & FD_CLOEXEC ? cloexec_mask : 0L);
			fp->fp_cloexec = (fp->fp_cloexec & ~cloexec_mask) | clo_value;
			return (OK);
		case F_GETFL:
			/* Get file status flags (O_NONBLOCK and O_APPEND). */
			fl = f->filp_flags & (O_NONBLOCK | O_APPEND | O_ACCMODE);
			return (fl);
		case F_SETFL:
			/* Set file status flags (O_NONBLOCK and O_APPEND). */
			fl = O_NONBLOCK | O_APPEND;
			f->filp_flags = (f->filp_flags & ~fl) | (m_in.addr & fl);
			return (OK);
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			/* Set or clear a file lock. */
			r = lock_op(f, m_in.request);
			return (r);

		case F_FREESP:	{
			/* Free a section of a file. Preparation is done here,
			 * actual freeing in freesp_inode().
			 */
			mnx_off_t start, end;
			struct flock flock_arg;
			signed long offset;

			/* Check if it's a regular file. */
			if ((f->filp_ino->i_mode & I_TYPE) != I_REGULAR) {
				ERROR_RETURN(EMOLINVAL);
			}

			/* Copy flock data from userspace. */
			// if((r = sys_datacopy(who_e, (vir_bytes) m_in.name1,
			//   SELF, (vir_bytes) &flock_arg,
			//   (phys_bytes) sizeof(flock_arg))) != OK)
			if ((r = mnx_vcopy(who_e, m_in.name1, SELF, &flock_arg, sizeof(flock_arg))) != OK)
				ERROR_RETURN(r);

			/* Convert starting offset to signed. */
			offset = (signed long) flock_arg.l_start;

			/* Figure out starting position base. */
			switch (flock_arg.l_whence) {
				case SEEK_SET: 
					start = 0; 
					if (offset < 0) 
						ERROR_RETURN(EMOLINVAL); 
					break;
				case SEEK_CUR: 
					start = f->filp_pos; 
					break;
				case SEEK_END: 
					start = f->filp_ino->i_size; 
					break;
				default: 
					ERROR_RETURN(EMOLINVAL);
			}

			/* Check for overflow or underflow. */
			if (offset > 0 && start + offset < start)
				ERROR_RETURN(EMOLINVAL);
			if (offset < 0 && start + offset > start) 
				ERROR_RETURN(EMOLINVAL);
			start += offset;
			if (flock_arg.l_len > 0) {
				end = start + flock_arg.l_len;
				if (end <= start) {
					ERROR_RETURN(EMOLINVAL);
				}
				r = freesp_inode(f->filp_ino, start, end);
			} else {
				r = truncate_inode(f->filp_ino, start);
			}
			return r;
		}
		default:
			ERROR_RETURN(EMOLINVAL);
  }
}

/*===========================================================================*
 *        do_sync              *
 *===========================================================================*/
int do_sync()
{
  /* Perform the sync() system call.  Flush all the tables.
   * The order in which the various tables are flushed is critical.  The
   * blocks must be flushed last, since rw_inode() leaves its results in
   * the block cache.
   */
  register struct inode *rip;
  register struct buf *bp;

  /* Write all the dirty inodes to the disk. */
  for (rip = &inode[0]; rip < &inode[NR_INODES]; rip++)
    if (rip->i_count > 0 && rip->i_dirt == DIRTY) rw_inode(rip, WRITING);

// SVRDEBUG("Llamada a FLUSHALL desde do_sync ------//////////////////////////////// ");

  /* Write all the dirty blocks to the disk, one drive at a time. */
  for (bp = &buf[0]; bp < &buf[NR_BUFS]; bp++)
    if (bp->b_dev != NO_DEV && bp->b_dirt == DIRTY) flushall(bp->b_dev);

  return (OK);  /* sync() can't fail */
}

/*===========================================================================*
 *        do_fsync             *
 *===========================================================================*/
int do_fsync()
{
  /* Perform the fsync() system call. For now, don't be unnecessarily smart. */

  do_sync();

  return (OK);
}


/*===========================================================================*
 *        do_set               *
 *===========================================================================*/
int do_set()
{
  /* Set uid_t or gid_t field. */

  register struct fproc *tfp;
  int proc;

  /* Only PM may make this call directly. */
  if (who_e != PM_PROC_NR) return (EMOLGENERIC);

  okendpt(m_in.endpt1, &proc);
  tfp = &fproc[proc];
  if (call_nr == MOLSETUID) {
    tfp->fp_realuid = (mnx_uid_t) m_in.real_user_id;
    tfp->fp_effuid =  (mnx_uid_t) m_in.eff_user_id;
  }
  if (call_nr == MOLSETGID) {
    tfp->fp_effgid =  (mnx_gid_t) m_in.eff_grp_id;
    tfp->fp_realgid = (mnx_gid_t) m_in.real_grp_id;
  }
  return (OK);
}

/*===========================================================================*
 *        do_revive            *
 *===========================================================================*/
int do_revive()
{
  /* A driver, typically TTY, has now gotten the characters that were needed for
   * a previous read.  The process did not get a reply when it made the call.
   * Instead it was suspended.  Now we can send the reply to wake it up.  This
   * business has to be done carefully, since the incoming message is from
   * a driver (to which no reply can be sent), and the reply must go to a process
   * that blocked earlier.  The reply to the caller is inhibited by returning the
   * 'SUSPEND' pseudo error, and the reply to the blocked process is done
   * explicitly in revive().
   */
  revive(m_in.REP_ENDPT, m_in.REP_STATUS);
  return (SUSPEND);   /* don't reply to the TTY task */
}

/*===========================================================================*
 *        do_svrctl            *
 *===========================================================================*/
int do_svrctl()
{
  switch (m_in.svrctl_req) {
  case FSSIGNON: {
    /* A server in user space calls in to manage a device. */
    struct fssignon device;
    int r, major, proc_nr_n;

    if (fp->fp_effuid != SU_UID && fp->fp_effuid != SERVERS_UID)
      return (EMOLPERM);

    /* Try to copy request structure to FS. */
    // if ((r = sys_datacopy(who_e, (vir_bytes) m_in.svrctl_argp,
    //                       FS_PROC_NR, (vir_bytes) &device,
    //                       (phys_bytes) sizeof(device))) != OK)
    if ((r = mnx_vcopy(who_e, m_in.svrctl_argp,
                          FS_PROC_NR, &device,
                          sizeof(device))) != OK)
      return (r);

    if (isokendpt(who_e, &proc_nr_n) != OK)
      return (EMOLINVAL);

    /* Try to update device mapping. */
    major = (device.dev >> MNX_MAJOR) & BYTE;
    r = map_driver(major, who_e, device.style);
    if (r == OK)
    {
      /* If a driver has completed its exec(), it can be announced
       * to be up.
      */
      if (fproc[proc_nr_n].fp_execced) {
        dev_up(major);
      } else {
        dmap_tab[major].dmap_flags |= DMAP_BABY;
      }
    }

    return (r);
  }
  case FSDEVUNMAP: {
    struct fsdevunmap fdu;
    int r, major;
    /* Try to copy request structure to FS. */
    if ((r = mnx_vcopy(who_e,  m_in.svrctl_argp,
                       FS_PROC_NR, &fdu,
                       sizeof(fdu))) != OK)
      return (r);
    major = (fdu.dev >> MNX_MAJOR) & BYTE;
    r = map_driver(major, NONE, 0);
    return (r);
  }
  default:
    return (EMOLINVAL);
  }
}