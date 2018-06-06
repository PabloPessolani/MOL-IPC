#include "../../lib/syslib/syslib.h"

dvs_usr_t dvs;
int local_nodeid;
   
void  main ( int argc, char *argv[] )
{
	int i, dcid, child_pid, proc_pid,  proc_nr, proc_ep, child_ep, child_nr, ret;
	message m, *m_ptr;
	dc_usr_t dcu, *dc_ptr;
	dvs_usr_t dvs, *dvs_ptr;
	proc_usr_t proc_usr, *proc_ptr, *proc_tab;
	priv_usr_t *priv_ptr, *priv_tab;
	clock_t timeout;

	int status;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <proc> <timeout> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	proc_nr = atoi(argv[2]);
	timeout = atoi(argv[3]);


	local_nodeid = mnx_getdvsinfo(&dvs);
printf("local_nodeid=%d\n",local_nodeid);

	proc_pid = getpid();

	printf("Binding process %d to DC%d with proc_nr=%d\n",proc_pid,dcid,proc_nr);
	proc_ep = mnx_bind(dcid, proc_nr);
	if( proc_ep < 0 ) {
		printf("BIND ERROR %d\n",proc_ep);
		exit(proc_ep);
	}
	printf("Process proc_ep=%d\n",proc_ep);

	printf("Sending SYS_SETALARM request to SYSTEM timeout=%d\n", timeout );
	ret = sys_setalarm(timeout, 0);

	sleep(60);	

 }



