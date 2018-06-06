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
#include "memory.h"

//#include <asm/ptrace.h>
//#include <stdio.h>
//#include <string.h>
//#include <unistd.h>
//#include <stdlib.h>
//#include "./kernel/minix/config.h"



//#include "../drivers.h"
//#include "../libdriver/driver.h"
//#include <sys/ioc_memory.h>
//#include "../../kernel/const.h"
//include "../../kernel/config.h"
//#include "../../kernel/type.h"

//#include <sys/vm.h>

//#include "assert.h"

//#include "local.h"

//#define NR_DEVS            7		/* number of minor devices */
#define NR_DEVS            1		/* number of minor devices - en nuestro caso sólo 1, la imagen q utilizamos*/

//defino un buffer igual al tamaño del dispositivo
#define MAXBUF	1024

struct device m_geom[NR_DEVS];  /* base and size of each device */
//PRIVATE int m_seg[NR_DEVS];  		/* segment index of each device */
//PRIVATE int m_device;			/* current device */
int m_device;			/* current device */

struct partition entry; /*no en código original, pero para completar los datos*/

//PRIVATE struct kinfo kinfo;		/* kernel information */ 
//struct kinfo kinfo;		/* kernel information */ 
//PRIVATE struct machine machine;		/* machine information */ 

//extern int errno;			/* error number for PM calls */

//FORWARD _PROTOTYPE( char *m_name, (void) 				);
_PROTOTYPE( char *m_name, (void) 				);
 
//FORWARD _PROTOTYPE( struct device *m_prepare, (int device) 		);
_PROTOTYPE( struct device *m_prepare, (int device) 		);

//FORWARD _PROTOTYPE( int m_transfer, (int proc_nr, int opcode, off_t position,
//					iovec_t *iov, unsigned nr_req) 	);
_PROTOTYPE( int m_transfer, (int proc_nr, int opcode, off_t position,
					iovec_t *iov, unsigned nr_req) 	);


//FORWARD _PROTOTYPE( int m_do_open, (struct driver *dp, message *m_ptr) 	);
_PROTOTYPE( int m_do_open, (struct driver *dp, message *m_ptr) 	);
//FORWARD _PROTOTYPE( void m_init, (void) );
//FORWARD _PROTOTYPE( int m_ioctl, (struct driver *dp, message *m_ptr) 	);
_PROTOTYPE( void m_geometry, (struct partition *entry) 		);

/* Entry points to this driver. */ /*todo driver genera un vector de este.*/
struct driver m_dtab = {
  m_name,	/* current device's name */
  m_do_open,	/* open or mount */
//  do_nop,	/* nothing on a close */
//  m_ioctl,	/* specify ram disk geometry */
  m_prepare,	/* prepare for I/O on a given minor device */
  m_transfer,	/* do the I/O */
//  nop_cleanup,	/* no need to clean up */
  m_geometry,	/* memory device "geometry" */
//  nop_signal,	/* system signals */
//  nop_alarm,
//  nop_cancel,
//  nop_select,
  NULL,
  NULL
};

/* Buffer for the /dev/zero null byte feed. */
//#define ZERO_BUF_SIZE 			1024
//PRIVATE char dev_zero[ZERO_BUF_SIZE];

//#define click_to_round_k(n) \
	((unsigned) ((((unsigned long) (n) << CLICK_SHIFT) + 512) / 1024))

//int load_dimage(char *img_name);

/*===========================================================================*
*				load_dimage				     *
*===========================================================================*/
int load_dimage(char *img_name)
{
	int rcode, img_fd, bytes, blocks, total;
	struct stat img_stat;
	char *ptr;
	char test[100];
	//int img_size;		/* testing image file size -ESTAS ESTÁN DEFINIDAS EN GLO.H PERO NO LAS RECONOCE */
	//char *img_ptr;		/* pointer to the first byte of the ram disk image - IDEM ANTERIOR*/

SVRDEBUG("image name=%s\n", img_name);


	/* get the image file size */
	rcode = stat(img_name,  &img_stat);
	if(rcode) ERROR_EXIT(errno);

SVRDEBUG("image size=%d[bytes]\n", img_stat.st_size);
SVRDEBUG("block size=%d[bytes]\n", img_stat.st_blksize);
	img_size = img_stat.st_size;

	/* alloc dynamic memory for image file size */
SVRDEBUG("Alloc dynamic memory for disk image file bytes=%d\n", img_size);
	posix_memalign( (void**) &img_ptr, getpagesize(), (img_size+getpagesize()));
	if(img_ptr == NULL) ERROR_EXIT(errno);

	/* Try to open the disk image */	
	img_fd = open(img_name, O_RDONLY);
	if(img_fd < 0) ERROR_EXIT(errno);
	
	/* dump the image file into the allocated memory */
	ptr = img_ptr;
	blocks = 0;
	total = 0;
	while( (bytes = read(img_fd, ptr, img_stat.st_blksize)) > 0 ) {
		blocks++;
		total += bytes;
		ptr += bytes;
	} 
SVRDEBUG("blocks read=%d bytes read=%d\n", blocks, total);
  
	/* close the disk image */
	rcode = close(img_fd);
	if(rcode) ERROR_EXIT(errno);

#define BLOCK_SIZE	1024
struct super_block *sb_ptr;
	sb_ptr = (struct super_block *) (img_ptr + BLOCK_SIZE);

SVRDEBUG(SUPER_BLOCK_FORMAT1, SUPER_BLOCK_FIELDS1(sb_ptr));
	

  return(OK);
}

/*fin copia de función load_image()*/

/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
//PUBLIC int main(void)
int main ( int argc, char *argv[] )
{
/* Main program. Initialize the memory driver and start the main loop. */
//  struct sigaction sa;

 

/*COPIO LA IMAGEN DEL DISCO, PREVIO A LLAMAR A m_init*/

/*copio código de main de FS donde cargo la imagen*/

	int vmid, error, result, ret;
	m_ptr = &m_in;

	if ( argc != 3) {
 	        printf( "Usage: %s <vmid> <image_name>\n", argv[0] );
 	        exit(1);
    	}
	
	vmid = atoi(argv[1]);
	vmid = atoi(argv[1]);
	if( vmid < 0 || vmid >= NR_VMS) ERROR_EXIT(EMOLRANGE);

	error = load_dimage(argv[2]);
	if(error) ERROR_EXIT(error);

/*fin de la copia del main de FS () donde carga la imagen*/

#ifdef ANULADO  
  sa.sa_handler = SIG_MESS;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGTERM,&sa,NULL)<0) panic("MEM","sigaction failed", errno);
#endif /*ANULADO*/  

/*el ifANULADO anterior fila:176-181; es para recibir el mensaje????, lo reemplazo por las nuevas funciones - lo coloqué en m_init*/


  m_init(vmid);	//fs_init(vmid); en file system		lo q hago en load_image- bind de esta tarea - lo anterior comentar no */

/*recibo el mensaje*/
while(1){
  sleep(2);	
  ret = mnx_receive( ANY , (long) &m_in);
  if( ret != OK){
		printf("RECEIVE ERROR %d\n",ret); 
		exit(1);
	}
  printf("RECEIVE msg: m_source=%d, m_type=%d, m2_i1=%d, m2_i2=%d, m2_l1=%u, m2_l2=%u, m2_p1:%p\n",
		m_in.m_source,
		m_in.m_type,
		m_in.m2_i1,
		m_in.m2_i2,
		m_in.m2_l1,
		m_in.m2_l2,
		m_in.m2_p1);

/*fin de recibir el mensaje*/	
  
/*previo a llamar a la driver_task; debo completar la struct m_dtab*/

SVRDEBUG("\n current device's name: %s\n prepare for I/O on a given minor device:(%u;%u), (%u;%u)\n", 
  m_name(), 
  m_prepare(RAM_DEV)->dv_base._[0],m_prepare(RAM_DEV)->dv_base._[1], m_prepare(RAM_DEV)->dv_size._[0], m_prepare(RAM_DEV)->dv_size._[1]
);
m_do_open(&m_dtab,&m_in);

//MENSAJE TIPO m2
//*    m_type      DEVICE    IO_ENDPT    COUNT    POSITION  ADRRESS
//* | DEV_SCATTER| device  | proc nr | iov len |  offset | iov ptr |
//	m.m_type		m.m2_i1	m.m2_i2		m.m2_l2	 m.m2_l1	m.m2_p1
//mensaje lo recibí en m_init con m_in

m_device = RAM_DEV;	


	 /* Copio el vector al espacio del server=memory. */
	static iovec_t iovec_m[NR_IOREQS]; /*genero el propio vector en memory*/			
	iovec_t *iov_m; /*puntero al vector*/
	unsigned nr_req_m; /*nr_req sólo para memory*/
	phys_bytes iovec_size;
	int ret, proc_nr;	
	 
	nr_req_m = m_in.m2_l2; 
	proc_nr =m_in.m_source;
	
    if (nr_req_m > NR_IOREQS) nr_req_m = NR_IOREQS;
    iovec_size = (phys_bytes) (nr_req_m * sizeof(iovec_m[0]));

    ret = mnx_vcopy(proc_nr, m_in.m2_p1, m_ep, iovec_m, iovec_size);
	
	if( ret != 0 )
		printf("VCOPY (vector) ret=%d\n",ret);			
		
    iov_m = iovec_m;
	
	printf("iovec_m[0].iov_addr %u\n", iovec_m[0].iov_addr); 
	printf("iovec_m[0].iov_size %u\n", iovec_m[0].iov_size); 
  
/*fin de copiar el vector*/	
	
m_transfer(m_in.m_source, m_in.m_type, m_in.m2_l1, iov_m, m_in.m2_l2);
//m_transfer(proc_nr, opcode, position, iov, nr_req);

SVRDEBUG("\n llamo a m_geometry\n");
m_geometry(&entry);

SVRDEBUG("\n salgo de m_geometry\n");
SVRDEBUG("\n m_geometry: cylinders: %u- heads: %u - sectors: %u\n", entry.cylinders, entry.heads, entry.sectors);
  #ifdef ANULADO
  driver_task(&m_dtab);		
  
  #endif /*ANULADO*/  
  }
  return(OK);				
}

/*===========================================================================*
 *				 m_name					     *
 *===========================================================================*/
  
 char *m_name()
{
/* Return a name for the current device. */
  //static char name[] = "memory";
  static char name[] = "mol_driver";
  return name;  
}
  
/*===========================================================================*
 *				m_prepare				     *
 *===========================================================================*/

struct device *m_prepare(device)
int device;
{
/* Prepare for I/O on a device: check if the minor device number is ok. */
  
  if (device < 0 || device >= NR_DEVS) return(NIL_DEV);
    m_device = device;
  
  return(&m_geom[device]);
}

/*===========================================================================*
 *				m_transfer				     *
 *===========================================================================*/
int m_transfer(proc_nr, opcode, position, iov, nr_req)
int proc_nr;			/* process doing the request */
int opcode;				/* DEV_GATHER or DEV_SCATTER */
off_t position;			/* offset on device to read or write */
iovec_t *iov;			/* pointer to read or write request vector */
unsigned nr_req;		/* length of request vector */
{

/* Read or write one the driver's minor devices. */
  phys_bytes mem_phys;
//  int seg;
  unsigned count; //, left, chunk;
  vir_bytes user_vir;
  struct device *dv;
  unsigned long dv_size;
  int ret,pos;
//  int s;

  /* Get minor device number and check for /dev/null. */
  dv = &m_geom[m_device];
  dv_size = cv64ul(dv->dv_size);
  //m_prepare(RAM_DEV)->dv_base._[0],m_prepare(RAM_DEV)->dv_base._[1], m_prepare(RAM_DEV)->dv_size._[0], m_prepare(RAM_DEV)->dv_size._[1]

  while (nr_req > 0) {
	
	/* How much to transfer and where to / from. */
	count = iov->iov_size;
	user_vir = iov->iov_addr;
	
	switch (m_device) {  /*en nuestro caso debería ser siempre 0 RAM_DEV*/
	
	/* No copying; ignore request. */
	#ifdef ANULADO
	case NULL_DEV:
	    if (opcode == DEV_GATHER) return(OK);	/* always at EOF */
	    break;

	/* Virtual copying. For RAM disk, kernel memory and boot device. */
	case RAM_DEV:
	case KMEM_DEV:
	case BOOT_DEV:
	    if (position >= dv_size) return(OK); 	/* check for EOF */
	    if (position + count > dv_size) count = dv_size - position;
	    seg = m_seg[m_device];

	    if (opcode == DEV_GATHER) {			/* copy actual data */
	        sys_vircopy(SELF,seg,position, proc_nr,D,user_vir, count);
	    } else {
	        sys_vircopy(proc_nr,D,user_vir, SELF,seg,position, count);
	    }
	    break;
    #endif /*ANULADO*/ 
	
	
	/* Physical copying. Only used to access entire memory. */
	/* por ahora este es el único caso que vamos a trabajar*/
	case RAM_DEV: //MEM_DEV:
		//if (position >= dv_size) return(OK); 	/* check for EOF */
		if (position >= dv_size) { printf("EOF\n"); return(OK);} 	/* check for EOF */
		//if (position + count > dv_size) count = dv_size - position;
	    if (position + count > dv_size) { count = dv_size - position; printf("count dv_size-position: %u\n", count); }
		//si (position=offset en el disco + count=cant bytes q voy a leer/esc) > tamaño del disco
		//a count le doy la diferencia de lo q se puede leer/esc dev_size - position
		mem_phys = cv64ul(dv->dv_base) + position;
		printf ("donde voy a escribir %u\n", mem_phys);

	    if (opcode == DEV_GATHER) {			/* copy data */ /*DEV_GATHER lee de un vector en com.h -SCATTER:escribe*/
			printf("\nEl código es DEV_GATHER\n");
	        /*sys_physcopy(NONE, PHYS_SEG, mem_phys, 
	        	proc_nr, D, user_vir, count);*/
			ret = mnx_vcopy(m_ep, mem_phys, proc_nr, user_vir, count);	
			SVRDEBUG("En el server, en offset %u, contenido a leer %s\n", mem_phys, mem_phys);			
			if( ret != 0 )
		    	printf("VCOPY1 ret=%d\n",ret);
			printf("\n");
			
	    } else {
			
	        //sys_physcopy(proc_nr, D, user_vir, NONE, PHYS_SEG, mem_phys, count); 
			
			printf("\nEl código es: DEV_SCATTER\n");
			//home/jara/mol-ipc/lib/syslib/sys_physcopy.c
			//proc_nr:source process	
			//D:source memory segment -->en el original definido en include/minix/const.h, acá no está.
			//user_vir:source virtual address
			//NONE:source virtual address--> /minix/com.h
			//PHYS_SEG:destination memory segment -> en /minix/const.h
			//mem_phys:destination virtual address
			//count:how many bytes
			//lo reemplazo por mnx_vcopy
			//mnx_vcopy(src_ep,src_addr,dst_ep, dst_addr, bytes)
			ret = mnx_vcopy(proc_nr, user_vir, m_ep, mem_phys, count);
			SVRDEBUG("Resultado mnx_vcopy CLIENTE -> SERVIDOR %d\n", ret);  
			SVRDEBUG("count %d\n", count);     
			SVRDEBUG("En el server, contenido de lo transferido desde el cliente %s\n", mem_phys);			
			
			if( ret != 0 )
		    	printf("VCOPY ret=%d\n",ret);
				
			printf("\n");
		}
	    break;
		
	#ifdef ANULADO  
	/* Null byte stream generator. */
	case ZERO_DEV:
	    if (opcode == DEV_GATHER) {
	        left = count;
	    	while (left > 0) {
	    	    chunk = (left > ZERO_BUF_SIZE) ? ZERO_BUF_SIZE : left;
	    	    if (OK != (s=sys_vircopy(SELF, D, (vir_bytes) dev_zero, 
	    	            proc_nr, D, user_vir, chunk)))
	    	        report("MEM","sys_vircopy failed", s);
	    	    left -= chunk;
 	            user_vir += chunk;
	    	}
	    }
	    break;
	#endif /*ANULADO*/
	
	#ifdef ANULADO  
	case IMGRD_DEV:
	    if (position >= dv_size) return(OK); 	/* check for EOF */
	    if (position + count > dv_size) count = dv_size - position;

	    if (opcode == DEV_GATHER) {			/* copy actual data */
	        sys_vircopy(SELF, D, (vir_bytes)&imgrd[position],
			proc_nr, D, user_vir, count);
	    } else {
	        sys_vircopy(proc_nr, D, user_vir,
			SELF, D, (vir_bytes)&imgrd[position], count);
	    }
	    break;
	#endif /*ANULADO*/
	/* Unknown (illegal) minor device. */
	default:
	    return(EINVAL);
	}
		
	/* Book the number of bytes transferred. Registra el número de bytes transferidos? */
	position += count; //incremento el offset en el disco por la cantidad (count) de lo q leí o escribí
	iov->iov_addr += count; //lo mismo para iov_addr donde está la dirección guardada
  	if ((iov->iov_size -= count) == 0) { iov++; nr_req--; } //si, decrementando en count es = 0 transferí todo lo pedido, paso al sgte elem iov, decremento el nr_req
	
	/*para probar*/
	//nr_req--; /*esta línea la agregué yo para cortar*/
  }
  return(OK);
}

/*===========================================================================*
 // *				m_do_open				     *
 *===========================================================================*/
int m_do_open(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
printf("m_do_open - device number: %d - OK to open\n", m_device);
/*único dispositivo abierto volcado en memoria, (puntero: ptr) en load_dimage() q se llama desde main, tendría q pasarla m_init*/

#ifdef ANULADO 
  int r;

/* Check device number on open. */
//DEVICE=m2_i1 campos del msj para definir major-minor device en com.h
//m2_i1 en ipc.h 
//#define m2_i1  m_u.m_m2.m2i1

  if (m_prepare(m_ptr->DEVICE) == NIL_DEV) return(ENXIO);
  if (m_device == MEM_DEV)
  {
	r = sys_enable_iop(m_ptr->IO_ENDPT);
	if (r != OK)
	{
		printf("m_do_open: sys_enable_iop failed for %d: %d\n",
			m_ptr->IO_ENDPT, r);
		return r;
	}
  }
   return(OK);
  #endif /*ANULADO*/
}

/*========================|===================================================*
 *				m_init					     *
 *===========================================================================*/
void m_init(int vmid)
{
	int rcode, i, ret;

	m_lpid = getpid();
	//m_ptr = &m_in;

	/* Bind MEM to the kernel */
SVRDEBUG("Binding process %d to VM%d with Memory_nr=%d\n",m_lpid,vmid,MEM_PROC_NR);
	m_ep = mnx_bind(vmid, MEM_PROC_NR);
	if(m_ep < 0) ERROR_EXIT(m_ep);

SVRDEBUG("Memory enpoint %d\n",m_ep);

/*recibo el mensaje*/
#ifdef ANULADO 
  sleep(2);	
  ret = mnx_receive( ANY , (long) &m_in);
  if( ret != OK){
		printf("RECEIVE ERROR %d\n",ret); 
		exit(1);
	}
  printf("RECEIVE msg: m_source=%d, m_type=%d, m2_i1=%d, m2_i2=%d, m2_l1=%u, m2_l2=%u, m2_p1:%p\n",
		m_in.m_source,
		m_in.m_type,
		m_in.m2_i1,
		m_in.m2_i2,
		m_in.m2_l1,
		m_in.m2_l2,
		m_in.m2_p1);

/*fin de recibir el mensaje*/	
#endif /*ANULADO*/
//return(OK);
//} hasta acá andaba bien

#ifdef ANULADO 
  /* Initialize this task. All minor devices are initialized one by one. */
  phys_bytes ramdev_size;
  /*campo de la estructura: struct kinfo, definida en /kernel/type.h - representa tamaño del /dev/boot*/
  phys_bytes ramdev_base;
  /*idem anterior; representa: boot device from boot image (/dev/boot) */
  
  message m; 
  int i, s;

  if (OK != (s=sys_getkinfo(&kinfo))) {
      panic("MEM","Couldn't get kernel information.",s);
	}
  
  /*sys_getkinfo(&kinfo); está definida en syslib.h y devuelve información del kernel en la estructura kinfo*/
  
  /* Install remote segment for /dev/kmem memory. */
  m_geom[KMEM_DEV].dv_base = cvul64(kinfo.kmem_base);
  m_geom[KMEM_DEV].dv_size = cvul64(kinfo.kmem_size);
  if (OK != (s=sys_segctl(&m_seg[KMEM_DEV], (u16_t *) &s, (vir_bytes *) &s, 
  		kinfo.kmem_base, kinfo.kmem_size))) {
      panic("MEM","Couldn't install remote segment.",s);
  }

  /* Install remote segment for /dev/boot memory, if enabled. */
  m_geom[BOOT_DEV].dv_base = cvul64(kinfo.bootdev_base);
  m_geom[BOOT_DEV].dv_size = cvul64(kinfo.bootdev_size);
  if (kinfo.bootdev_base > 0) {
      if (OK != (s=sys_segctl(&m_seg[BOOT_DEV], (u16_t *) &s, (vir_bytes *) &s, 
              kinfo.bootdev_base, kinfo.bootdev_size))) {
          panic("MEM","Couldn't install remote segment.",s);
      }
  }
#endif /*ANULADO*/
  
  /* See if there are already RAM disk details at the Data Store server. */
  /* Ve si hay datos de un disco RAM en el Data Store server - en nuestro caso serán los datos de la imagen del disco que subimos*/
  
  //m.DS_KEY = MEMORY_MAJOR;
  //if (OK == (s = _taskcall(DS_PROC_NR, DS_RETRIEVE, &m))) {
  //	ramdev_size = m.DS_VAL_L1;
  //	ramdev_base = m.DS_VAL_L2;
  //	printf("MEM retrieved size %u and base %u from DS, status %d\n",
  //  		ramdev_size, ramdev_base, s);
  //	if (OK != (s=sys_segctl(&m_seg[RAM_DEV], (u16_t *) &s, 
  //		(vir_bytes *) &s, ramdev_base, ramdev_size))) {
  //    		panic("MEM","Couldn't install remote segment.",s);
  //	}
  
  //	m_geom[RAM_DEV].dv_base = cvul64(ramdev_base); reemplazo x línea de abajo --> los datos de nuestra imagen
	SVRDEBUG("Byte offset to the partition start (img_ptr): %u\n", img_ptr);
	m_geom[RAM_DEV].dv_base = cvul64(img_ptr);
	SVRDEBUG("Byte offset to the partition start (m_geom[RAM_DEV].dv_base): %u\n", m_geom[RAM_DEV].dv_base);
	
  //	m_geom[RAM_DEV].dv_size = cvul64(ramdev_size); idem anterior
	SVRDEBUG("Number of bytes in the partition (img_size): %u\n", img_size);
	m_geom[RAM_DEV].dv_size = cvul64(img_size);
	SVRDEBUG("Number of bytes in the partition (m_geom[RAM_DEV].dv_size): %u\n", m_geom[RAM_DEV].dv_size);
	
  //	printf("MEM stored retrieved details as new RAM disk\n");
  //}

return(OK);
}
#ifdef ANULADO
  /* Ramdisk image built into the memory driver */
  m_geom[IMGRD_DEV].dv_base= cvul64(0);
  m_geom[IMGRD_DEV].dv_size= cvul64(imgrd_size);

  
  /* Initialize /dev/zero. Simply write zeros into the buffer. */
  for (i=0; i<ZERO_BUF_SIZE; i++) {
       dev_zero[i] = '\0';
  }

  /* Set up memory ranges for /dev/mem. */
#if (CHIP == INTEL)
  if (OK != (s=sys_getmachine(&machine))) {
      panic("MEM","Couldn't get machine information.",s);
  }
  if (! machine.prot) {
	m_geom[MEM_DEV].dv_size =   cvul64(0x100000); /* 1M for 8086 systems */
  } else {
#if _WORD_SIZE == 2
	m_geom[MEM_DEV].dv_size =  cvul64(0x1000000); /* 16M for 286 systems */
#else
	m_geom[MEM_DEV].dv_size = cvul64(0xFFFFFFFF); /* 4G-1 for 386 systems */
#endif
  }
#else /* !(CHIP == INTEL) */
#if (CHIP == M68000)
  m_geom[MEM_DEV].dv_size = cvul64(MEM_BYTES);
#else /* !(CHIP == M68000) */
#error /* memory limit not set up */
#endif /* !(CHIP == M68000) */
#endif /* !(CHIP == INTEL) */
}
#endif /*ANULADO*/
/*===========================================================================*
 *				m_ioctl					     *
 *===========================================================================*/
#ifdef ANULADO  
 int m_ioctl(dp, m_ptr)
struct driver *dp;			/* pointer to driver structure */
message *m_ptr;				/* pointer to control message */
{
/* I/O controls for the memory driver. Currently there is one I/O control:
 * - MIOCRAMSIZE: to set the size of the RAM disk.
 */
  struct device *dv;

  switch (m_ptr->REQUEST) {
    case MIOCRAMSIZE: {
	/* Someone wants to create a new RAM disk with the given size. */
	static int first_time= 1;

	u32_t ramdev_size;
	phys_bytes ramdev_base;
	message m;
	int s;

	/* A ramdisk can be created only once, and only on RAM disk device. */
	if (!first_time) return(EPERM);
	if (m_ptr->DEVICE != RAM_DEV) return(EINVAL);
        if ((dv = m_prepare(m_ptr->DEVICE)) == NIL_DEV) return(ENXIO);

#if 0
	ramdev_size= m_ptr->POSITION;
#else
	/* Get request structure */
	s= sys_vircopy(m_ptr->IO_ENDPT, D, (vir_bytes)m_ptr->ADDRESS,
		SELF, D, (vir_bytes)&ramdev_size, sizeof(ramdev_size));
	if (s != OK)
		return s;
#endif

#if DEBUG
	printf("allocating ramdisk of size 0x%x\n", ramdev_size);
#endif

	/* Try to allocate a piece of memory for the RAM disk. */
        if (allocmem(ramdev_size, &ramdev_base) < 0) {
            report("MEM", "warning, allocmem failed", errno);
            return(ENOMEM);
        }

	/* Store the values we got in the data store so we can retrieve
	 * them later on, in the unfortunate event of a crash.
	 */
	m.DS_KEY = MEMORY_MAJOR;
	m.DS_VAL_L1 = ramdev_size;
	m.DS_VAL_L2 = ramdev_base;
	if (OK != (s = _taskcall(DS_PROC_NR, DS_PUBLISH, &m))) {
      		panic("MEM","Couldn't store RAM disk details at DS.",s);
	}
#if DEBUG
	printf("MEM stored size %u and base %u at DS, status %d\n",
	    ramdev_size, ramdev_base, s);
#endif

  	if (OK != (s=sys_segctl(&m_seg[RAM_DEV], (u16_t *) &s, 
		(vir_bytes *) &s, ramdev_base, ramdev_size))) {
      		panic("MEM","Couldn't install remote segment.",s);
  	}

	dv->dv_base = cvul64(ramdev_base);
	dv->dv_size = cvul64(ramdev_size);
	/* first_time= 0; */
	break;
    }
    case MIOCMAP:
    case MIOCUNMAP: {
    	int r, do_map;
    	struct mapreq mapreq;

	if ((*dp->dr_prepare)(m_ptr->DEVICE) == NIL_DEV) return(ENXIO);
    	if (m_device != MEM_DEV)
    		return ENOTTY;

	do_map= (m_ptr->REQUEST == MIOCMAP);	/* else unmap */

	/* Get request structure */
	r= sys_vircopy(m_ptr->IO_ENDPT, D, (vir_bytes)m_ptr->ADDRESS,
		SELF, D, (vir_bytes)&mapreq, sizeof(mapreq));
	if (r != OK)
		return r;
	r= sys_vm_map(m_ptr->IO_ENDPT, do_map,
		(phys_bytes)mapreq.base, mapreq.size, mapreq.offset);
	return r;
    }

    default:
  	return(do_diocntl(&m_dtab, m_ptr));
  }
  return(OK);
}
#endif /*ANULADO*/
/*===========================================================================*
 *				m_geometry				     *
 *===========================================================================*/
void m_geometry(entry)
struct partition *entry; /*in <partition.h>*/
{
  /* Memory devices don't have a geometry, but the outside world insists. */
  entry->cylinders = div64u(m_geom[m_device].dv_size, SECTOR_SIZE) / (64 * 32);
  SVRDEBUG("\n cylinders: %u\n", entry->cylinders);
  //div64u en driver.h (x mí)  - SECTOR_SIZE en driver.h = 512 -> physical sector size in bytes 
  entry->heads = 64;
  SVRDEBUG("\n heads: %u\n", entry->heads);
  entry->sectors = 32;
  SVRDEBUG("\n sectors: %u\n", entry->sectors);
}


  