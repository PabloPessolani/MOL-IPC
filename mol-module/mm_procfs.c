/****************************************************************/
/****************************************************************/
/*			MINIX OVER LINUX PROC FS ROUTINES 			*/
/****************************************************************/

#include "./minix/const.h"


/*
       * How to be a proc read function
       * ------------------------------
                     * Prototype:
                     *    int f(char *buffer, char **start, off_t offset,
                     *          int count, int *peof, void *dat)
                     *
                     * Assume that the buffer is "count" bytes in size.
                     *
                     * If you know you have supplied all the data you
                     * have, set *peof.
                     *
                     * You have three ways to return data:
                     * 0) Leave *start = NULL.  (This is the default.)
                     *    Put the data of the requested offset at that
                     *    offset within the buffer.  Return the number (n)
                     *    of bytes there are from the beginning of the
                     *    buffer up to the last byte of data.  If the
                     *    number of supplied bytes (= n - offset) is 
                     *    greater than zero and you didn't signal eof
                     *    and the reader is prepared to take more data
                     *    you will be called again with the requested
                     *    offset advanced by the number of bytes 
                     *    absorbed.  This interface is useful for files
                     *    no larger than the buffer.
                     * 1) Set *start = an unsigned long value less than
                     *    the buffer address but greater than zero.
                     *    Put the data of the requested offset at the
                     *    beginning of the buffer.  Return the number of
                     *    bytes of data placed there.  If this number is
                     *    greater than zero and you didn't signal eof
                     *    and the reader is prepared to take more data
                     *    you will be called again with the requested
                     *    offset advanced by *start.  This interface is
                     *    useful when you have a large file consisting
                     *    of a series of blocks which you want to count
                     *    and return as wholes.
                     *    (Hack by Paul.Russell@rustcorp.com.au)
                     * 2) Set *start = an address within the buffer.
                     *    Put the data of the requested offset at *start.
                     *    Return the number of bytes of data placed there.
                     *    If this number is greater than zero and you
                     *    didn't signal eof and the reader is prepared to
                     *    take more data you will be called again with the
                     *    requested offset advanced by the number of bytes
                     *    absorbed.
                     */
/*--------------------------------------------------------------*/
/*			/proc read functions			*/
/*--------------------------------------------------------------*/


/*--------------------------------------------------------------*/
/*			/proc/dvs/info 			*/
/*--------------------------------------------------------------*/
int info_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len=0;

  	len += sprintf(page+len, "nodeid=%d\n", atomic_read(&local_nodeid));
  	len += sprintf(page+len, "nr_dcs=%d\n", dvs.d_nr_dcs);
  	len += sprintf(page+len, "nr_nodes=%d\n", dvs.d_nr_nodes);
  	len += sprintf(page+len, "max_nr_procs=%d\n", dvs.d_nr_procs);
  	len += sprintf(page+len, "max_nr_tasks=%d\n", dvs.d_nr_tasks);
  	len += sprintf(page+len, "max_sys_procs=%d\n", dvs.d_nr_sysprocs);
  	len += sprintf(page+len, "max_copy_buf=%d\n", dvs.d_max_copybuf);
  	len += sprintf(page+len, "max_copy_len=%d\n", dvs.d_max_copylen);
  	len += sprintf(page+len, "dbglvl=%lX\n", dvs.d_dbglvl);
  	len += sprintf(page+len, "version=%d.%d\n", dvs.d_version, dvs.d_subver);
  	len += sprintf(page+len, "sizeof(proc)=%d\n", sizeof(struct proc));
  	len += sprintf(page+len, "sizeof(proc) aligned=%d\n", sizeof_proc_aligned);
  	len += sprintf(page+len, "sizeof(dc)=%d\n", sizeof(dc_desc_t));
  	len += sprintf(page+len, "sizeof(node)=%d\n", sizeof(cluster_node_t));

  return len;
}


/*--------------------------------------------------------------*/
/*			/proc/dvs/DCx/info 						*/
/*--------------------------------------------------------------*/
int dc_info_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len=0, masklen, dcid;
	int *ptr;
	dc_desc_t *dc_ptr;
	char bmbuf[BITMAP_32BITS+1];

	masklen = (BITS_TO_LONGS(NR_CPUS) * 10) + 1;

	ptr  = (int *) data;
	dcid = *ptr;
	MOLDEBUG(DBGPARAMS,"dcid=%d\n", dcid);
	dc_ptr = &dc[dcid];

	RLOCK_DC(dc_ptr);
	bm2ascii(bmbuf, dc_ptr->dc_usr.dc_nodes);      

	if(dc_ptr->dc_usr.dc_flags != DC_FREE) {
		len += sprintf(page, "dcid=%d\nflags=%X\nnr_procs=%d\nnr_tasks=%d\n"
		"nr_sysprocs=%d\nnr_nodes=%d\ndc_nodes=%lX\n"
		"warn2proc=%d\nwarnmsg=%d\ndc_name=%s\n",
			dc_ptr->dc_usr.dc_dcid,
			dc_ptr->dc_usr.dc_flags,
			dc_ptr->dc_usr.dc_nr_procs,
			dc_ptr->dc_usr.dc_nr_tasks,
			dc_ptr->dc_usr.dc_nr_sysprocs,
			dc_ptr->dc_usr.dc_nr_nodes,
			dc_ptr->dc_usr.dc_nodes,
			dc_ptr->dc_usr.dc_warn2proc,
			dc_ptr->dc_usr.dc_warnmsg,
			dc_ptr->dc_usr.dc_name);
		len += sprintf(page+len,"nodes 33222222222211111111110000000000\n"); 
		len += sprintf(page+len,"      10987654321098765432109876543210\n");
		len += sprintf(page+len,"      %s\n", bmbuf);
		len += sprintf(page+len,"cpumask=");
		len += cpumask_scnprintf(page+len, masklen, &dc_ptr->dc_usr.dc_cpumask);
		len += sprintf(page+len,"\n");
	}
	
	RUNLOCK_DC(dc_ptr);
  return len;
}

/*--------------------------------------------------------------*/
/*			/proc/dvs/DCx/procs			*/
/*--------------------------------------------------------------*/
int dc_procs_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len, dcid;
	int *ptr;
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;
	static int index, n;
	
	ptr  = (int *) data;
	dcid = *ptr;
MOLDEBUG(DBGPARAMS,"dcid=%d off=%ld count=%d index=%d eof=%d\n", dcid, off, count, index, *eof);
	dc_ptr = &dc[dcid];
#define LINELEN	80

	len = 0;
	RLOCK_DC(dc_ptr);
	if( dc_ptr->dc_usr.dc_flags != DC_FREE)  {
		if(index == 0) {
			len = sprintf(page, "DC p_nr -endp- -lpid- node flag misc -getf- -sndt- -wmig- -prxy- name\n");	
		}
		while(index < (dc_ptr->dc_usr.dc_nr_tasks + dc_ptr->dc_usr.dc_nr_procs)) {
			proc_ptr = DC_PROC(dc_ptr,index);
			RLOCK_PROC(proc_ptr);
			if (!test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) {
				n++;
				RUNLOCK_PROC(proc_ptr);
				break;
			}
			RUNLOCK_PROC(proc_ptr);
			index++;
		}
		
		if(index < (dc_ptr->dc_usr.dc_nr_tasks + dc_ptr->dc_usr.dc_nr_procs) ) {
			MOLDEBUG(INTERNAL,"index=%d\n", index);
			index++;
			len += sprintf(page+len, "%2d %4d %6d %6d %4d %4lX %4lX %6d %6d %6d %6d %-15.15s\n",
					proc_ptr->p_usr.p_dcid,
					proc_ptr->p_usr.p_nr,
					proc_ptr->p_usr.p_endpoint,
					proc_ptr->p_usr.p_lpid,
					proc_ptr->p_usr.p_nodeid,
					proc_ptr->p_usr.p_rts_flags,
					proc_ptr->p_usr.p_misc_flags,
					proc_ptr->p_usr.p_getfrom,
					proc_ptr->p_usr.p_sendto,
					proc_ptr->p_usr.p_waitmigr,
					proc_ptr->p_usr.p_proxy,
					proc_ptr->p_usr.p_name);
		} else {
			index=0;
			n=0;
		}
	}
	RUNLOCK_DC(dc_ptr);
	if(n==0) len = 0;
	*start=(char *)len;
	*eof = 1;
    return len;
}

/*--------------------------------------------------------------*/
/*			/proc/dvs/DCx/stats			*/
/*--------------------------------------------------------------*/
int dc_stats_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len, dcid;
	int *ptr;
	dc_desc_t *dc_ptr;
	struct proc *proc_ptr;
	static int index, n;
	
	ptr  = (int *) data;
	dcid = *ptr;
	MOLDEBUG(DBGPARAMS,"dcid=%d off=%ld count=%d index=%d eof=%d\n", dcid, off, count, index, *eof);
	dc_ptr = &dc[dcid];
#define LINELEN	80

	len = 0;
	RLOCK_DC(dc_ptr);
	if( dc_ptr->dc_usr.dc_flags != DC_FREE)  {
		if(index == 0) {
			len = sprintf(page, "DCID p_nr -endp- -lpid- node --lsnt-- --rsnt-- -lcopy-- -rcopy--\n");	
		}
		while(index < (dc_ptr->dc_usr.dc_nr_tasks + dc_ptr->dc_usr.dc_nr_procs)) {
			proc_ptr = DC_PROC(dc_ptr,index);
			RLOCK_PROC(proc_ptr);
			if (!test_bit(BIT_SLOT_FREE, &proc_ptr->p_usr.p_rts_flags)) {
				n++;
				RUNLOCK_PROC(proc_ptr);
				break;
			}
			RUNLOCK_PROC(proc_ptr);
			index++;
		}
		
		if(index < (dc_ptr->dc_usr.dc_nr_tasks + dc_ptr->dc_usr.dc_nr_procs) ) {
			MOLDEBUG(INTERNAL,"index=%d\n", index);
			index++;
			len += sprintf(page+len, "%4d %4d %6d %6d %4d %8ld %8ld %8ld %8ld\n",
					proc_ptr->p_usr.p_dcid,
					proc_ptr->p_usr.p_nr,
					proc_ptr->p_usr.p_endpoint,
					proc_ptr->p_usr.p_lpid,
					proc_ptr->p_usr.p_nodeid,
					proc_ptr->p_usr.p_lclsent,
					proc_ptr->p_usr.p_rmtsent,
					proc_ptr->p_usr.p_lclcopy,
					proc_ptr->p_usr.p_rmtcopy);
		} else {
			index=0;
			n=0;
		}
	}
	RUNLOCK_DC(dc_ptr);
	if(n==0) len = 0;
	*start=(char *)len;
	*eof = 1;
    return len;
}


/*--------------------------------------------------------------*/
/*			/proc/dvs/nodes 				*/
/*--------------------------------------------------------------*/
int nodes_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int i, len;
	cluster_node_t *n_ptr;
	char bmbuf[BITMAP_32BITS+1];
	
	len = sprintf(page, "ID Flags Proxies -pxsent- -pxrcvd- 10987654321098765432109876543210 Name\n");
	for (i = 0; i < dvs.d_nr_nodes; i++) {
		n_ptr = &node[i];
		RLOCK_NODE(n_ptr);
		if( n_ptr->n_usr.n_flags != NODE_FREE) {
			bm2ascii(bmbuf, n_ptr->n_usr.n_dcs);      
			len += sprintf(page+len, "%2d %5lX %7d %8ld %8ld %32s %-16.16s\n",
				n_ptr->n_usr.n_nodeid,
				n_ptr->n_usr.n_flags,
				n_ptr->n_usr.n_proxies,
				n_ptr->n_usr.n_pxsent,
				n_ptr->n_usr.n_pxrcvd,
				bmbuf,
				n_ptr->n_usr.n_name);
		}
		RUNLOCK_NODE(n_ptr);
	}

   return len;
}


int node_info_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len=0, nodeid;
	int *ptr;
	cluster_node_t *n_ptr;
	char bmbuf[BITMAP_32BITS+1];

	ptr  = (int *) data;
	nodeid = *ptr;
	MOLDEBUG(DBGPARAMS,"nodeid=%d\n", nodeid);
	
	len = sprintf(page, "Node Flags Proxies 10987654321098765432109876543210 Name\n");
	n_ptr = &node[nodeid];
	RLOCK_NODE(n_ptr);
	bm2ascii(bmbuf, n_ptr->n_usr.n_dcs);      
	len += sprintf(page+len, "%4d %5lX %7d %32s %-16.16s\n",
		n_ptr->n_usr.n_nodeid,
		n_ptr->n_usr.n_flags,
		n_ptr->n_usr.n_proxies,
		bmbuf,
		n_ptr->n_usr.n_name);
	RUNLOCK_NODE(n_ptr);

   return len;
}

int node_stats_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{

	int len=0, nodeid;
	cluster_node_t *n_ptr;
	int *ptr;

	ptr  = (int *) data;
	nodeid = *ptr;
	MOLDEBUG(DBGPARAMS,"nodeid=%d\n", nodeid);
	n_ptr = &node[nodeid];
	RLOCK_NODE(n_ptr);
	len = sprintf(page, "Node Flags\n");
	len += sprintf(page+len, "%-8.8s %lX \n", 
		n_ptr->n_usr.n_name, 
		n_ptr->n_usr.n_flags);
	RUNLOCK_NODE(n_ptr);

   return len;
}

/*--------------------------------------------------------------*/
/*			/proc/dvs/proxies/info			*/
/*--------------------------------------------------------------*/
int proxies_info_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int i, len;
	struct proc *proc_ptr;
	proxies_t *px_ptr;
	char bmbuf[BITMAP_32BITS+1];

	len = sprintf(page, "Proxies Flags Sender Receiver --Proxies_Name- 10987654321098765432109876543210 \n");
	for (i = 0; i < dvs.d_nr_nodes; i++) {
		px_ptr = &proxies[i];
		RLOCK_PROXY(px_ptr);
		if( px_ptr->px_usr.px_flags != PROXIES_FREE) {
			proc_ptr = &px_ptr->px_sproxy;
			bm2ascii(bmbuf, proc_ptr->p_usr.p_nodemap);     
			MOLDEBUG(INTERNAL,"flags=%lX pxnr=%d map=%lX\n",
				px_ptr->px_usr.px_flags, i, proc_ptr->p_usr.p_nodemap);
 			len += sprintf(page+len, "%7d %5lX %6d %8d %15s %s\n",
				px_ptr->px_usr.px_id,
				px_ptr->px_usr.px_flags,
				px_ptr->px_sproxy.p_usr.p_lpid,
				px_ptr->px_rproxy.p_usr.p_lpid,
				px_ptr->px_usr.px_name,
				bmbuf);
		}
		RUNLOCK_PROXY(px_ptr);
	}

   return len;
}

/*--------------------------------------------------------------*/
/*			/proc/dvs/proxies/procs			*/
/*--------------------------------------------------------------*/
int proxies_procs_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int i, len;
	proxies_t *px_ptr;
	struct proc *sproc_ptr,*rproc_ptr;

	len = 0;
	
	len = sprintf(page, "ID Type -lpid- -flag- -misc- -pxsent- -pxrcvd- -getf- -sendt -wmig- name\n");	
	for (i = 0; i < dvs.d_nr_nodes; i++) {
		px_ptr = &proxies[i];
		RLOCK_PROXY(px_ptr);
		if( px_ptr->px_usr.px_flags == PROXIES_FREE) {
			RUNLOCK_PROXY(px_ptr);
			continue;
		}

		sproc_ptr = &proxies[i].px_sproxy;
		RLOCK_PROC(sproc_ptr);
		len += sprintf(page+len, "%2d %6s %6d %6lX %6lX %8ld %8ld %6d %6d %6d %-15.15s\n",
					i,
					"send",
					sproc_ptr->p_usr.p_lpid,
					sproc_ptr->p_usr.p_rts_flags,
					sproc_ptr->p_usr.p_misc_flags,
					sproc_ptr->p_usr.p_rmtsent,		
					sproc_ptr->p_usr.p_lclsent,
					sproc_ptr->p_usr.p_getfrom,
					sproc_ptr->p_usr.p_sendto,
					sproc_ptr->p_usr.p_waitmigr,
					sproc_ptr->p_usr.p_name);
		RUNLOCK_PROC(sproc_ptr);

		rproc_ptr = &proxies[i].px_rproxy;
		RLOCK_PROC(rproc_ptr);
		len += sprintf(page+len, "%2d %6s %6d %6lX %6lX %8ld %8ld %6d %6d %6d %-15.15s\n",
					i,
					"recv",
					rproc_ptr->p_usr.p_lpid,
					rproc_ptr->p_usr.p_rts_flags,
					rproc_ptr->p_usr.p_misc_flags,
					rproc_ptr->p_usr.p_rmtsent,		
					rproc_ptr->p_usr.p_lclsent,
					rproc_ptr->p_usr.p_getfrom,
					rproc_ptr->p_usr.p_sendto,
					rproc_ptr->p_usr.p_waitmigr,
					rproc_ptr->p_usr.p_name);
		RUNLOCK_PROC(rproc_ptr);

		RUNLOCK_PROXY(px_ptr);
	}

    return len;
}



