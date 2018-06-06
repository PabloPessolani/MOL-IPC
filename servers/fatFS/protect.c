/* This file deals with protection in the file system.  It contains the code
 * for four system calls that relate to protection.
 *
 * The entry points into this file are
 *   do_chmod:	perform the CHMOD system call
 *   do_chown:	perform the CHOWN system call
 *   do_umask:	perform the UMASK system call
 *   do_access:	perform the ACCESS system call
 *   forbidden:	check to see if a given access is allowed on a given inode
 */

// #define SVRDBG    1

#include "fs.h"

/*===========================================================================*
 *        do_chmod             *
 *===========================================================================*/
int do_chmod()
{
/* Perform the chmod(name, mode) system call. */

  register struct inode *rip;
  register int r;

  /* Temporarily open the file. */
  if (fetch_name(m_in.name, m_in.name_length, M3) != OK) ERROR_RETURN(err_code);
  if ( (rip = eat_path(user_path)) == NIL_INODE) ERROR_RETURN(err_code);

  /* Only the owner or the super_user may change the mode of a file.
   * No one may change the mode of a file on a read-only file system.
   */
  if (rip->i_uid != fp->fp_effuid && !super_user)
  r = EMOLPERM;
  else
  r = read_only(rip);

  /* If error, return inode. */
  if (r != OK)  {
  put_inode(rip);
  return(r);
  }

  /* Now make the change. Clear setgid bit if file is not in caller's grp */
  rip->i_mode = (rip->i_mode & ~ALL_MODES) | (m_in.mode & ALL_MODES);
  if (!super_user && rip->i_gid != fp->fp_effgid)rip->i_mode &= ~I_SET_GID_BIT;
  rip->i_update |= CTIME;
  rip->i_dirt = DIRTY;

  put_inode(rip);
  return(OK);
}

/*===========================================================================*
 *        do_chown             *
 *===========================================================================*/
int do_chown()
{
/* Perform the chown(name, owner, group) system call. */

  register struct inode *rip;
  register int r;

  /* Temporarily open the file. */
  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK) ERROR_RETURN(err_code);
  if ( (rip = eat_path(user_path)) == NIL_INODE) ERROR_RETURN(err_code);

  /* Not permitted to change the owner of a file on a read-only file sys. */
  r = read_only(rip);
  if (r == OK) {
  /* FS is R/W.  Whether call is allowed depends on ownership, etc. */
  if (super_user) {
    /* The super user can do anything. */
    rip->i_uid = m_in.owner;  /* others later */
  } else {
    /* Regular users can only change groups of their own files. */
    if (rip->i_uid != fp->fp_effuid) r = EMOLPERM;
    if (rip->i_uid != m_in.owner) r = EMOLPERM;  /* no giving away */
    if (fp->fp_effgid != m_in.group) r = EMOLPERM;
  }
  }
  if (r == OK) {
  rip->i_gid = m_in.group;
  rip->i_mode &= ~(I_SET_UID_BIT | I_SET_GID_BIT);
  rip->i_update |= CTIME;
  rip->i_dirt = DIRTY;
  }

  put_inode(rip);
  return(r);
}

/*===========================================================================*
 *        do_umask             *
 *===========================================================================*/
int do_umask()
{
/* Perform the umask(co_mode) system call. */
  register mnx_mode_t r;

  r = ~fp->fp_umask;    /* set 'r' to complement of old mask */
  SVRDEBUG("Before r=%d, \n", r);
  SVRDEBUG("RWX_MODES =%d, \n", RWX_MODES);  
  SVRDEBUG("m_in.co_mode=%d, \n", m_in.co_mode);    
  SVRDEBUG("m_in.co_mode & RWX_MODES=%d, \n", m_in.co_mode & RWX_MODES);    
  SVRDEBUG("~(m_in.co_mode & RWX_MODES)=%d, \n", ~(m_in.co_mode & RWX_MODES));      
  fp->fp_umask = ~(m_in.co_mode & RWX_MODES);
  return(r);      /* return complement of old mask */
}

/*===========================================================================*
 *        do_access            *
 *===========================================================================*/
int do_access()
{
/* Perform the access(name, mode) system call. */

  struct inode *rip;
  register int r;

  SVRDEBUG("R_OK=%d, \n", R_OK);
  SVRDEBUG("W_OK=%d, \n", W_OK);
  SVRDEBUG("X_OK=%d, \n", X_OK);
  SVRDEBUG("F_OK=%d, \n", F_OK);
  SVRDEBUG("m_in.mode=%d, \n", m_in.mode);
  /* First check to see if the mode is correct. */
  if ( (m_in.mode & ~(R_OK | W_OK | X_OK)) != 0 && m_in.mode != F_OK)
  ERROR_RETURN(EMOLINVAL);

  /* Temporarily open the file whose access is to be checked. */
  if (fetch_name(m_in.name, m_in.name_length, M3) != OK) ERROR_RETURN(err_code);
  if ( (rip = eat_path(user_path)) == NIL_INODE) ERROR_RETURN(err_code);

  /* Now check the permissions. */
  r = forbidden(rip, (mnx_mode_t) m_in.mode);
  put_inode(rip);
  return(r);
}

/*===========================================================================*
 *        forbidden            *
 *===========================================================================*/
int forbidden(struct inode *rip, mnx_mode_t access_desired)
{
/* Given a pointer to an inode, 'rip', and the access desired, determine
 * if the access is allowed, and if not why not.  The routine looks up the
 * caller's uid in the 'fproc' table.  If access is allowed, OK is returned
 * if it is forbidden, EACCES is returned.
 */

  struct inode *old_rip = rip;
  struct super_block *sp;
  mnx_mode_t bits, perm_bits;
  int r, shift, test_uid, test_gid, type;

  if (rip->i_mount == I_MOUNT)  /* The inode is mounted on. */
  for (sp = &super_block[1]; sp < &super_block[NR_SUPERS]; sp++)
    if (sp->s_imount == rip) {
      rip = get_inode(sp->s_dev, ROOT_INODE);
      break;
    } /* if */

  /* Isolate the relevant rwx bits from the mode. */
  bits = rip->i_mode;
  test_uid = (call_nr == EMOLACCES ? fp->fp_realuid : fp->fp_effuid);
  test_gid = (call_nr == EMOLACCES ? fp->fp_realgid : fp->fp_effgid);
  if (test_uid == SU_UID) {
  /* Grant read and write permission.  Grant search permission for
   * directories.  Grant execute permission (for non-directories) if
   * and only if one of the 'X' bits is set.
   */
  if ( (bits & I_TYPE) == I_DIRECTORY ||
       bits & ((X_BIT << 6) | (X_BIT << 3) | X_BIT))
    perm_bits = R_BIT | W_BIT | X_BIT;
  else
    perm_bits = R_BIT | W_BIT;
  } else {
  if (test_uid == rip->i_uid) shift = 6;    /* owner */
  else if (test_gid == rip->i_gid ) shift = 3;  /* group */
  else shift = 0;         /* other */
  perm_bits = (bits >> shift) & (R_BIT | W_BIT | X_BIT);
  }

  /* If access desired is not a subset of what is allowed, it is refused. */
  r = OK;
SVRDEBUG("access_desired=%d \n", access_desired);
SVRDEBUG("perm_bits=%d perm_bits(OCT)=%#o\n", perm_bits, perm_bits); 
SVRDEBUG("access_desired=%d access_desired(OCT)=%#o\n", access_desired, access_desired);     

	if ((perm_bits | access_desired) != perm_bits) r = EACCES;


  /* Check to see if someone is trying to write on a file system that is
   * mounted read-only.
   */
  type = rip->i_mode & I_TYPE;
  if (r == OK)
  if (access_desired & W_BIT)
    r = read_only(rip);

  if (rip != old_rip) put_inode(rip);

	SVRDEBUG("r=%d\n", r);
// 
  return(r);
}

/*===========================================================================*
 *        read_only            *
 *===========================================================================*/
int read_only(struct inode *ip)
//struct inode *ip;   /* ptr to inode whose file sys is to be cked */
{
/* Check to see if the file system on which the inode 'ip' resides is mounted
 * read only.  If so, return EROFS, else return OK.
 */

  struct super_block *sp;
  // SVRDEBUG("READONLY\n"); 
  sp = ip->i_sp;
  return(sp->s_rd_only ? EMOLROFS : OK);
}