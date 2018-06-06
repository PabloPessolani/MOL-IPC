#define BITMAP_32BITS		32 
#define MNX_CHAR_BIT       8	/* # bits in a char */

#define NODE_RUNNING	0x00000000
#define BIT_DC_FREE	1	/* VM is free */

#define DC_RUNNING	0x00			/* VM is RUNNING */
#define DC_FREE		(1<<BIT_DC_FREE)	/* VM is free */

#define DVS_RUNNING	0x00	/* DVS is RUNNING*/
#define DVS_NO_INIT	(-1)	/* DVS has not been initialized */
#define PROC_NO_PID	(-1)	

#define SUPER_USER (mnx_uid_t) 0	/* uid_t of superuser */
#define MOL_LONG_MAX  2147483647L	/* maximum value of a long */

/* Flag bits for i_mode in the inode. */
#define I_TYPE          0170000	/* this field gives inode type */
#define I_SYMBOLIC_LINK 0120000	/* file is a symbolic link */
#define I_REGULAR       0100000	/* regular file, not dir or special */
#define I_BLOCK_SPECIAL 0060000	/* block special file */
#define I_DIRECTORY     0040000	/* file is a directory */
#define I_CHAR_SPECIAL  0020000	/* character special file */
#define I_NAMED_PIPE	0010000 /* named pipe (FIFO) */
#define I_SET_UID_BIT   0004000	/* set effective uid_t on exec */
#define I_SET_GID_BIT   0002000	/* set effective gid_t on exec */
#define ALL_MODES       0006777	/* all bits for user, group and others */
#define RWX_MODES       0000777	/* mode bits for RWX only */
#define R_BIT           0000004	/* Rwx protection bit */
#define W_BIT           0000002	/* rWx protection bit */
#define X_BIT           0000001	/* rwX protection bit */
#define I_NOT_ALLOC     0000000	/* this inode is free */

/* Some limits. */
#define MAX_BLOCK_NR  ((block_t) 077777777)	/* largest block number */
#define HIGHEST_ZONE   ((zone_t) 077777777)	/* largest zone number */
#define MAX_INODE_NR ((mnx_ino_t) 037777777777)	/* largest inode number */
#define MAX_FILE_POS ((mnx_off_t) 037777777777)	/* largest legal file offset */

#define MAX_SYM_LOOPS	8	/* how many symbolic links are recursed */

//Agregado PID MOLFS
#define NO_BLOCK              ((mnx_block_t) 0)	/* absence of a block number */
#define NO_ENTRY                ((mnx_ino_t) 0)	/* absence of a dir entry */
#define NO_ZONE                ((mnx_zone_t) 0)	/* absence of a zone number */
#define NO_DEV                  ((mnx_dev_t) 0)	/* absence of a device numb */


//ESTO VIENE DE dirent.h ver si incluirlo o usar directo la header !!!

/* The block size must be at least 1024 bytes, because otherwise
 * the superblock (at 1024 bytes) overlaps with other filesystem data.
 */
#define _MIN_BLOCK_SIZE		 1024

/* The below is allocated in some parts of the system as the largest
 * a filesystem block can be. For instance, the boot monitor allocates
 * 3 of these blocks and has to fit within 64kB, so this can't be
 * increased without taking that into account.
 */
#define _MAX_BLOCK_SIZE		 4096

#define _STATIC_BLOCK_SIZE   1024

#define MINIX_HZ	          60	/* clock freq (software settable on IBM-PC) */

/* Miscellaneous */
#define TRUE               1	/* used for turning integers into Booleans */
#define FALSE              0	/* used for turning integers into Booleans */ 
#define BYTE            0377	/* mask for 8 bits */
#define READING            0	/* copy data to user */
#define WRITING            1	/* copy data from user */
#define NO_NUM        0x8000	/* used as numerical argument to panic() */
#define NIL_PTR   (char *) 0	/* generally useful expression */
#define HAVE_SCATTERED_IO  1	/* scattered I/O is now standard */ 

//#define NR_IOREQS      MIN(NR_BUFS, 64)
//#define NR_IOREQS      		64
#define NR_IOREQS		(NR_BUFS<64)?NR_BUFS:64 /* Minimum */

#define CPVEC_NR          16	/* max # of entries in a SYS_VCOPY request */
#define CPVVEC_NR         64	/* max # of entries in a SYS_VCOPY request */


