/* This file contains the heart of the mechanism used to read (and write)
 * files.  Read and write requests are split up into chunks that do not cross
 * block boundaries.  Each chunk is then processed in turn.  Reads on special
 * files are also detected and handled.
 *
 * The entry points into this file are
 *   do_read:	 perform the READ system call by calling read_write
 *   read_write: actually do the work of READ and WRITE
 *   read_map:	 given an inode and file position, look up its zone number
 *   rd_indir:	 read an entry in an indirect block 
 *   read_ahead: manage the block read ahead business
 */

// #define SVRDBG    1

#include "fs.h"
int rw_chunk(register struct inode *rip, mnx_off_t position, unsigned off, int chunk, unsigned left, int rw_flag, char *buff,
 int seg, int usr, int block_size, int *completed);

/*===========================================================================*
 *        do_read              *
 *===========================================================================*/
int do_read()
{
  return(read_write(READING));
}

/*===========================================================================*
 *        read_write             *
 *===========================================================================*/
int read_write(int rw_flag)
//int rw_flag;      /* READING or WRITING */
{
/* Perform read(fd, buffer, nbytes) or write(fd, buffer, nbytes) call. */

  register struct inode *rip;
  register struct filp *f;
  mnx_off_t bytes_left, f_size, position;
  unsigned int off, cum_io;
  int op, oflags, r, chunk, usr, seg, block_spec, char_spec;
  int regular, partial_pipe = 0, partial_cnt = 0;
  mnx_mode_t mode_word;
  struct filp *wf;
  int block_size;
  int completed, r2 = OK;
  phys_bytes p;


  /* PM loads segments by putting funny things in other bits of the
   * message, indicated by a high bit in fd.
   */
  if (who_e == PM_PROC_NR && (m_in.fd & _PM_SEG_FLAG)) {
  seg = (int) m_in.m1_p2;
  usr = (int) m_in.m1_p3;
  m_in.fd &= ~(_PM_SEG_FLAG); /* get rid of flag bit */
  } else {
  usr = who_e;    /* normal case */
  seg = D;
  }

 // SVRDEBUG("m_in.nbytes %d\n", m_in.nbytes);
  /* If the file descriptor is valid, get the inode, size and mode. */
  if (m_in.nbytes < 0) return(EMOLINVAL);
  if ((f = get_filp(m_in.fd)) == NIL_FILP) return(err_code);
// SVRDEBUG("NO ES NIL_FILP %d\n", NIL_FILP);
  if (((f->filp_mode) & (rw_flag == READING ? R_BIT : W_BIT)) == 0) {
  return(f->filp_mode == FILP_CLOSED ? EMOLIO : EMOLBADF);
  }
  if (m_in.nbytes == 0)
     return(0); /* so char special files need not check for 0*/

  /* check if user process has the memory it needs.
   * if not, copying will fail later.
   * do this after 0-check above because umap doesn't want to map 0 bytes.
   */
  // if ((r = sys_umap(usr, seg, (vir_bytes) m_in.buffer, m_in.nbytes, &p)) != OK) {
  // printf("FS: read_write: umap failed for process %d\n", usr);
  // return r;
  // }
  position = f->filp_pos;
  oflags = f->filp_flags;
  rip = f->filp_ino;
  f_size = rip->i_size;
  r = OK;
  // SVRDEBUG("READWRITEEEEEEEEEEEEEEEEEEEEEEEEEEEEE %d\n", 0);
  if (rip->i_pipe == I_PIPE) {
  /* fp->fp_cum_io_partial is only nonzero when doing partial writes */
  cum_io = fp->fp_cum_io_partial; 
  } else {
  cum_io = 0;
  }
  op = (rw_flag == READING ? DEV_READ : DEV_WRITE);
  mode_word = rip->i_mode & I_TYPE;
  regular = mode_word == I_REGULAR || mode_word == I_NAMED_PIPE;

  if ((char_spec = (mode_word == I_CHAR_SPECIAL ? 1 : 0))) {
    if (rip->i_zone[0] == NO_DEV)
      panic(__FILE__,"read_write tries to read from "
        "character device NO_DEV", NO_NUM);
    block_size = get_block_size(rip->i_zone[0]);
  }
  if ((block_spec = (mode_word == I_BLOCK_SPECIAL ? 1 : 0))) {
    f_size = ULONG_MAX;
    if (rip->i_zone[0] == NO_DEV)
      panic(__FILE__,"read_write tries to read from "
      " block device NO_DEV", NO_NUM);
    block_size = get_block_size(rip->i_zone[0]);
  }

  if (!char_spec && !block_spec)
    block_size = rip->i_sp->s_block_size;

  rdwt_err = OK;    /* set to EIO if disk error occurs */

  /* Check for character special files. */
  if (char_spec) {
    mnx_dev_t dev;
// SVRDEBUG("dev %d\n", dev);
// SVRDEBUG("usr %d\n", usr);    
  dev = (mnx_dev_t) rip->i_zone[0]; //TODO: Ver aca sino se puede hacer el mismo truco que con commmon_open
  //dev = rip->i_dev;//TODO: Ver si este reemplazo es correcto!!!!!!
SVRDEBUG(" dev_io -> op=%d, dev=%d, POS=%d, BYTES=%d\n",  op, dev, position, m_in.nbytes);    
  r = dev_io(op, dev, usr, m_in.buffer, position, m_in.nbytes, oflags);

// SVRDEBUG("dev %d\n", dev);
// SVRDEBUG("usr %d\n", usr);
// SVRDEBUG("oflags %d\n", oflags);
// SVRDEBUG("position %d\n", position);
// SVRDEBUG("DEV_READ %d\n", DEV_READ);
// SVRDEBUG("DEV_WRITE %d\n", DEV_WRITE);
// SVRDEBUG("op %d\n", op); 

// SVRDEBUG("memcpy de sbbuf \n");  
//   memcpy(m_in.buffer, position, m_in.nbytes);  
  if (r >= 0) {
    cum_io = r;
    position += r;
    r = OK;
  }
  } else {
  if (rw_flag == WRITING && block_spec == 0) {
    /* Check in advance to see if file will grow too big. */
    if (position > rip->i_sp->s_max_size - m_in.nbytes) 
      return(EMOLFBIG);

    /* Check for O_APPEND flag. */
    if (oflags & O_APPEND) position = f_size;

    /* Clear the zone containing present EOF if hole about
     * to be created.  This is necessary because all unwritten
     * blocks prior to the EOF must read as zeros.
     */
    if (position > f_size) clear_zone(rip, f_size, 0);
  }
#ifdef ANULADO

  /* Pipes are a little different.  Check. */
  if (rip->i_pipe == I_PIPE) {
         r = pipe_check(rip, rw_flag, oflags,
            m_in.nbytes, position, &partial_cnt, 0);
         if (r <= 0) return(r);
  }

#endif /*ANULADO*/  

  if (partial_cnt > 0) partial_pipe = 1;

  /* Split the transfer into chunks that don't span two blocks. */
  while (m_in.nbytes != 0) {

    off = (unsigned int) (position % block_size);/* offset in blk*/
    if (partial_pipe) {  /* pipes only */
      chunk = MIN(partial_cnt, block_size - off);
    } else
      chunk = MIN(m_in.nbytes, block_size - off);
    if (chunk < 0) chunk = block_size - off;

    if (rw_flag == READING) {
      bytes_left = f_size - position;
      if (position >= f_size) break;  /* we are beyond EOF */
      if (chunk > bytes_left) chunk = (int) bytes_left;
    }

// SVRDEBUG("READING %d\n", READING);
// SVRDEBUG("WRITING %d\n", WRITING);
// SVRDEBUG("rw_flag %d\n", rw_flag);
    /* Read or write 'chunk' bytes. */
    r = rw_chunk(rip, position, off, chunk, (unsigned) m_in.nbytes,
           rw_flag, m_in.buffer, seg, usr, block_size, &completed);

// SVRDEBUG("despues de rw_chunk %d\n", r);
    if (r != OK) break; /* EOF reached */
    if (rdwt_err < 0) break;

    /* Update counters and pointers. */
    m_in.buffer += chunk; /* user buffer address */
    m_in.nbytes -= chunk; /* bytes yet to be read */
    cum_io += chunk;  /* bytes read so far */
    position += chunk;  /* position within the file */

    if (partial_pipe) {
      partial_cnt -= chunk;
      if (partial_cnt <= 0)  break;
    }
  }
  }

  /* On write, update file size and access time. */
  if (rw_flag == WRITING) {
  if (regular || mode_word == I_DIRECTORY) {
    if (position > f_size) rip->i_size = position;
  }
  } else {
  if (rip->i_pipe == I_PIPE) {
    if ( position >= rip->i_size) {
      /* Reset pipe pointers. */
      rip->i_size = 0;  /* no data left */
      position = 0;   /* reset reader(s) */
      wf = find_filp(rip, W_BIT);
      if (wf != NIL_FILP) wf->filp_pos = 0;
    }
  }
  }
  f->filp_pos = position;

  /* Check to see if read-ahead is called for, and if so, set it up. */
  if (rw_flag == READING && rip->i_seek == NO_SEEK && position % block_size== 0
    && (regular || mode_word == I_DIRECTORY)) {
  rdahed_inode = rip;
  rdahedpos = position;
  }
  rip->i_seek = NO_SEEK;

  if (rdwt_err != OK) r = rdwt_err; /* check for disk error */
  if (rdwt_err == END_OF_FILE) r = OK;

  /* if user-space copying failed, read/write failed. */
  if (r == OK && r2 != OK) {
  r = r2;
  }
  if (r == OK) {
  if (rw_flag == READING) rip->i_update |= ATIME;
  if (rw_flag == WRITING) rip->i_update |= CTIME | MTIME;
  rip->i_dirt = DIRTY;    /* inode is thus now dirty */
  if (partial_pipe) {
    partial_pipe = 0;
      /* partial write on pipe with */
    /* O_NONBLOCK, return write count */
    if (!(oflags & O_NONBLOCK)) {
      fp->fp_cum_io_partial = cum_io;
      suspend(XPIPE);   /* partial write on pipe with */
      return(SUSPEND);  /* nbyte > PIPE_SIZE - non-atomic */
    }
  }
  fp->fp_cum_io_partial = 0;
  return(cum_io);
  }
  return(r);
}

/*===========================================================================*
 *        rw_chunk             *
 *===========================================================================*/
int rw_chunk(register struct inode *rip, mnx_off_t position, unsigned off, int chunk, unsigned left, int rw_flag, char *buff,
 int seg, int usr, int block_size, int *completed)
// register struct inode *rip; /* pointer to inode for file to be rd/wr */
// off_t position;     /* position within file to read or write */
// unsigned off;     /* off within the current block */
// int chunk;      /* number of bytes to read or write */
// unsigned left;       max number of bytes wanted after position 
// int rw_flag;      /* READING or WRITING */
// char *buff;     /* virtual address of the user buffer */
// int seg;      /* T or D segment in user space */
// int usr;      /* which user process */
// int block_size;     /* block size of FS operating on */
// int *completed;     /* number of bytes copied */
{
/* Read or write (part of) a block. */

  register struct buf *bp;
  register int r = OK;
  int n, block_spec;
  mnx_block_t b;
  mnx_dev_t dev;

  *completed = 0;
// SVRDEBUG("INICIO CHUNK %d\n", 1);
  block_spec = (rip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
  if (block_spec) {
  b = position/block_size;
  dev = (mnx_dev_t) rip->i_zone[0];
  } else {
  b = read_map(rip, position);
  dev = rip->i_dev;
  }
// SVRDEBUG("DESPUES DE READMAP %d\n", 1);
  if (!block_spec && b == NO_BLOCK) {
  if (rw_flag == READING) {
// SVRDEBUG("READING NO_BLOCK %d\n", NO_BLOCK);

    /* Reading from a nonexistent block.  Must read as all zeros.*/
    bp = get_block(NO_DEV, NO_BLOCK, NORMAL);    /* get a buffer */
// SVRDEBUG("READING get_block(NO_DEV=%d, NO_BLOCK=%d, NORMAL=%d) bp=%d\n", NO_DEV, NO_BLOCK, NORMAL, bp);
    zero_block(bp);
  } else {
    /* Writing to a nonexistent block. Create and enter in inode.*/
    if ((bp= new_block(rip, position)) == NIL_BUF)return(err_code);
  }
  } else if (rw_flag == READING) {
  /* Read and read ahead if convenient. */
  bp = rahead(rip, b, position, left);
  } else {
  /* Normally an existing block to be partially overwritten is first read
   * in.  However, a full block need not be read in.  If it is already in
   * the cache, acquire it, otherwise just acquire a free buffer.
   */
  n = (chunk == block_size ? NO_READ : NORMAL);
  if (!block_spec && off == 0 && position >= rip->i_size) n = NO_READ;
// SVRDEBUG("READING %d\n", READING);
// SVRDEBUG("WRITING %d\n", WRITING);
// SVRDEBUG("rw_flag %d\n", rw_flag);
// SVRDEBUG("dev, %d, b, %d n, %d\n", dev, b, n);
SVRDEBUG("READING get_block(dev=%d, b=%d, n=%d) \n", dev, b, n);  
  bp = get_block(dev, b, n);
  }

  /* In all cases, bp now points to a valid buffer. */
  if (bp == NIL_BUF) {
    panic(__FILE__,"bp not valid in rw_chunk, this can't happen", NO_NUM);
  }
  if (rw_flag == WRITING && chunk != block_size && !block_spec &&
          position >= rip->i_size && off == 0) {
  zero_block(bp);
  }

  if (rw_flag == READING) {
  /* Copy a chunk from the block buffer to user space. */
	//mnx_vcopy(who_e, path, FS_PROC_NR, user_path, len);  	
     //r = mnx_vcopy(FS_PROC_NR, D, usr, seg, chunk);
     r = mnx_vcopy(FS_PROC_NR, (bp->b_data+off), usr, buff, chunk);
SVRDEBUG("Resultado mnx_vcopy SERVIDOR -> CLIENTE %d\n", r);  
SVRDEBUG("chunk %d\n", chunk);     
     //SVRDEBUG("buff %s\n", buff);
// SVRDEBUG("bp->b_data+off %s\n", bp->b_data+off);
  // r = sys_vircopy(FS_PROC_NR, D, (phys_bytes) (bp->b_data+off),
  //     usr, seg, (phys_bytes) buff,
  //     (phys_bytes) chunk);
  } else {
  /* Copy a chunk from user space to the block buffer. */
	//mnx_vcopy(who_e, path, FS_PROC_NR, user_path, len);  	  	
  // r = sys_vircopy(usr, seg, (phys_bytes) buff,
  //     FS_PROC_NR, D, (phys_bytes) (bp->b_data+off),
  //     (phys_bytes) chunk);
     //r = mnx_vcopy(usr, seg, FS_PROC_NR, D, chunk);         
     r = mnx_vcopy(usr, buff, FS_PROC_NR, (bp->b_data+off), chunk);         
SVRDEBUG("Resultado mnx_vcopy CLIENTE -> SERVIDOR %d\n", r);  
SVRDEBUG("chunk %d\n", chunk);  
//SVRDEBUG("buff %s\n", buff);
// SVRDEBUG("bp->b_data+off %s\n", bp->b_data+off);
     bp->b_dirt = DIRTY;
  }
  n = (off + chunk == block_size ? FULL_DATA_BLOCK : PARTIAL_DATA_BLOCK);
  // SVRDEBUG("ANTES put_block(bp=%d, n=%d) \n", bp, n);  
  put_block(bp, n);

  return(r);
}

/*===========================================================================*
 *        read_map             *
 *===========================================================================*/
mnx_block_t read_map(struct inode *rip, mnx_off_t position)
// register struct inode *rip; /* ptr to inode to map from */
// off_t position;     /* position in file whose blk wanted */
{
/* Given an inode and a position within the corresponding file, locate the
 * block (not zone) number in which that position is to be found and return it.
 */

  struct buf *bp;
  mnx_zone_t z;
  int scale, boff, dzones, nr_indirects, index, zind, ex;
  mnx_block_t b;
  long excess, zone, block_pos;
  
  // SVRDEBUG("READMAP\n");  
  scale = rip->i_sp->s_log_zone_size; /* for block-zone conversion */
  block_pos = position/rip->i_sp->s_block_size; /* relative blk # in file */
  zone = block_pos >> scale;  /* position's zone */
  boff = (int) (block_pos - (zone << scale) ); /* relative blk # within zone */
  dzones = rip->i_ndzones;
  nr_indirects = rip->i_nindirs;

  /* Is 'position' to be found in the inode itself? */
  if (zone < dzones) {
  zind = (int) zone;  /* index should be an int */
  z = rip->i_zone[zind];
  if (z == NO_ZONE) return(NO_BLOCK);
  b = ((mnx_block_t) z << scale) + boff;
  return(b);
  }

  /* It is not in the inode, so it must be single or double indirect. */
  excess = zone - dzones; /* first Vx_NR_DZONES don't count */

  if (excess < nr_indirects) {
  /* 'position' can be located via the single indirect block. */
  z = rip->i_zone[dzones];
  } else {
  /* 'position' can be located via the double indirect block. */
  if ( (z = rip->i_zone[dzones+1]) == NO_ZONE) return(NO_BLOCK);
  excess -= nr_indirects;     /* single indir doesn't count*/
  b = (mnx_block_t) z << scale;
  // SVRDEBUG("ANTES DE DOUBLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);  
  bp = get_block(rip->i_dev, b, NORMAL);  /* get double indirect block */
  // SVRDEBUG("DESPUES DE DOUBLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);  
  index = (int) (excess/nr_indirects);
  z = rd_indir(bp, index);    /* z= zone for single*/
  put_block(bp, INDIRECT_BLOCK);    /* release double ind block */
  excess = excess % nr_indirects;   /* index into single ind blk */
  }

  /* 'z' is zone num for single indirect block; 'excess' is index into it. */
  if (z == NO_ZONE) return(NO_BLOCK);
  b = (mnx_block_t) z << scale;     /* b is blk # for single ind */
  // SVRDEBUG("ANTES DE SIMPLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);  
  bp = get_block(rip->i_dev, b, NORMAL);  /* get single indirect block */
  // SVRDEBUG("DESPUES DE SIMPLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);  
  ex = (int) excess;        /* need an integer */
  z = rd_indir(bp, ex);       /* get block pointed to */
  put_block(bp, INDIRECT_BLOCK);    /* release single indir blk */
  if (z == NO_ZONE) return(NO_BLOCK);
  b = ((mnx_block_t) z << scale) + boff;
  return(b);
}

/*===========================================================================*
 *        rd_indir             *
 *===========================================================================*/
mnx_zone_t rd_indir(struct buf *bp, int index)
// struct buf *bp;      pointer to indirect block 
// int index;      /* index into *bp */
{
/* Given a pointer to an indirect block, read one entry.  The reason for
 * making a separate routine out of this is that there are four cases:
 * V1 (IBM and 68000), and V2 (IBM and 68000).
 */

  struct super_block *sp;
  mnx_zone_t zone;      /* V2 zones are longs (shorts in V1) */

  if(bp == NIL_BUF)
  panic(__FILE__, "rd_indir() on NIL_BUF", NO_NUM);

  sp = get_super(bp->b_dev);  /* need super block to find file sys type */

  /* read a zone from an indirect block */
  if (sp->s_version == V1)
  zone = (mnx_zone_t) conv2(sp->s_native, (int)  bp->b_v1_ind[index]);
  else
  zone = (mnx_zone_t) conv4(sp->s_native, (long) bp->b_v2_ind[index]);

  if (zone != NO_ZONE &&
    (zone < (mnx_zone_t) sp->s_firstdatazone || zone >= sp->s_zones)) {
  printf("Illegal zone number %ld in indirect block, index %d\n",
         (long) zone, index);
  panic(__FILE__,"check file system", NO_NUM);
  }
  return(zone);
}

/*===========================================================================*
 *        read_ahead             *
 *===========================================================================*/
void read_ahead()
{
/* Read a block into the cache before it is needed. */
  int block_size;
  register struct inode *rip;
  struct buf *bp;
  mnx_block_t b;

  rip = rdahed_inode;   /* pointer to inode to read ahead from */
  block_size = get_block_size(rip->i_dev);
  rdahed_inode = NIL_INODE; /* turn off read ahead */
  if ( (b = read_map(rip, rdahedpos)) == NO_BLOCK) return;  /* at EOF */
  bp = rahead(rip, b, rdahedpos, block_size);
  put_block(bp, PARTIAL_DATA_BLOCK);
}

/*===========================================================================*
 *        rahead               *
 *===========================================================================*/
struct buf *rahead(register struct inode *rip, mnx_block_t baseblock, mnx_off_t position, unsigned bytes_ahead)
// register struct inode *rip; /* pointer to inode for file to be read */
// block_t baseblock;     block at current position 
// off_t position;     /* position within file */
// unsigned bytes_ahead;   /* bytes beyond position for immediate use */
{
/* Fetch a block from the cache or the device.  If a physical read is
 * required, prefetch as many more blocks as convenient into the cache.
 * This usually covers bytes_ahead and is at least BLOCKS_MINIMUM.
 * The device driver may decide it knows better and stop reading at a
 * cylinder boundary (or after an error).  Rw_scattered() puts an optional
 * flag on all reads to allow this.
 */
  int block_size;
/* Minimum number of blocks to prefetch. */
# define BLOCKS_MINIMUM   (NR_BUFS < 50 ? 18 : 32)
  int block_spec, scale, read_q_size;
  unsigned int blocks_ahead, fragment;
  mnx_block_t block, blocks_left;
  mnx_off_t ind1_pos;
  mnx_dev_t dev;
  struct buf *bp;
  static struct buf *read_q[NR_BUFS];

  block_spec = (rip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
  if (block_spec) {
  dev = (mnx_dev_t) rip->i_zone[0];
  } else {
  dev = rip->i_dev;
  }
  block_size = get_block_size(dev);

  block = baseblock;
  // SVRDEBUG("ANTES get_block PREFETCH\n");  
  bp = get_block(dev, block, PREFETCH);
  // SVRDEBUG("DESPUES get_block PREFETCH\n");  
  if (bp->b_dev != NO_DEV) return(bp);

  /* The best guess for the number of blocks to prefetch:  A lot.
   * It is impossible to tell what the device looks like, so we don't even
   * try to guess the geometry, but leave it to the driver.
   *
   * The floppy driver can read a full track with no rotational delay, and it
   * avoids reading partial tracks if it can, so handing it enough buffers to
   * read two tracks is perfect.  (Two, because some diskette types have
   * an odd number of sectors per track, so a block may span tracks.)
   *
   * The disk drivers don't try to be smart.  With todays disks it is
   * impossible to tell what the real geometry looks like, so it is best to
   * read as much as you can.  With luck the caching on the drive allows
   * for a little time to start the next read.
   *
   * The current solution below is a bit of a hack, it just reads blocks from
   * the current file position hoping that more of the file can be found.  A
   * better solution must look at the already available zone pointers and
   * indirect blocks (but don't call read_map!).
   */

  fragment = position % block_size;
  position -= fragment;
  bytes_ahead += fragment;

  blocks_ahead = (bytes_ahead + block_size - 1) / block_size;

  if (block_spec && rip->i_size == 0) {
  blocks_left = NR_IOREQS;
  } else {
  blocks_left = (rip->i_size - position + block_size - 1) / block_size;

  /* Go for the first indirect block if we are in its neighborhood. */
  if (!block_spec) {
    scale = rip->i_sp->s_log_zone_size;
    ind1_pos = (mnx_off_t) rip->i_ndzones * (block_size << scale);
    if (position <= ind1_pos && rip->i_size > ind1_pos) {
      blocks_ahead++;
      blocks_left++;
    }
  }
  }

  /* No more than the maximum request. */
  if (blocks_ahead > NR_IOREQS) blocks_ahead = NR_IOREQS;

  /* Read at least the minimum number of blocks, but not after a seek. */
  if (blocks_ahead < BLOCKS_MINIMUM && rip->i_seek == NO_SEEK)
  blocks_ahead = BLOCKS_MINIMUM;

  /* Can't go past end of file. */
  if (blocks_ahead > blocks_left) blocks_ahead = blocks_left;

  read_q_size = 0;

  /* Acquire block buffers. */
  for (;;) {
  read_q[read_q_size++] = bp;

  if (--blocks_ahead == 0) break;

  /* Don't trash the cache, leave 4 free. */
  if (bufs_in_use >= NR_BUFS - 4) break;

  block++;

// SVRDEBUG("ANTES get_block PREFETCH\n");  
  bp = get_block(dev, block, PREFETCH);
  // SVRDEBUG("DESPUES get_block PREFETCH\n");  
  if (bp->b_dev != NO_DEV) {
    /* Oops, block already in the cache, get out. */
    put_block(bp, FULL_DATA_BLOCK);
    break;
  }
  }
  rw_scattered(dev, read_q, read_q_size, READING);
  return(get_block(dev, baseblock, NORMAL));
}