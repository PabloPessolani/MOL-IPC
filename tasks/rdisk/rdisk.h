#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <sched.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../kernel/minix/config.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/syslib.h"
//#include "../../kernel/minix/u64.h"
#include "../../kernel/minix/partition.h"

#include "../../kernel/minix/dvs_usr.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/node_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/com.h"
//#include "../../kernel/minix/proxy_usr.h"
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../kernel/minix/dmap.h"
#include "../../stub_syscall.h"
#include "../rdisk/rdisk_usr.h"

#include <getopt.h>

#define BUFF_SIZE		MAXCOPYBUF
//#define BUFF_SIZE	     4096
#define MAX_MESSLEN     (BUFF_SIZE+1024)
#define MAX_VSSETS      10
#define MAX_MEMBERS     NR_NODES
#define NR_DEVS            2		/* number of minor devices - example*/
#define COMP            1
#define UNCOMP          0

devvec_t devvec[NR_DEVS];

/*Spread message: message m3-ipc, transfer data*/
// typedef struct {message msg; unsigned buffer_data[BUFF_SIZE];} SP_message;	 
typedef struct {
	message msg; 
	struct{
		int flag_buff;	/*compress or uncompress data into buffer*/
		long buffer_size; /* bytes compress or uncompress */
		unsigned buffer_data[BUFF_SIZE];
		} buf;
	}SP_message;	 

SP_message msg_lz4cd; 


// typedef struct {		vector for minor devices
  // char *img_ptr;		pointer - file name to the ram disk image
  // int img_p; 			file descriptor - disk image
  // off_t st_size;    	of stat
  // blksize_t st_blksize; of stat
  // unsigned *localbuff;	buffer to the device
  // unsigned	buff_size;	buffer size for this device
  // int active;			if device active for open value=1, else 0
  // int available;		if device is available to use value=1, else 0. For example, its in configure file
  // int replicated;		
  // /* agregar flags para replicate, bitmap nodes, nr_nodes, compresión, encriptado*/
// } devvec_t;



#include "sp.h"
#include "proto.h"
#include "glo.h"

//#include "super.h" //AGREGUÉ NR_SUPERS =1
//#include "../const.h"
#include "../debug.h" 
#include "../macros.h"

#include "../libdriver/driver.h"

#define SET_BIT(bitmap, bit_nr)    (bitmap |= (1 << bit_nr))
#define CLR_BIT(bitmap, bit_nr)    (bitmap &= ~(1 << bit_nr))
#define TEST_BIT(bitmap, bit_nr)   (bitmap & (1 << bit_nr))

/*For m_dtab*/
#define OPER_NAME 0
#define OPER_OPEN 1
#define OPER_NOP 2
#define OPER_IOCTL 3
#define OPER_PREPARE 4
#define OPER_TRANSF 5
#define OPER_CLEAN 6
#define OPER_GEOM 7
#define OPER_SIG 8
#define OPER_ALARM 9
#define OPER_CANC 10
#define OPER_SEL 11

/* MULTICAST MESSAGE TYPES */
#define		STS_DISCONNECTED	-1
#define RDISK_MULTICAST		0x80	
#define MC_STATUS_INFO     (RDISK_MULTICAST + 1)
#define MC_SYNCHRONIZED    (RDISK_MULTICAST + 2)

#define		STS_SYNCHRONIZED	0
#define	    STS_NEW				1
#define		STS_WAIT4PRIMARY	2
#define		STS_WAIT4SYNC		3
#define		STS_LEAVE			4

#define NO_PRIMARY			(-1)

#define DONOT_REPLICATE		0
#define DO_REPLICATE		1


#define RDISK_TIMEOUT_SEC	5
#define RDISK_TIMEOUT_MSEC	0

_PROTOTYPE( char *m_name, (void) 				);
_PROTOTYPE( struct device *m_prepare, (int device) 		);
_PROTOTYPE( int m_transfer, (int proc_nr, int opcode, off_t position,
					iovec_t *iov, unsigned nr_req) 	);
_PROTOTYPE( int m_do_open, (struct driver *dp, message *m_ptr));
_PROTOTYPE( int m_do_close, (struct driver *dp, message *m_ptr));
_PROTOTYPE( int m_init, (void) );
_PROTOTYPE( void m_geometry, (struct partition *entry));
_PROTOTYPE( int do_nop, (struct driver *dp, message *m_ptr));
_PROTOTYPE( void lz4_data_cd, (unsigned * in_buffer, size_t inbuffer_size, int flag_in));
_PROTOTYPE( void test_config, (char *f_conf));






