/*
inet/qp.c

Query parameters

Created:	June 1995 by Philip Homburg <philip@f-mnx.phicoh.com>
*/

#include "inet.h"

#include "minix3/queryparam.h"
#include "generic/clock.h"
#include "generic/sr.h"

#include "sys/svrctl.h"

#include "mq.h"
#include "qp.h"
#include "sr_int.h"

int get_userdata ( int proc, vir_bytes vaddr, vir_bytes vlen,void *buffer);
int put_userdata ( int proc, vir_bytes vaddr, vir_bytes vlen,void *buffer);
int iqp_getc(void);
void iqp_putc(int c);

struct export_param_list inet_ex_list[]=
{
	QP_VARIABLE(sr_fd_table),
	QP_VARIABLE(ip_dev),
	QP_VARIABLE(tcp_fd_table),
	QP_VARIABLE(tcp_conn_table),
	QP_VARIABLE(tcp_cancel_f),
	QP_VECTOR(udp_port_table, udp_port_table, ip_conf_nr),
	QP_VARIABLE(udp_fd_table),
	QP_END()
};

struct export_params inet_ex_params= { inet_ex_list, NULL };

struct queryvars {
	int proc;
	struct svrqueryparam qpar;
	char parbuf[256], valbuf[256];
	char *param, *value;
	int r;
} *qvars;

void qp_init(void )
{
	SVRDEBUG("\n");

	qp_export(&inet_ex_params);
}

int qp_query(int proc, vir_bytes argp)
{
	/* Return values, sizes, or addresses of variables in MM space. */

	struct queryvars qv;
	void *addr;
	mnx_size_t n, size;
	int byte;
	int more;
	static char hex[]= "0123456789ABCDEF";

	qv.r= get_userdata(proc, argp, sizeof(qv.qpar), &qv.qpar);

	/* Export these to mq_getc() and mq_putc(). */
	qvars= &qv;
	qv.proc= proc;
	qv.param= qv.parbuf + sizeof(qv.parbuf);
	qv.value= qv.valbuf;

	do {
		more= queryparam(iqp_getc, &addr, &size);
		for (n= 0; n < size; n++) {
			byte= ((u8_t *) addr)[n];
			iqp_putc(hex[byte >> 4]);
			iqp_putc(hex[byte & 0x0F]);
		}
		iqp_putc(more ? ',' : 0);
	} while (more);
	return qv.r;
}


int iqp_getc(void )
{
	/* Return one character of the names to search for. */
	struct queryvars *qv= qvars;
	mnx_size_t n;

	if (qv->r != OK || qv->qpar.psize == 0) return 0;
	if (qv->param == qv->parbuf + sizeof(qv->parbuf)) {
		/* Need to fill the parameter buffer. */
		n= sizeof(qv->parbuf);
		if (qv->qpar.psize < n) n= qv->qpar.psize;
		qv->r= get_userdata(qv->proc, (vir_bytes) qv->qpar.param, n,
								qv->parbuf);
		if (qv->r != OK) return 0;
		qv->qpar.param+= n;
		qv->param= qv->parbuf;
	}
	qv->qpar.psize--;
	return (u8_t) *qv->param++;
}


void iqp_putc(int c)
{
	/* Send one character back to the user. */
	struct queryvars *qv= qvars;
	mnx_size_t n;

	if (qv->r != OK || qv->qpar.vsize == 0) return;
	*qv->value++= c;
	qv->qpar.vsize--;
	if (qv->value == qv->valbuf + sizeof(qv->valbuf)
					|| c == 0 || qv->qpar.vsize == 0) {
		/* Copy the value buffer to user space. */
		n= qv->value - qv->valbuf;
		qv->r= put_userdata(qv->proc, (vir_bytes) qv->qpar.value, n,
								qv->valbuf);
		qv->qpar.value+= n;
		qv->value= qv->valbuf;
	}
}

int get_userdata(int proc, vir_bytes vaddr,vir_bytes vlen,void *buffer)
{ 
	int rcode;
	SVRDEBUG("proc=%d, vlen=%d, vaddr=%X buffer=%X\n");
	rcode = sys_vircopy(proc,(void *)vaddr, SELF,(void *) buffer, vlen);
	return(rcode);
}


int put_userdata( int proc,vir_bytes vaddr,vir_bytes vlen,void *buffer)
{
	int rcode;
	SVRDEBUG("proc=%d, vlen=%d, vaddr=%X buffer=%X\n");
	rcode = sys_vircopy(SELF, (void *)buffer, proc, (void *)vaddr, vlen);
	return(rcode);
}



/*
 * $PchId: qp.c,v 1.7 2005/06/28 14:25:25 philip Exp $
 */
