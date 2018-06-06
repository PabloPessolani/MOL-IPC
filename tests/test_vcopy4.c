#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#define PAGE_SIZE 4096
#include "./kernel/minix/cmd.h"
#include "./kernel/minix/proxy_usr.h"


char buffer[MAXBUFSIZE];
      
void  main ( int argc, char *argv[] )
{
	int dcid, svr_pid, svr_ep, svr_nr, ret;
	int clt1_pid, clt1_ep, clt1_nr;
	int clt2_pid, clt2_ep, clt2_nr;
	message m, m1, m2;

    	if ( argc != 5) {
 	        printf( "Usage: %s <dcid>  <svr_nr> <clt1_nr> <clt2_nr> \n", argv[0] );
 	        exit(1);
	    }

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
	
	strcpy(buffer,"THIS MESSAGE IS FOR ALL PROCESSES");
	
	if( (clt1_pid = fork()) != 0 )	{		/* PARENT = REQUESTER */
	
		if( (clt2_pid = fork()) != 0 )	{		/* PARENT = REQUESTER */
	
			
			printf("REQUESTER pause before VCOPY\n");
			sleep(5);
			
			clt1_ep =  mnx_getep(clt1_pid);
			if( clt1_ep < 0 ) 
			printf("GETEP ERROR clt1_ep=%d\n",clt1_ep);

			clt2_ep =  mnx_getep(clt2_pid);
			if( clt2_ep < 0 ) 
			printf("GETEP ERROR clt2_ep=%d\n",clt2_ep);
			
			printf("Dump procs of DC%d\n",dcid); 
			ret = mnx_proc_dump(dcid);
			if( ret != OK){
				printf("PROCDUMP ERROR %d\n",ret); 
				exit(1);
			}
			
			ret = mnx_vcopy(clt1_ep, buffer, clt2_ep, buffer, strlen(buffer));	
			if( ret != 0 )
				printf("VCOPY ret=%d\n",ret);
			
			ret = mnx_receive(ANY, (long) &m1);		/* FOR ONE CLIENT */ 
				if( ret != OK){
				printf("RECEIVE ERROR %d\n",ret); 
				exit(1);
			}

			printf("RECEIVE1 msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
				m1.m_source,
				m1.m_type,
				m1.m1_i1,
				m1.m1_i2,
				m1.m1_i3);  
		
			m1.m_type = 0xF0; /*pseudo REPLY type*/
			ret = mnx_send(m1.m_source, (long) &m1);
			if( ret != 0 )
				printf("SEND ret=%d\n",ret);
			printf("REQUESTER BUFFER:%s\n",buffer);
			
					
			ret = mnx_receive(ANY, (long) &m2);		/* FOR THE OTHER CLIENT CLIENT */ 
			if( ret != OK){
				printf("RECEIVE ERROR %d\n",ret); 
				exit(1);
			}

			printf("RECEIVE2 msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
				m2.m_source,
				m2.m_type,
				m2.m1_i1,
				m2.m1_i2,
				m2.m1_i3);		
				
			m2.m_type = 0xF0; /*pseudo REPLY type*/
			ret = mnx_send(m2.m_source, (long) &m2);
			if( ret != 0 )
				printf("SEND ret=%d\n",ret);
			printf("REQUESTER BUFFER:%s\n",buffer);

			
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

			m.m_type= 0x0F;
			m.m1_i1 = 0x07;
			m.m1_i2 = 0x08;
			m.m1_i3 = 0x09;
			printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
				m.m_type,
				m.m1_i1,
				m.m1_i2,
				m.m1_i3);

			ret = mnx_send(svr_ep, (long) &m);
			if( ret != 0 )
					printf("SEND ret=%d\n",ret);

			ret = mnx_receive(svr_ep, (long) &m);
			if( ret != 0 )
					printf("SEND ret=%d\n",ret);

			printf("REPLY msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
				m.m_source,
				m.m_type,
				m.m1_i1,
				m.m1_i2,
				m.m1_i3);

			printf("RECEIVER OUT: BUFFER:%s\n",buffer);

			sleep(10);
			
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
    	printf("BIND SENDER dcid=%d clt1_pid=%d clt1_nr=%d clt1_ep=%d\n",
			dcid,
			clt1_pid,
			clt1_nr,
			clt1_ep);

	  	m.m_type= 0x0F;
		m.m1_i1 = 0x01;
		m.m1_i2 = 0x02;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

		strcpy(buffer,"SENDER BUFFER BEFORE SEND AND RECEIVE");
			ret = mnx_send(svr_ep, (long) &m);
			if( ret != 0 )
					printf("SEND ret=%d\n",ret);

			ret = mnx_receive(svr_ep, (long) &m);
			if( ret != 0 )
					printf("SEND ret=%d\n",ret);

		printf("SENDER BUFFER OUT:%s\n",buffer);

		printf("REPLY msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);
		
		sleep(10);
		
    	printf("UNBIND SENDER dcid=%d clt1_pid=%d clt1_nr=%d clt1_ep=%d\n",
			dcid,
			clt1_pid,
			clt1_nr,
			clt1_ep);
	    	mnx_unbind(dcid, clt1_ep);
	}

 exit(0);
 }
