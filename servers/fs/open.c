/* This file contains the procedures for creating, opening, closing, and
 * seeking on files.
 *
 * The entry points into this file are
 *   do_creat:  perform the CREAT system call
 *   do_open: perform the OPEN system call
 *   do_mknod:  perform the MKNOD system call
 *   do_mkdir:  perform the MKDIR system call
 *   do_close:  perform the CLOSE system call
 *   do_lseek:  perform the LSEEK system call
 */
// #define SVRDBG    1

#include "fs.h"

#define offset m2_l1

char mode_map[] = {R_BIT, W_BIT, R_BIT | W_BIT, 0};
int common_open(int oflags, mnx_mode_t omode);

/*===========================================================================*
 *        do_creat             *
 *===========================================================================*/
int do_creat()
{
  /* Perform the creat(name, mode) system call. */
  int r;

  if (fetch_name(m_in.name, m_in.name_length, M3) != OK) return (err_code);
  r = common_open(O_WRONLY | O_CREAT | O_TRUNC, (mnx_mode_t) m_in.mode);
  return (r);
}
/*===========================================================================*
 *        do_open              *
 *===========================================================================*/
int do_open()
{
  /* Perform the open(name, flags,...) system call. */

  int create_mode = 0;    /* is really mode_t but this gives problems */
  int r;

// SVRDEBUG("who_e=%d name=%p fs_ep=%d user_path=%s len=%d\n",
//  who_e, m_in.name1, FS_PROC_NR, user_path, m_in.name1_length);

// SVRDEBUG("who_e=%d name=%p fs_ep=%d user_path=%s len=%d\n",
//  who_e, m_in.name, FS_PROC_NR, user_path, m_in.name_length);

// SVRDEBUG("m_in.mode=%d\n", m_in.mode);
// SVRDEBUG("m_in.c_name=%s\n", m_in.c_name);
// SVRDEBUG("m_in.name=%s\n", m_in.name);
// SVRDEBUG("O_CREAT=%d O_CREAT(OCT)=%#o\n", O_CREAT, O_CREAT);
  /* If O_CREAT is set, open has three parameters, otherwise two. */
  if (m_in.mode & O_CREAT) {
    create_mode = m_in.c_mode;
    SVRDEBUG("M1\n");
    r = fetch_name(m_in.c_name, m_in.name1_length, M1);
  }
  else {
//SVRDEBUG("flag=%s\n", flag);
    SVRDEBUG("M3\n");
// SVRDEBUG("m_in.name=%s\n", m_in.name);
// SVRDEBUG("m_in.name_length=%d\n", m_in.name_length);
    r = fetch_name(m_in.name, m_in.name_length, M3);
  }

// SVRDEBUG("who_e=%d name=%p fs_ep=%d user_path=%s len=%d\n",
//   who_e, m_in.c_name, FS_PROC_NR, user_path, m_in.name1_length);

// SVRDEBUG("create_mode=%d\n",create_mode);
  if (r != OK) ERROR_RETURN(err_code); /* name was bad */
  r = common_open(m_in.mode, create_mode);
  return (r);
}

/*===========================================================================*
 *        common_open            *
 *===========================================================================*/
int common_open(int oflags, mnx_mode_t omode)
{
  /* Common code from do_creat and do_open. */

  struct inode *rip, *ldirp;
  int r, b, exist = TRUE;
  mnx_dev_t dev;
  mnx_mode_t bits;
  mnx_off_t pos;
  struct filp *fil_ptr, *filp2;

  SVRDEBUG("oflags=%X omode=%X\n", oflags, omode);
  /* Remap the bottom two bits of oflags. */
  bits = (mnx_mode_t) mode_map[oflags & O_ACCMODE];
  /* See if file descriptor and filp slots are available. */
  if ( (r = get_fd(0, bits, &m_in.fd, &fil_ptr)) != OK) ERROR_RETURN(r);

  SVRDEBUG("m_in.fd=%d oflags=%X \n", m_in.fd, oflags);

  /* If O_CREATE is set, try to make the file. */
  if (oflags & O_CREAT) {
    /* Create a new inode by calling new_node(). */
    SVRDEBUG("user_path=%s\n", user_path);
    omode = I_REGULAR | (omode & ALL_MODES & fp->fp_umask);
    rip = new_node(&ldirp, user_path, omode, NO_ZONE, oflags & O_EXCL, NULL);
    r = err_code;
    put_inode(ldirp);
    if (r == OK) exist = FALSE;      /* we just created the file */
    else if (r != EMOLEXIST) {ERROR_RETURN(r);} /* other error */
    else {exist = !(oflags & O_EXCL);} /* file exists, if the O_EXCL
              flag is set this is an error */
  } else {
    /* Scan path name. */
    SVRDEBUG("user_path=%s\n", user_path);
    if ( (rip = eat_path(user_path)) == NIL_INODE) {ERROR_RETURN(err_code);}
    // SVRDEBUG("rip->i_zone[0] =%d \n", (mnx_dev_t) rip->i_zone[0]);
  }

// SVRDEBUG(INODE_FORMAT1, INODE_FIELDS1(rip));
  /* Claim the file descriptor and filp slot and fill them in. */
  fp->fp_filp[m_in.fd] = fil_ptr;
  FD_SET(m_in.fd, &fp->fp_filp_inuse);
  fil_ptr->filp_count = 1;
  fil_ptr->filp_ino = rip;
  fil_ptr->filp_flags = oflags;

  SVRDEBUG("FD: m_in.fd=%d \n", m_in.fd);
  SVRDEBUG(FS_FILP_FORMAT, FS_FILP_FIELDS(fil_ptr));

  SVRDEBUG("exist=%X \n", exist);

  /* Only do the normal open code if we didn't just create the file. */
  if (exist) {
    // /* Check protections. */
    if ((r = forbidden(rip, bits)) == OK) {
      SVRDEBUG("r forbidden = %d\n", r);
      /* Opening reg. files directories and special files differ. */
      switch (rip->i_mode & I_TYPE) {
      case I_REGULAR:
        /* Truncate regular file if O_TRUNC. */
        SVRDEBUG(" I_REGULAR\n");
        if (oflags & O_TRUNC) {
          if ((r = forbidden(rip, W_BIT)) != OK) {/*SVRDEBUG("r forbidden !=OK %d\n", r); */ break;}
          // SVRDEBUG("truncate_inode %d\n", 0);
          truncate_inode(rip, 0);
          // SVRDEBUG("wipe_inode %d\n", 0);
          wipe_inode(rip);
          /* Send the inode from the inode cache to the
           * block cache, so it gets written on the next
           * cache flush.
           */
		// SVRDEBUG("WRITING=%d \n", WRITING);
          rw_inode(rip, WRITING);
        }
        break;
      case I_DIRECTORY:
        // SVRDEBUG(" I_DIRECTORY dev %d\n", dev);
        /* Directories may be read but not written. */
        r = (bits & W_BIT ? EMOLISDIR : OK);
        SVRDEBUG("r=%d \n", r);
        break;
      case I_CHAR_SPECIAL:
      case I_BLOCK_SPECIAL:
        dev = (mnx_dev_t) rip->i_zone[0];
        SVRDEBUG(" I_CHAR_SPECIAL/I_BLOCK_SPECIAL dev %d\n", dev);
        /* Invoke the driver for special processing. */
        //dev = rip->i_dev;//TODO: Ver si este reemplazo es correcto!!!!!!
        r = dev_open(dev, who_e, bits | (oflags & ~O_ACCMODE));
        SVRDEBUG("r=%d\n", r);
        break;
#ifdef ANULADO
      case I_NAMED_PIPE:
        oflags |= O_APPEND; /* force append mode */
        fil_ptr->filp_flags = oflags;
        r = pipe_open(rip, bits, oflags);
        if (r != ENXIO) {
          /* See if someone else is doing a rd or wt on
           * the FIFO.  If so, use its filp entry so the
           * file position will be automatically shared.
           */
          b = (bits & R_BIT ? R_BIT : W_BIT);
          fil_ptr->filp_count = 0; /* don't find self */
          if ((filp2 = find_filp(rip, b)) != NIL_FILP) {
            /* Co-reader or writer found. Use it.*/
            fp->fp_filp[m_in.fd] = filp2;
            filp2->filp_count++;
            filp2->filp_ino = rip;
            filp2->filp_flags = oflags;

            /* i_count was incremented incorrectly
             * by eatpath above, not knowing that
             * we were going to use an existing
             * filp entry.  Correct this error.
             */
            rip->i_count--;
          } else {
            /* Nobody else found.  Restore filp. */
            fil_ptr->filp_count = 1;
            if (b == R_BIT)
              pos = rip->i_zone[V2_NR_DZONES + 0];
            else
              pos = rip->i_zone[V2_NR_DZONES + 1];
            fil_ptr->filp_pos = pos;
          }
        }
        break;
#endif /* ANULADO */
      }
    }
  }

	/* If error, release inode. */
	if (r < OK) {
		if (r == SUSPEND) ERROR_RETURN(r);   /* Oops, just suspended */
		fp->fp_filp[m_in.fd] = NIL_FILP;
		FD_CLR(m_in.fd, &fp->fp_filp_inuse);
		fil_ptr->filp_count = 0;
		put_inode(rip);
		ERROR_RETURN(r);
	}
	SVRDEBUG("m_in.fd=%d \n", m_in.fd);
	return (m_in.fd);
}

/*===========================================================================*
 *        new_node             *
 *===========================================================================*/
struct inode *new_node(struct inode **ldirp, char *path, mnx_mode_t bits, mnx_zone_t z0, int opaque, char *parsed)
{
  /* New_node() is called by common_open(), do_mknod(), and do_mkdir().
   * In all cases it allocates a new inode, makes a directory entry for it on
   * the path 'path', and initializes it.  It returns a pointer to the inode if
   * it can do this; otherwise it returns NIL_INODE.  It always sets 'err_code'
   * to an appropriate value (OK or an error code).
   *
   * The parsed path rest is returned in 'parsed' if parsed is nonzero. It
   * has to hold at least NAME_MAX bytes.
   */

  register struct inode *rip;
  register int r;
  char string[NAME_MAX];

  *ldirp = parse_path(path, string, opaque ? LAST_DIR : LAST_DIR_EATSYM);
  if (*ldirp == NIL_INODE) return (NIL_INODE);

  /* The final directory is accessible. Get final component of the path. */
  rip = advance(ldirp, string);

  if (S_ISDIR(bits) &&
      (*ldirp)->i_nlinks >= ((*ldirp)->i_sp->s_version == V1 ?
                             CHAR_MAX : SHRT_MAX)) {
    /* New entry is a directory, alas we can't give it a ".." */
    put_inode(rip);
    err_code = EMLINK;
    return (NIL_INODE);
  }

  if ( rip == NIL_INODE && err_code == EMOLNOENT) {
    /* Last path component does not exist.  Make new directory entry. */
    if ( (rip = alloc_inode((*ldirp)->i_dev, bits)) == NIL_INODE) {
      /* Can't creat new inode: out of inodes. */
      return (NIL_INODE);
    }

    /* Force inode to the disk before making directory entry to make
     * the system more robust in the face of a crash: an inode with
     * no directory entry is much better than the opposite.
     */
    rip->i_nlinks++;
    rip->i_zone[0] = z0;    /* major/minor device numbers */
    rw_inode(rip, WRITING);   /* force inode to disk now */

    /* New inode acquired.  Try to make directory entry. */
    if ((r = search_dir(*ldirp, string, &rip->i_num, ENTER)) != OK) {
      rip->i_nlinks--;  /* pity, have to free disk inode */
      rip->i_dirt = DIRTY;  /* dirty inodes are written out */
      put_inode(rip); /* this call frees the inode */
      err_code = r;
      return (NIL_INODE);
    }

  } else {
    /* Either last component exists, or there is some problem. */
    if (rip != NIL_INODE)
      r = EMOLEXIST;
    else
      r = err_code;
  }

  if (parsed) { /* Give the caller the parsed string if requested. */
    strncpy(parsed, string, NAME_MAX - 1);
    parsed[NAME_MAX - 1] = '\0';
  }

  /* The caller has to return the directory inode (*ldirp).  */
  err_code = r;
  return (rip);
}

/*===========================================================================*
 *        do_mknod             *
 *===========================================================================*/
int do_mknod()
{
  /* Perform the mknod(name, mode, addr) system call. */

  register mnx_mode_t bits, mode_bits;
  struct inode *ip, *ldirp;

  /* Only the super_user may make nodes other than fifos. */
  mode_bits = (mnx_mode_t) m_in.mk_mode;    /* mode of the inode */
  if (!super_user && ((mode_bits & I_TYPE) != I_NAMED_PIPE)) return (EMOLPERM);
  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK) return (err_code);
  bits = (mode_bits & I_TYPE) | (mode_bits & ALL_MODES & fp->fp_umask);
  ip = new_node(&ldirp, user_path, bits, (mnx_zone_t) m_in.mk_z0, TRUE, NULL);

  SVRDEBUG(INODE_FORMAT1, INODE_FIELDS1(ip));
  SVRDEBUG(INODE_FORMAT1, INODE_FIELDS1(ldirp));

  put_inode(ip);
  put_inode(ldirp);
  return (err_code);
}

/*===========================================================================*
 *        do_mkdir             *
 *===========================================================================*/
int do_mkdir()
{
  /* Perform the mkdir(name, mode) system call. */

  int r1, r2;     /* status codes */
  mnx_ino_t dot, dotdot;    /* inode numbers for . and .. */
  mnx_mode_t bits;      /* mode bits for the new inode */
  char string[NAME_MAX];  /* last component of the new dir's path name */
  struct inode *rip, *ldirp;

  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK) return (err_code);

  /* Next make the inode. If that fails, return error code. */
  bits = I_DIRECTORY | (m_in.mode & RWX_MODES & fp->fp_umask);
  rip = new_node(&ldirp, user_path, bits, (mnx_zone_t) 0, TRUE, string);
  if (rip == NIL_INODE || err_code == EEXIST) {
    put_inode(rip);   /* can't make dir: it already exists */
    put_inode(ldirp);
    return (err_code);
  }

  /* Get the inode numbers for . and .. to enter in the directory. */
  dotdot = ldirp->i_num;  /* parent's inode number */
  dot = rip->i_num;   /* inode number of the new dir itself */

  /* Now make dir entries for . and .. unless the disk is completely full. */
  /* Use dot1 and dot2, so the mode of the directory isn't important. */
  rip->i_mode = bits; /* set mode */
  r1 = search_dir(rip, dot1, &dot, ENTER);  /* enter . in the new dir */
  r2 = search_dir(rip, dot2, &dotdot, ENTER); /* enter .. in the new dir */

  /* If both . and .. were successfully entered, increment the link counts. */
  if (r1 == OK && r2 == OK) {
    /* Normal case.  It was possible to enter . and .. in the new dir. */
    rip->i_nlinks++;  /* this accounts for . */
    ldirp->i_nlinks++;  /* this accounts for .. */
    ldirp->i_dirt = DIRTY;  /* mark parent's inode as dirty */
  } else {
    /* It was not possible to enter . or .. probably disk was full -
     * links counts haven't been touched.
     */
    if (search_dir(ldirp, string, (mnx_ino_t *) 0, DELETE) != OK)
      panic(__FILE__, "Dir disappeared ", rip->i_num);
    rip->i_nlinks--;  /* undo the increment done in new_node() */
  }
  rip->i_dirt = DIRTY;    /* either way, i_nlinks has changed */

  put_inode(ldirp);   /* return the inode of the parent dir */
  put_inode(rip);   /* return the inode of the newly made dir */
  return (err_code);  /* new_node() always sets 'err_code' */
}


/*===========================================================================*
 *        do_close             *
 *===========================================================================*/
int do_close()
{
  /* Perform the close(fd) system call. */

  struct filp *rfilp;
  struct inode *rip;
  struct mnx_file_lock *flp;
  int rw, mode_word, lock_count;
  mnx_dev_t dev;

  /* First locate the inode that belongs to the file descriptor. */
  if ( (rfilp = get_filp(m_in.fd)) == NIL_FILP) return (err_code);
  rip = rfilp->filp_ino;  /* 'rip' points to the inode */

  if (rfilp->filp_count - 1 == 0 && rfilp->filp_mode != FILP_CLOSED) {
    /* Check to see if the file is special. */
    mode_word = rip->i_mode & I_TYPE;

// SVRDEBUG("ANTES DE mode_word %d\n", mode_word);
// SVRDEBUG("I_CHAR_SPECIAL %d\n", I_CHAR_SPECIAL);
// SVRDEBUG("I_BLOCK_SPECIAL %d\n", I_BLOCK_SPECIAL);
    if (mode_word == I_CHAR_SPECIAL || mode_word == I_BLOCK_SPECIAL) {
      dev = (mnx_dev_t) rip->i_zone[0];
      if (mode_word == I_BLOCK_SPECIAL)  {
        /* Invalidate cache entries unless special is mounted
         * or ROOT
         */
// SVRDEBUG("Llamada a do_sync ------//////////////////////////////// ");
        //if (!mounted(rip)) {
        (void) do_sync(); /* purge cache */
        //invalidate(dev);
        //}
      }

      /* Do any special processing on device close. */
      dev_close(dev);
    }
  }

#ifdef ANULADO
  /*ATENCION ver esto por error de compilacion en select.c y pippe.c dependiendo de int select_callback(struct filp *fp, int ops)*/
  /* If the inode being closed is a pipe, release everyone hanging on it. */
  if (rip->i_pipe == I_PIPE) {
    rw = (rfilp->filp_mode & R_BIT ? MOLWRITE : MOLREAD);
    release(rip, rw, dc_ptr->dc_nr_procs);
  }

#endif /* ANULADO */

  /* If a write has been done, the inode is already marked as DIRTY. */
  if (--rfilp->filp_count == 0) {
    if (rip->i_pipe == I_PIPE && rip->i_count > 1) {
      /* Save the file position in the i-node in case needed later.
       * The read and write positions are saved separately.  The
       * last 3 zones in the i-node are not used for (named) pipes.
       */
      if (rfilp->filp_mode == R_BIT)
        rip->i_zone[V2_NR_DZONES + 0] = (mnx_zone_t) rfilp->filp_pos;
      else
        rip->i_zone[V2_NR_DZONES + 1] = (mnx_zone_t) rfilp->filp_pos;
    }
    put_inode(rip);
  }

  fp->fp_cloexec &= ~(1L << m_in.fd); /* turn off close-on-exec bit */
  fp->fp_filp[m_in.fd] = NIL_FILP;
  FD_CLR(m_in.fd, &fp->fp_filp_inuse);

  /* Check to see if the file is locked.  If so, release all locks. */
  if (nr_locks == 0) return (OK);
  lock_count = nr_locks;  /* save count of locks */
  for (flp = &file_lock[0]; flp < &file_lock[NR_LOCKS]; flp++) {
    if (flp->lock_type == 0) continue;  /* slot not in use */
    if (flp->lock_inode == rip && flp->lock_pid == fp->fp_pid) {
      flp->lock_type = 0;
      nr_locks--;
    }
  }
  //if (nr_locks < lock_count) lock_revive(); /* lock released */
  return (OK);
}

/*===========================================================================*
 *        do_lseek             *
 *===========================================================================*/
int do_lseek()
{
  /* Perform the lseek(ls_fd, offset, whence) system call. */

  register struct filp *rfilp;
  register mnx_off_t pos;

  /* Check to see if the file descriptor is valid. */
  if ( (rfilp = get_filp(m_in.ls_fd)) == NIL_FILP) return (err_code);

  /* No lseek on pipes. */
  if (rfilp->filp_ino->i_pipe == I_PIPE) return (EMOLSPIPE);

  /* The value of 'whence' determines the start position to use. */
  switch (m_in.whence) {
  case SEEK_SET: pos = 0; break;
  case SEEK_CUR: pos = rfilp->filp_pos; break;
  case SEEK_END: pos = rfilp->filp_ino->i_size; break;
  default: return (EMOLINVAL);
  }

  /* Check for overflow. */
  if (((long)m_in.offset > 0) && ((long)(pos + m_in.offset) < (long)pos))
    return (EMOLINVAL);
  if (((long)m_in.offset < 0) && ((long)(pos + m_in.offset) > (long)pos))
    return (EMOLINVAL);
  pos = pos + m_in.offset;

  if (pos != rfilp->filp_pos)
    rfilp->filp_ino->i_seek = ISEEK;  /* inhibit read ahead */
  rfilp->filp_pos = pos;
  m_out.reply_l1 = pos;   /* insert the long into the output message */
  return (OK);
}

/*===========================================================================*
 *                             do_slink              *
 *===========================================================================*/
int do_slink()
{
  /* Perform the symlink(name1, name2) system call. */

  register int r;              /* error code */
  char string[NAME_MAX];       /* last component of the new dir's path name */
  struct inode *sip;           /* inode containing symbolic link */
  struct buf *bp;              /* disk buffer for link */
  struct inode *ldirp;         /* directory containing link */

  if (fetch_name(m_in.name2, m_in.name2_length, M1) != OK)
    return (err_code);

  if (m_in.name1_length <= 1 || m_in.name1_length >= _MIN_BLOCK_SIZE)
    return (EMOLNAMETOOLONG);

  /* Create the inode for the symlink. */
  sip = new_node(&ldirp, user_path, (mnx_mode_t) (I_SYMBOLIC_LINK | RWX_MODES),
                 (mnx_zone_t) 0, TRUE, string);

  /* Allocate a disk block for the contents of the symlink.
   * Copy contents of symlink (the name pointed to) into first disk block.
   */
  // if ((r = err_code) == OK) {
  //      r = (bp = new_block(sip, (mnx_off_t) 0)) == NIL_BUF
  //          ? err_code
  //          : sys_vircopy(who_e, D, (vir_bytes) m_in.name1,
  //                      SELF, D, (vir_bytes) bp->b_data,
  //          (vir_bytes) m_in.name1_length-1);
  //
  if ((r = err_code) == OK) {
    r = (bp = new_block(sip, (mnx_off_t) 0)) == NIL_BUF
        ? err_code
        : mnx_vcopy(who_e,  m_in.name1,
                    SELF,  bp->b_data,
                    m_in.name1_length - 1);

    if (r == OK) {
      bp->b_data[_MIN_BLOCK_SIZE - 1] = '\0';
      sip->i_size = strlen(bp->b_data);
      if (sip->i_size != m_in.name1_length - 1) {
        /* This can happen if the user provides a buffer
         * with a \0 in it. This can cause a lot of trouble
         * when the symlink is used later. We could just use
         * the strlen() value, but we want to let the user
         * know he did something wrong. ENAMETOOLONG doesn't
         * exactly describe the error, but there is no
         * ENAMETOOWRONG.
         */
        r = EMOLNAMETOOLONG;
      }
    }

    put_block(bp, DIRECTORY_BLOCK);  /* put_block() accepts NIL_BUF. */

    if (r != OK) {
      sip->i_nlinks = 0;
      if (search_dir(ldirp, string, (mnx_ino_t *) 0, DELETE) != OK)
        panic(__FILE__, "Symbolic link vanished", NO_NUM);
    }
  }

  /* put_inode() accepts NIL_INODE as a noop, so the below are safe. */
  put_inode(sip);
  put_inode(ldirp);

  return (r);
}