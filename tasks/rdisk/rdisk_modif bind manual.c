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
#define _RDISK

//#define TASKDBG		1

#include "rdisk.h"
#include "data_usr.h"


//#include "../libdriver/driver.h"
//#include <sys/ioc_memory.h>
//#include "../../kernel/const.h"
//#include "../../kernel/config.h"
//#include "../../kernel/type.h"

//#include <sys/vm.h>

//#include "assert.h"

//#include "local.h"

//#define NR_DEVS            7		/* number of minor devices */
// #define NR_DEVS            2		/* number of minor devices - example*/

//PRIVATE struct device m_geom[NR_DEVS];  /* base and size of each device */
struct device m_geom[NR_DEVS];  /* base and size of each device */

//PRIVATE int m_seg[NR_DEVS];  		/* segment index of each device */

//PRIVATE int m_device;			/* current device */
int m_device;			/* current device */

// static devvec_t devvec[NR_DEVS];

//PRIVATE struct kinfo kinfo;		/* kernel information */ 
//PRIVATE struct machine machine;		/* machine information */ 

//extern int errno;			/* error number for PM calls */

struct partition entry; /*no en código original, pero para completar los datos*/

/* Entry points to this driver. */
	
message mess;
SP_message sp_msg; /*message Spread*/	

// int img_p; /*puntero a la imagen de disco*/

// unsigned *localbuff;		/* pointer to the first byte of the local buffer (=disk image)*/	??
unsigned *localbuff;
// unsigned BUFF_SIZE1; /* value option -b (buffer) */

 struct mdevvec *devinfo_ptr;

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

/*getopt_long_only*/
/* Flag set by ‘--replicate’. */
static int replicate_flag; 
static int bind_flag; 

void usage(char* errmsg, ...) {
	if(errmsg) {
		printf("ERROR: %s\n", errmsg);
	} else {
		fprintf(stderr, "por ahora nada nbd-client imprime la versión\n");
	}
	// fprintf(stderr, "Usage: rdisk -dcid|-v [virtualmachine] -first|-f [minor number1] -First|-F [file image1] -second|-s [minor number2] -Second|-S [file image2] -buffer_size|-b [buffer size]\n");
	// fprintf(stderr, "Or   : rdisk -r|--replicate (with same arguments as above)\n");
	// fprintf(stderr, "Default value for buffer (BUFF_SIZE) is %d\n", BUFF_SIZE);
	fprintf(stderr, "Usage: rdisk <config_file>\n");
	
}

/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
//PUBLIC int main(void)
int main (int argc, char *argv[] )
{
	/* Main program.*/
	int rcode, c, i, j, l_dev, f_dev;  
	char *c_file;
	struct stat img_stat,img_stat1;
 
	if ( argc != 3) {
		usage( "No arguments (<config file> <1 - bind> <1 - replicate>", optarg );
		exit(1);
	}
	
	bind_flag = atoi(argv[2]);
	replicate_flag = atoi(argv[3]);
	TASKDEBUG("Flags -  bind %d, replicate %d\n",bind_flag, replicate_flag);
	
	
	
	/* No availables minor device  */
	for( i = 0; i < NR_DEVS; i++){
		devvec[i].available = 0;
	}

	c_file = argv[1];
	count_availables = 0;		
	test_config(c_file);
 	if (count_availables == 0){
		fprintf( stderr,"\nERROR. No availables devices in %s\n", c_file );
		exit(1);
	}
	TASKDEBUG("count_availables=%d\n",count_availables);

	/* the same inode of file image*/
	f_dev=0; /*count first device*/
	l_dev=1; /*count last device*/
		
	while (f_dev < NR_DEVS){
		for( i = f_dev; i < NR_DEVS; i++){
			if( devvec[i].available == 0){
				f_dev++;
			}
			rcode = stat(devvec[i].img_ptr, &img_stat);
			TASKDEBUG("stat0 %s rcode=%d\n",devvec[i].img_ptr, rcode);
			if(rcode){
				fprintf( stderr,"\nERROR %d: Device %s minor_number %d is not valid\n", rcode , c_file, i );
				f_dev++;
			}
			for( j = l_dev; j < NR_DEVS; j++){
				if( devvec[j].available == 0){
					l_dev++;
				}	
				rcode = stat(devvec[j].img_ptr, &img_stat1);
				TASKDEBUG("stat1 %s rcode=%d\n",devvec[i].img_ptr, rcode);
				if(rcode){
					fprintf( stderr,"\nERROR %d: Device %s minor_number %d is not valid\n", rcode, c_file, j );
					l_dev++;
				}
				TASKDEBUG("devvec[%d].img_ptr=%s,devvec[%d].img_ptr=%s\n", 
					i, devvec[i].img_ptr,j,devvec[j].img_ptr);
				
				if ( img_stat.st_ino == img_stat1.st_ino ){
					fprintf( stderr,"\nERROR. Minor numbers %d - %d are the same file\n", i, j );
					devvec[j].available = 0;
					fprintf( stderr,"\nDevice with minor numbers: %d is not available now\n", j );
				}
			}
			f_dev++;
			l_dev++;
		}	
	}

/* get the image file size */
	for( i = 0; i < NR_DEVS; i++){
		// rcode = stat(img_ptr,  &img_stat);
		// if (devvec[i].img_ptr){
		if (devvec[i].available == 0){
			TASKDEBUG("Minor device %d is not available\n", i);
		}else{
			TASKDEBUG("devvec[%d].img_ptr=%s\n", i, devvec[i].img_ptr);
			rcode = stat(devvec[i].img_ptr, &img_stat);
			if(rcode) ERROR_EXIT(errno);
			devvec[i].st_size = img_stat.st_size;
			TASKDEBUG("image size=%d[bytes] %d\n", img_stat.st_size, devvec[i].st_size);
			devvec[i].st_blksize = img_stat.st_blksize;
			TASKDEBUG("block size=%d[bytes] %d\n", img_stat.st_blksize, devvec[i].st_blksize);
		}
	}
/* the same inode of file image*/
		
  rcode = rd_init();
  if(rcode) ERROR_RETURN(rcode);  
  driver_task(&m_dtab);	
  
  free(localbuff);
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
  static char name[] = "rd_driver";
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
  
	if (device < 0 || device >= NR_DEVS || devvec[device].active != 1) {
		TASKDEBUG("Error en m_prepare\n");
		return(NIL_DEV);
		}
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
	unsigned count, tbytes, stbytes, bytes, count_s, bytes_c; //left, chunk; 
	vir_bytes user_vir, addr_s;
	struct device *dv;
	unsigned long dv_size;
	int rcode;
	off_t posit;
	message msg;
	
	tbytes = 0;
	bytes = 0;
	bytes_c = 0;
	
	TASKDEBUG("m_device: %d\n", m_device); /* se completa en driver.c ; en las wr ya que ellas ven el mensaje y llaman antes a m_prepare*/
    
	if (devvec[m_device].active != 1) { /*minor device active must be -1-*/  
		TASKDEBUG("Minor device = %d\n is not active", m_device);
		ERROR_RETURN(EMOLNODEV);	
	}
	/* Get minor device number and check for /dev/null. */
	dv = &m_geom[m_device];
	dv_size = cv64ul(dv->dv_size); 
	//dv_size = size_buff;
	
	posit = position;
	TASKDEBUG("posit: %X\n", posit);	
	TASKDEBUG("nr_req: %d\n", nr_req);	

	while (nr_req > 0) { /*2*/
	  
		/* How much to transfer and where to / from. */
		count = iov->iov_size;
		TASKDEBUG("count: %u\n", count);	
		
		if(primary_mbr == local_nodeid) {
			count_s = iov->iov_size; /*PRYMARY: bytes iniciales de c/posición del vector a copiar, ver no sé para q lo voy a usar*/
			TASKDEBUG("count_s: %u\n", count_s);	
		}
		
		user_vir = iov->iov_addr;
		addr_s = iov->iov_addr;
		TASKDEBUG("user_vir %X\n", user_vir);	
		
		// switch (m_device) { /*en nuestro caso debería ser siempre 0 RAM_DEV*/
		
		/* Physical copying. Only used to access entire memory. */
		/* Este es el único caso que vamos a trabajar*/
		//case MEM_DEV: /*código original*/
		// case RAM_DEV: /* minor number*/
			//if (position >= dv_size) return(OK); 	/* check for EOF */
		if (position >= dv_size) {
			TASKDEBUG("EOF\n"); 
			return(OK);
		} 	/* check for EOF */
			
			//if (position + count > dv_size) count = dv_size - position;
		if (position + count > dv_size) { 
			count = dv_size - position; 
			TASKDEBUG("count dv_size-position: %u\n", count); 
		}
		//si (position=offset en el disco + count=cant bytes q voy a leer/esc) > tamaño del disco
		//a count le doy la diferencia de lo q se puede leer/esc dev_size - position
			
		mem_phys = cv64ul(dv->dv_base) + position;
		//mem_phys = localbuff + position;
		TASKDEBUG("DRIVER - position I/O(mem_phys) %X\n", mem_phys);
			
		if ((opcode == DEV_GATHER) ||(opcode == DEV_CGATHER))  {			/* copy data */ /*DEV_GATHER read from an array (com.h)*/
			TASKDEBUG("\n<DEV_GATHER>\n");
				
			stbytes = 0;
			do	{
				/* read to the virtual disk-file- into the buffer --> to the FS´s buffer*/
				// bytes = (count > BUFF_SIZE1)?BUFF_SIZE1:count;
				bytes = (count > devvec[m_device].buff_size)?devvec[m_device].buff_size:count;
				TASKDEBUG("bytes: %d\n", bytes);		
			
				/* read data from the virtual device file into the local buffer  */			
				// rcode = pread(img_p, localbuff, bytes, position);
				bytes = pread(devvec[m_device].img_p, devvec[m_device].localbuff, bytes, position);
				// if(rcode) ERROR_EXIT(errno);
				TASKDEBUG("pread: bytes=%d\n", bytes);
				
				if(bytes < 0) ERROR_EXIT(errno);
				
				if ( opcode == DEV_CGATHER ) {
					TASKDEBUG("Compress data for to the requester process\n");
					
					/*compress data buffer*/
										
					TASKDEBUG("lz4_data_cd (in_buffer=%X, inbuffer_size=%d, condition UNCOMP =%d\n",
						devvec[m_device].localbuff,bytes,UNCOMP);
					
					lz4_data_cd(devvec[m_device].localbuff, bytes, UNCOMP);
					
					buffer_size = msg_lz4cd.buf.buffer_size;
					TASKDEBUG("buffer_size =%d\n", buffer_size);
					memcpy(devvec[m_device].localbuff, msg_lz4cd.buf.buffer_data, buffer_size);
					TASKDEBUG("buffer_data =%s\n", devvec[m_device].localbuff);
					mess.m2_l2 = buffer_size;	
					TASKDEBUG("mess.m2_l2 =%d\n", mess.m2_l2);
					/* copy the data from the local buffer to the requester process address space in other VM - compress data */
					rcode = mnx_vcopy(rd_ep, devvec[m_device].localbuff, proc_nr, user_vir, buffer_size);
					/*END compress data buffer*/
								
				}else{
					/* copy the data from the local buffer to the requester process address space in other VM */
					rcode = mnx_vcopy(rd_ep, devvec[m_device].localbuff, proc_nr, user_vir, bytes); 
				}
				
				
				TASKDEBUG("DRIVER: mnx_vcopy(DRIVER -> proc_nr) rcode=%d\n", rcode);  
				TASKDEBUG("bytes= %d\n", bytes);
//				TASKDEBUG("DRIVER - Offset (read) %X, Data: %s\n", devvec[m_device].localbuff, devvec[m_device].localbuff);			
				TASKDEBUG("DRIVER - Offset (read) %X\n", devvec[m_device].localbuff);			
				TASKDEBUG("mem_phys: %X (in DRIVER)\n", devvec[m_device].localbuff);			
				TASKDEBUG("user_vir: %X (in proc_nr %d)\n", user_vir, proc_nr);			
			
				if (rcode != 0 ) {
					fprintf( stderr,"VCOPY rcode=%d\n",rcode);
					break;
				}
				stbytes += bytes; /*total bytes transfers*/								
				position += bytes;
				iov->iov_addr += bytes;
				user_vir = iov->iov_addr;
				TASKDEBUG("user_vir (do-buffer) %X\n", user_vir);	
				count -= bytes;
				TASKDEBUG("count=%d stbytes=%d position=%ld\n", count, stbytes, position);	
			} while(count > 0);
			/* END DEV_GATHER*/
		} else { /*DEV_SCATTER write from an array*/
			TASKDEBUG("\n<DEV_SCATTER>\n");
			stbytes = 0;
			TASKDEBUG("\dc_ptr->dc_nr_nodes=%d, nr_nodes=%d\n",dc_ptr->dc_nr_nodes, nr_nodes);
			
			if( dc_ptr->dc_nr_nodes > 1) { /* PRIMARY;  MULTICAST to other nodes the device operation */
				if(primary_mbr == local_nodeid) {
					
					nr_optrans = 0;
					TASKDEBUG("nr_optrans: %d\n", nr_optrans); /*para enviar rta por cada nr_req*/
					
					pthread_mutex_lock(&rd_mutex);			
					TASKDEBUG("\n<LOCK x nr_req=%d>\n", nr_req);
					
					do {
						/* from to buffer RDISK -> to local buffer and write into the file*/
							
						// bytes = (count > BUFF_SIZE1)?BUFF_SIZE1:count;
						bytes = (count > devvec[m_device].buff_size)?devvec[m_device].buff_size:count;
						TASKDEBUG("bytes: %d\n", bytes);
					
						TASKDEBUG("WRITE - CLIENT TO PRIMARY\n");
						TASKDEBUG("proc_rn= %d\n", proc_nr);  
						TASKDEBUG("user_vir= %X\n", user_vir);     
						TASKDEBUG("rd_ep=%d\n", rd_ep);			
						TASKDEBUG("localbuff: %X\n", devvec[m_device].localbuff);			
					
						/* copy the data from the requester process address space in other VM  to the local buffer */
						// rcode = mnx_vcopy(proc_nr, user_vir, rd_ep, localbuff, bytes); /*escribo bufferFS -> bufferlocal*/
						rcode = mnx_vcopy(proc_nr, user_vir, rd_ep, devvec[m_device].localbuff, bytes); /*escribo bufferFS -> bufferlocal*/
						
						TASKDEBUG("DRIVER: mnx_vcopy(proc_nr -> DRIVER)= %d\n", rcode);  
						TASKDEBUG("bytes= %d\n", bytes);     
						TASKDEBUG("mem_phys: %X (in DRIVER)\n", devvec[m_device].localbuff);			
						TASKDEBUG("user_vir: %X (in proc_nr %d)\n", user_vir, proc_nr);			
									
						if (rcode != 0 ){
							fprintf(stderr, "VCOPY rcode=%d\n", rcode);
							break;
						}else{
							stbytes = stbytes + bytes; /*si mnx_vcopy fue exitosa, devuelve cantidad de bytes transferidos*/
						}		
						
						/* write data from local buffer to the  virtual device file */
				
						// rcode = pwrite(img_p, localbuff, bytes, position);
						TASKDEBUG("devvec[m_device].img_p=%d, devvec[m_device].localbuff=%X, bytes=%d, position=%u\n", 
							devvec[m_device].img_p, devvec[m_device].localbuff, bytes, position);			
						bytes = pwrite(devvec[m_device].img_p, devvec[m_device].localbuff, bytes, position);
						TASKDEBUG("pwrite: %d\n", bytes);
						// if(rcode) ERROR_EXIT(errno);
						if(bytes < 0) ERROR_EXIT(errno);	
						nr_optrans++;
						TASKDEBUG("Operaciones de transferencias de bytes (cantidad vcopy)= %d\n", nr_optrans);
								
						position += bytes;
						iov->iov_addr += bytes;
						user_vir = iov->iov_addr;
						TASKDEBUG("user_vir (do-buffer) %X count %d bytes %d\n", user_vir, count, bytes);	
						count -= bytes;
						TASKDEBUG("count=%d stbytes=%d position=%ld\n", count, stbytes, position);	
					
						if( nr_nodes > 1) { /* PRIMARY;  MULTICAST to other nodes the device operation */
							/*mesagge for broadcast - according to vcopy*/
							sp_msg.msg.m_source = local_nodeid;			/* this is the primary */
							sp_msg.msg.m_type = DEV_WRITE;
							sp_msg.msg.DEVICE = m_device;
							TASKDEBUG("sp_msg.msg.DEVICE= %d\n", sp_msg.msg.DEVICE);
							sp_msg.msg.IO_ENDPT = proc_nr; /*process number = m_source, original message*/
				
							sp_msg.msg.POSITION = position;
							TASKDEBUG("(Armo) sp_msg.msg.POSITION %X\n", sp_msg.msg.POSITION);	
							sp_msg.msg.COUNT = bytes; /*por ahora sólo los bytes=vcopy; pero ver? - sólo los bytes q escribí*/
							TASKDEBUG("sp_msg.msg.COUNT %u %d\n", sp_msg.msg.COUNT, sp_msg.msg.COUNT);	
							sp_msg.msg.ADDRESS = addr_s; /*= iov->iov_addr; address del cliente */
							
							/*datos que deben escribirse en la réplica*/
							// memcpy(sp_msg.buffer_data,localbuff,bytes); 
						
							/*antes buffer comprimido*/
							// memcpy(sp_msg.buf.buffer_data,devvec[m_device].localbuff,bytes); 
							// TASKDEBUG("sizeof(sp_msg.buff) %d\n", sizeof(sp_msg.buf.buffer_data));		
							/*antes buffer comprimido*/
						
							/*compress data buffer*/
							TASKDEBUG("COMPRESS DATA BUFFER BEFORE BROADCAST\n");
							sp_msg.buf.buffer_size = sizeof(sp_msg.buf.buffer_data);
						
							TASKDEBUG("lz4_data_cd (in_buffer=%X, inbuffer_size=%d, condition UNCOMP =%d\n",
							devvec[m_device].localbuff,bytes,UNCOMP);
						
							lz4_data_cd(devvec[m_device].localbuff, bytes, UNCOMP);
						
							sp_msg.buf.flag_buff = msg_lz4cd.buf.flag_buff;
							TASKDEBUG("sp_msg.buf.flag_buff =%d\n", sp_msg.buf.flag_buff);
							sp_msg.buf.buffer_size = msg_lz4cd.buf.buffer_size;
							TASKDEBUG("sp_msg.buf.buffer_size =%d\n", sp_msg.buf.buffer_size);
							memcpy(sp_msg.buf.buffer_data, msg_lz4cd.buf.buffer_data, sp_msg.buf.buffer_size);
//							TASKDEBUG("sp_msg.buf.buffer_data =%s\n", sp_msg.buf.buffer_data);
							/*END compress data buffer*/

							sp_msg.msg.m2_l2 = ( count > 0 )?nr_optrans:0; /*se usa este campo para saber el número de operaciones*/
						
							bytes_c = sp_msg.buf.buffer_size + sizeof(int) + sizeof(long);
							TASKDEBUG("bytes_c\n=%d", bytes_c);
						
											
							TASKDEBUG("sp_msg replica m_source=%d, m_type=%d, DEVIDE=%d, IO_ENDPT=%d, POSITION=%X, COUNT=%u, ADDRESS=%X, nr_optrans=%d, BYTES_COMPRESS=%d\n", 
								  sp_msg.msg.m_source, sp_msg.msg.m_type, sp_msg.msg.DEVICE, sp_msg.msg.IO_ENDPT, sp_msg.msg.POSITION, sp_msg.msg.COUNT, 
								  sp_msg.msg.ADDRESS, sp_msg.msg.m2_l2, sp_msg.buf.buffer_size);
							
							TASKDEBUG("broadcast x cada vcopy\n");
							rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) replica_name,  
								DEV_WRITE, (sizeof(message) + bytes_c), (char *) &sp_msg); 
								// DEV_WRITE, (sizeof(message) + bytes), (char *) &sp_msg); 
							TASKDEBUG("SP_multicast mensaje enviado\n");
							if(rcode <0) {
								pthread_mutex_unlock(&rd_mutex);	
								ERROR_RETURN(rcode);
							}
						}	
					} while(count > 0);
					TASKDEBUG("Antes de desbloquear, nr_nodes%d\n", nr_nodes);
					if( nr_nodes > 1) { /* PRIMARY;  MULTICAST to other nodes the device operation */
						pthread_cond_wait(&update_barrier,&rd_mutex); /*wait until  the process will be the PRIMARY  */	
					}
					TASKDEBUG("deesbloqueó???\n");	
					pthread_mutex_unlock(&rd_mutex);			
					TASKDEBUG("\n<UNLOCK x nr_req=%d>\n", nr_req);
					/* FIN - DESDE EL BUFFER DEL RDISK -> AL BUFFER LOCAL Y ESCRIBIR EN EL ARCHIVO*/
				}else{
					TASKDEBUG("WRITE <bytes=%d> <position=%X > <nr_optrans=%d> <nr_req=%d>\n", 
							  count,
							  position,	
							  nr_optrans,
							  nr_req);			
				
					stbytes = stbytes + count; /*sólo acumulo el total de bytes, escribí todo de una vez*/
					TASKDEBUG("BACKUP REPLY: %d\n", nr_optrans);			
				
					if (nr_optrans == 0) {
							
						TASKDEBUG("BACKUP multicast DEV_WRITE REPLY to %d nr_req=%d\n",
							primary_mbr,nr_req);
					
						msg.m_source= local_nodeid;			
						msg.m_type 	= MOLTASK_REPLY;
						rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) replica_name,  
								MOLTASK_REPLY, sizeof(message), (char *) &msg); 
			
						if(rcode <0) ERROR_RETURN(rcode);
						CLR_BIT(bm_acks, primary_mbr);
					}
				}
			}
		}
		/* Book the number of bytes transferred. Registra el número de bytes transferidos? */
		//position += count; //incremento el offset en el disco por la cantidad (count) de lo q leí o escribí
		//iov->iov_addr += count; //lo mismo para iov_addr donde está la dirección guardada
		TASKDEBUG("subtotal de bytes\n");	
		if ((iov->iov_size -= stbytes) == 0) { iov++; nr_req--; }  /*subtotal bytes, por cada iov_size según posición del vector*/
		tbytes += stbytes; /*total de bytes leídos o escritos*/
	}
	
	return(tbytes);
}

/*===========================================================================*
 *				m_do_open				     *
 *===========================================================================*/
//PRIVATE int m_do_open(dp, m_ptr)
int m_do_open(struct driver *dp, message *m_ptr) 
{
	int rcode;
	message msg;
	// struct mdevvec devinfo[NR_DEVS], *devinfo_ptr;
		
	
	TASKDEBUG("m_do_open - device number: %d - OK to open\n", m_ptr->DEVICE);


	
/*único dispositivo abierto volcado en memoria, (puntero: ptr) en load_dimage() q se llama desde main, tendría q pasarla rd_init*/
/*open sobre el archivo imagen*/
	rcode = OK;
	TASKDEBUG("rcode %d\n", rcode);
	do {
		// img_p = open(img_ptr, O_RDWR);
		if ( devvec[m_ptr->DEVICE].available == 0 ){
			TASKDEBUG("devvec[m_ptr->DEVICE].available=%d\n", devvec[m_ptr->DEVICE].available);
			rcode = errno;
			TASKDEBUG("rcode=%d\n", rcode);
			break;
			}
			
		devvec[m_ptr->DEVICE].img_p = open(devvec[m_ptr->DEVICE].img_ptr, O_RDWR);
		TASKDEBUG("Open imagen FD=%d\n", devvec[m_ptr->DEVICE].img_p);
			
			
		// if(img_p < 0) {
		if(devvec[m_ptr->DEVICE].img_p < 0) {
			TASKDEBUG("devvec[m_ptr->DEVICE].img_p=%d\n", devvec[m_ptr->DEVICE].img_p);
			rcode = errno;
			TASKDEBUG("rcode=%d\n", rcode);
			break;
			}
			
		/* local buffer to the minor device */
		// rcode = posix_memalign( (void**) &localbuff, getpagesize(), BUFF_SIZE1);
		rcode = posix_memalign( (void**) &localbuff, getpagesize(), devvec[m_ptr->DEVICE].buff_size);
		devvec[m_ptr->DEVICE].localbuff = localbuff;
		if( rcode ) {
			fprintf(stderr,"posix_memalign rcode=%d, device=%d\n", rcode, m_ptr->DEVICE);
			exit(1);
			}
		// TASKDEBUG("Aligned Buffer size=%d on address %X, device=%d\n", BUFF_SIZE1, devvec[m_ptr->DEVICE].localbuff, m_ptr->DEVICE);
		TASKDEBUG("Aligned Buffer size=%d on address %X, device=%d\n", devvec[m_ptr->DEVICE].buff_size, devvec[m_ptr->DEVICE].localbuff, m_ptr->DEVICE);
		TASKDEBUG("Local Buffer %X\n", devvec[m_ptr->DEVICE].localbuff);
		TASKDEBUG("Buffer size %d\n", devvec[m_ptr->DEVICE].buff_size);
			
		devvec[m_ptr->DEVICE].active = 1;
		TASKDEBUG("Device %d is active %d\n", m_ptr->DEVICE, devvec[m_ptr->DEVICE].active);
		
		/* Check device number on open. */
		if (m_prepare(m_ptr->DEVICE) == NIL_DEV) {
			TASKDEBUG("'m_prepare()' %d - NIL_DEV:%d\n", m_prepare(m_ptr->DEVICE), NIL_DEV);
			rcode = ENXIO;
			break;
		}
 	//if (m_preparem_prepare(m_ptr->m2_i1) == NIL_DEV) return(ENXIO);  
	}while(0);
	
	TASKDEBUG("\dc_ptr->dc_nr_nodes=%d\n",dc_ptr->dc_nr_nodes);
	
   	if( nr_nodes > 1) { /* PRIMARY;  MULTICAST to other nodes the device operation */
		if(primary_mbr == local_nodeid) {
			TASKDEBUG("PRIMARY multicast DEV_OPEN dev=%d\n", m_ptr->DEVICE);
			
			if(rcode) return(rcode);
			msg.m_source= local_nodeid;			/* this is the primary */
			msg.m_type 	= DEV_OPEN;
			msg.m2_i1	= m_ptr->DEVICE;
			
			pthread_mutex_lock(&rd_mutex);
			rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) replica_name,  
						DEV_OPEN, sizeof(message), (char *) &msg); 
						
			if(rcode <0) {
				pthread_mutex_unlock(&rd_mutex);	
				ERROR_RETURN(rcode);
			}
			pthread_cond_wait(&update_barrier,&rd_mutex); /*wait until  the process will be the PRIMARY  */	
			pthread_mutex_unlock(&rd_mutex);	
			rcode = OK;
			TASKDEBUG("END PRIMARY\n");
		} else { 	/*  BACKUP:   MULTICAST to PRIMARY the ACKNOWLEDGE  */
			TASKDEBUG("BACKUP multicast DEV_OPEN REPLY to %d rcode=%d\n", primary_mbr ,rcode);
			
			
			msg.m_source= local_nodeid;			
			msg.m_type 	= MOLTASK_REPLY;
			msg.m2_i1	= m_ptr->DEVICE;
			msg.m2_i2	= DEV_OPEN;
			msg.m2_i3	= rcode;
			rcode = SP_multicast (sysmbox, SAFE_MESS, (char *) replica_name,  
						MOLTASK_REPLY, sizeof(message), (char *) &msg); 
						
			if(rcode <0) ERROR_RETURN(rcode);
			CLR_BIT(bm_acks, primary_mbr);
		}
		
	}
  
  devinfo_ptr  = &devvec[m_ptr->DEVICE];
  //TASKDEBUG
  TASKDEBUG(DEV_USR_FORMAT,DEV_USR_FIELDS(devinfo_ptr));
	
  TASKDEBUG("END m_do_open\n");  return(rcode);
}

/*===========================================================================*
 *				rd_init					     *
 *===========================================================================*/
//PRIVATE void rd_init()
int rd_init(void )
{
	int rcode, i;

 	rd_lpid = getpid();

#define WAIT4BIND_MS 1000

if (bind_flag != 1){	
	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		TASKDEBUG("RDISK: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			TASKDEBUG("RDISK: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	
	/* NODE info */
	local_nodeid = mnx_getdrvsinfo(&drvs);
	if(local_nodeid < 0 )
		ERROR_EXIT(EMOLDVSINIT);
	drvs_ptr = &drvs;
	TASKDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(drvs_ptr));
	TASKDEBUG("local_nodeid=%d\n", local_nodeid);
	
	TASKDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&drvs);
	if(rcode) ERROR_EXIT(rcode);
	TASKDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(drvs_ptr));
	
	TASKDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	TASKDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	TASKDEBUG("Get RDISK info from SYSTASK\n");
	rcode = sys_getproc(&proc_rd, RDISK_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	rd_ptr = &proc_rd;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(rd_ptr));
	if( TEST_BIT(rd_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"RDISK task not started\n");
		fflush(stderr);		
		ERROR_EXIT(EMOLNOTBIND);
	}
	
	rd_ep = rd_ptr->p_endpoint;
		
	for( i = 0; i < NR_DEVS; i++){
		// rcode = stat(img_ptr,  &img_stat);
		if ( devvec[i].available != 0 ){
			printf("Byte offset to the partition start (Device = %d - img_ptr): %X\n", i, devvec[i].img_ptr);
			m_geom[i].dv_base = cvul64(devvec[i].img_ptr);
			printf("Byte offset to the partition start (m_geom[DEV=%d].dv_base): %X\n", i, m_geom[i].dv_base);
	
			printf("Number of bytes in the partition (Device = %d - img_size): %u\n", i, devvec[i].st_size);
			m_geom[i].dv_size = cvul64(devvec[i].st_size);	
			printf("Number of bytes in the partition (m_geom[DEV=%d].dv_size): %u\n", i, m_geom[i].dv_size);
			}
	}
	
	if (replicate_flag == 1){	
		TASKDEBUG("Initializing REPLICATE\n");
		rcode = init_replicate();	
		if( rcode)ERROR_EXIT(rcode);
		
		TASKDEBUG("Starting REPLICATE thread\n");
		rcode = pthread_create( &replicate_thread, NULL, replicate_main, 0 );
		if( rcode)ERROR_EXIT(rcode);

		pthread_mutex_lock(&rd_mutex);
		pthread_cond_wait(&rd_barrier,&rd_mutex); /* unlock, wait, and lock again rd_mutex */	
		TASKDEBUG("RDISK has been signaled by the REPLICATE thread  FSM_state=%d\n",  FSM_state);
		if( FSM_state == STS_LEAVE) {	/* An error occurs trying to join the spread group */
			pthread_mutex_unlock(&rd_mutex);
			ERROR_RETURN(EMOLCONNREFUSED);
		}	

		TASKDEBUG("Replicated driver. nr_nodes=%d primary_mbr=%d\n",  nr_nodes, primary_mbr);
		TASKDEBUG("primary_mbr=%d - local_nodeid=%d\n", primary_mbr, local_nodeid);
		if ( primary_mbr != local_nodeid) {
			TASKDEBUG("wait until  the process will be the PRIMARY\n");
			pthread_cond_wait(&primary_barrier,&rd_mutex); /*wait until  the process will be the PRIMARY  */	
			TASKDEBUG("RDISK_PROC_NR(%d) endpoint %d\n", RDISK_PROC_NR, rd_ep);
			rcode = mnx_migr_start(dc_ptr->dc_dcid, RDISK_PROC_NR);
			TASKDEBUG("mnx_migr_start rcode=%d\n",	rcode);
			rcode = mnx_migr_commit(rd_lpid, dc_ptr->dc_dcid, RDISK_PROC_NR, local_nodeid);
			TASKDEBUG("mnx_migr_commit rcode=%d\n",	rcode);			
		}
		pthread_mutex_unlock(&rd_mutex);
	}
}
else{
	/*manual bind*/
		
	/* NODE info */
	local_nodeid = mnx_getdrvsinfo(&drvs);
	if(local_nodeid < 0 )
		ERROR_EXIT(EMOLDVSINIT);
	drvs_ptr = &drvs;
	TASKDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(drvs_ptr));
	TASKDEBUG("local_nodeid=%d\n", local_nodeid);
	
	TASKDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&drvs);
	if(rcode) ERROR_EXIT(rcode);
	TASKDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(drvs_ptr));
	
	TASKDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	TASKDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	// TASKDEBUG("Get RDISK info from SYSTASK\n");
	// rcode = sys_getproc(&proc_rd, RDISK_PROC_NR);
	// if(rcode) ERROR_EXIT(rcode);
	// rd_ptr = &proc_rd;
	// TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(rd_ptr));
	// if( TEST_BIT(rd_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		// fprintf(stderr,"RDISK task not started\n");
		// fflush(stderr);		
		// ERROR_EXIT(EMOLNOTBIND);
	// }
	
	// rd_ep = rd_ptr->p_endpoint;
		
	for( i = 0; i < NR_DEVS; i++){
		// rcode = stat(img_ptr,  &img_stat);
		if ( devvec[i].available != 0 ){
			printf("Byte offset to the partition start (Device = %d - img_ptr): %X\n", i, devvec[i].img_ptr);
			m_geom[i].dv_base = cvul64(devvec[i].img_ptr);
			printf("Byte offset to the partition start (m_geom[DEV=%d].dv_base): %X\n", i, m_geom[i].dv_base);
	
			printf("Number of bytes in the partition (Device = %d - img_size): %u\n", i, devvec[i].st_size);
			m_geom[i].dv_size = cvul64(devvec[i].st_size);	
			printf("Number of bytes in the partition (m_geom[DEV=%d].dv_size): %u\n", i, m_geom[i].dv_size);
			}
	}
	
	if (replicate_flag == 1){	
		TASKDEBUG("Initializing REPLICATE\n");
		rcode = init_replicate();	
		if( rcode)ERROR_EXIT(rcode);
		
		TASKDEBUG("Starting REPLICATE thread\n");
		rcode = pthread_create( &replicate_thread, NULL, replicate_main, 0 );
		if( rcode)ERROR_EXIT(rcode);

		pthread_mutex_lock(&rd_mutex);
		pthread_cond_wait(&rd_barrier,&rd_mutex); /* unlock, wait, and lock again rd_mutex */	
		TASKDEBUG("RDISK has been signaled by the REPLICATE thread  FSM_state=%d\n",  FSM_state);
		if( FSM_state == STS_LEAVE) {	/* An error occurs trying to join the spread group */
			pthread_mutex_unlock(&rd_mutex);
			ERROR_RETURN(EMOLCONNREFUSED);
		}	

		TASKDEBUG("Replicated driver. nr_nodes=%d primary_mbr=%d\n",  nr_nodes, primary_mbr);
		TASKDEBUG("primary_mbr=%d - local_nodeid=%d\n", primary_mbr, local_nodeid);
		if ( primary_mbr != local_nodeid) {
			TASKDEBUG("wait until  the process will be the PRIMARY\n");
			pthread_cond_wait(&primary_barrier,&rd_mutex); /*wait until  the process will be the PRIMARY  */	
			TASKDEBUG("RDISK_PROC_NR(%d) endpoint %d\n", RDISK_PROC_NR, rd_ep);
			rcode = mnx_migr_start(dc_ptr->dc_dcid, RDISK_PROC_NR);
			TASKDEBUG("mnx_migr_start rcode=%d\n",	rcode);
			rcode = mnx_migr_commit(rd_lpid, dc_ptr->dc_dcid, RDISK_PROC_NR, local_nodeid);
			TASKDEBUG("mnx_migr_commit rcode=%d\n",	rcode);			
		}
		pthread_mutex_unlock(&rd_mutex);
	}
}
	return(OK);
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
int m_do_close(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
int rcode;

	// rcode = close(img_p);
	if (devvec[m_ptr->DEVICE].active != 1) { //VER SI HAY QUE FIJAR UN ERROR
		TASKDEBUG("Device %d, is not open\n", m_ptr->DEVICE);
		rcode = -1; 
		}
	else{	
		TASKDEBUG("devvec[m_ptr->DEVICE].img_p=%d\n",devvec[m_ptr->DEVICE].img_p);
		rcode = close(devvec[m_ptr->DEVICE].img_p);
		if(rcode < 0) ERROR_EXIT(errno); 
		
		TASKDEBUG("Close device number: %d\n", m_ptr->DEVICE);
		devvec[m_ptr->DEVICE].img_ptr = NULL;
		devvec[m_ptr->DEVICE].img_p = NULL;
		devvec[m_ptr->DEVICE].st_size = 0;
		devvec[m_ptr->DEVICE].st_blksize = 0;
		devvec[m_ptr->DEVICE].localbuff = NULL;
		devvec[m_ptr->DEVICE].active = 0;
		devvec[m_ptr->DEVICE].available = 0;
	
		TASKDEBUG("Buffer %X\n", devvec[m_ptr->DEVICE].localbuff);
		free(devvec[m_ptr->DEVICE].localbuff);
		TASKDEBUG("Free buffer\n");
		}
	// if(rcode < 0) ERROR_EXIT(errno); 
	
	
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

