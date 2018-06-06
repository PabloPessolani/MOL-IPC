#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

#define MOLERR(ep, rcode) \
 do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",argv[0], __FUNCTION__ ,__LINE__,rcode); \
	mnx_unbind(dcid, ep); \
	exit(rcode); \
 }while(0)
 
void  main ( int argc, char *argv[] )
{
	int dcid, svr_pid, svr_ep, svr_nr, ret;
	int clt1_pid, clt1_ep, clt1_nr;
	int clt2_pid, clt2_ep, clt2_nr;
	message m;

    	if ( argc != 5) {
 	        printf( "Usage: %s <dcid>  <svr_nr> <clt1_nr> <clt2_nr> \n", argv[0] );
 	        exit(1);
	    }
	printf("CLIENT2-send()->SERVER-relay()->CLIENT1\n");
	printf("then CLIENT1-send()->CLIENT2");

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	svr_nr = atoi(argv[2]);
	clt1_nr = atoi(argv[3]);
	clt2_nr = atoi(argv[4]);
	
	svr_pid = getpid();
    svr_ep = mnx_bind(dcid, svr_nr);
	printf("BIND SERVER dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
		dcid,
		svr_pid,
		svr_nr,
		svr_ep);
	
	
	if( (clt1_pid = fork()) != 0 )	{		/* PARENT = REQUESTER */
	
		if( (clt2_pid = fork()) != 0 )	{		/* PARENT = REQUESTER */
	
			while(1) {
				printf("SERVER waits RECEIVING\n");
				ret = mnx_receive(ANY, (long) &m);		/* FOR ONE CLIENT */ 
				if( ret != OK) MOLERR(svr_ep,ret);
   			
				printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
					m.m_source,
					m.m_type,
					m.m1_i1,
					m.m1_i2,
					m.m1_i3);

				sleep(2);

				clt2_ep = mnx_getep(clt2_pid);
				ret = mnx_relay(clt2_ep, (long) &m);
				if( ret != OK) MOLERR(svr_ep, ret);
			
				printf("RELAY to %d msg: m_source=%d m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n", 
					clt2_ep,
					m.m_source,
					m.m_type,
					m.m1_i1,
					m.m1_i2,
					m.m1_i3);
			}
			sleep(10);

			printf("UNBIND SERVER dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
				dcid,
				svr_pid,
				svr_nr,
				svr_ep);
				mnx_unbind(dcid, svr_ep);
		}else{					/* SON2 = RECEIVER		*/
			clt2_pid = getpid();
			clt2_ep = mnx_bind(dcid, clt2_nr);
    		printf("BIND RECEIVER dcid=%d clt2_pid=%d clt2_nr=%d clt2_ep=%d\n",
			dcid,
			clt2_pid,
			clt2_nr,
			clt2_ep);

			while(1) {
				printf("CLIENT2 waits RECEIVING\n");
				
				ret = mnx_receive(ANY, (long) &m);		/* FOR ONE CLIENT */ 
				if( ret != OK) MOLERR(clt2_ep, ret);
				printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
					m.m_source,
					m.m_type,
					m.m1_i1,
					m.m1_i2,
					m.m1_i3);
	
				m.m_type = 0x80;		
				ret = mnx_send(m.m_source, (long) &m);
				if( ret != OK) MOLERR(clt2_ep, ret);
			}
			sleep(5);
			printf("UNBIND RECEIVER dcid=%d clt2_pid=%d clt2_nr=%d clt2_ep=%d\n",
				dcid,
				clt2_pid,
				clt2_nr,
				clt2_ep);
			mnx_unbind(dcid, clt2_ep);	
		}
	}else{						/* SON1  = SENDER		*/
			clt1_pid = getpid();
			clt1_ep = mnx_bind(dcid, clt1_nr);
    		printf("BIND CLIENT1 dcid=%d clt1_pid=%d clt1_nr=%d clt1_ep=%d\n",
			dcid,
			clt1_pid,
			clt1_nr,
			clt1_ep);

			printf("CLIENT1(%d) makes sendrec to SERVER(%d)\n", svr_ep, clt1_ep);
		  	m.m_type= 0x00;
			m.m1_i1 = 0x01;
			m.m1_i2 = 0x02;
			m.m1_i3 = 0x03;
   		printf("SENDREC msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);
	   		ret = mnx_sendrec(svr_ep, (long) &m);
			if( ret != 0 ) MOLERR(clt1_ep,ret);

		printf("UNBIND CLIENT1 dcid=%d clt1_pid=%d clt1_nr=%d clt1_ep=%d\n",
				dcid,
				clt1_pid,
				clt1_nr,
				clt1_ep);
		mnx_unbind(dcid, clt1_ep);	
	}

 exit(0);
 }
