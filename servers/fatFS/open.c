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
// #define SVRDBG 1

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

  if (fetch_name(m_in.name, m_in.name_length, M3) != OK)
    return (err_code);
  r = common_open(O_WRONLY | O_CREAT | O_TRUNC, (mnx_mode_t)m_in.mode);
  return (r);
}

int do_open()
{
  int create_mode = 0; /* ver si esto aplica */
  int r;

  SVRDEBUG("m_in.mode=%d\n", m_in.mode);
  // SVRDEBUG("O_CREAT=%d O_CREAT(OCT)=%#o\n", O_CREAT, O_CREAT);
  if (m_in.mode & O_CREAT)
  {
    SVRDEBUG("M1\n");
    create_mode = FA_CREATE_NEW | FA_CREATE_ALWAYS;
    r = fetch_name(m_in.c_name, m_in.name1_length, M1);
  }
  else
  { 
    SVRDEBUG("M3\n");
    if(m_in.mode & O_RDONLY)
      create_mode = FA_OPEN_EXISTING | FA_READ;
    else if  (m_in.mode & O_WRONLY)
      create_mode = FA_OPEN_EXISTING | FA_WRITE;
    else
      create_mode = FA_OPEN_EXISTING | FA_READ | FA_WRITE;

    // SVRDEBUG("m_in.name=%p\n", m_in.name);
    SVRDEBUG("m_in.name_length=%d\n", m_in.name_length);
    r = fetch_name(m_in.name, m_in.name_length, M3);    
  }

  r = common_open(m_in.mode, create_mode);
  //TODO: ver aca si hay que realizar algun control antes de retornar el FD devuelto por common_open()

  return (r); 
}

/*===========================================================================*
 *        common_open            *
 *===========================================================================*/
int common_open(int oflags, mnx_mode_t omode)
{
  /* Common code from do_creat and do_open. */

  int r;
  //int exist = TRUE;
  // mnx_dev_t dev;
  mnx_mode_t bits;
  struct filp *fil_ptr;

  FIL file;
  FRESULT fr;
  FILINFO fno;  
  FF_DIR dir;

  SVRDEBUG("oflags=%X omode=%X\n", oflags, omode);
  /* Remap the bottom two bits of oflags. */
  bits = (mnx_mode_t)mode_map[oflags & O_ACCMODE];
  /* See if file descriptor and filp slots are available. */
  if ((r = get_fd(0, bits, &m_in.fd, &fil_ptr)) != OK)
    ERROR_RETURN(r);

  SVRDEBUG("oflags=%X \n", oflags);
  SVRDEBUG("user_path=%s\n", user_path);

  if (omode & O_CREAT)
  {
    SVRDEBUG("Crear archivo!\n");
    // FATFS f_open needs the right access to create a file
    omode = FA_CREATE_NEW | FA_CREATE_ALWAYS;
    SVRDEBUG("oflags=%X omode=%X\n", oflags, omode);
  }

  fr = f_stat(user_path, &fno);
  SVRDEBUG("f_stat RESULT =%d\n", fr);

  if (fno.fattrib & AM_DIR) /* Directory */
  {
    SVRDEBUG("ES DIRECTORIO=%d\n", AM_DIR); 
    fr = f_opendir(&dir, user_path);  
    SVRDEBUG("f_opendir RESULT =%d\n", fr); 
  }
  else
  {
    fr = f_open(&file, user_path, omode);
    SVRDEBUG("f_open RESULT =%d\n", fr);
    fr = f_sync(&file); // 
    SVRDEBUG("f_sync RESULT =%d\n", fr);
    fr = f_stat(user_path, &fno);
    SVRDEBUG("f_stat RESULT =%d\n", fr);    
    SVRDEBUG("Size: %lu\n", fno.fsize);
  }

  
  //TODO: Controlar errores para devolver FD menor que cero!!!!!
  

  /* Claim the file descriptor and filp slot and fill them in. */
  fp->fp_filp[m_in.fd] = fil_ptr;
  MOL_FD_SET(m_in.fd, &fp->fp_filp_inuse);
  // SVRDEBUG("IS SET?????=%d\n", MOL_FD_ISSET(m_in.fd, &fp->fp_filp_inuse));
  fil_ptr->filp_count = 1;
  fil_ptr->fileFat = file; //Saving the FATFS File Structure
  fil_ptr->dirFat = dir; //Saving the FATFS Directory Structure
  fil_ptr->fatFilename = user_path; //Saving the FATFS Filename for future possible purposes
  fil_ptr->filp_flags = oflags;

  SVRDEBUG("FD: m_in.fd=%d \n", m_in.fd);
  // SVRDEBUG("1-) fileFat SIZE=%d\n", fil_ptr->fileFat.obj.objsize);
  // SVRDEBUG(FS_FILP_FORMAT, FS_FILP_FIELDS(fil_ptr));

  // SVRDEBUG("exist=%d \n", exist);

  // /* If error, release inode. */
  // if (r < OK) {
  //   if (r == SUSPEND) ERROR_RETURN(r);   /* Oops, just suspended */
  //   //Ver que tipo de error para poner aca y si es necesario
  //   ERROR_RETURN(r);
  // }
  // SVRDEBUG("m_in.fd=%d \n", m_in.fd);
  
  // free(fno);
  // free(file);
  // free(dir);
  return (m_in.fd);
}

/*===========================================================================*
 *        do_mkdir             *
 *===========================================================================*/
int do_mkdir()
{
  /* Perform the mkdir(name, mode) system call. */

  // FIL file;
  FRESULT fr;
  // FILINFO fno;  
  // FF_DIR dir;
  
  if (fetch_name(m_in.name1, m_in.name1_length, M1) != OK)
    return (err_code);

  fr = f_mkdir(user_path);
  SVRDEBUG("f_mkdir RESULT =%d\n", fr);

  switch (fr) {
    case FR_OK:
      err_code = OK;
      break;
    case FR_DISK_ERR:
    SVRDEBUG("FR_DISK_ERR =%d\n", FR_DISK_ERR);
    SVRDEBUG("EMOLIO =%d\n", EMOLIO);
      err_code = EMOLIO;
      break;
    case FR_INT_ERR:
      err_code = EMOLINTR;
      break;
    case FR_NOT_READY:
      err_code = EMOLBUSY;
      break;
    case FR_NO_PATH:
      err_code = EMOLNXIO;
      break;
    case FR_INVALID_NAME:
      err_code = EMOLINVAL;
      break;
    case FR_DENIED:
      err_code = EMOLPERM;
      break;
    case FR_EXIST:
      SVRDEBUG("FR_EXIST =%d\n", FR_EXIST);
      SVRDEBUG("EMOLEXIST =%d\n", EMOLEXIST);    
      err_code = EMOLEXIST;
      break;
    case FR_WRITE_PROTECTED:
      err_code = EMOLROFS;
      break;
    case FR_INVALID_DRIVE:
      err_code = EMOLNXIO;
      break;
    case FR_NOT_ENABLED:
      err_code = EMOLPERM;
      break;
    case FR_NO_FILESYSTEM:
      err_code = EMOLNODEV;
      break;
    case FR_TIMEOUT:
      err_code = EMOLTIMEDOUT;
      break;
    case FR_NOT_ENOUGH_CORE:
      err_code = EMOLNOMEM;
      break;      
    default:
      err_code = EMOLINVAL;
    }

  return (err_code); /* new_node() always sets 'err_code' */
}

/*===========================================================================*
 *        do_close             *
 *===========================================================================*/
int do_close()
{
  /* Perform the close(fd) system call. */

  struct filp *rfilp;
  FIL file;
  FRESULT fr;
  FILINFO fno;  
  FF_DIR dir;

  /* First locate the inode that belongs to the file descriptor. */
  if ((rfilp = get_filp(m_in.fd)) == NIL_FILP) //TODO: Ver aca luego para cerrar el archivo en FATFS
    ERROR_RETURN (err_code);

  SVRDEBUG("Closing file=%s\n", rfilp->fatFilename);

   fr = f_stat(rfilp->fatFilename, &fno);
  SVRDEBUG("f_stat RESULT =%d\n", fr);

  if (fno.fattrib & AM_DIR) /* Directory */
  {
    SVRDEBUG("ES DIRECTORIO=%d\n", AM_DIR); 
    fr = f_closedir(&(rfilp->dirFat));  
    SVRDEBUG("f_closedir RESULT =%d\n", fr); 
  }
  else
  {
    fr = f_close(&(rfilp->fileFat));
    SVRDEBUG("f_close RESULT =%d\n", fr);
  }
 
  
  rfilp->filp_count = 0;
  fp->fp_cloexec &= ~(1L << m_in.fd); /* turn off close-on-exec bit */
  fp->fp_filp[m_in.fd] = NIL_FILP;
  MOL_FD_CLR(m_in.fd, &fp->fp_filp_inuse);

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

  FRESULT fr;

  /* Check to see if the file descriptor is valid. */
  if ((rfilp = get_filp(m_in.ls_fd)) == NIL_FILP)
    ERROR_RETURN (err_code);

  /* No lseek on pipes. */
  // if (rfilp->filp_ino->i_pipe == I_PIPE)
  //   return (EMOLSPIPE);

  /* The value of 'whence' determines the start position to use. */
  switch (m_in.whence)
  {
  case SEEK_SET:
    pos = 0;
    SVRDEBUG("SEEK_SET pos=%d\n", pos);
    break;
  case SEEK_CUR:
    //FAT filepointer field
    pos = rfilp->fileFat.fptr;
    SVRDEBUG("SEEK_CUR fileFat pointer=%d\n", pos);
    break;
  case SEEK_END:    
    SVRDEBUG("SEEK_END fileFat pointer=SIZE=%d\n", pos);
    //FAT filesize field
    pos = rfilp->fileFat.obj.objsize;
    break;
  default:
    ERROR_RETURN (EMOLINVAL);
  }

  /* Check for overflow. */
  if (((long)m_in.offset > 0) && ((long)(pos + m_in.offset) < (long)pos))
    ERROR_RETURN (EMOLINVAL);
  if (((long)m_in.offset < 0) && ((long)(pos + m_in.offset) > (long)pos))
    ERROR_RETURN (EMOLINVAL);
  pos = pos + m_in.offset;

  //FAT f_seek
  fr = f_lseek(&(rfilp->fileFat), pos);
  //TODO: Chequear fr para posibles errores para devolver con ERROR_RETURN

  rfilp->filp_pos = pos;
  m_out.reply_l1 = pos; /* insert the long into the output message */
  return (OK);
}

/*===========================================================================*
 *                             do_slink              *
 *===========================================================================*/
int do_slink()
{
  /* Perform the symlink(name1, name2) system call. */

  register int r;        /* error code */
  // char string[NAME_MAX]; /* last component of the new dir's path name */
  // struct inode *sip;     /* inode containing symbolic link */
  // struct buf *bp;        /* disk buffer for link */
  // struct inode *ldirp;   /* directory containing link */

  // if (fetch_name(m_in.name2, m_in.name2_length, M1) != OK)
  //   return (err_code);

  // if (m_in.name1_length <= 1 || m_in.name1_length >= _MIN_BLOCK_SIZE)
  //   return (EMOLNAMETOOLONG);

  // /* Create the inode for the symlink. */
  // sip = new_node(&ldirp, user_path, (mnx_mode_t)(I_SYMBOLIC_LINK | RWX_MODES),
  //                (mnx_zone_t)0, TRUE, string);

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
  // if ((r = err_code) == OK)
  // {
  //   r = (bp = new_block(sip, (mnx_off_t)0)) == NIL_BUF
  //           ? err_code
  //           : mnx_vcopy(who_e, m_in.name1,
  //                       SELF, bp->b_data,
  //                       m_in.name1_length - 1);

  //   if (r == OK)
  //   {
  //     bp->b_data[_MIN_BLOCK_SIZE - 1] = '\0';
  //     sip->i_size = strlen(bp->b_data);
  //     if (sip->i_size != m_in.name1_length - 1)
  //     {
  //       /* This can happen if the user provides a buffer
  //        * with a \0 in it. This can cause a lot of trouble
  //        * when the symlink is used later. We could just use
  //        * the strlen() value, but we want to let the user
  //        * know he did something wrong. ENAMETOOLONG doesn't
  //        * exactly describe the error, but there is no
  //        * ENAMETOOWRONG.
  //        */
  //       r = EMOLNAMETOOLONG;
  //     }
  //   }

  //   put_block(bp, DIRECTORY_BLOCK); /* put_block() accepts NIL_BUF. */

  //   if (r != OK)
  //   {
  //     sip->i_nlinks = 0;
  //     if (search_dir(ldirp, string, (mnx_ino_t *)0, DELETE) != OK)
  //       panic(__FILE__, "Symbolic link vanished", NO_NUM);
  //   }
  // }

  // /* put_inode() accepts NIL_INODE as a noop, so the below are safe. */
  // put_inode(sip);
  // put_inode(ldirp);

  return (r);
}