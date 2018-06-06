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


#define BUFFER 50
#define nro_IOREQS 3 /*por ahora sólo pruebo con un requerimiento, para la longitud del vector*/


void  main ( int argc, char *argv[] )
{
    int dcid;
    int clt_pid, svr_pid;
    int clt_ep, svr_ep;
    int clt_nr, svr_nr, ret, proc_nr;
	//iovec_t iov1;
	off_t position;
	unsigned count;
    message m;
	int i;

    int bytesCount = 0;

	char buffer[BUFFER],buffer1[BUFFER],buffer2[BUFFER];

	for ( i = 0 ; i < (BUFFER); i++) 
		buffer[i] = 'A';
	
	for ( i = 0 ; i < (BUFFER); i++) 
		buffer1[i] = 'B';
	
	for ( i = 0 ; i < (BUFFER); i++) 
		buffer2[i] = 'C';
		
		
	printf("En el cliente buffer: %s\n", buffer);
	printf("En el cliente buffer1: %s\n", buffer1);
	printf("En el cliente buffer2: %s\n", buffer2);

    if ( argc != 6)
    {
        printf( "Usage: %s <dcid> <clt_nr> <svr_nr> <position> <bytesCount> \n", argv[0] );
        exit(1);
    }

	/*verifico q el nro de vm sea válido*/
    dcid = atoi(argv[1]);
    if ( dcid < 0 || dcid >= NR_DCS)
    {
        printf( "Invalid dcid [0-%d]\n", NR_DCS - 1 );
        exit(1);
    }

	/*entiendo nro cliente "cualquiera" - sería este proceso el q va a ser el cliente*/
    clt_nr = atoi(argv[2]);
	
	/*server endpoint es el q ya tengo de memory*/
    svr_ep = svr_nr = atoi(argv[3]);

	/*hago el binding del proceso cliente, o sea este*/
    clt_pid = getpid();
    clt_ep = mnx_bind(dcid, clt_nr);
	proc_nr = clt_ep;
	
    if ( clt_ep < 0 )
        printf("BIND ERROR clt_ep=%d\n", clt_ep);

    printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
           dcid,
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
	

    // armado del mensaje SCATTER, siguiendo lo indicado en driver.c
	//MENSAJE TIPO m2
	//*    m_type      DEVICE    IO_ENDPT    COUNT    POSITION  ADRRESS
	//* | DEV_SCATTER| device  | proc nr | iov len |  offset | iov ptr |
	//	m.m_type		m.m2_i1	m.m2_i2		m.m2_l2	 m.m2_l1	m.m2_p1
	
    m.m_type = DEV_SCATTER;
	//m.m_type = WRITE; /*para probar q no hace nada el server con otro tipo q no sea SC O GA*/
	printf("DEV_SCATTER: m_type=%d\n", m.m_type);
	//int device; parámetro de función m_prepare en memory.co o not_prepare en driver.c
    m.DEVICE = RAM_DEV; //A VER SI LO ENVÍO COMO PARÁMETRO 
	printf("DEV_SCATTER - Device: %d\n", m.DEVICE);
	
    m.IO_ENDPT = proc_nr; //process doing the request (usado en memory.c)
	printf("DEV_SCATTER - proc_nr: %d\n", m.IO_ENDPT);
	
	//off_t position;			/* offset on device to read or write */
	//puede ser q off_t esté typedef unsigned long  mnx_off_t;	???   /* offset within a file */
	
	m.POSITION = position;
	printf("DEV_SCATTER - offset(position): %u\n", m.POSITION);
	
	//**************************************************************************************************/
	/*genero el vector, como en do_vrdwt() en driver.c*/
	//static iovec_t iovec[NR_IOREQS]; así está declarado donde:
	//NR_IOREQS --> en mol /memory/const.h y es = a MIN(NR_BUFS, 64) /* maximum number of entries in an iorequest  utilizo para este cliente long vector =1*/
	//MIN (NR_BUFS, 64) --> NR_BUFS está en config.h: bloques en de buffer caché, dependiendo de la mq??? ver cómo lo defino MACHINE ==?
	
	static iovec_t iovec[nro_IOREQS]; /*el vector*/			
	iovec_t *iov; /*puntero al vector*/
	
	iovec[0].iov_addr=buffer; /*este es el buffer q cree, sólo cargo la primera posición del vector, q tiene sólo un elemento*/
	iovec[0].iov_size=BUFFER; /*sólo 20 bytes del buffer voy a copiar*/
	printf("iovec[0].iov_addr %X\n", iovec[0].iov_addr); 
	printf("iovec[0].iov_size %u\n", iovec[0].iov_size); 
	
	iovec[1].iov_addr=buffer1; /*este es el buffer q cree, sólo cargo la primera posición del vector, q tiene sólo un elemento*/
	iovec[1].iov_size=BUFFER; /*sólo 20 bytes del buffer voy a copiar*/
	printf("iovec[1].iov_addr %X\n", iovec[1].iov_addr); 
	printf("iovec[1].iov_size %u\n", iovec[1].iov_size); 

	iovec[2].iov_addr=buffer2; /*este es el buffer q cree, sólo cargo la primera posición del vector, q tiene sólo un elemento*/
	iovec[2].iov_size=BUFFER; /*sólo 20 bytes del buffer voy a copiar*/
	printf("iovec[2].iov_addr %X\n", iovec[2].iov_addr); 
	printf("iovec[2].iov_size %u\n", iovec[2].iov_size); 	
	
	iov = iovec; 
	count =  nro_IOREQS; /*len iov*/
	
	printf("DEV_SCATTER: iov len= count %u\n", count);
	printf("DEV_SCATTER: iov addr = address puntero al vector=%p\n", iov);
	printf("contenido del buffer cliente, desde iovec[0].iov_addr %s\n", iovec[0].iov_addr);
	m.COUNT = count;
    m.ADDRESS = iov; /*puntero al vecotr iovec*/ 

	printf("SENDREC msg SCATTER: m_type=%d, DEVICE=%d, IO_ENDPT=%d, POSITION=%u, COUNT=%u, ADDRESS=%p\n",
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