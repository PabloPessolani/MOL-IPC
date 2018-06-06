#define  MOL_USERSPACE	1

#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
//#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "../tasks/memory/memory.h"
//#include "./kernel/minix/kipc.h"
//#include "./kernel/minix/callnr.h"
//#include "limits.h"

#define BUFFER 4096
#define nro_IOREQS 3 /*por ahora sólo pruebo con un requerimiento, para la longitud del vector*/
#define WRITE 2000

void  main ( int argc, char *argv[] )
{
    int vmid;
    int clt_pid, svr_pid;
    int clt_ep, svr_ep;
    int clt_nr, svr_nr, ret, proc_nr;
	//iovec_t iov1;
	off_t position;
	unsigned count;
    message m;
	int i;

    int bytesCount = 0;

	char buffer[BUFFER];
	
    if ( argc != 7)
    {
        printf( "Usage: %s <vmid> <clt_nr> <svr_nr> <position> <bytesCount> <minor_device>\n", argv[0] );
        exit(1);
    }

	
	
	/*verifico q el nro de vm sea válido*/
    vmid = atoi(argv[1]);
    if ( vmid < 0 || vmid >= NR_VMS)
    {
        printf( "Invalid vmid [0-%d]\n", NR_VMS - 1 );
        exit(1);
    }

	/*entiendo nro cliente "cualquiera" - sería este proceso el q va a ser el cliente*/
    clt_nr = atoi(argv[2]);
	
	/*server endpoint es el q ya tengo de memory*/
    svr_ep = svr_nr = atoi(argv[3]);

	/*hago el binding del proceso cliente, o sea este*/
    clt_pid = getpid();
    clt_ep = mnx_bind(vmid, clt_nr);
	proc_nr = clt_ep;
	
    if ( clt_ep < 0 )
        printf("BIND ERROR clt_ep=%d\n", clt_ep);

    printf("BIND CLIENT vmid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
           vmid,
           clt_pid,
           clt_nr,
           clt_ep);
		   
	printf("CLIENT pause before SENDREC\n");
	sleep(2); 

	
	position = atoi(argv[4]);
    
    bytesCount = atoi(argv[5]);
    if ( bytesCount == 0)
    {
        printf( "bytesCount no debe ser CERO:%d\n", bytesCount);
        exit(1);
    }
	

    // armado del mensaje READ, siguiendo lo indicado en driver.c
	//MENSAJE TIPO m2
	//*    m_type      DEVICE    IO_ENDPT    COUNT    POSITION  ADRRESS
	 //  DEV_WRITE		device	 proc nr	bytes	  offset	buf ptr
	
	//	m.m_type		m.m2_i1	m.m2_i2		m.m2_l2	 m.m2_l1	m.m2_p1
	
    m.m_type = DEV_READ;
	//m.m_type = WRITE; /*para probar q no hace nada el server con otro tipo q no sea SC O GA*/
	printf("DEV_READ: m_type=%d\n", m.m_type);
	//int device; parámetro de función m_prepare en memory.co o not_prepare en driver.c
    m.DEVICE = RAM_DEV; //A VER SI LO ENVÍO COMO PARÁMETRO 
	m.DEVICE = atoi(argv[6]); //A VER SI LO ENVÍO COMO PARÁMETRO 
	printf("DEV_WRITE - Device: m_m2_i1=%d\n", m.DEVICE);
	
    m.IO_ENDPT = proc_nr; //process doing the request (usado en memory.c)
	printf("DEV_WRITE - proc_nr: m_m2_i2=%d\n", m.IO_ENDPT);
	
	//off_t position;			/* offset on device to read or write */
	//puede ser q off_t esté typedef unsigned long  mnx_off_t;	???   /* offset within a file */
	
	m.POSITION = position;
	printf("DEV_WRITE - offset(position): m_m2_l1=%u\n", m.POSITION);
	
	char *buff; /*puntero al buffer*/
	buff = buffer;
		
	printf("DEV_WRITE: count %u\n", bytesCount);
	printf("DEV_WRITE: buff (puntero al buffer)=%p\n", buff);
//	printf("contenido del buffer %s\n", buffer);
	m.COUNT = bytesCount; /*cantidad de bytes a escribir*/
    m.ADDRESS = buff; /*buffer del cliente*/ 

	printf("SENDREC msg DEV_WRITE: m_type=%d, DEVICE=%d, IO_ENDPT=%d, POSITION=%u, COUNT=%u, ADDRESS=%p\n",
           m.m_type,
           m.DEVICE,
           m.IO_ENDPT,
		   m.POSITION,
		   m.COUNT,
           m.ADDRESS);

    ret = mnx_sendrec(svr_ep, (long) &m); /*para enviar y recibir de un cliente a un servidor*/
	if( ret != 0 )
    	printf("sendrec - ret=%d\n",ret);

	
	/*sólo envío por ahora, no espero respuesta del servidor*/
	
	
	sleep(2);
	printf("contenido del buffer %s\n", buffer);
		
	printf("REPLY: -> m_type=%d, (REP_ENDPT)=%d, (REP_STATUS)=%d\n",
		m.m_type,
		m.REP_ENDPT,
		m.REP_STATUS);	
		
	while(1){
		printf("Ctrl-C to exit\n");
		sleep(1);
	}
	
 }
	   
		   
    //ret = mnx_sendrec(svr_ep, (long) &m);
    //if ( ret != OK )
    //    printf("ERROR SENDREC ret=%d\n", ret);
    //else
    //    printf("SENDREC ret=%d\n", ret);

    /*printf("REPLY OPEN msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%s\n",
           m.m_source,
           m.m_type,
           m.m1_i1,
           m.m1_i2,
           m.m1_p1);

    fileDescriptor = m.m1_i1;
    
    /*READ!!!!*********************************************************************/
    /*printf("CLIENT pause before SENDREC (READ)\n");
    sleep(2);*/

//}