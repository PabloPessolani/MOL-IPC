#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, src_nr, src_ep, dst_ep, ret;
	message m;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <src_nr> <dst_ep> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	src_nr = atoi(argv[2]);
	dst_ep = atoi(argv[3]);
	pid = getpid();

    printf("Binding process %d to DC%d with src_nr=%d\n",pid,dcid,src_nr);
    src_ep = mnx_bind(dcid, src_nr);
	if( src_ep < 0 ) {
		printf("BIND ERROR %d\n",src_ep);
		exit(src_ep);
	}
	printf("Process src_ep=%d\n",src_ep);

/*
	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      	if( ret != OK){
		printf("mol_proc_dump error=%d\n",ret); 
		exit(1);
	}
*/
  	m.m_type= 0xFF;
	m.m1_i1 = 0x01;
	m.m1_i2 = 0x02;
	m.m1_i3 = 0x03;
   	printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m.m_type,
		m.m1_i1,
		m.m1_i2,
		m.m1_i3);
    ret = mnx_send(dst_ep, (long) &m);
	if( ret != 0 )
	    	printf("SEND ret=%d\n",ret);
while(1){
printf("Ctrl-C to exit\n");
	sleep(1);
}
 }



