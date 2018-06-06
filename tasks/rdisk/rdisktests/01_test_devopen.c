#define  MOL_USERSPACE	1
//#define TASKDBG		1
#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
// #include "stub_syscall.h"
#include "../../../kernel/minix/config.h"

#include "../../../tasks/rdisk/rdisk.h"
//#include "./kernel/minix/kipc.h"
//#include "./kernel/minix/callnr.h"
//#include "limits.h"
#include "../../../kernel/minix/syslib.h"
#include "../../../kernel/minix/proc_usr.h"


//#define BUFFER 20
//#define nro_IOREQS 3 /*por ahora sólo pruebo con un requerimiento, para la longitud del vector*/
//#define WRITE 2000

#define WAIT4BIND_MS 1000 
int dcid, clt_ep, clt_nr, clt_lpid;
int local_nodeid;
drvs_usr_t drvs, *drvs_ptr;
proc_usr_t rd, *rd_ptr;
proc_usr_t clt, *clt_ptr;
DC_usr_t  vmu, *dc_ptr;

/*===========================================================================*
 *				init_m3ipc					     *
 *===========================================================================*/
void init_m3ipc(void)
{
	int rcode;

	clt_lpid = getpid();
	do {
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		TASKDEBUG("CLIENT mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			TASKDEBUG("CLIENT mnx_wait4bind_T TIMEOUT\n");
			continue ;
		} else if ( rcode < 0)
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);

	TASKDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&drvs);
	if (rcode) ERROR_EXIT(rcode);
	drvs_ptr = &drvs;
	TASKDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(drvs_ptr));

	TASKDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if (rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	TASKDEBUG(DC_USR_FORMAT, DC_USR_FIELDS(dc_ptr));

	TASKDEBUG("Get RDISK info from SYSTASK\n");
	rcode = sys_getproc(&rd, RDISK_PROC_NR);
	if (rcode) ERROR_EXIT(rcode);
	rd_ptr = &rd;
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(rd_ptr));
	if ( TEST_BIT(rd_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr, "RDISK not started\n");
		fflush(stderr);
		ERROR_EXIT(EMOLNOTBIND);
	}

	TASKDEBUG("Get Client info from SYSTASK\n");
	rcode = sys_getproc(&clt, SELF);
	if (rcode) ERROR_EXIT(rcode);
	clt_ptr = &clt;
	TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(clt_ptr));

}
void  main ( int argc, char *argv[] )
{
    int dcid;
    int clt_pid, svr_pid;
    int clt_ep, svr_ep;
    int clt_nr, svr_nr, ret, proc_nr;
	off_t position;
	unsigned count;
    message m;
	int i;

    int bytesCount = 0;

	
	/*    m_type      DEVICE    IO_ENDPT    COUNT    POSITION  ADRRESS*/
	/* ----------------------------------------------------------------*/
	/* |  DEV_OPEN  | device  | proc nr |         |         |         |*/
	//	m.m_type		m.m2_i1	m.m2_i2		

		

    // if ( argc != 5)
    // {
        // printf( "Usage: %s <dcid> <clt_nr> <svr_nr> <minor_dev>\n", argv[0] );
        // exit(1);
    // }
	
	if ( argc != 2)
    {
        printf( "Usage: %s <minor_dev>\n", argv[0] );
        exit(1);
    }

	/*verifico q el nro de vm sea válido*/
    // dcid = atoi(argv[1]);
    // if ( dcid < 0 || dcid >= NR_VMS)
    // {
        // printf( "Invalid dcid [0-%d]\n", NR_VMS - 1 );
        // exit(1);
    // }

	/*entiendo nro cliente "cualquiera" - sería este proceso el q va a ser el cliente*/
    // clt_nr = atoi(argv[2]);
	
	/*server endpoint es el q ya tengo de memory*/
    // svr_ep = svr_nr = atoi(argv[3]);

	/*hago el binding del proceso cliente, o sea este*/
    // clt_pid = getpid();
    // clt_ep = mnx_bind(dcid, clt_nr);
	// proc_nr = clt_ep;
	
    // if ( clt_ep < 0 )
        // printf("BIND ERROR clt_ep=%d\n", clt_ep);
	
	/*BIND - DEMONIZE*/
	
	init_m3ipc();
	
	/*BIND - DEMONIZE*/

    // printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
           // dcid,
           // clt_pid,
           // clt_nr,
           // clt_ep);
		   
	TASKDEBUG("CLIENT pause before SENDREC\n");
	sleep(2); 
	
	svr_ep = RDISK_PROC_NR;
	
	proc_nr = clt_lpid;

	m.m_type = DEV_OPEN;
	
	printf("m_type: %d -  DEV_OPEN: %d\n", m.m_type, DEV_OPEN);

	
    //m.m_type = DEV_CLOSE;
	//printf("m_type: %d -  DEV_OPEN: %d\n", m.m_type, DEV_CLOSE);
	
    //m.DEVICE = RAM_DEV; 
	//m.DEVICE = IMGRD_DEV;
	
	// m.DEVICE = RAM_DEV;
	m.DEVICE = atoi(argv[1]);
	printf("DEVICE: %d\n", m.DEVICE);

    m.IO_ENDPT = proc_nr; //process doing the request (usado en memory.c)
	printf("DEV_OPEN - proc_nr: m_m2_i2=%d\n", m.IO_ENDPT);	

	printf("SENDREC msg  m_type=%d, DEVICE=%d, IO_ENDPT(proc_nr)=%d\n",
           m.m_type,
           m.DEVICE,
		   m.IO_ENDPT);

    ret = mnx_sendrec(svr_ep, (long) &m); /*para enviar y recibir de un cliente a un servidor*/
	
	if( ret != 0 )
    	printf("SEND ret=%d\n",ret);

	sleep(2);
	
	printf("REPLY: -> m_type=%d, (REP_ENDPT)=%d, (REP_STATUS)=%d\n",
		m.m_type,
		m.REP_ENDPT,
		m.REP_STATUS);	
		
	//while(1){
	//	printf("Ctrl-C to exit\n");
		sleep(1);
		return;
	//}
	
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