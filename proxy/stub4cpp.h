/* stub4cpp.h */
#if defined (__cplusplus)
extern "C" {
#endif

long CPP_put2lcl(proxy_hdr_t *hdr_ptr, proxy_payload_t *pl_ptr);
long CPP_bind(int dcid,int endpoint);
long CPP_unbind(int dcid,int endpoint);
long CPP_proxy_conn(int pxid, int status);
long CPP_wait4bind_T(long ms);
long CPP_get2rmt(proxy_hdr_t *hdr_ptr, proxy_payload_t *pl_ptr);
long CPP_getdvsinfo(dvs_usr_t *dvs_usr_ptr);
long CPP_proxies_bind(char *px_name, int px_nr, int spid, int rpid, int maxcopybuf);
long CPP_node_up(char *node_name, int nodeid, int px_nr);

#if defined (__cplusplus)
}
#endif








