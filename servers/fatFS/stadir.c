/* This file contains the code for performing four system calls relating to
 * status and directories.
 *
 * The entry points into this file are
 *   do_chdir:	perform the CHDIR system call
 *   do_chroot:	perform the CHROOT system call
 *   do_stat:	perform the STAT system call
 *   do_fstat:	perform the FSTAT system call
 *   do_fstatfs: perform the FSTATFS system call
 *   do_lstat:  perform the LSTAT system call
 *   do_rdlink: perform the RDLNK system call
 */
// #define SVRDBG    1

#include "fs.h"

int change(struct inode **iip, char *name_ptr, int len);
int change_into(struct inode **iip, struct inode *rip);
int stat_inode(char *fileName, char *user_addr);

/*===========================================================================*
 *        do_fchdir            *
 *===========================================================================*/
int do_fchdir()
{
  /* Change directory on already-opened fd. */
  struct filp *rfilp;

  /* Is the file descriptor valid? */
  if ((rfilp = get_filp(m_in.fd)) == NIL_FILP)
    ERROR_RETURN(err_code);
  dup_inode(rfilp->filp_ino);
  return change_into(&fp->fp_workdir, rfilp->filp_ino);
}

/*===========================================================================*
 *        do_chdir             *
 *===========================================================================*/
int do_chdir()
{
  /* Change directory.  This function is  also called by MM to simulate a chdir
 * in order to do EXEC, etc.  It also changes the root directory, the uids and
 * gids, and the umask. 
 */

  int r;
  register struct fproc *rfp;

  if (who_e == PM_PROC_NR)
  {
    int slot;
    if (isokendpt(m_in.endpt1, &slot) != OK)
      return EMOLINVAL;
    rfp = &fproc[slot];
    put_inode(fp->fp_rootdir);
    dup_inode(fp->fp_rootdir = rfp->fp_rootdir);
    put_inode(fp->fp_workdir);
    dup_inode(fp->fp_workdir = rfp->fp_workdir);

    /* MM uses access() to check permissions.  To make this work, pretend
   * that the user's real ids are the same as the user's effective ids.
   * FS calls other than access() do not use the real ids, so are not
   * affected.
   */
    fp->fp_realuid =
        fp->fp_effuid = rfp->fp_effuid;
    fp->fp_realgid =
        fp->fp_effgid = rfp->fp_effgid;
    fp->fp_umask = rfp->fp_umask;
    return (OK);
  }

  /* Perform the chdir(name) system call. */
  r = change(&fp->fp_workdir, m_in.name, m_in.name_length);
  return (r);
}

/*===========================================================================*
 *        do_chroot            *
 *===========================================================================*/
int do_chroot()
{
  /* Perform the chroot(name) system call. */

  register int r;

  if (!super_user)
    ERROR_RETURN(EMOLPERM); /* only su may chroot() */
  r = change(&fp->fp_rootdir, m_in.name, m_in.name_length);
  return (r);
}

/*===========================================================================*
 *        change               *
 *===========================================================================*/
int change(struct inode **iip, char *name_ptr, int len)
//struct inode **iip;   /* pointer to the inode pointer for the dir */
//char *name_ptr;     /* pointer to the directory name to change to */
//int len;      /* length of the directory name string */
{
  /* Do the actual work for chdir() and chroot(). */
  struct inode *rip;

  /* Try to open the new directory. */
  if (fetch_name(name_ptr, len, M3) != OK)
    ERROR_RETURN(err_code);
  if ((rip = eat_path(user_path)) == NIL_INODE)
    ERROR_RETURN(err_code);
  return change_into(iip, rip);
}

/*===========================================================================*
 *        change_into            *
 *===========================================================================*/
int change_into(struct inode **iip, struct inode *rip)
//struct inode **iip;   /* pointer to the inode pointer for the dir */
//struct inode *rip;    /* this is what the inode has to become */
{
  register int r;

  /* It must be a directory and also be searchable. */
  if ((rip->i_mode & I_TYPE) != I_DIRECTORY)
    r = EMOLNOTDIR;
  else
    r = forbidden(rip, X_BIT); /* check if dir is searchable */

  /* If error, return inode. */
  if (r != OK)
  {
    put_inode(rip);
    return (r);
  }

  /* Everything is OK.  Make the change. */
  put_inode(*iip); /* release the old directory */
  *iip = rip;      /* acquire the new one */
  return (OK);
}

//   mnx_dev_t st_dev;			/* major/minor device number */
//   mnx_ino_t st_ino;			/* i-node number */
//   mnx_mode_t st_mode;		/* file mode, protection bits, etc. */
//   short int st_nlink;		/* # links; TEMPORARY HACK: should be nlink_t*/
//   mnx_uid_t st_uid;			/* uid of the file's owner */
//   short int st_gid;		    /* gid; TEMPORARY HACK: should be gid_t */
//   mnx_dev_t st_rdev;

//   mnx_off_t st_size;		/* file size */
//   mnx_time_t st_atime;		/* time of last access */
//   mnx_time_t st_mtime;		/* time of last data modification */
//   mnx_time_t st_ctime;		/* time of last file status change */

// fno.fsize
// fno.fdate
// fno.ftime

// FSIZE_t	fsize;			/* File size */
// FF_WORD	fdate;			/* Modified date */
// FF_WORD	ftime;			/* Modified time */
// FF_BYTE	fattrib;		/* File attribute */

// #define	AM_RDO	0x01	/* Read only */
// #define	AM_HID	0x02	/* Hidden */
// #define	AM_SYS	0x04	/* System */
// #define AM_DIR	0x10	/* Directory */
// #define AM_ARC	0x20	/* Archive */

/*===========================================================================*
 *        do_stat              *
 *===========================================================================*/
int do_stat()
{
  /* Perform the stat(name, buf) system call. */

  // register struct inode *rip;
  register int r;
  /* Both stat() and fstat() use the same routine to do the real work.  That
   * routine expects an inode, so acquire it temporarily.
   */
  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK)
    ERROR_RETURN(err_code);
  r = stat_inode(user_path, m_in.name2); /* actually do the work.*/

  SVRDEBUG("user_path=%s\n", user_path);
  return (r);
}

/*===========================================================================*
 *        do_fstat             *
 *===========================================================================*/
int do_fstat()
{
  /* Perform the fstat(fd, buf) system call. */

  register struct filp *rfilp;

  /* Is the file descriptor valid? */
  if ((rfilp = get_filp(m_in.fd)) == NIL_FILP)
    ERROR_RETURN(err_code);

  SVRDEBUG("m_in.fd=%d\n", m_in.fd);
  SVRDEBUG("rfilp->fatFilename=%s\n", rfilp->fatFilename);  
  return (stat_inode(rfilp->fatFilename, m_in.buffer));
}

/*===========================================================================*
//  *        stat_inode             *
//  *===========================================================================*/
int stat_inode(char *fileName, char *user_addr)
//register struct inode *rip; /* pointer to inode to stat */
//struct filp *fil_ptr;   /* filp pointer, supplied by 'fstat' */
//char *user_addr;    /* user space address where stat buf goes */
{
  /* Common code for stat and fstat system calls. */

  struct mnx_stat statbuf = {0};
  // struct mnx_stat statbuf;
  mnx_time_t cur_time;
  int r;
  FRESULT fr;
  FILINFO fno;
  struct tm tmdate = {0};

  SVRDEBUG("fileName=%s\n", fileName);
  
  fr = f_stat(fileName, &fno);

  SVRDEBUG("Size: %lu\n", fno.fsize);
  
  // SVRDEBUG("fr: %d\n", fr);
  // SVRDEBUG("FR_OK: %d\n", FR_OK);

  statbuf.st_dev = 0; // No ES RELEVANTE para FUSE, VER?
  statbuf.st_ino = 10; // Es el nro magico ó 0 Es o No ES RELEVANTE?
 
  //statbuf.st_mode = rip->i_mode; //REEMPLAZAR POR LAS CONSTANTES APROPIADAS
  // SVRDEBUG("Antes de IF DIRECTORIO O ARCHIVO=\n");
  // SVRDEBUG("fno.fattrib=%d\n", fno.fattrib);
  // SVRDEBUG("fno.fattrib & AM_DIR=%d\n", fno.fattrib & AM_DIR);
  // SVRDEBUG("fno.fattrib & AM_ARC=%d\n", fno.fattrib & AM_ARC);
  if (fno.fattrib & AM_DIR) /* Directory */
  {
    SVRDEBUG("ES DIRECTORIO=%d\n", AM_DIR);    
    statbuf.st_mode |= S_IFDIR;
  }

  if (fno.fattrib & AM_ARC) /* Archive */
  {
    SVRDEBUG("ES ARCHIVO=%d\n", AM_ARC);
    statbuf.st_mode |= S_IFREG;
  }    

  if (fno.fattrib & AM_RDO)
    statbuf.st_mode |= 0555;
  else
    statbuf.st_mode |= 0777;

  statbuf.st_nlink = 1;                //ES RELEVANTE ?
  statbuf.st_uid = (mnx_uid_t)SU_UID;  //DEFINO ROOT
  statbuf.st_gid = (mnx_gid_t)SYS_GID; //DEFINO GRUPO SYS
  statbuf.st_rdev = 0;                 //NO ES RELEVANTE

  statbuf.st_size = fno.fsize;

  tmdate.tm_year = (fno.fdate >> 9) + 1980 - 1900;
  tmdate.tm_mon = ((fno.fdate >> 5) & 15) - 1;
  tmdate.tm_mday = fno.fdate & 31;
  tmdate.tm_hour = (fno.ftime >> 11);
  tmdate.tm_min = ((fno.ftime >> 5) & 63);  
  
  cur_time = mktime( &tmdate ) - timezone;  

  //TODO: ATENCION ver esta conversion de tiempo!
  // typedef unsigned long	FF_DWORD;
  // typedef long mnx_time_t;		/* time in sec since 1 Jan 1970 0000 GMT */

  SVRDEBUG("Timestamp: %u/%02u/%02u, %02u:%02u\n",
          (fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
          fno.ftime >> 11, fno.ftime >> 5 & 63);

  statbuf.st_atime = cur_time;
  statbuf.st_mtime = cur_time;
  statbuf.st_ctime = cur_time;

  /* Copy the struct to user space. */
  // r = sys_datacopy(FS_PROC_NR, (vir_bytes) &statbuf,
  //     who_e, (vir_bytes) user_addr, (phys_bytes) sizeof(statbuf));
  r = mnx_vcopy(FS_PROC_NR, &statbuf, who_e, user_addr, sizeof(statbuf));
  return (r);
}

// /*===========================================================================*
//  *        stat_inode             *
//  *===========================================================================*/
// int stat_inode(register struct inode *rip, struct filp *fil_ptr, char *user_addr)
// //register struct inode *rip; /* pointer to inode to stat */
// //struct filp *fil_ptr;   /* filp pointer, supplied by 'fstat' */
// //char *user_addr;    /* user space address where stat buf goes */
// {
// /* Common code for stat and fstat system calls. */

//   struct mnx_stat statbuf;
//   mode_t mo;
//   int r, s;

//   /* Update the atime, ctime, and mtime fields in the inode, if need be. */
//   if (rip->i_update) update_times(rip);

//   /* Fill in the statbuf struct. */
//   mo = rip->i_mode & I_TYPE;

//   /* true iff special */
//   s = (mo == I_CHAR_SPECIAL || mo == I_BLOCK_SPECIAL);

//   statbuf.st_dev = rip->i_dev;
//   statbuf.st_ino = rip->i_num;
//   statbuf.st_mode = rip->i_mode;
//   statbuf.st_nlink = rip->i_nlinks;
//   statbuf.st_uid = rip->i_uid;
//   statbuf.st_gid = rip->i_gid;
//   statbuf.st_rdev = (dev_t) (s ? rip->i_zone[0] : NO_DEV);
//   statbuf.st_size = rip->i_size;

//   if (rip->i_pipe == I_PIPE) {
//   statbuf.st_mode &= ~I_REGULAR;  /* wipe out I_REGULAR bit for pipes */
//   if (fil_ptr != NIL_FILP && fil_ptr->filp_mode & R_BIT)
//     statbuf.st_size -= fil_ptr->filp_pos;
//   }

//   statbuf.st_atime = rip->i_atime;
//   statbuf.st_mtime = rip->i_mtime;
//   statbuf.st_ctime = rip->i_ctime;

//   /* Copy the struct to user space. */
//   // r = sys_datacopy(FS_PROC_NR, (vir_bytes) &statbuf,
//   //     who_e, (vir_bytes) user_addr, (phys_bytes) sizeof(statbuf));
//   r = mnx_vcopy(FS_PROC_NR, &statbuf, who_e, user_addr, sizeof(statbuf));
//   return(r);
// }

/*===========================================================================*
 *        do_fstatfs             *
 *===========================================================================*/
int do_fstatfs()
{
  /* Perform the fstatfs(fd, buf) system call. */
  struct statfs st;
  register struct filp *rfilp;
  int r;

  /* Is the file descriptor valid? */
  if ((rfilp = get_filp(m_in.fd)) == NIL_FILP)
    ERROR_RETURN(err_code);

  st.f_bsize = rfilp->filp_ino->i_sp->s_block_size;

  // r = sys_datacopy(FS_PROC_NR, (vir_bytes) &st,
  //     who_e, (vir_bytes) m_in.buffer, (phys_bytes) sizeof(st));

  r = mnx_vcopy(FS_PROC_NR, &st, who_e, m_in.buffer, sizeof(st));

  return (r);
}

/*===========================================================================*
 *                             do_lstat                                     *
 *===========================================================================*/
int do_lstat()
{
  /* Perform the lstat(name, buf) system call. */

  register int r;             /* return value */
  register struct inode *rip; /* target inode */

  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK)
    ERROR_RETURN(err_code);
  if ((rip = parse_path(user_path, (char *)0, EAT_PATH_OPAQUE)) == NIL_INODE)
    ERROR_RETURN(err_code);
  // r = stat_inode(rip, NIL_FILP, m_in.name2);
  r = 0; //TODO: PARA COMPATIBILIDAD VER DE QUE FORMA
  put_inode(rip);
  return (r);
}

/*===========================================================================*
 *                             do_rdlink                                    *
 *===========================================================================*/
int do_rdlink()
{
  /* Perform the readlink(name, buf) system call. */

  register int r;             /* return value */
  mnx_block_t b;              /* block containing link text */
  struct buf *bp;             /* buffer containing link text */
  register struct inode *rip; /* target inode */
  int copylen;
  copylen = m_in.m1_i2;
  // SVRDEBUG("copylen=%d, \n", copylen);
  if (copylen < 0)
    ERROR_RETURN(EMOLINVAL);

  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK)
    ERROR_RETURN(err_code);
  if ((rip = parse_path(user_path, (char *)0, EAT_PATH_OPAQUE)) == NIL_INODE)
    ERROR_RETURN(err_code);

  r = EMOLACCES;
  // SVRDEBUG("rip->i_mode=%d, \n", rip->i_mode);
  // SVRDEBUG("S_ISLNK(rip->i_mode)=%d, \n", S_ISLNK(rip->i_mode));
  // SVRDEBUG("m_in.name2_length=%d, \n", m_in.name2_length);
  // SVRDEBUG("copylen=%d, \n", copylen);
  if (S_ISLNK(rip->i_mode) && (b = read_map(rip, (mnx_off_t)0)) != NO_BLOCK)
  {
    if (m_in.name2_length <= 0)
      r = EMOLINVAL;
    else if (m_in.name2_length < rip->i_size)
      r = EMOLRANGE;
    else
    {
      if (rip->i_size < copylen)
        copylen = rip->i_size;
      bp = get_block(rip->i_dev, b, NORMAL);
      // SVRDEBUG("DESPUES get_block\n");
      //            r = sys_vircopy(SELF, D, (vir_bytes) bp->b_data,
      // who_e, D, (vir_bytes) m_in.name2, (vir_bytes) copylen);
      // SVRDEBUG("ANTES mnx_vcopy\n");
      // SVRDEBUG("bp->b_data=%s, \n", bp->b_data);
      // SVRDEBUG("m_in.name2=%s, \n", m_in.name2);
      r = mnx_vcopy(SELF, bp->b_data, who_e, m_in.name2, copylen);
      // SVRDEBUG("DESPUES mnx_vcopy\n");

      if (r == OK)
        r = copylen;
      put_block(bp, DIRECTORY_BLOCK);
    }
  }

  put_inode(rip);
  return (r);
}