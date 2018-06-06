/* The <sys/mnx_stat.h> header defines a struct that is used in the mnx_stat() and
 * fstat functions.  The information in this struct comes from the i-node of
 * some file.  These calls are the only approved way to inspect i-nodes.
 */
#ifndef _MNX_STAT_H
#define _MNX_STAT_H 1

struct mnx_stat {
  mnx_dev_t st_dev;			/* major/minor device number */
  mnx_ino_t st_ino;			/* i-node number */
  mnx_mode_t st_mode;		/* file mode, protection bits, etc. */
  short int st_nlink;		/* # links; TEMPORARY HACK: should be nlink_t*/
  mnx_uid_t st_uid;			/* uid of the file's owner */
  short int st_gid;		/* gid; TEMPORARY HACK: should be gid_t */
  mnx_dev_t st_rdev;
  mnx_off_t st_size;		/* file size */
  mnx_time_t st_atime;		/* time of last access */
  mnx_time_t st_mtime;		/* time of last data modification */
  mnx_time_t st_ctime;		/* time of last file status change */
};

/* Traditional mask definitions for st_mode. */
#define S_IFMT  0170000	/* type of file */
#define S_IFLNK 0120000	/* symbolic link */
#define S_IFREG 0100000	/* regular */
#define S_IFBLK 0060000	/* block special */
#define S_IFDIR 0040000	/* directory */
#define S_IFCHR 0020000	/* character special */
#define S_IFIFO 0010000	/* this is a FIFO */
#define S_ISUID 0004000	/* set user id on execution */
#define S_ISGID 0002000	/* set group id on execution */
				/* next is reserved for future use */
#define S_ISVTX   01000		/* save swapped text even after use */

/* POSIX masks for st_mode. */
#define S_IRWXU   00700		/* owner:  rwx------ */
#define S_IRUSR   00400		/* owner:  r-------- */
#define S_IWUSR   00200		/* owner:  -w------- */
#define S_IXUSR   00100		/* owner:  --x------ */

#define S_IRWXG   00070		/* group:  ---rwx--- */
#define S_IRGRP   00040		/* group:  ---r----- */
#define S_IWGRP   00020		/* group:  ----w---- */
#define S_IXGRP   00010		/* group:  -----x--- */

#define S_IRWXO   00007		/* others: ------rwx */
#define S_IROTH   00004		/* others: ------r-- */ 
#define S_IWOTH   00002		/* others: -------w- */
#define S_IXOTH   00001		/* others: --------x */

/* Synonyms for above. */
#define S_IEXEC		S_IXUSR
#define S_IWRITE	S_IWUSR
#define S_IREAD		S_IRUSR

/* The following macros test st_mode (from POSIX Sec. 5.6.1.1). */
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)	/* is a reg file */
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)	/* is a directory */
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)	/* is a char spec */
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)	/* is a block spec */
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)	/* is a symlink */
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	/* is a pipe/FIFO */

#endif /* _STAT_H */
