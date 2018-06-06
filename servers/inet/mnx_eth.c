/*
inet/mnx_eth.c

Created:	Jan 2, 1992 by Philip Homburg

Copyright 1995 Philip Homburg
*/

#include "inet.h"

#ifdef ANULADO 
#include "proto.h"
#include "osdep_eth.h"
#include "generic/type.h"

#include "generic/mol_assert.h"
#include "generic/buf.h"
#include "generic/clock.h"
#include "generic/eth.h"
#include "generic/eth_int.h"
#include "generic/sr.h"
#endif /* ANULADO */ 


static int recv_debug= 0;

void setup_read(eth_port_t *eth_port) ;
void read_int(eth_port_t *eth_port, int count) ;
void write_int(eth_port_t *eth_port) ;
void eth_recvev(event_t *ev, ev_arg_t ev_arg) ;
void eth_mnx_sendev(event_t *ev, ev_arg_t ev_arg) ;
eth_port_t *find_port(message *m) ;
void eth_restart(eth_port_t *eth_port, int tasknr) ;

void osdep_eth_init(void)
{
	int i, r, tasknr, rport;
	struct eth_conf *ecp;
	eth_port_t *eth_port, *rep;
	message mess, *m_ptr;

	SVRDEBUG("\n");

	/* First initialize normal ethernet interfaces */
	for (i= 0, ecp= eth_conf, eth_port= eth_port_table;
		i<eth_conf_nr; i++, ecp++, eth_port++) {
		if (eth_is_vlan(ecp))
			continue;
		r = mol_pm_findproc(ecp->ec_task, &tasknr);
		if (r != OK) {
			/* Eventually, we expect ethernet drivers to be
			 * started after INET. So we always end up here. And
			 * the findproc can be removed.
			 */
			printf("eth%d: unable to find task %s: %d\n",
				i, ecp->ec_task, r);
			tasknr= ANY;
		}
		SVRDEBUG("ETH found\n");

 		eth_port->etp_osdep.etp_port= ecp->ec_port;
		eth_port->etp_osdep.etp_task= tasknr;
		ev_init(&eth_port->etp_osdep.etp_recvev);

		mess.m_type= DL_INIT;
		mess.DL_PORT= eth_port->etp_osdep.etp_port;
		mess.DL_PROC= this_proc;
		mess.DL_MODE= DL_NOMODE;

		if (tasknr == ANY)
			r= ENXIO;
		else {
			r= mnx_send(eth_port->etp_osdep.etp_task, &mess);
			if (r<0) {
				printf(
		"osdep_eth_init: unable to mnx_send to ethernet task, error= %d\n",
					r);
			}else{
				m_ptr = &mess;
				SVRDEBUG(MSG2_FORMAT,MSG2_FIELDS(m_ptr));
			}

		}

		if (r == OK){
			r= mnx_receive(eth_port->etp_osdep.etp_task, &mess);
			if (r<0) {
				printf(
	"osdep_eth_init: unable to mnx_receive from ethernet task, error= %d\n",
					r);
			}else{
				m_ptr = &mess;
				SVRDEBUG(MSG3_FORMAT,MSG3_FIELDS(m_ptr));
			}
		}

		if (r == OK) {
			r= mess.m3_i1;
			if (r == ENXIO) {
				printf(
		"osdep_eth_init: no ethernet device at task=%d,port=%d\n",
					eth_port->etp_osdep.etp_task, 
					eth_port->etp_osdep.etp_port);
			}else if (r < 0) {
				ip_panic((
				"osdep_eth_init: DL_INIT returned error %d\n",
					r));
			}else if (mess.m3_i1 != eth_port->etp_osdep.etp_port) {
				ip_panic((
	"osdep_eth_init: got reply for wrong port (got %d, expected %d)\n",
					mess.m3_i1,
					eth_port->etp_osdep.etp_port));
			}
		}
			
		sr_add_minor(if2minor(ecp->ec_ifno, ETH_DEV_MINOR),
			i, eth_open, eth_close, eth_read, 
			eth_write, eth_ioctl, eth_cancel, eth_select);

		eth_port->etp_flags |= EPF_ENABLED;
		eth_port->etp_vlan= 0;
		eth_port->etp_vlan_port= NULL;
		eth_port->etp_wr_pack= 0;
		eth_port->etp_rd_pack= 0;
		if (r == OK){
			eth_port->etp_ethaddr= *(mnx_ethaddr_t *)mess.m3_ca1;
			eth_port->etp_flags |= EPF_GOT_ADDR;
			setup_read (eth_port);
		}
	}

	/* And now come the VLANs */
	for (i= 0, ecp= eth_conf, eth_port= eth_port_table;
		i<eth_conf_nr; i++, ecp++, eth_port++) {
		if (!eth_is_vlan(ecp))
			continue;

 		eth_port->etp_osdep.etp_port= ecp->ec_port;
		eth_port->etp_osdep.etp_task= ANY;
		ev_init(&eth_port->etp_osdep.etp_recvev);

		rport= eth_port->etp_osdep.etp_port;
		mol_assert(rport >= 0 && rport < eth_conf_nr);
		rep= &eth_port_table[rport];
		if (!(rep->etp_flags & EPF_ENABLED)) {
			printf(
			"eth%d: underlying ethernet device %d not enabled",
				i, rport);
			continue;
		}
		if (rep->etp_vlan != 0) {
			printf(
			"eth%d: underlying ethernet device %d is a VLAN",
				i, rport);
			continue;
		}
		
		if (rep->etp_flags & EPF_GOT_ADDR) {
			eth_port->etp_ethaddr= rep->etp_ethaddr;
			eth_port->etp_flags |= EPF_GOT_ADDR;
		}

		sr_add_minor(if2minor(ecp->ec_ifno, ETH_DEV_MINOR),
			i, eth_open, eth_close, eth_read, 
			eth_write, eth_ioctl, eth_cancel, eth_select);

		eth_port->etp_flags |= EPF_ENABLED;
		eth_port->etp_vlan= ecp->ec_vlan;
		eth_port->etp_vlan_port= rep;
		mol_assert(eth_port->etp_vlan != 0);
		eth_port->etp_wr_pack= 0;
		eth_port->etp_rd_pack= 0;
		eth_reg_vlan(rep, eth_port);
	}
}

void eth_write_port(eth_port_t *eth_port, acc_t *pack)
{
	eth_port_t *loc_port;
	message mess1, block_msg;
	int i, pack_size;
	acc_t *pack_ptr;
	iovec_t *iovec;
	u8_t *eth_dst_ptr;
	int multicast, r;
	ev_arg_t ev_arg;
	message *m_ptr;

	SVRDEBUG("eth_port=%d\n", eth_port->etp_osdep.etp_port);

	mol_assert(!no_ethWritePort);
	mol_assert(!eth_port->etp_vlan);

	mol_assert(eth_port->etp_wr_pack == NULL);
	eth_port->etp_wr_pack= pack;

	iovec= eth_port->etp_osdep.etp_wr_iovec;
	pack_size= 0;
	for (i=0, pack_ptr= pack; i<IOVEC_NR && pack_ptr; i++,
		pack_ptr= pack_ptr->acc_next)	{
		iovec[i].iov_addr= (vir_bytes)ptr2acc_data(pack_ptr);
		pack_size += iovec[i].iov_size= pack_ptr->acc_length;
	}
	if (i>= IOVEC_NR)	{
		pack= bf_pack(pack);		/* packet is too fragmented */
		eth_port->etp_wr_pack= pack;
		pack_size= 0;
		for (i=0, pack_ptr= pack; i<IOVEC_NR && pack_ptr;
			i++, pack_ptr= pack_ptr->acc_next)	{
			iovec[i].iov_addr= (vir_bytes)ptr2acc_data(pack_ptr);
			pack_size += iovec[i].iov_size= pack_ptr->acc_length;
		}
	}
	mol_assert (i< IOVEC_NR);
	mol_assert (pack_size >= ETH_MIN_PACK_SIZE);

	if (i == 1) {
		/* simple packets can be sent using DL_WRITE instead of 
		 * DL_WRITEV.
		 */
		mess1.DL_COUNT= iovec[0].iov_size;
		mess1.DL_ADDR= (char *)iovec[0].iov_addr;
		mess1.m_type= DL_WRITE;
	}else{
		mess1.DL_COUNT= i;
		mess1.DL_ADDR= (char *)iovec;
		mess1.m_type= DL_WRITEV;
	}
	mess1.DL_PORT= eth_port->etp_osdep.etp_port;
	mess1.DL_PROC= this_proc;
	mess1.DL_MODE= DL_NOMODE;

	for (;;) {
		m_ptr = &mess1;
		SVRDEBUG("dst=%d " MSG2_FORMAT, eth_port->etp_osdep.etp_task, MSG2_FIELDS(m_ptr));
		r= mnx_sendrec(eth_port->etp_osdep.etp_task, &mess1);
		if (r != EMOLLOCKED)
			break;

		/* ethernet task is mnx_sending to this task, I hope */
		m_ptr = &block_msg;
		r= mnx_receive(eth_port->etp_osdep.etp_task, &block_msg);
		if (r < 0)
			ip_panic(("unable to mnx_receive"));
		SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));

		loc_port= eth_port;
		if (loc_port->etp_osdep.etp_port != block_msg.DL_PORT ||
			loc_port->etp_osdep.etp_task != block_msg.m_source)	{
			loc_port= find_port(&block_msg);
		}
		mol_assert(block_msg.DL_STAT & (DL_PACK_SEND|DL_PACK_RECV));
		if (block_msg.DL_STAT & DL_PACK_SEND) {
			mol_assert(loc_port != eth_port);
			loc_port->etp_osdep.etp_sendrepl= block_msg;
			ev_arg.ev_ptr= loc_port;
			ev_enqueue(&loc_port->etp_sendev, eth_mnx_sendev, ev_arg);
		}
		if (block_msg.DL_STAT & DL_PACK_RECV){
			if (recv_debug)	{
				printf(
			"eth_write_port(block_msg): eth%d got DL_PACK_RECV\n",
					loc_port-eth_port_table);
			}
			loc_port->etp_osdep.etp_recvrepl= block_msg;
			ev_arg.ev_ptr= loc_port;
			ev_enqueue(&loc_port->etp_osdep.etp_recvev,
				eth_recvev, ev_arg);
		}
	}

	if (r < 0){
		printf("eth_write_port: mnx_sendrec to %d failed: %d\n",
			eth_port->etp_osdep.etp_task, r);
		return;
	}

	SVRDEBUG("m_type=%X DL_PORT=%d etp_port=%d\n", 
			mess1.m_type, mess1.DL_PORT,eth_port->etp_osdep.etp_port);
	SVRDEBUG("DL_PROC=%d this_proc=%d DL_STAT=%X \n", 
			mess1.DL_PROC,this_proc, mess1.DL_STAT);
			
	mol_assert(mess1.m_type == DL_TASK_REPLY &&
		mess1.DL_PORT == eth_port->etp_osdep.etp_port &&
		mess1.DL_PROC == this_proc);
	mol_assert((mess1.DL_STAT >> 16) == OK);

	if (mess1.DL_STAT & DL_PACK_RECV) {
		SVRDEBUG("Packet received\n");
		if (recv_debug)	{
			printf(
			"eth_write_port(mess1): eth%d got DL_PACK_RECV\n",
				mess1.DL_PORT);
		}
		eth_port->etp_osdep.etp_recvrepl= mess1;
		ev_arg.ev_ptr= eth_port;
		ev_enqueue(&eth_port->etp_osdep.etp_recvev, eth_recvev,
			ev_arg);
	}
	if (!(mess1.DL_STAT & DL_PACK_SEND)){
		SVRDEBUG("Packet is not yet sent. \n");
		/* Packet is not yet sent. */
		return;
	}else{
		SVRDEBUG("Packet sent on eth_port=%d\n", eth_port->etp_osdep.etp_port);
	}

	/* If the port is in promiscuous mode or the packet is
	 * broad- or multicast, enqueue the reply packet.
	 */
	eth_dst_ptr= (u8_t *)ptr2acc_data(pack);
	multicast= (*eth_dst_ptr & 1);	/* low order bit indicates multicast */
	if (multicast || (eth_port->etp_osdep.etp_recvconf & NWEO_EN_PROMISC)){
		SVRDEBUG("Packet multicast\n");
		eth_port->etp_osdep.etp_sendrepl= mess1;
		ev_arg.ev_ptr= eth_port;
		ev_enqueue(&eth_port->etp_sendev, eth_mnx_sendev, ev_arg);
		/* Pretend that we didn't get a reply. */
		return;
	}

	/* packet was sent */
	SVRDEBUG("Free packet buffer\n");
	bf_afree(eth_port->etp_wr_pack);
	eth_port->etp_wr_pack= NULL;
}

void eth_rec(message *m)
{
	int i;
	eth_port_t *loc_port;
	int stat;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	mol_assert(m->m_type == DL_TASK_REPLY);

	set_time (m->DL_CLCK);

	for (i=0, loc_port= eth_port_table; i<eth_conf_nr; i++, loc_port++){
		if (loc_port->etp_osdep.etp_port == m->DL_PORT &&
			loc_port->etp_osdep.etp_task == m->m_source)
			break;
	}
	if (i == eth_conf_nr){
		ip_panic(("message from unknown source: %d:%d",
			m->m_source, m->DL_PORT));
	}

	stat= m->DL_STAT & 0xffff;

	mol_assert(stat & (DL_PACK_SEND|DL_PACK_RECV));
	if (stat & DL_PACK_SEND)
		write_int(loc_port);
	if (stat & DL_PACK_RECV){
		if (recv_debug)	{
			printf("eth_rec: eth%d got DL_PACK_RECV\n",
				m->DL_PORT);
		}
		read_int(loc_port, m->DL_COUNT);
	}
}

void eth_check_drivers(message *m)
{
	int i, r, tasknr;
	struct eth_conf *ecp;
	eth_port_t *eth_port;
	char *drivername;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	tasknr= m->m_source;
	printf("eth_check_drivers: got a notification from %d\n", tasknr);

	m->m_type= DL_GETNAME;
	SVRDEBUG(MSG1_FORMAT, MSG1_FIELDS(m));
	r= mnx_sendrec(tasknr, m);
	if (r != OK){
		printf("eth_check_drivers: mnx_sendrec to %d failed: %d\n",
			tasknr, r);
		return;
	}
	SVRDEBUG(MSG1_FORMAT, MSG1_FIELDS(m));

	if (m->m_type != DL_NAME_REPLY){
		printf(	"eth_check_drivers: got bad getname reply (%d) from %d\n",
			m->m_type, tasknr);
		return;
	}

	drivername= m->m3_ca1;
	printf("eth_check_drivers: got name: %s\n", drivername);

	/* Re-init ethernet interfaces */
	for (i= 0, ecp= eth_conf, eth_port= eth_port_table;
		i<eth_conf_nr; i++, ecp++, eth_port++){
		if (eth_is_vlan(ecp))
			continue;

		if (strcmp(ecp->ec_task, drivername) != 0)	{
			/* Wrong driver */
			continue;
		}

		eth_restart(eth_port, tasknr);
	}
}

int eth_get_stat(eth_port_t *eth_port, eth_stat_t *eth_stat)
{
	int r;
	message mess, mlocked;

	SVRDEBUG("port=%d task=%d\n", eth_port->etp_osdep.etp_port, eth_port->etp_osdep.etp_task);
		
	mol_assert(!eth_port->etp_vlan);

	mess.m_type= DL_GETSTAT;
	mess.DL_PORT= eth_port->etp_osdep.etp_port;
	mess.DL_PROC= this_proc;
	mess.DL_ADDR= (char *)eth_stat;

	for (;;) {
		r= mnx_sendrec(eth_port->etp_osdep.etp_task, &mess);
		if (r != EMOLLOCKED)
			break;

		r= mnx_receive(eth_port->etp_osdep.etp_task, &mlocked);
		mol_assert(r == OK);

		compare(mlocked.m_type, ==, DL_TASK_REPLY);
		eth_rec(&mlocked);
	}

	if (r != OK) {
		printf("eth_get_stat: mnx_sendrec to %d failed: %d\n",
			eth_port->etp_osdep.etp_task, r);
		return EIO;
	}

	mol_assert(mess.m_type == DL_TASK_REPLY);

	r= mess.DL_STAT >> 16;
	mol_assert (r == 0);

	if (mess.DL_STAT) {
		eth_rec(&mess);
	}
	return OK;
}

void eth_set_rec_conf (eth_port_t *eth_port, u32_t flags)
{
	int r;
	unsigned dl_flags;
	message mess, repl_mess;

	SVRDEBUG("port=%d flags=%X\n", eth_port->etp_osdep.etp_port, flags);

	mol_assert(!eth_port->etp_vlan);

	if (!(eth_port->etp_flags & EPF_GOT_ADDR)) {
		/* We have never seen the device. */
		printf("eth_set_rec_conf: waiting for device to appear\n");
		return;
	}

	eth_port->etp_osdep.etp_recvconf= flags;
	dl_flags= DL_NOMODE;
	if (flags & NWEO_EN_BROAD)
		dl_flags |= DL_BROAD_REQ;
	if (flags & NWEO_EN_MULTI)
		dl_flags |= DL_MULTI_REQ;
	if (flags & NWEO_EN_PROMISC)
		dl_flags |= DL_PROMISC_REQ;

	mess.m_type= DL_INIT;
	mess.DL_PORT= eth_port->etp_osdep.etp_port;
	mess.DL_PROC= this_proc;
	mess.DL_MODE= dl_flags;

	do {
		r= mnx_sendrec(eth_port->etp_osdep.etp_task, &mess);
		if (r == EMOLLOCKED) {
			/* etp_task is mnx_sending to this task,  I hope */
			if (mnx_receive (eth_port->etp_osdep.etp_task, 
				&repl_mess)< 0) {
				ip_panic(("unable to mnx_receive"));
			}

			compare(repl_mess.m_type, ==, DL_TASK_REPLY);
			eth_rec(&repl_mess);
		}
	} while (r == EMOLLOCKED);
	
	if (r < 0) {
		printf("eth_set_rec_conf: mnx_sendrec to %d failed: %d\n",
			eth_port->etp_osdep.etp_task, r);
		return;
	}

	mol_assert (mess.m_type == DL_INIT_REPLY);
	if (mess.m3_i1 != eth_port->etp_osdep.etp_port){
		ip_panic(("got reply for wrong port"));
	}
}

void write_int(eth_port_t *eth_port)
{
	acc_t *pack;
	int multicast;
	u8_t *eth_dst_ptr;

	SVRDEBUG("port=%d\n", eth_port->etp_osdep.etp_port);

	pack= eth_port->etp_wr_pack;
	eth_port->etp_wr_pack= NULL;

	eth_dst_ptr= (u8_t *)ptr2acc_data(pack);
	multicast= (*eth_dst_ptr & 1);	/* low order bit indicates multicast */
	if (multicast || (eth_port->etp_osdep.etp_recvconf & NWEO_EN_PROMISC)){
		mol_assert(!no_ethWritePort);
		no_ethWritePort= 1;
		eth_arrive(eth_port, pack, bf_bufsize(pack));
		mol_assert(no_ethWritePort);
		no_ethWritePort= 0;
	} else
		bf_afree(pack);

	eth_restart_write(eth_port);
}

void read_int(eth_port_t *eth_port, int count)
{
	acc_t *pack, *cut_pack;

	SVRDEBUG("port=%d count=%d\n", eth_port->etp_osdep.etp_port, count);

	pack= eth_port->etp_rd_pack;
	eth_port->etp_rd_pack= NULL;

	cut_pack= bf_cut(pack, 0, count);
	bf_afree(pack);

	mol_assert(!no_ethWritePort);
	no_ethWritePort= 1;
	eth_arrive(eth_port, cut_pack, count);
	mol_assert(no_ethWritePort);
	no_ethWritePort= 0;
	
	eth_port->etp_flags &= ~(EPF_READ_IP|EPF_READ_SP);
	setup_read(eth_port);
}

void setup_read( eth_port_t *eth_port)
{
	eth_port_t *loc_port;
	acc_t *pack, *pack_ptr;
	message mess1, block_msg, *m_ptr;
	iovec_t *iovec;
	ev_arg_t ev_arg;
	int i, r;

	SVRDEBUG(ETH_FORMAT, ETH_FIELDS(eth_port));
	
	mol_assert(!eth_port->etp_vlan);
	mol_assert(!(eth_port->etp_flags & (EPF_READ_IP|EPF_READ_SP)));

	do 	{
		mol_assert (!eth_port->etp_rd_pack);

		iovec= eth_port->etp_osdep.etp_rd_iovec;
		pack= bf_memreq (ETH_MAX_PACK_SIZE_TAGGED);

		for (i=0, pack_ptr= pack; i<RD_IOVEC && pack_ptr;
			i++, pack_ptr= pack_ptr->acc_next) {
			iovec[i].iov_addr= (vir_bytes)ptr2acc_data(pack_ptr);
			iovec[i].iov_size= (vir_bytes)pack_ptr->acc_length;
		}
		mol_assert (!pack_ptr);

		mess1.m_type= DL_READV;
		mess1.DL_PORT= eth_port->etp_osdep.etp_port;
		mess1.DL_PROC= this_proc;
		mess1.DL_COUNT= i;
		mess1.DL_ADDR= (char *)iovec;

		for (;;) {
			SVRDEBUG("eth%d: mnx_sending DL_READV\n",mess1.DL_PORT);
			m_ptr = &mess1;
			SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));
			r= mnx_sendrec(eth_port->etp_osdep.etp_task, &mess1);
			SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));
			if (r != EMOLLOCKED)
				break;

			/* ethernet task is mnx_sending to this task, I hope */
			m_ptr = &block_msg;
			r= mnx_receive(eth_port->etp_osdep.etp_task, &block_msg);
			if (r < 0)
				ip_panic(("unable to mnx_receive"));
			SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));

			loc_port= eth_port;
			if (loc_port->etp_osdep.etp_port != block_msg.DL_PORT ||
				loc_port->etp_osdep.etp_task !=
				block_msg.m_source) {
				loc_port= find_port(&block_msg);
			}
			mol_assert(block_msg.DL_STAT &
				(DL_PACK_SEND|DL_PACK_RECV));
			if (block_msg.DL_STAT & DL_PACK_SEND){
				loc_port->etp_osdep.etp_sendrepl= block_msg;
				ev_arg.ev_ptr= loc_port;
				ev_enqueue(&loc_port->etp_sendev, eth_mnx_sendev,
					ev_arg);
			}
			if (block_msg.DL_STAT & DL_PACK_RECV){
				if (recv_debug)	{
					printf(	"setup_read(block_msg): eth%d got DL_PACK_RECV\n",
						block_msg.DL_PORT);
				}
				mol_assert(loc_port != eth_port);
				loc_port->etp_osdep.etp_recvrepl= block_msg;
				ev_arg.ev_ptr= loc_port;
				ev_enqueue(&loc_port->etp_osdep.etp_recvev,
					eth_recvev, ev_arg);
			}
		}

		if (r < 0)	{
			printf("mnx_eth`setup_read: mnx_sendrec to %d failed: %d\n",
				eth_port->etp_osdep.etp_task, r);
			eth_port->etp_rd_pack= pack;
			eth_port->etp_flags |= EPF_READ_IP;
			continue;
		}
		
		SVRDEBUG("m_type=%X DL_PORT=%d etp_port=%d\n", 
			mess1.m_type, mess1.DL_PORT,eth_port->etp_osdep.etp_port);
		SVRDEBUG("DL_PROC=%d this_proc=%d DL_STAT=%X \n", 
			mess1.DL_PROC,this_proc, mess1.DL_STAT);

		mol_assert (mess1.m_type == DL_TASK_REPLY &&
			mess1.DL_PORT == mess1.DL_PORT &&
			mess1.DL_PROC == this_proc);
		compare((mess1.DL_STAT >> 16), ==, OK);

		if (mess1.DL_STAT & DL_PACK_RECV){
			if (recv_debug)	{
				printf("setup_read(mess1): eth%d: got DL_PACK_RECV\n",
					mess1.DL_PORT);
			}
			/* packet mnx_received */
			pack_ptr= bf_cut(pack, 0, mess1.DL_COUNT);
			bf_afree(pack);

			mol_assert(!no_ethWritePort);
			no_ethWritePort= 1;
			eth_arrive(eth_port, pack_ptr, mess1.DL_COUNT);
			mol_assert(no_ethWritePort);
			no_ethWritePort= 0;
		}else{
			/* no packet mnx_received */
			eth_port->etp_rd_pack= pack;
			eth_port->etp_flags |= EPF_READ_IP;
		}

		if (mess1.DL_STAT & DL_PACK_SEND){
			eth_port->etp_osdep.etp_sendrepl= mess1;
			ev_arg.ev_ptr= eth_port;
			ev_enqueue(&eth_port->etp_sendev, eth_mnx_sendev, ev_arg);
		}
	} while (!(eth_port->etp_flags & EPF_READ_IP));
	eth_port->etp_flags |= EPF_READ_SP;
}

void eth_recvev(event_t *ev, ev_arg_t ev_arg)
{
	eth_port_t *eth_port;
	message *m_ptr;

	SVRDEBUG("\n");

	eth_port= ev_arg.ev_ptr;
	mol_assert(ev == &eth_port->etp_osdep.etp_recvev);
	m_ptr= &eth_port->etp_osdep.etp_recvrepl;

	mol_assert(m_ptr->m_type == DL_TASK_REPLY);
	mol_assert(eth_port->etp_osdep.etp_port == m_ptr->DL_PORT &&
		eth_port->etp_osdep.etp_task == m_ptr->m_source);

	mol_assert(m_ptr->DL_STAT & DL_PACK_RECV);
	m_ptr->DL_STAT &= ~DL_PACK_RECV;

	if (recv_debug) {
		printf("eth_recvev: eth%d got DL_PACK_RECV\n", m_ptr->DL_PORT);
	}

	read_int(eth_port, m_ptr->DL_COUNT);
}

void eth_mnx_sendev(event_t *ev, ev_arg_t ev_arg)
{
	eth_port_t *eth_port;
	message *m_ptr;

	SVRDEBUG("\n");

	eth_port= ev_arg.ev_ptr;
	mol_assert(ev == &eth_port->etp_sendev);
	m_ptr= &eth_port->etp_osdep.etp_sendrepl;

	mol_assert (m_ptr->m_type == DL_TASK_REPLY);
	mol_assert(eth_port->etp_osdep.etp_port == m_ptr->DL_PORT &&
		eth_port->etp_osdep.etp_task == m_ptr->m_source);

	mol_assert(m_ptr->DL_STAT & DL_PACK_SEND);
	m_ptr->DL_STAT &= ~DL_PACK_SEND;

	/* packet is sent */
	write_int(eth_port);
}

eth_port_t *find_port(message *m)
{
	eth_port_t *loc_port;
	int i;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	for (i=0, loc_port= eth_port_table; i<eth_conf_nr; i++, loc_port++){
		if (loc_port->etp_osdep.etp_port == m->DL_PORT &&
			loc_port->etp_osdep.etp_task == m->m_source)
			break;
	}
	mol_assert (i<eth_conf_nr);
	return loc_port;
}

void eth_restart(eth_port_t *eth_port,int tasknr)
{
	int i, r;
	unsigned flags, dl_flags;
	message mess;
	eth_port_t *loc_port;

	SVRDEBUG("port=%d tasknr=%d\n", eth_port->etp_osdep.etp_port, tasknr);

	printf("eth_restart: restarting eth%d, task %d, port %d\n",
		eth_port-eth_port_table, tasknr,
		eth_port->etp_osdep.etp_port);

	eth_port->etp_osdep.etp_task= tasknr;

	flags= eth_port->etp_osdep.etp_recvconf;
	dl_flags= DL_NOMODE;
	if (flags & NWEO_EN_BROAD)
		dl_flags |= DL_BROAD_REQ;
	if (flags & NWEO_EN_MULTI)
		dl_flags |= DL_MULTI_REQ;
	if (flags & NWEO_EN_PROMISC)
		dl_flags |= DL_PROMISC_REQ;
	mess.m_type= DL_INIT;
	mess.DL_PORT= eth_port->etp_osdep.etp_port;
	mess.DL_PROC= this_proc;
	mess.DL_MODE= dl_flags;

	r= mnx_sendrec(eth_port->etp_osdep.etp_task, &mess);
	/* YYY */
	if (r<0) {
		printf("eth_restart: mnx_sendrec to ethernet task %d failed: %d\n",
			eth_port->etp_osdep.etp_task, r);
		return;
	}

	if (mess.m3_i1 == ENXIO) {
		printf("osdep_eth_init: no ethernet device at task=%d,port=%d\n",
			eth_port->etp_osdep.etp_task, 
			eth_port->etp_osdep.etp_port);
		return;
	}
	if (mess.m3_i1 < 0)
		ip_panic(("osdep_eth_init: DL_INIT returned error %d\n",
			mess.m3_i1));
		
	if (mess.m3_i1 != eth_port->etp_osdep.etp_port){
		ip_panic((
			"osdep_eth_init: got reply for wrong port (got %d, expected %d)\n",
			mess.m3_i1, eth_port->etp_osdep.etp_port));
	}

	eth_port->etp_flags |= EPF_ENABLED;

	eth_port->etp_ethaddr= *(mnx_ethaddr_t *)mess.m3_ca1;
	if (!(eth_port->etp_flags & EPF_GOT_ADDR)){
		eth_port->etp_flags |= EPF_GOT_ADDR;
		eth_restart_ioctl(eth_port);

		/* Also update any VLANs on this device */
		for (i=0, loc_port= eth_port_table; i<eth_conf_nr;
			i++, loc_port++){
			if (!(loc_port->etp_flags & EPF_ENABLED))
				continue;
			if (loc_port->etp_vlan_port != eth_port)
				continue;
			 
			loc_port->etp_ethaddr= eth_port->etp_ethaddr;
			loc_port->etp_flags |= EPF_GOT_ADDR;
			eth_restart_ioctl(loc_port);
		}
	}

	if (eth_port->etp_wr_pack){
		bf_afree(eth_port->etp_wr_pack);
		eth_port->etp_wr_pack= NULL;
		eth_restart_write(eth_port);
	}
	if (eth_port->etp_rd_pack) {
		bf_afree(eth_port->etp_rd_pack);
		eth_port->etp_rd_pack= NULL;
		eth_port->etp_flags &= ~(EPF_READ_IP|EPF_READ_SP);
	}
	setup_read (eth_port);
}

/*
 * $PchId: mnx_eth.c,v 1.16 2005/06/28 14:24:37 philip Exp $
 */
