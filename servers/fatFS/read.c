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
  // SVRDEBUG("do_read()\n");
  return (read_write(READING));
}

/*===========================================================================*
 *        read_write             *
 *===========================================================================*/
int read_write(int rw_flag)
//int rw_flag;      /* READING or WRITING */
{
  /* Perform read(fd, buffer, nbytes) or write(fd, buffer, nbytes) call. */

  //register struct inode *rip;
  register struct filp *f;
  // mnx_off_t bytes_left, f_size, position;
  // unsigned int off, cum_io;
  //int op, oflags, r, chunk, usr, seg, block_spec, char_spec;
  int r, usr, seg, res;
  //int regular, partial_pipe = 0, partial_cnt = 0;
  //mnx_mode_t mode_word;
  //struct filp *wf;
  //int block_size;
  //int completed, r2 = OK;
  // phys_bytes p;

  //PARA FATFS
  FRESULT fr;     /* FatFs function common result code */
  FF_UINT br, bw; /* File read/write count */
  FIL file;
  FILINFO fno;
  FF_DIR *dp;

  // SVRDEBUG("ENTRO A READWRITE\n");
  /* PM loads segments by putting funny things in other bits of the
   * message, indicated by a high bit in fd.
   */
  if (who_e == PM_PROC_NR && (m_in.fd & _PM_SEG_FLAG))
  {
    seg = (int)m_in.m1_p2;
    usr = (int)m_in.m1_p3;
    m_in.fd &= ~(_PM_SEG_FLAG); /* get rid of flag bit */
  }
  else
  {
    usr = who_e; /* normal case */
    seg = D;
  }

  // 	/*---------------------- BUFFER ALIGNMENT-------------------*/
	// res = posix_memalign((void **)localbuff, getpagesize(), MAXCOPYBUF); //OJO VER ESTO
	// if (res)
	// {
	// 	ERROR_EXIT(res);
	// 	exit(1);
	// }

  SVRDEBUG("m_in.nbytes %d\n", m_in.nbytes);
  // SVRDEBUG("NIL_FILP %d\n", NIL_FILP);
  /* If the file descriptor is valid, get the inode, size and mode. */
  if (m_in.nbytes < 0)
    ERROR_RETURN(EMOLINVAL);
  if ((f = get_filp(m_in.fd)) == NIL_FILP)
    ERROR_RETURN(err_code);
  // SVRDEBUG("NO ES NIL_FILP %d\n", NIL_FILP);
  if (((f->filp_mode) & (rw_flag == READING ? R_BIT : W_BIT)) == 0)
  {
    ERROR_RETURN(f->filp_mode == FILP_CLOSED ? EMOLIO : EMOLBADF);
  }
  if (m_in.nbytes == 0)
    ERROR_RETURN(0); /* so char special files need not check for 0*/

  fr = f_stat(f->fatFilename, &fno);

  SVRDEBUG("Filename:%s \n", f->fatFilename);

  // SVRDEBUG("ANTES DE POSITION ETC\n");

  // position = f->filp_pos;// Con estos dos campos
  // oflags = f->filp_flags;// hasta ahora parece ser suficiente para la lectura.
  // rip = f->filp_ino;
  // f_size = rip->i_size;
  // SVRDEBUG("despues de R = OK\n");
  /* Check for character special files. */
  if (!rw_flag) //READING
  {
    // if (fno.fattrib & AM_DIR) /* Directory */
    // {
    //   SVRDEBUG("ES DIRECTORIO=%d\n", AM_DIR);
    //   fr = f_readdir(&dir, user_path);
    //   SVRDEBUG("f_opendir RESULT =%d\n", fr);
    // }
    // else
    // {
      SVRDEBUG("1-) bytesToRead=%d\n", m_in.nbytes);
      fr = f_read(&(f->fileFat), localbuff, m_in.nbytes, &br); /* Read a chunk of source file */
      SVRDEBUG("2-) f_read RESULT:%d \n", fr);
      // SVRDEBUG("3-) localbuff:%s \n", localbuff);
      r = mnx_vcopy(FS_PROC_NR, localbuff, who_e, m_in.buffer, br);
      SVRDEBUG("Resultado mnx_vcopy SERVIDOR -> CLIENTE%d\n", r);
      SVRDEBUG("4-) bytes READ:%d \n", br);
    // }
  }
  else
  {
    SVRDEBUG("1-) bytesToWrite=%d\n", m_in.nbytes);
    r = mnx_vcopy(who_e, m_in.buffer, FS_PROC_NR, localbuff, m_in.nbytes);
    // SVRDEBUG("2-) localbuff:%s \n", localbuff);
    SVRDEBUG("Resultado mnx_vcopy CLIENTE -> SERVIDOR %d\n", r);
    fr = f_write(&(f->fileFat), localbuff, m_in.nbytes, &bw); /* Write a chunk of source file */
    SVRDEBUG("3-) f_write RESULT:%d \n", fr);
    SVRDEBUG("4-) bytes WRITTEN:%d \n", bw);
  }

  // free(localbuff);

  if (r >= 0)
    r = rw_flag ? bw : br;//Si era escritura conteo de bytes escritos y si era lectura de leidos
  return (r);
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
  if (block_spec)
  {
    b = position / block_size;
    dev = (mnx_dev_t)rip->i_zone[0];
  }
  else
  {
    b = read_map(rip, position);
    dev = rip->i_dev;
  }
  // SVRDEBUG("DESPUES DE READMAP %d\n", 1);
  if (!block_spec && b == NO_BLOCK)
  {
    if (rw_flag == READING)
    {
      // SVRDEBUG("READING NO_BLOCK %d\n", NO_BLOCK);

      /* Reading from a nonexistent block.  Must read as all zeros.*/
      bp = get_block(NO_DEV, NO_BLOCK, NORMAL); /* get a buffer */
                                                // SVRDEBUG("READING get_block(NO_DEV=%d, NO_BLOCK=%d, NORMAL=%d) bp=%d\n", NO_DEV, NO_BLOCK, NORMAL, bp);
      zero_block(bp);
    }
    else
    {
      /* Writing to a nonexistent block. Create and enter in inode.*/
      if ((bp = new_block(rip, position)) == NIL_BUF)
        ERROR_RETURN(err_code);
    }
  }
  else if (rw_flag == READING)
  {
    /* Read and read ahead if convenient. */
    bp = rahead(rip, b, position, left);
  }
  else
  {
    /* Normally an existing block to be partially overwritten is first read
   * in.  However, a full block need not be read in.  If it is already in
   * the cache, acquire it, otherwise just acquire a free buffer.
   */
    n = (chunk == block_size ? NO_READ : NORMAL);
    if (!block_spec && off == 0 && position >= rip->i_size)
      n = NO_READ;
    // SVRDEBUG("READING %d\n", READING);
    // SVRDEBUG("WRITING %d\n", WRITING);
    // SVRDEBUG("rw_flag %d\n", rw_flag);
    // SVRDEBUG("dev, %d, b, %d n, %d\n", dev, b, n);
    SVRDEBUG("READING get_block(dev=%d, b=%d, n=%d) \n", dev, (int)b, n);
    bp = get_block(dev, b, n);
  }

  /* In all cases, bp now points to a valid buffer. */
  if (bp == NIL_BUF)
  {
    panic(__FILE__, "bp not valid in rw_chunk, this can't happen", NO_NUM);
  }
  if (rw_flag == WRITING && chunk != block_size && !block_spec &&
      position >= rip->i_size && off == 0)
  {
    zero_block(bp);
  }

  if (rw_flag == READING)
  {
    /* Copy a chunk from the block buffer to user space. */
    //mnx_vcopy(who_e, path, FS_PROC_NR, user_path, len);
    //r = mnx_vcopy(FS_PROC_NR, D, usr, seg, chunk);
    r = mnx_vcopy(FS_PROC_NR, (bp->b_data + off), usr, buff, chunk);
    SVRDEBUG("Resultado mnx_vcopy SERVIDOR -> CLIENTE %d\n", r);
    SVRDEBUG("chunk %d\n", chunk);
    //SVRDEBUG("buff %s\n", buff);
    // SVRDEBUG("bp->b_data+off %s\n", bp->b_data+off);
    // r = sys_vircopy(FS_PROC_NR, D, (phys_bytes) (bp->b_data+off),
    //     usr, seg, (phys_bytes) buff,
    //     (phys_bytes) chunk);
  }
  else
  {
    /* Copy a chunk from user space to the block buffer. */
    //mnx_vcopy(who_e, path, FS_PROC_NR, user_path, len);
    // r = sys_vircopy(usr, seg, (phys_bytes) buff,
    //     FS_PROC_NR, D, (phys_bytes) (bp->b_data+off),
    //     (phys_bytes) chunk);
    //r = mnx_vcopy(usr, seg, FS_PROC_NR, D, chunk);
    r = mnx_vcopy(usr, buff, FS_PROC_NR, (bp->b_data + off), chunk);
    SVRDEBUG("Resultado mnx_vcopy CLIENTE -> SERVIDOR %d\n", r);
    SVRDEBUG("chunk %d\n", chunk);
    //SVRDEBUG("buff %s\n", buff);
    // SVRDEBUG("bp->b_data+off %s\n", bp->b_data+off);
    bp->b_dirt = DIRTY;
  }
  n = (off + chunk == block_size ? FULL_DATA_BLOCK : PARTIAL_DATA_BLOCK);
  // SVRDEBUG("ANTES put_block(bp=%d, n=%d) \n", bp, n);
  put_block(bp, n);

  return (r);
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
  scale = rip->i_sp->s_log_zone_size;             /* for block-zone conversion */
  block_pos = position / rip->i_sp->s_block_size; /* relative blk # in file */
  zone = block_pos >> scale;                      /* position's zone */
  boff = (int)(block_pos - (zone << scale));      /* relative blk # within zone */
  dzones = rip->i_ndzones;
  nr_indirects = rip->i_nindirs;

  /* Is 'position' to be found in the inode itself? */
  if (zone < dzones)
  {
    zind = (int)zone; /* index should be an int */
    z = rip->i_zone[zind];
    if (z == NO_ZONE)
      ERROR_RETURN(NO_BLOCK);
    b = ((mnx_block_t)z << scale) + boff;
    return (b);
  }

  /* It is not in the inode, so it must be single or double indirect. */
  excess = zone - dzones; /* first Vx_NR_DZONES don't count */

  if (excess < nr_indirects)
  {
    /* 'position' can be located via the single indirect block. */
    z = rip->i_zone[dzones];
  }
  else
  {
    /* 'position' can be located via the double indirect block. */
    if ((z = rip->i_zone[dzones + 1]) == NO_ZONE)
      ERROR_RETURN(NO_BLOCK);
    excess -= nr_indirects; /* single indir doesn't count*/
    b = (mnx_block_t)z << scale;
    // SVRDEBUG("ANTES DE DOUBLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);
    bp = get_block(rip->i_dev, b, NORMAL); /* get double indirect block */
    // SVRDEBUG("DESPUES DE DOUBLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);
    index = (int)(excess / nr_indirects);
    z = rd_indir(bp, index);        /* z= zone for single*/
    put_block(bp, INDIRECT_BLOCK);  /* release double ind block */
    excess = excess % nr_indirects; /* index into single ind blk */
  }

  /* 'z' is zone num for single indirect block; 'excess' is index into it. */
  if (z == NO_ZONE)
    ERROR_RETURN(NO_BLOCK);
  b = (mnx_block_t)z << scale; /* b is blk # for single ind */
  // SVRDEBUG("ANTES DE SIMPLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);
  bp = get_block(rip->i_dev, b, NORMAL); /* get single indirect block */
  // SVRDEBUG("DESPUES DE SIMPLE INDIRECTO DEV=%d, b=%d\n", rip->i_dev, b);
  ex = (int)excess;              /* need an integer */
  z = rd_indir(bp, ex);          /* get block pointed to */
  put_block(bp, INDIRECT_BLOCK); /* release single indir blk */
  if (z == NO_ZONE)
    ERROR_RETURN(NO_BLOCK);
  b = ((mnx_block_t)z << scale) + boff;
  return (b);
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
  mnx_zone_t zone; /* V2 zones are longs (shorts in V1) */

  if (bp == NIL_BUF)
    panic(__FILE__, "rd_indir() on NIL_BUF", NO_NUM);

  sp = get_super(bp->b_dev); /* need super block to find file sys type */

  /* read a zone from an indirect block */
  if (sp->s_version == V1)
    zone = (mnx_zone_t)conv2(sp->s_native, (int)bp->b_v1_ind[index]);
  else
    zone = (mnx_zone_t)conv4(sp->s_native, (long)bp->b_v2_ind[index]);

  if (zone != NO_ZONE &&
      (zone < (mnx_zone_t)sp->s_firstdatazone || zone >= sp->s_zones))
  {
    printf("Illegal zone number %ld in indirect block, index %d\n",
           (long)zone, index);
    panic(__FILE__, "check file system", NO_NUM);
  }
  return (zone);
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

  rip = rdahed_inode; /* pointer to inode to read ahead from */
  block_size = get_block_size(rip->i_dev);
  rdahed_inode = NIL_INODE; /* turn off read ahead */
  if ((b = read_map(rip, rdahedpos)) == NO_BLOCK)
    return; /* at EOF */
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
#define BLOCKS_MINIMUM (NR_BUFS < 50 ? 18 : 32)
  int block_spec, scale, read_q_size;
  unsigned int blocks_ahead, fragment;
  mnx_block_t block, blocks_left;
  mnx_off_t ind1_pos;
  mnx_dev_t dev;
  struct buf *bp;
  static struct buf *read_q[NR_BUFS];

  block_spec = (rip->i_mode & I_TYPE) == I_BLOCK_SPECIAL;
  if (block_spec)
  {
    dev = (mnx_dev_t)rip->i_zone[0];
  }
  else
  {
    dev = rip->i_dev;
  }
  block_size = get_block_size(dev);

  block = baseblock;
  // SVRDEBUG("ANTES get_block PREFETCH\n");
  bp = get_block(dev, block, PREFETCH);
  // SVRDEBUG("DESPUES get_block PREFETCH\n");
  if (bp->b_dev != NO_DEV)
    return (bp);

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

  if (block_spec && rip->i_size == 0)
  {
    blocks_left = NR_IOREQS;
  }
  else
  {
    blocks_left = (rip->i_size - position + block_size - 1) / block_size;

    /* Go for the first indirect block if we are in its neighborhood. */
    if (!block_spec)
    {
      scale = rip->i_sp->s_log_zone_size;
      ind1_pos = (mnx_off_t)rip->i_ndzones * (block_size << scale);
      if (position <= ind1_pos && rip->i_size > ind1_pos)
      {
        blocks_ahead++;
        blocks_left++;
      }
    }
  }

  /* No more than the maximum request. */
  if (blocks_ahead > NR_IOREQS)
    blocks_ahead = NR_IOREQS;

  /* Read at least the minimum number of blocks, but not after a seek. */
  if (blocks_ahead < BLOCKS_MINIMUM && rip->i_seek == NO_SEEK)
    blocks_ahead = BLOCKS_MINIMUM;

  /* Can't go past end of file. */
  if (blocks_ahead > blocks_left)
    blocks_ahead = blocks_left;

  read_q_size = 0;

  /* Acquire block buffers. */
  for (;;)
  {
    read_q[read_q_size++] = bp;

    if (--blocks_ahead == 0)
      break;

    /* Don't trash the cache, leave 4 free. */
    if (bufs_in_use >= NR_BUFS - 4)
      break;

    block++;

    // SVRDEBUG("ANTES get_block PREFETCH\n");
    bp = get_block(dev, block, PREFETCH);
    // SVRDEBUG("DESPUES get_block PREFETCH\n");
    if (bp->b_dev != NO_DEV)
    {
      /* Oops, block already in the cache, get out. */
      put_block(bp, FULL_DATA_BLOCK);
      break;
    }
  }
  rw_scattered(dev, read_q, read_q_size, READING);
  return (get_block(dev, baseblock, NORMAL));
}