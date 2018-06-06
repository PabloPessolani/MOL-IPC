#ifndef _DMAP_H
#define _DMAP_H

#include <minix/sys_config.h>
#include <minix/ipc.h>

/*===========================================================================*
 *               	 Device <-> Driver Table  			     *
 *===========================================================================*/

/* Device table.  This table is indexed by major device number.  It provides
 * the link between major device numbers and the routines that process them.
 * The table can be update dynamically. The field 'dmap_flags' describe an 
 * entry's current status and determines what control options are possible. 
 */
#define DMAP_MUTABLE	0x01	/* mapping can be overtaken */
#define DMAP_BUSY		0x02	/* driver busy with request */
#define DMAP_BABY		0x04	/* driver exec() not done yet */
#define DMAP_CONFIGURED 0x08	/* the driver has a configuration file entry  */

enum dev_style { STYLE_DEV, STYLE_NDEV, STYLE_TTY, STYLE_CLONE };

struct dconf_s {
   char* dev_name;
   int major;
   int minor;
   int type;
   char* filename;
   int	image_fd;
   int root_dev;
   int buffer_size;
   int volatile_type;
   char* server;
   int port;
   int compression;
 } ;
typedef struct dconf_s dconf_t;

struct dmap_s {
	int (*dmap_opcl) (int, mnx_dev_t, int, int);
	int (*dmap_io) (int, message *);
	int dmap_driver;
	int dmap_flags;
	dconf_t dmap_cfg;
 };
typedef struct dmap_s dmap_t;



#define DMAP_FORMAT "\tmap_driver=%d \tdmap_flags=%X\n"
#define DMAP_FIELDS(p) p->dmap_driver,p->dmap_flags
#define DCONF_FORMAT "\tname=%s \tmajor=%d \n\tminor=%d \n\ttype=%d \n\timage_file=%s \n\troot_dev=%d \n\tbuffer_size=%d \n\tvolatile=%d \n\tserver=%s \n\tport=%d \n\tcompression=%d\n"
#define DCONF_FIELDS(p) p->dev_name,p->major,p->minor,p->type,p->filename,p->root_dev,p->buffer_size,p->volatile_type,p->server ,p->port,p->compression

/*===========================================================================*
 *               	 Major and minor device numbers  		     *
 *===========================================================================*/

/* Total number of different devices. */
#define NR_DEVICES   		     32			/* number of (major) devices */

/* Major and minor device numbers for MEMORY driver. */
#define MEMORY_MAJOR  		   1	/* major device for memory devices */
#  define RAM_DEV     		   0	/* minor device for /dev/ram */
#  define MEM_DEV     		   1	/* minor device for /dev/mem */
#  define KMEM_DEV    		   2	/* minor device for /dev/kmem */
#  define NULL_DEV    		   3	/* minor device for /dev/null */
#  define BOOT_DEV    		   4	/* minor device for /dev/boot */
#  define ZERO_DEV    		   5	/* minor device for /dev/zero */

# ifdef MOLFS
#  define IMGRD_DEV   		   RAM_DEV	 /* minor device for /dev/imgrd */
#  define IMGRD_DEV1   		   RAM_DEV+1 /* minor device for /dev/imgrd */
# else /*MOLFS*/
#  define IMGRD_DEV   		   6	/* minor device for /dev/imgrd */
# endif /*MOLFS*/


#define CTRLR(n) ((n)==0 ? 3 : (8 + 2*((n)-1)))	/* magic formula */


/******************** MOLFS IMG DEVICE TYPE *****************************/
// Defined in moldevcfg.c
// #define  MOL_FILE_IMG    0x0100  /* FILE image typedef   */
// #define  MOL_MEMORY_IMG  0x0200  /* MEMORY image typedef */
// #define  MOL_RDISK_IMG   0x0300  /* RDISK image typedef  */
// #define  MOL_NBD_IMG     0x0400  /* NBD image typedef  */

/* Full device numbers that are special to the boot monitor and FS. */
# define DEV_RAM	     0x0100	/* device number of /dev/ram */
# define DEV_RAM1	     DEV_RAM+1	/* device number of /dev/ram */ 
# define DEV_BOOT	     0x0104	/* device number of /dev/boot */

# ifdef MOLFS
#  define DEV_IMGRD	      DEV_RAM	/* device number of /dev/imgrd */
# else /*MOLFS*/
#  define DEV_IMGRD	      0x0106	/* device number of /dev/imgrd */
# endif /*MOLFS*/

# define FLOPPY_MAJOR	    2	  /* major device for floppy disks */
# define TTY_MAJOR		    4	  /* major device for ttys */
# define CTTY_MAJOR		    5	  /* major device for /dev/tty */

# define INET_MAJOR		    7	  /* major device for inet */

# define RESCUE_MAJOR	    9   /* major device for rescue */

# ifdef MOLFS
  #define FILMNT_MAJOR	    12  /* major device for local file mount MOLFS */
# endif /*MOLFS*/

# define LOG_MAJOR		    15  /* major device for log driver */
# define IS_KLOG_DEV	       0	  /* minor device for /dev/klog */


#endif /* _DMAP_H */
