/* Types and constants shared between the generic and device dependent
 * device driver code.
 */
 
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1

/* Info about and entry points into the device dependent code. */
struct driver {
  _PROTOTYPE( char *(*dr_name), (void) );
  _PROTOTYPE( int (*dr_open), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( int (*dr_close), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( int (*dr_ioctl), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( struct device *(*dr_prepare), (int device) );
  _PROTOTYPE( int (*dr_transfer), (int proc_nr, int opcode, off_t position,
					iovec_t *iov, unsigned nr_req) );
  _PROTOTYPE( void (*dr_cleanup), (void) );
  _PROTOTYPE( void (*dr_geometry), (struct partition *entry) );
  _PROTOTYPE( void (*dr_signal), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( void (*dr_alarm), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( int (*dr_cancel), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( int (*dr_select), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( int (*dr_other), (struct driver *dp, message *m_ptr) );
  _PROTOTYPE( int (*dr_hw_int), (struct driver *dp, message *m_ptr) );
};

/* Base and size of a partition in bytes. */
struct device {
  u64_t dv_base;
  u64_t dv_size;
};

#define NIL_DEV		((struct device *) 0)

/* Functions defined by driver.c: */
_PROTOTYPE( void driver_task, (struct driver *dr) );
// _PROTOTYPE( char *no_name, (void) );
//_PROTOTYPE( int do_nop, (struct driver *dp, message *m_ptr) );
// _PROTOTYPE( struct device *nop_prepare, (int device) );
_PROTOTYPE( void nop_cleanup, (void) );
// _PROTOTYPE( void nop_task, (void) );
// _PROTOTYPE( void nop_signal, (struct driver *dp, message *m_ptr) );
// _PROTOTYPE( void nop_alarm, (struct driver *dp, message *m_ptr) );
// _PROTOTYPE( int nop_cancel, (struct driver *dp, message *m_ptr) );
// _PROTOTYPE( int nop_select, (struct driver *dp, message *m_ptr) );
// _PROTOTYPE( int do_diocntl, (struct driver *dp, message *m_ptr) );

/* Parameters for the disk drive. */
#define SECTOR_SIZE      512	/* physical sector size in bytes */
#define SECTOR_SHIFT       9	/* for division */
#define SECTOR_MASK      511	/* and remainder */

/* Size of the DMA buffer buffer in bytes. */
#define USE_EXTRA_DMA_BUF  0	/* usually not needed */
#define DMA_BUF_SIZE	(DMA_SECTORS * SECTOR_SIZE)

extern phys_bytes tmp_phys;		/* phys address of DMA buffer */

/* Functions defined by rdisk_config.c: */
//_PROTOTYPE( void test_config, (char **f_conf) );

/* Parameters for compress or decompress data. */
#define DEV_CREAD		(DEV_READ * 3) /* read compress data */
#define DEV_CWRITE		(DEV_WRITE * 3 )	/* write compress data */
#define DEV_CGATHER	(DEV_GATHER * 3) 
#define DEV_CSCATTER	(DEV_SCATTER * 3) 
