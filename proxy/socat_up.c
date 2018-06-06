/********************************************************/
/* 		SOCAT PROXIES UP 						*/
/********************************************************/
/*
	node0> # LCLNODE=node0
	node0> # LCLPORT=3000
	node0> # RMTNODE=node1
	node0> # RMTPORT=3001
	node0> # PXID=1
	node0> # socat_sproxy | socat - TCP:$RMTNODE:$RMTPORT  retry=<num> interval=<timespec> &
	node0> # SPID=$!
 	node0> # socat - TCP-LISTEN:$LCLNODE:$LCLPORT retry=<num> interval=<timespec>| socat_rproxy  &
	node0> # RPID=$!
	node0> # socat_up $RMTNODE $PXID $SPID $RPID
	MORE INFO: http://www.dest-unreach.org/socat/doc/socat.html
*/
#include "proxy.h"
#include "debug.h"
#include "macros.h"
       
int local_nodeid;
dvs_usr_t dvs;   

/*----------------------------------------------*/
/*		MAIN: 			*/
/*----------------------------------------------*/
void  main ( int argc, char *argv[] )
{
    int spid, rpid;
    int rcode;
    dvs_usr_t *d_ptr;    
	unsigned int	pxid; 		/* The number of pair of proxies	*/
	char			pxname[MAXPROXYNAME];
	
    if (argc != 5) {
     	fprintf(stderr,"Usage: %s <pxname> <pxid> <sproxy_PID> <rproxy_PID>\n", argv[0]);
    	exit(0);
    }

    strncpy(pxname, argv[1], MAXPROXYNAME);
    printf("SOCAT Proxy Pair name: %s\n",pxname);
 
    pxid = atoi(argv[2]);
    printf("SOCAT Proxy Pair id: %d\n",pxid);

    local_nodeid = mnx_getdvsinfo(&dvs);
    d_ptr=&dvs;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(d_ptr));

    spid = atoi(argv[3]);
    printf("SOCAT sender proxy PID: %d\n",spid); 

    rpid = atoi(argv[4]);
    printf("SOCAT receiver proxy PID: %d\n",rpid); 

    /* register the proxies */
//    rcode = mnx_proxies_bind(pxname, pxid, spid, rpid);
//    if( rcode < 0) ERROR_EXIT(rcode);
	rcode = mnx_proxy_conn(pxid, CONNECT_SPROXY);
    if( rcode < 0) ERROR_EXIT(rcode);
	rcode = mnx_proxy_conn(pxid, CONNECT_RPROXY);
    if( rcode < 0) ERROR_EXIT(rcode);
	rcode= mnx_node_up(pxname, pxid, pxid);	
    if( rcode < 0) ERROR_EXIT(rcode);

    exit(0);
}

