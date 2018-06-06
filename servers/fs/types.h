#define CHAR_BIT           8		/* # bits in a char */
#define TIMEOUT_MS		60000	/* IPC Timeout in Miliseconds */

typedef unsigned long irq_id_t;	
typedef short sys_id_t;			/* system process index */
typedef unsigned short bitchunk_t; /* collection of bits in a bitmap */
typedef unsigned long ksigset_t;

#define BITCHUNK_BITS   (sizeof(bitchunk_t) * CHAR_BIT)
#define BITMAP_CHUNKS(nr_bits) (((nr_bits)+BITCHUNK_BITS-1)/BITCHUNK_BITS)

typedef struct {			/* bitmap for system indexes */
  bitchunk_t chunk[BITMAP_CHUNKS(NR_SYS_PROCS)];
} sys_map_t;

typedef struct {			/* bitmap for VM indexes */
  bitchunk_t chunk[BITMAP_CHUNKS(NR_VMS)];
} vm_map_t;

typedef struct {			/* bitmap for NODES indexes */
  bitchunk_t chunk[BITMAP_CHUNKS(NR_NODES)];
} node_map_t;


/* Signal handler type, e.g. SIG_IGN */
typedef void (*sighandler_t)(int);

/* Types used in disk, inode, etc. data structures. */
typedef short          mnx_dev_t;	   /* holds (major|minor) device pair */
typedef char           mnx_gid_t;	   /* group id */
typedef unsigned long  mnx_ino_t; 	   /* i-node number (V3 filesystem) */
typedef unsigned short mnx_mode_t;	   /* file type and permissions bits */
typedef short          mnx_nlink_t;	   /* number of links to a file */
typedef unsigned long  mnx_off_t;	   /* offset within a file */
typedef int            mnx_pid_t;	   /* process id (must be signed) */
typedef short          mnx_uid_t;	   /* user id */
typedef unsigned long  mnx_zone_t;	   /* zone number */
typedef unsigned long  mnx_block_t;	   /* block number */
typedef unsigned long  mnx_bit_t;	   /* bit number in a bit map */
typedef unsigned short mnx_zone1_t;	   /* zone number for V1 file systems */
typedef unsigned short mnx_bitchunk_t; /* collection of bits in a bitmap */

typedef unsigned char   u8_t;	   /* 8 bit type */
typedef unsigned short u16_t;	   /* 16 bit type */
typedef unsigned long  u32_t;	   /* 32 bit type */

typedef char            i8_t;      /* 8 bit signed type */
typedef short          i16_t;      /* 16 bit signed type */
typedef long           i32_t;      /* 32 bit signed type */

typedef struct { u32_t _[2]; } u64_t;

typedef unsigned long  Ino_t;

#if _EM_WSIZE == 2
/*typedef unsigned int      Ino_t; Ino_t is now 32 bits */
typedef unsigned int    Zone1_t;
typedef unsigned int Bitchunk_t;
typedef unsigned int      U16_t;
typedef unsigned int  _mnx_Mode_t;

#else /* _EM_WSIZE == 4, or _EM_WSIZE undefined */
/*typedef int	          Ino_t; Ino_t is now 32 bits */
typedef int 	        Zone1_t;
typedef int	     Bitchunk_t;
typedef int	          U16_t;
typedef int         _mnx_Mode_t;

#endif /* _EM_WSIZE == 2, etc */

/* Devices. */   
#define MAJOR              8    /* major device = (dev>>MAJOR) & 0377 */
#define MINOR              0    /* minor device = (dev>>MINOR) & 0377 */

#ifndef minor
#define minor(dev)      (((dev) >> MINOR) & 0xff)
#endif

#ifndef major
#define major(dev)      (((dev) >> MAJOR) & 0xff)
#endif

#ifndef makedev
#define makedev(major, minor)   \
                        ((dev_t) (((major) << MAJOR) | ((minor) << MINOR)))
#endif
