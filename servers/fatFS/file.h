/* This is the filp table.  It is an intermediary between file descriptors and
 * inodes.  A slot is free if filp_count == 0.
 */

EXTERN struct filp {
  mnx_mode_t filp_mode;		/* RW bits, telling how file is opened */
  int filp_flags;		/* flags from open and fcntl */
  int filp_count;		/* how many file descriptors share this slot?*/
  struct inode *filp_ino;	/* pointer to the inode */
  mnx_off_t filp_pos;		/* file position */

  /* the following fields are for select() and are owned by the generic
   * select() code (i.e., fd-type-specific select() code can't touch these).
   */
  int filp_selectors;		/* select()ing processes blocking on this fd */
  int filp_select_ops;		/* interested in these SEL_* operations */
  
  /* FATFS file or directory struct  */
  FIL fileFat;
  char *fatFilename; /*FATFS filename to simplify stat syscalls*/
  FF_DIR dirFat;

  /* following are for fd-type-specific select() */
  int filp_pipe_select_ops;
} filp[NR_FILPS];

#define FS_FILP_FORMAT "filp_mode=%d filp_flags=%d filp_count=%d filp_pos=%d filp_selectors=%d filp_select_ops=%d filp_pipe_select_ops=%d\n"
#define FS_FILP_FIELDS(p) (int) p->filp_mode,p->filp_flags, p->filp_count, (int) p->filp_pos, p->filp_selectors, p->filp_select_ops, p->filp_pipe_select_ops

#define FILP_CLOSED	0	/* filp_mode: associated device closed */

#define NIL_FILP (struct filp *) 0	/* indicates absence of a filp slot */
