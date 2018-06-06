
#include "proxy.h"

long CPP_put2lcl(proxy_hdr_t *hdr_ptr, proxy_payload_t *pl_ptr) 
{
	return(mnx_put2lcl(hdr_ptr, pl_ptr));
}

long CPP_bind(int dcid,int endpoint)
{
	return(mnx_bind(dcid,endpoint));
}

long CPP_unbind(int dcid,int endpoint)
{
	return(mnx_unbind(dcid,endpoint));
}
	
long CPP_proxy_conn(int pxid, int status)
{
    return(mnx_proxy_conn(pxid, status));
}

long CPP_wait4bind_T(long ms)
{
	return(mnx_wait4bind_T(ms));
}

long CPP_get2rmt(proxy_hdr_t *hdr_ptr, proxy_payload_t *pl_ptr) 
{
	return(mnx_get2rmt(hdr_ptr, pl_ptr));
}

long CPP_getdvsinfo(dvs_usr_t *dvs_usr_ptr)
{
	return(mnx_getdvsinfo(dvs_usr_ptr));
}

long CPP_proxies_bind(char *px_name, int px_nr, int spid, int rpid, int maxcopybuf)
{  
	return(mnx_proxies_bind(px_name, px_nr, spid, rpid, maxcopybuf));
}

long CPP_node_up(char *node_name, int nodeid, int px_nr)
{
	int ret;
	ret = mnx_node_up(node_name, nodeid, px_nr);
    fprintf(stderr,"mnx_node_up: ret=%d\n", ret); 
	return(ret);
}








