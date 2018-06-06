/* This file contains the device dependent part of the drivers for the
 * following special files:
 *     /dev/ram		- RAM disk 
 *     /dev/mem		- absolute memory
 *     /dev/kmem	- kernel virtual memory
 *     /dev/null	- null device (data sink)
 *     /dev/boot	- boot device loaded from boot image 
 *     /dev/zero	- null byte stream generator
 *
 *  Changes:
 *	Apr 29, 2005	added null byte generator  (Jorrit N. Herder)
 *	Apr 09, 2005	added support for boot device  (Jorrit N. Herder)
 *	Jul 26, 2004	moved RAM driver to user-space  (Jorrit N. Herder)
 *	Apr 20, 1992	device dependent/independent split  (Kees J. Bot)
 */
#define _TABLE
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1
#include "memory.h"

//#include "../libdriver/driver.h"
//#include <sys/ioc_memory.h>
//#include "../../kernel/const.h"
//#include "../../kernel/config.h"
//#include "../../kernel/type.h"

//#include <sys/vm.h>

//#include "assert.h"

//#include "local.h"

//#define NR_DEVS            7		/* number of minor devices */
#define NR_DEVS            1		/* number of minor devices - en nuestro caso sólo 1, la imagen q utilizamos*/

//PRIVATE struct device m_geom[NR_DEVS];  /* base and size of each device */
struct device m_geom[NR_DEVS];  /* base and size of each device */

//PRIVATE int m_seg[NR_DEVS];  		/* segment index of each device */

//PRIVATE int m_device;			/* current device */
int m_device;			/* current device */

//PRIVATE struct kinfo kinfo;		/* kernel information */ 
//PRIVATE struct machine machine;		/* machine information */ 

//extern int errno;			/* error number for PM calls */

struct partition entry; /*no en código original, pero para completar los datos*/

//_FORWARD _PROTOTYPE( char *m_name, (void) 				);
_PROTOTYPE( char *m_name, (void) 				);

//FORWARD _PROTOTYPE( struct device *m_prepare, (int device) 		);
_PROTOTYPE( struct device *m_prepare, (int device) 		);

//FORWARD _PROTOTYPE( int m_transfer, (int proc_nr, int opcode, off_t position,
//					iovec_t *iov, unsigned nr_req) 	);
_PROTOTYPE( int m_transfer, (int proc_nr, int opcode, off_t position,
					iovec_t *iov, unsigned nr_req) 	);

//FORWARD _PROTOTYPE( int m_do_open, (struct driver *dp, message *m_ptr) 	);
_PROTOTYPE( int m_do_open, (struct driver *dp, message *m_ptr) 	);
/*MOL: función para "CLOSE"*/
_PROTOTYPE( int m_do_close, (struct driver *dp, message *m_ptr) 	);

//FORWARD _PROTOTYPE( void m_init, (void) );
_PROTOTYPE( void m_init, (void) );

//FORWARD _PROTOTYPE( int m_ioctl, (struct driver *dp, message *m_ptr) 	);

//FORWARD _PROTOTYPE( void m_geometry, (struct partition *entry) 		);
_PROTOTYPE( void m_geometry, (struct partition *entry) 		);


_PROTOTYPE( int do_nop, (struct driver *dp, message *m_ptr) 	);


/* Entry points to this driver. */
//PRIVATE struct driver m_dtab = {
struct driver m_dtab = {
  m_name,	/* current device's name */
  m_do_open,	/* open or mount */
	m_do_close,	/* nothing on a close */
	do_nop,// m_ioctl,	/* specify ram disk geometry */
  m_prepare,	/* prepare for I/O on a given minor device */
  m_transfer,	/* do the I/O */
  nop_cleanup,	/* no need to clean up */
  m_geometry,	/* memory device "geometry" */
	do_nop, //  nop_signal,	/* system signals */
	do_nop, //  nop_alarm,
	do_nop,	//  nop_cancel,
	do_nop, //  nop_select,
  NULL,
  NULL
};

	
message mess;
int img_p; /*puntero a la imagen de disco*/

/*DEFINO EL BUFFER*/
#define BUFF_SIZE				MAXCOPYBUF

// unsigned *localbuff;		/* pointer to the first byte of the local buffer (=disk image)*/	??
unsigned *localbuff;

/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
//PUBLIC int main(void)
int main (int argc, char *argv[] )
{
/* Main program. Initialize the memory driver and start the main loop. */
int vmid, error; /*defino variables locales*/
struct stat img_stat;

	if ( argc != 3) {
 	       fprintf( stderr,"Usage: %s <vmid> <image_name>\n", argv[0] );
 	        exit(1);
    	}
	
	vmid = atoi(argv[1]);
	
	if( vmid < 0 || vmid >= NR_VMS) ERROR_EXIT(EMOLRANGE);
	
/*Binding del proceso memory*/
	m_lpid = getpid(); 
	
TASKDEBUG("Binding process %d to VM %d with MEM_PROC_NR=%d\n", m_lpid, vmid, MEM_PROC_NR);
	m_ep = mnx_bind(vmid, MEM_PROC_NR);
	
	if(m_ep < 0) ERROR_EXIT(m_ep);

TASKDEBUG("MEM_PROC_NR endpoint %d\n", m_ep);

/*fin binding*/

/*datos del archivo imagen*/
	img_ptr = argv[2];
									
	TASKDEBUG("Image name - Driver = %s\n", img_ptr);

	/* get the image file size */
	error = stat(img_ptr,  &img_stat);
	if(error) ERROR_EXIT(errno);

	TASKDEBUG("image size=%d[bytes]\n", img_stat.st_size);
	TASKDEBUG("block size=%d[bytes]\n", img_stat.st_blksize);
	img_size = img_stat.st_size;								
	
/*fin datos del archivo imagen*/


  m_init();	
  driver_task(&m_dtab);		
  return(OK);				
}

/*===========================================================================*
 *				 m_name					     *
 *===========================================================================*/
//PRIVATE char *m_name()
char *m_name()
{
/* Return a name for the current device. */
  //static char name[] = "memory";
  static char name[] = "mol_driver";
  TASKDEBUG("n_name(): %s\n", name);
 		
  return name;  
}

/*===========================================================================*
 *				m_prepare				     *
 *===========================================================================*/
//PRIVATE struct device *m_prepare(device)
struct device *m_prepare(device)
int device;
{
/* Prepare for I/O on a device: check if the minor device number is ok. */
  
  if (device < 0 || device >= NR_DEVS) return(NIL_DEV);
  m_device = device;
  TASKDEBUG("device = %d (m_device = %d)\n", device, m_device);
  TASKDEBUG("Prepare for I/O on a given minor device: (%X;%X), (%u;%u)\n", 
  m_geom[device].dv_base._[0],m_geom[device].dv_base._[1], m_geom[device].dv_size._[0], m_geom[device].dv_size._[1]);
  
  return(&m_geom[device]);
 
}

/*===========================================================================*
 *				m_transfer				     *
 *===========================================================================*/
//PRIVATE int m_transfer(proc_nr, opcode, position, iov, nr_req)
int m_transfer(proc_nr, opcode, position, iov, nr_req)
int proc_nr;			/* process doing the request */
int opcode;			/* DEV_GATHER or DEV_SCATTER */
off_t position;			/* offset on device to read or write */
iovec_t *iov;			/* pointer to read or write request vector */
unsigned nr_req;		/* length of request vector */
{
/* Read or write one the driver's minor devices. */
  phys_bytes mem_phys;
  //int seg;
  unsigned count, tbytes, stbytes, bytes; //left, chunk;
  vir_bytes user_vir;
  struct device *dv;
  unsigned long dv_size;
  int rcode;
  //int s;

  /* Get minor device number and check for /dev/null. */
  dv = &m_geom[m_device];
  dv_size = cv64ul(dv->dv_size); 
  //dv_size = size_buff;
  TASKDEBUG("proc_nr: %d\n", proc_nr);	
  TASKDEBUG("m_ep: %d\n", m_ep);	
  tbytes = 0;
  bytes = 0;
    
  while (nr_req > 0) {
	  
	/* How much to transfer and where to / from. */
	count = iov->iov_size;
	TASKDEBUG("count: %u\n", count);	
	
	user_vir = iov->iov_addr;
	TASKDEBUG("user_vir %X\n", user_vir);	
	
	switch (m_device) { /*en nuestro caso debería ser siempre 0 RAM_DEV*/

	/* Physical copying. Only used to access entire memory. */
	/* Este es el único caso que vamos a trabajar*/
	//case MEM_DEV:
	case RAM_DEV:
	    //if (position >= dv_size) return(OK); 	/* check for EOF */
		if (position >= dv_size) { TASKDEBUG("EOF\n"); return(OK);} 	/* check for EOF */
		
	    //if (position + count > dv_size) count = dv_size - position;
		if (position + count > dv_size) { count = dv_size - position; TASKDEBUG("count dv_size-position: %u\n", count); }
		//si (position=offset en el disco + count=cant bytes q voy a leer/esc) > tamaño del disco
		//a count le doy la diferencia de lo q se puede leer/esc dev_size - position
		
	    mem_phys = cv64ul(dv->dv_base) + position;
		//mem_phys = localbuff + position;
		TASKDEBUG("DRIVER - position I/O(mem_phys) %X\n", mem_phys);
		
	    if (opcode == DEV_GATHER) {			/* copy data */ /*DEV_GATHER lee de un vector en com.h -SCATTER:escribe*/
			/*sys_physcopy(NONE, PHYS_SEG, mem_phys, 
	        	proc_nr, D, user_vir, count);*/
			
			TASKDEBUG("\n<DEV_GATHER>\n");
			
		    stbytes = 0;
			 do	{
				/* LEER DESDE EL ARCHIVO Y CARGAR AL BUFFER -> LUEGO AL BUFFER DEL FS*/
				rcode = lseek(img_p, position, SEEK_SET);  /*puntero q obtuve en m_do_open, position es el offset*/
				TASKDEBUG("lseek: %d\n", rcode);

				bytes = (count > BUFF_SIZE)?BUFF_SIZE:count;
				TASKDEBUG("bytes: %d\n", bytes);		
			
				/* read data from the virtual device file into the local buffer  */
				bytes = read(img_p, localbuff, bytes); /*en localbuff, lee los "bytes" desde img_p*/
				TASKDEBUG("read: %d\n", bytes);
					
				/* copy the data from the local buffer to the requester process address space in other VM */
				rcode = mnx_vcopy(m_ep, localbuff, proc_nr, user_vir, bytes);
				
				TASKDEBUG("DRIVER: mnx_vcopy(DRIVER -> proc_nr) rcode=%d\n", rcode);  
				TASKDEBUG("bytes= %d\n", bytes);     
				TASKDEBUG("DRIVER - Offset (read) %X, Data: %s\n", localbuff, localbuff);			
				TASKDEBUG("mem_phys: %X (in DRIVER)\n", localbuff);			
				TASKDEBUG("user_vir: %X (in proc_nr %d)\n", user_vir, proc_nr);			
			
				if (rcode != 0 ) {
					fprintf( stderr,"VCOPY rcode=%d\n",rcode);
					break;
				}
				stbytes = stbytes + bytes; /*si mnx_vcopy fue exitosa, devuelve cantidad de bytes transferidos*/
												
				position += bytes;
				iov->iov_addr += bytes;
				user_vir = iov->iov_addr;
				TASKDEBUG("user_vir (do-buffer) %X\n", user_vir);	
				count -= bytes;
				} while(count > 0);
			
						
			/* FIN - LEER DESDE EL ARCHIVO Y CARGAR AL BUFFER -> LUEGO AL BUFFER DEL FS*/

		}			
	     else {
			/*sys_physcopy: copiar usando direccionamiento físico*/
			
	        /*sys_physcopy(proc_nr, D, user_vir, 
	        	NONE, PHYS_SEG, mem_phys, count);*/
				
			TASKDEBUG("\n<DEV_SCATTER>\n");
			stbytes = 0;
			do {
				/* DESDE EL BUFFER DEL FS -> AL BUFFER LOCAL Y ESCRIBIR EN EL ARCHIVO*/
			
				rcode = lseek(img_p, position, SEEK_SET);  /*puntero q obtuve en m_do_open, position es el offset*/
				TASKDEBUG("lseek: %d\n", rcode);
				
				bytes = (count > BUFF_SIZE)?BUFF_SIZE:count;
				TASKDEBUG("bytes: %d\n", bytes);
						
				/* copy the data from the requester process address space in other VM  to the local buffer */
				rcode = mnx_vcopy(proc_nr, user_vir, m_ep, localbuff, bytes); /*escribo bufferFS -> bufferlocal*/
				
				TASKDEBUG("DRIVER: mnx_vcopy(proc_nr -> DRIVER)= %d\n", rcode);  
				TASKDEBUG("bytes= %d\n", bytes);     
				TASKDEBUG("mem_phys: %X (in DRIVER)\n", localbuff);			
				TASKDEBUG("user_vir: %X (in proc_nr %d)\n", user_vir, proc_nr);			

				if (rcode != 0 ){
					fprintf(stderr, "VCOPY rcode=%d\n", rcode);
					break;
				}
				else
					stbytes = stbytes + bytes; /*si mnx_vcopy fue exitosa, devuelve cantidad de bytes transferidos*/
																						
				/* write data from local buffer to the  virtual device file */
				bytes = write(img_p, localbuff, bytes); /*en img_p, escribe los "bytes" desde localbuff*/
				TASKDEBUG("write: %d\n", bytes);
				TASKDEBUG("DRIVER - Offset (write) %X, Data: %s\n", localbuff, localbuff);			
				
				position += bytes;
				iov->iov_addr += bytes;
				user_vir = iov->iov_addr;
				TASKDEBUG("user_vir (do-buffer) %X\n", user_vir);	
				count -= bytes;
		
			} while(count > 0);
		
				
			/* FIN - DESDE EL BUFFER DEL FS -> AL BUFFER LOCAL Y ESCRIBIR EN EL ARCHIVO*/
										
	    }
	    break;
	/* Unknown (illegal) minor device. */
	default:
	    return(EINVAL);
	}

	/* Book the number of bytes transferred. Registra el número de bytes transferidos? */
	//position += count; //incremento el offset en el disco por la cantidad (count) de lo q leí o escribí
	//iov->iov_addr += count; //lo mismo para iov_addr donde está la dirección guardada
	
	if ((iov->iov_size -= stbytes) == 0) { iov++; nr_req--; }  /*subtotal bytes, por cada iov_size según posición del vector*/
	
	tbytes += stbytes; /*total de bytes leídos o escritos*/
		
  }
  
 // return(OK);
return(tbytes);
}

/*===========================================================================*
 *				m_do_open				     *
 *===========================================================================*/
//PRIVATE int m_do_open(dp, m_ptr)
int m_do_open(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
TASKDEBUG("m_do_open - device number: %d - OK to open\n", m_device);
/*único dispositivo abierto volcado en memoria, (puntero: ptr) en load_dimage() q se llama desde main, tendría q pasarla m_init*/
/*open sobre el archivo imagen*/
	img_p = open(img_ptr, O_RDWR);
	TASKDEBUG("Open imagen FD=%d\n", img_p);
	TASKDEBUG("Local Buffer %X\n", localbuff);
	TASKDEBUG("Buffer size %d\n", BUFF_SIZE);
	if(img_p < 0) ERROR_EXIT(errno);

// int r;

/* Check device number on open. */
    if (m_prepare(m_ptr->DEVICE) == NIL_DEV) {
		TASKDEBUG("'m_prepare()' %d - NIL_DEV:%d\n", m_prepare(m_ptr->DEVICE), NIL_DEV);
		return(ENXIO);
		}
 	//if (m_prepare(m_ptr->m2_i1) == NIL_DEV) return(ENXIO);  
  
  return(OK);
}

/*===========================================================================*
 *				m_init					     *
 *===========================================================================*/
//PRIVATE void m_init()
void m_init()
{
	int rcode;
	
	rcode = posix_memalign( (void**) &localbuff, getpagesize(), BUFF_SIZE);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
			exit(1);
	}
printf("Aligned Buffer size=%d on address %X\n", BUFF_SIZE, localbuff);
		
/*reemplazo para RAM_DEV posición y tamaño de la imagen de disco volcada*/  
printf("Byte offset to the partition start (img_ptr): %X\n", img_ptr);
	m_geom[RAM_DEV].dv_base = cvul64(img_ptr);
printf("Byte offset to the partition start (m_geom[RAM_DEV].dv_base): %X\n", m_geom[RAM_DEV].dv_base);
	

printf("Number of bytes in the partition (img_size): %u\n", img_size);
	m_geom[RAM_DEV].dv_size = cvul64(img_size);
printf("Number of bytes in the partition (m_geom[RAM_DEV].dv_size): %u\n", m_geom[RAM_DEV].dv_size);

}

/*===========================================================================*
 *				m_geometry				     *
 *===========================================================================*/
//PRIVATE void m_geometry(entry)
void m_geometry(entry)
struct partition *entry;
{
  /* Memory devices don't have a geometry, but the outside world insists. */
  entry->cylinders = div64u(m_geom[m_device].dv_size, SECTOR_SIZE) / (64 * 32);
  entry->heads = 64;
  entry->sectors = 32;
}

/*===========================================================================*
 *				do_close										     *
 *===========================================================================*/
int m_do_close( dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
int rcode;

   TASKDEBUG("Close device number: %d\n", m_device);

	rcode = close(img_p);
	if(rcode < 0) ERROR_EXIT(errno);

return(rcode);	
}

/*===========================================================================*
 *				do_nop									     *
 *===========================================================================*/
int do_nop(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
	TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));
return(OK);	
}

