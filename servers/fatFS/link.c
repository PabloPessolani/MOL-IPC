/* This file handles the LINK and UNLINK system calls.  It also deals with
 * deallocating the storage used by a file when the last UNLINK is done to a
 * file and the blocks must be returned to the free block pool.
 *
 * The entry points into this file are
 *   do_link:         perform the LINK system call
 *   do_unlink:	      perform the UNLINK and RMDIR system calls
 *   do_rename:	      perform the RENAME system call
 *   do_truncate:     perform the TRUNCATE system call
 *   do_ftruncate:    perform the FTRUNCATE system call
 *   truncate_inode:  release the blocks associated with an inode up to a size
 *   freesp_inode:    release a range of blocks without setting the size
 */

// #define SVRDBG    1

#include "fs.h"

#define SAME 1000
#define FIRST_HALF	0
#define LAST_HALF	1

/*PROTOTYPES*/
int remove_dir(struct inode *rldirp, struct inode *rip, char dir_name[NAME_MAX]);
int unlink_file(struct inode *dirp, struct inode *rip, char file_name[NAME_MAX]);
mnx_off_t nextblock(mnx_off_t pos, int zone_size);
void zeroblock_half(struct inode *rip, mnx_off_t pos, int half);
void zeroblock_range(struct inode *rip, mnx_off_t pos, mnx_off_t len);

/*===========================================================================*
 *				do_link					     *
 *===========================================================================*/
int do_link()
{
/* Perform the link(name1, name2) system call. */

//   struct inode *ip, *rip;
  register int r;
//   char string[NAME_MAX];
//   struct inode *new_ip;

//   /* See if 'name' (file to be linked) exists. */
//   if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK) return(err_code);
//   if ( (rip = eat_path(user_path)) == NIL_INODE) return(err_code);

//   /* Check to see if the file has maximum number of links already. */
//   r = OK;
//   if (rip->i_nlinks >= (rip->i_sp->s_version == V1 ? CHAR_MAX : SHRT_MAX))
// 	r = EMOLMLINK;

//   /* Only super_user may link to directories. */
//   if (r == OK)
// 	if ( (rip->i_mode & I_TYPE) == I_DIRECTORY && !super_user) r = EMOLPERM;

//   /* If error with 'name', return the inode. */
//   if (r != OK) {
// 	put_inode(rip);
// 	return(r);
//   }

//   /* Does the final directory of 'name2' exist? */
//   if (fetch_name(m_in.name2, m_in.name2_length, M1) != OK) {
// 	put_inode(rip);
// 	return(err_code);
//   }
//   if ( (ip = last_dir(user_path, string)) == NIL_INODE) r = err_code;

//   /* If 'name2' exists in full (even if no space) set 'r' to error. */
//   if (r == OK) {
// 	if ( (new_ip = advance(&ip, string)) == NIL_INODE) {
// 		r = err_code;
// 		if (r == EMOLNOENT) r = OK;
// 	} else {
// 		put_inode(new_ip);
// 		r = EMOLEXIST;
// 	}
//   }

//   /* Check for links across devices. */
//   if (r == OK)
// 	if (rip->i_dev != ip->i_dev) r = EMOLXDEV;

//   /* Try to link. */
//   if (r == OK)
// 	r = search_dir(ip, string, &rip->i_num, ENTER);

//   /* If success, register the linking. */
//   if (r == OK) {
// 	rip->i_nlinks++;
// 	rip->i_update |= CTIME;
// 	rip->i_dirt = DIRTY;
//   }

//   /* Done.  Release both inodes. */
//   put_inode(rip);
//   put_inode(ip);
  return(r);
}

/*===========================================================================*
 *				do_unlink				     *
 *===========================================================================*/
int do_unlink()
{
/* Perform the unlink(name) or rmdir(name) system call. The code for these two
 * is almost the same.  They differ only in some condition testing.  Unlink()
 * may be used by the superuser to do dangerous things; rmdir() may not.
 */

  register struct inode *rip;
  struct inode *rldirp;
  int r;
  char string[NAME_MAX];

  /* Get the last directory in the path. */
  if (fetch_name(m_in.name, m_in.name_length, M3) != OK) ERROR_RETURN(err_code);
  if ( (rldirp = last_dir(user_path, string)) == NIL_INODE)
	ERROR_RETURN(err_code);

  /* The last directory exists.  Does the file also exist? */
  r = OK;
  if ( (rip = advance(&rldirp, string)) == NIL_INODE) r = err_code;

  /* If error, return inode. */
  if (r != OK) {
	put_inode(rldirp);
	return(r);
  }

  /* Do not remove a mount point. */
  if (rip->i_num == ROOT_INODE) {
	put_inode(rldirp);
	put_inode(rip);
	ERROR_RETURN(EMOLBUSY);
  }

  /* Now test if the call is allowed, separately for unlink() and rmdir(). */
  if (call_nr == MOLUNLINK) {
	/* Only the su may unlink directories, but the su can unlink any dir.*/
	if ( (rip->i_mode & I_TYPE) == I_DIRECTORY && !super_user) r = EMOLPERM;

	/* Don't unlink a file if it is the root of a mounted file system. */
	if (rip->i_num == ROOT_INODE) r = EMOLBUSY;

	/* Actually try to unlink the file; fails if parent is mode 0 etc. */
	if (r == OK) r = unlink_file(rldirp, rip, string);

  } else {
	r = remove_dir(rldirp, rip, string); /* call is RMDIR */
  }

  /* If unlink was possible, it has been done, otherwise it has not. */
  put_inode(rip);
  put_inode(rldirp);
  return(r);
}

/*===========================================================================*
 *				do_rename				     *
 *===========================================================================*/
int do_rename()
{
/* Perform the rename(name1, name2) system call. */

  int r = OK;				/* error flag; initially no error */
  char old_name[NAME_MAX], new_name[NAME_MAX];
  FRESULT fr;
  
  /* See if 'name1' (existing file) exists.  Get dir and file inodes. */
  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK) ERROR_RETURN(err_code);
  strcpy(old_name, user_path);  //user_path is filled in fetch_name

  /* See if 'name2' (new name) exists.  Get dir and file inodes. */
  if (fetch_name(m_in.name2, m_in.name2_length, M1) != OK) r = err_code;
	//   memcpy(new_name, user_path, strlen(user_path)+1);  //user_path is filled in fetch_name
  strcpy(new_name, user_path);  //user_path is filled in fetch_name

	/* The old or new name must not be . or .. */
	if (strcmp(old_name, ".")==0 || strcmp(old_name, "..")==0 ||
		strcmp(new_name, ".")==0 || strcmp(new_name, "..")==0) r = EMOLINVAL;

	/* Both parent directories must be on the same device. */
	// if (old_dirp->i_dev != new_dirp->i_dev) r = EXDEV; //VER ESTO !

	//TODO: llamar a f_rename de FATFS
	fr = f_rename(old_name, new_name);
	SVRDEBUG("f_rename RESULT =%d\n", fr); //TODO: matchear con los resultados delas syscalls reales 
	if (fr == FR_OK){
		r = SAME;
	}		

  return(r == SAME ? OK : r);
}

/*===========================================================================*
 *				do_truncate				     *
 *===========================================================================*/
int do_truncate()
{
/* truncate_inode() does the actual work of do_truncate() and do_ftruncate().
 * do_truncate() and do_ftruncate() have to get hold of the inode, either
 * by name or fd, do checks on it, and call truncate_inode() to do the
 * work.
 */
	int r;
	struct inode *rip;	/* pointer to inode to be truncated */

	if (fetch_name(m_in.m2_p1, m_in.m2_i1, M1) != OK)
		return err_code;
	if( (rip = eat_path(user_path)) == NIL_INODE)
		return err_code;
	if ( (rip->i_mode & I_TYPE) != I_REGULAR)
		r = EMOLINVAL;
	else
		r = truncate_inode(rip, m_in.m2_l1); 
	put_inode(rip);

	return r;
}

/*===========================================================================*
 *				do_ftruncate				     *
 *===========================================================================*/
int do_ftruncate()
{
/* As with do_truncate(), truncate_inode() does the actual work. */
	struct filp *rfilp;
	if ( (rfilp = get_filp(m_in.m2_i1)) == NIL_FILP)
		return err_code;
	if ( (rfilp->filp_ino->i_mode & I_TYPE) != I_REGULAR)
		return EMOLINVAL;
	return truncate_inode(rfilp->filp_ino, m_in.m2_l1);
}

/*===========================================================================*
 *				truncate_inode				     *
 *===========================================================================*/
int truncate_inode(struct inode *rip, mnx_off_t newsize)
// register struct inode *rip;	 pointer to inode to be truncated 
// off_t newsize;			/* inode must become this size */
{
/* Set inode to a certain size, freeing any zones no longer referenced
 * and updating the size in the inode. If the inode is extended, the
 * extra space is a hole that reads as zeroes.
 *
 * Nothing special has to happen to file pointers if inode is opened in
 * O_APPEND mode, as this is different per fd and is checked when 
 * writing is done.
 */
  mnx_zone_t zone_size;
  int scale, file_type, waspipe;
  mnx_dev_t dev;

  file_type = rip->i_mode & I_TYPE;	/* check to see if file is special */
  if (file_type == I_CHAR_SPECIAL || file_type == I_BLOCK_SPECIAL)
	return EMOLINVAL;
  if(newsize > rip->i_sp->s_max_size)	/* don't let inode grow too big */
	return EMOLFBIG;

  dev = rip->i_dev;		/* device on which inode resides */
  scale = rip->i_sp->s_log_zone_size;
  zone_size = (mnx_zone_t) rip->i_sp->s_block_size << scale;

  /* Pipes can shrink, so adjust size to make sure all zones are removed. */
  waspipe = rip->i_pipe == I_PIPE;	/* TRUE if this was a pipe */
  if (waspipe) {
	if(newsize != 0)
		return EMOLINVAL;	/* Only truncate pipes to 0. */
	rip->i_size = PIPE_SIZE(rip->i_sp->s_block_size);
  }

  /* Free the actual space if relevant. */
  if(newsize < rip->i_size)
	  freesp_inode(rip, newsize, rip->i_size);

  /* Next correct the inode size. */
  if(!waspipe) rip->i_size = newsize;
  else wipe_inode(rip);	/* Pipes can only be truncated to 0. */
  rip->i_dirt = DIRTY;

  return OK;
}

/*===========================================================================*
 *				freesp_inode				     *
 *===========================================================================*/
int freesp_inode(struct inode *rip, mnx_off_t start, mnx_off_t end)
// register struct inode *rip;	 pointer to inode to be partly freed 
// off_t start, end;		/* range of bytes to free (end uninclusive) */
{
/* Cut an arbitrary hole in an inode. The caller is responsible for checking
 * the reasonableness of the inode type of rip. The reason is this is that
 * this function can be called for different reasons, for which different
 * sets of inode types are reasonable. Adjusting the final size of the inode
 * is to be done by the caller too, if wished.
 *
 * Consumers of this function currently are truncate_inode() (used to
 * free indirect and data blocks for any type of inode, but also to
 * implement the ftruncate() and truncate() system calls) and the F_FREESP
 * fcntl().
 */
	mnx_off_t p, e;
	int zone_size, dev;

	if(end > rip->i_size)		/* freeing beyond end makes no sense */
		end = rip->i_size;
	if(end <= start)		/* end is uninclusive, so start<end */
		return EMOLINVAL;
        zone_size = rip->i_sp->s_block_size << rip->i_sp->s_log_zone_size;
	dev = rip->i_dev;             /* device on which inode resides */

	/* If freeing doesn't cross a zone boundary, then we may only zero
	 * a range of the block.
	 */
	if(start/zone_size == (end-1)/zone_size) {
		zeroblock_range(rip, start, end-start);
	} else { 
		/* First zero unused part of partly used blocks. */
		if(start%zone_size)
			zeroblock_half(rip, start, LAST_HALF);
		if(end%zone_size && end < rip->i_size)
			zeroblock_half(rip, end, FIRST_HALF);
	}

	/* Now completely free the completely unused blocks.
	 * write_map() will free unused (double) indirect
	 * blocks too. Converting the range to zone numbers avoids
	 * overflow on p when doing e.g. 'p += zone_size'.
	 */
	e = end/zone_size;
	if(end == rip->i_size && (end % zone_size)) e++;
	for(p = nextblock(start, zone_size)/zone_size; p < e; p ++)
		write_map(rip, p*zone_size, NO_ZONE, WMAP_FREE);

	return OK;
}


/*===========================================================================*
 *				nextblock				     *
 *===========================================================================*/
mnx_off_t nextblock(mnx_off_t pos, int zone_size)
// off_t pos;
// int zone_size;
{
/* Return the first position in the next block after position 'pos'
 * (unless this is the first position in the current block).
 * This can be done in one expression, but that can overflow pos.
 */
	mnx_off_t p;
	p = (pos/zone_size)*zone_size;
	if((pos % zone_size)) p += zone_size;	/* Round up. */
	return p;
}

/*===========================================================================*
 *				zeroblock_half				     *
 *===========================================================================*/
void zeroblock_half(struct inode *rip, mnx_off_t pos, int half)
// struct inode *rip;
// off_t pos;
// int half;
{
/* Zero the upper or lower 'half' of a block that holds position 'pos'.
 * half can be FIRST_HALF or LAST_HALF.
 *
 * FIRST_HALF: 0..pos-1 will be zeroed
 * LAST_HALF:  pos..blocksize-1 will be zeroed
 */
	int offset, len;

	 /* Offset of zeroing boundary. */
	 offset = pos % rip->i_sp->s_block_size;

	 if(half == LAST_HALF)  {
	   	len = rip->i_sp->s_block_size - offset;
	 } else {
		len = offset;
		pos -= offset;
		offset = 0;
	 }

	zeroblock_range(rip, pos, len);
}

/*===========================================================================*
 *				zeroblock_range				     *
 *===========================================================================*/
void zeroblock_range(struct inode *rip, mnx_off_t pos, mnx_off_t len)
// struct inode *rip;
// off_t pos;
// off_t len;
{
/* Zero a range in a block.
 * This function is used to zero a segment of a block, either 
 * FIRST_HALF of LAST_HALF.
 * 
 */
	mnx_block_t b;
	struct buf *bp;
	mnx_off_t offset;

	if(!len) return; /* no zeroing to be done. */
	if( (b = read_map(rip, pos)) == NO_BLOCK) return;
	if( (bp = get_block(rip->i_dev, b, NORMAL)) == NIL_BUF)
	   panic(__FILE__, "zeroblock_range: no block", NO_NUM);
	offset = pos % rip->i_sp->s_block_size;
	if(offset + len > rip->i_sp->s_block_size)
	   panic(__FILE__, "zeroblock_range: len too long", len);
	memset(bp->b_data + offset, 0, len);
	bp->b_dirt = DIRTY;
	put_block(bp, FULL_DATA_BLOCK);
}

/*===========================================================================*
 *				remove_dir				     *
 *===========================================================================*/
int remove_dir(struct inode *rldirp, struct inode *rip, char dir_name[NAME_MAX])
//struct inode *rldirp;		 	/* parent directory */
//struct inode *rip;			/* directory to be removed */
//char dir_name[NAME_MAX];		/* name of directory to be removed */
{
  /* A directory file has to be removed. Five conditions have to met:
   * 	- The file must be a directory
   *	- The directory must be empty (except for . and ..)
   *	- The final component of the path must not be . or ..
   *	- The directory must not be the root of a mounted file system
   *	- The directory must not be anybody's root/working directory
   */

  int r;
  register struct fproc *rfp;

  /* search_dir checks that rip is a directory too. */
  if ((r = search_dir(rip, "", (mnx_ino_t *) 0, IS_EMPTY)) != OK) return r;

  if (strcmp(dir_name, ".") == 0 || strcmp(dir_name, "..") == 0) ERROR_RETURN(EMOLINVAL);
  if (rip->i_num == ROOT_INODE) return(EMOLBUSY); /* can't remove 'root' */
  
  for (rfp = &fproc[INIT_PROC_NR + 1]; rfp < &fproc[dc_ptr->dc_nr_procs]; rfp++)
	if (rfp->fp_pid != PID_FREE &&
	     (rfp->fp_workdir == rip || rfp->fp_rootdir == rip))
		ERROR_RETURN(EMOLBUSY); /* can't remove anybody's working dir */

  /* Actually try to unlink the file; fails if parent is mode 0 etc. */
  if ((r = unlink_file(rldirp, rip, dir_name)) != OK) return r;

  /* Unlink . and .. from the dir. The super user can link and unlink any dir,
   * so don't make too many assumptions about them.
   */
  (void) unlink_file(rip, NIL_INODE, dot1);
  (void) unlink_file(rip, NIL_INODE, dot2);
  return(OK);
}

/*===========================================================================*
 *				unlink_file				     *
 *===========================================================================*/
int unlink_file(struct inode *dirp, struct inode *rip, char file_name[NAME_MAX])
//struct inode *dirp;		/* parent directory of file */
//struct inode *rip;		/* inode of file, may be NIL_INODE too. */
//char file_name[NAME_MAX];	/* name of file to be removed */
{
/* Unlink 'file_name'; rip must be the inode of 'file_name' or NIL_INODE. */

  ino_t numb;			/* inode number */
  int	r;

  /* If rip is not NIL_INODE, it is used to get faster access to the inode. */
  if (rip == NIL_INODE) {
  	/* Search for file in directory and try to get its inode. */
	err_code = search_dir(dirp, file_name, &numb, LOOK_UP);
	if (err_code == OK) rip = get_inode(dirp->i_dev, (int) numb);
	if (err_code != OK || rip == NIL_INODE) ERROR_RETURN(err_code);
  } else {
	dup_inode(rip);		/* inode will be returned with put_inode */
  }

  r = search_dir(dirp, file_name, (ino_t *) 0, DELETE);

  if (r == OK) {
	rip->i_nlinks--;	/* entry deleted from parent's dir */
	rip->i_update |= CTIME;
	rip->i_dirt = DIRTY;
  }

  put_inode(rip);
  return(r);
}