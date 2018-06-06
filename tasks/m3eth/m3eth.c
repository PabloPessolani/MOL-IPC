/*
 * mhved.c
 *
 * This file contains a ethernet device driver for MHYPER Virtual Ethernet cards.
 *
 * The valid messages and their parameters are:
 *
 *   m_type       DL_PORT    DL_PROC   DL_COUNT   DL_MODE   DL_ADDR
 * |------------+----------+---------+----------+---------+---------|
 * | HARDINT    |          |         |          |         |         |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_WRITE   | port nr  | proc nr | count    | mode    | address |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_WRITEV  | port nr  | proc nr | count    | mode    | address |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_READ    | port nr  | proc nr | count    |         | address |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_READV   | port nr  | proc nr | count    |         | address |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_INIT    | port nr  | proc nr | mode     |         | address |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_GETSTAT | port nr  | proc nr |          |         | address |
 * |------------|----------|---------|----------|---------|---------|
 * | DL_STOP    | port_nr  |         |          |         |         |
 * |------------|----------|---------|----------|---------|---------|
 *
 *
 * Adapted for MHYPER: Sep 22, 2014 by ppessolani@hotmail.com
 */
 

/*
*===========================================================================*
 *                  	   Messages for networking layer	COM.H				     *
 *===========================================================================*

 Message types for network layer requests. This layer acts like a driver
#define NW_OPEN		DEV_OPEN
#define NW_CLOSE	DEV_CLOSE
#define NW_READ		DEV_READ
#define NW_WRITE	DEV_WRITE
#define NW_IOCTL	DEV_IOCTL
#define NW_CANCEL	CANCEL

Base type for data link layer requests and responses. 
#define DL_RQ_BASE	0x800		
#define DL_RS_BASE	0x900		

Message types for data link layer requests. 
#define DL_WRITE	(DL_RQ_BASE + 3)
#define DL_WRITEV	(DL_RQ_BASE + 4)
#define DL_READ		(DL_RQ_BASE + 5)
#define DL_READV	(DL_RQ_BASE + 6)
#define DL_INIT		(DL_RQ_BASE + 7)
#define DL_STOP		(DL_RQ_BASE + 8)
#define DL_GETSTAT	(DL_RQ_BASE + 9)
#define DL_GETNAME	(DL_RQ_BASE +10)

 Message type for data link layer replies. 
#define DL_INIT_REPLY	(DL_RS_BASE + 20)
#define DL_TASK_REPLY	(DL_RS_BASE + 21)
#define DL_NAME_REPLY	(DL_RS_BASE + 22)

Field names for data link layer messages.
#define DL_PORT		m2_i1
#define DL_PROC		m2_i2	
#define DL_COUNT		m2_i3
#define DL_MODE		m2_l1
#define DL_CLCK		m2_l2
#define DL_ADDR		m2_p1
#define DL_STAT		m2_l1
#define DL_NAME		m3_ca1

Bits in 'DL_STAT' field of DL replies. 
#  define DL_PACK_SEND		0x01
#  define DL_PACK_RECV		0x02
#  define DL_READ_IP		0x04

Bits in 'DL_MODE' field of DL requests. 
#  define DL_NOMODE		0x0
#  define DL_PROMISC_REQ	0x2
#  define DL_MULTI_REQ		0x4
#  define DL_BROAD_REQ		0x8
*/ 


#define _GNU_SOURCE     
#define _MULTI_THREADED
#define  MOL_USERSPACE	1
//#define TASKDBG			1
#define _TABLE

// before eth.h 
// #define M3IPC_LOCK	1  
#include "m3eth.h"

#define TRUE 1

#define WAIT4BIND_MS 1000
// bits for iface_rqst = ( iface_nr << 4) | iface_op
#define ETH_IFACE_NONE 		0x0000
#define ETH_IFACE_READ		0x0001
#define ETH_IFACE_WRITE		0x0002

// bits for rqst_source
#define	VOID_REQUEST		0x0000	
#define	TAP_REQUEST			0x0001			
#define INET_REQUEST 		0x0002

char rbuf[ETH_FRAME_LEN];
pthread_t eth_rx_th[EC_PORT_NR_MAX]; 	
pthread_t eth_tx_th[EC_PORT_NR_MAX];
pthread_t inet2eth_th;
	
void reply(eth_card_t *ec_ptr, int err, int may_block);

// Define a struct for ARP header
struct _arp_hdr {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
};
typedef struct _arp_hdr _arp_hdr_t;
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 1    // Taken from <linux/if_arp.h>
#define ARP_FORMAT "ARP_HDR: htype=%X ptype=%X hlen=%d plen=%d opcode=%d\n"
#define ARP_FIELDS(p) 	p->htype,p->ptype, p->hlen, p->plen, p->opcode

typedef struct _ip_hdr
{
	u8_t vers_ihl,
		tos;
	u16_t length,
		id,
		flags_fragoff;
	u8_t ttl,
		proto;
	u16_t hdr_chk;
	ipaddr_t src, dst;
} _ip_hdr_t;
#define IP4_HDRLEN 20      // IPv4 header length
#define IPHDR_FORMAT 		"IP_HDR: vers=%X tos=%X len=%X id=%X flag=%X ttl=%X proto=%X chk=%X src=%lX dst=%lX\n"
#define IPHDR_FIELDS(p) 	p->vers_ihl, p->tos, p->length, p->id, \
							p->flags_fragoff, p->ttl, p->proto, p->hdr_chk, \
							p->src, p->dst

typedef struct _eth_hdr
{
	ether_addr_t dst;
	ether_addr_t src;
	ether_type_t proto;
} _eth_hdr_t;
#define ETH_HDRLEN 14      // Ethernet header length
#define ETHHDR_FORMAT	"ETH_HDR: dst=%0X:%0X:%0X:%0X:%0X:%0X src=%0X:%0X:%0X:%0X:%0X:%0X proto=%X\n" 
#define ETHHDR_FIELDS(p) p->dst.ea_addr[0],p->dst.ea_addr[1], p->dst.ea_addr[2], \
				p->dst.ea_addr[3], p->dst.ea_addr[4], p->dst.ea_addr[5], \
				p->src.ea_addr[0], p->src.ea_addr[1], p->src.ea_addr[2], \
				p->src.ea_addr[3], p->src.ea_addr[4], p->src.ea_addr[5], p->proto

				
typedef struct _icmp_hdr
{ 
	u8_t ih_type, ih_code;
	u16_t ih_chksum;
#ifdef FIELD_NOT_USED	
	union
	{
		u32_t ihh_unused;
		icmp_id_seq_t ihh_idseq;
		ipaddr_t ihh_gateway;
		icmp_ram_t ihh_ram;
		icmp_pp_t ihh_pp;
		icmp_mtu_t ihh_mtu;
	} ih_hun;
	union
	{
		icmp_ip_id_t ihd_ipid;
		u8_t uhd_data[1];
	} ih_dun;
#endif // FIELD_NOT_USED	
} _icmp_hdr_t;
#define ICMP_FORMAT		"ICMP_HDR: type=%X code=%X check=%X \n"
#define ICMP_FIELDS(p)	p->ih_type, p->ih_code, p->ih_chksum


#define MTX_LOCK(x) do{ \
		TASKDEBUG("MTX_LOCK %s \n", #x);\
		pthread_mutex_lock(&x);\
		}while(0)
			
#define MTX_UNLOCK(x) do{ \
		pthread_mutex_unlock(&x);\
		TASKDEBUG("MTX_UNLOCK %s \n", #x);\
		}while(0)	
			
#define COND_WAIT(x,y) do{ \
		TASKDEBUG("COND_WAIT %s %s\n", #x,#y );\
		pthread_cond_wait(&x, &y);\
		}while(0)	
 
#define COND_SIGNAL(x) do{ \
		pthread_cond_signal(&x);\
		TASKDEBUG("COND_SIGNAL %s\n", #x);\
		}while(0)	

#define IFCONFIG_BIN 	"/sbin/ifconfig "
#define IFCONFIG_ARGS 	"%s netmask %s"
#define IFTAP 			"tap0 "	

static long M3IPC_hdw_notify(int dcid, int endpoint)
{
	int ret;
#ifdef M3IPC_LOCK
	MTX_LOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	TASKDEBUG("dcid=%d endpoint=%d\n", dcid, endpoint);
	ret = mnx_hdw_notify(dcid, endpoint);
#ifdef M3IPC_LOCK
	MTX_UNLOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	return(ret);
}

static long M3IPC_notify(int endpoint)
{
	int ret;
#ifdef M3IPC_LOCK
	MTX_LOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	TASKDEBUG("endpoint=%d\n", endpoint);
	ret = mnx_notify(endpoint);
#ifdef M3IPC_LOCK
	MTX_UNLOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	return(ret);
}

static long M3IPC_send(int dst_ep, message *m_ptr)
{
	int ret;
#ifdef M3IPC_LOCK
	MTX_LOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	TASKDEBUG("dst_ep=%d\n", dst_ep);
	ret = mnx_send(dst_ep, m_ptr);	
#ifdef M3IPC_LOCK
	MTX_UNLOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	return(ret);
}

static long M3IPC_receive_T(int src_ep, message *m_ptr, long timeout)
{
	int ret;
#ifdef M3IPC_LOCK
	MTX_LOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	TASKDEBUG("src_ep=%d\n", src_ep);
	ret = mnx_receive_T(src_ep, m_ptr, timeout);
#ifdef M3IPC_LOCK
	MTX_UNLOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	return(ret);
}

static long M3IPC_vcopy(int src_ep, void *src_addr, int dst_ep, void *dst_addr, long bytes)
{
	int ret;
#ifdef M3IPC_LOCK
	MTX_LOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	TASKDEBUG("src_ep=%d dst_ep=%d bytes=%ld\n",src_ep, dst_ep, bytes);
	ret = mnx_vcopy(src_ep, src_addr, dst_ep, dst_addr, bytes);
#ifdef M3IPC_LOCK
	MTX_UNLOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	return(ret);
}

static long M3IPC_sendrec(int srcdst_ep, message *m_ptr)
{
	int ret;
#ifdef M3IPC_LOCK
	MTX_LOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	TASKDEBUG("srcdst_ep=%d\n", srcdst_ep);
	ret = mnx_sendrec(srcdst_ep, m_ptr);	
#ifdef M3IPC_LOCK
	MTX_UNLOCK(m3ipc_mutex);
#endif // M3IPC_LOCK
	return(ret);
}
	
	
/*===========================================================================*
 *                              get_userdata                                 *
 *===========================================================================*/
static void get_userdata(int user_proc, vir_bytes user_addr, 
	vir_bytes count, void *loc_addr)
{
	int cps;
	TASKDEBUG("user_proc=%d count=%ld\n", user_proc, (long)count);
	cps = M3IPC_vcopy(user_proc, user_addr, SELF, (vir_bytes) loc_addr, count);
	if (cps != OK) printf("lance: warning, scopy failed: %d\n", cps);
}

/*===========================================================================*
 *                           ec_next_iovec                                   *
 *===========================================================================*/
static void ec_next_iovec(iovec_dat_t *iov_addr)
{
	TASKDEBUG("\n");

	iov_addr->iod_iovec_s -= IOVEC_NR;
	iov_addr->iod_iovec_addr += IOVEC_NR * sizeof(iovec_t);

	get_userdata(iov_addr->iod_proc_nr, iov_addr->iod_iovec_addr, 
				 (iov_addr->iod_iovec_s > IOVEC_NR ? 
				  IOVEC_NR : iov_addr->iod_iovec_s) * sizeof(iovec_t), 
				 iov_addr->iod_iovec); 
}

/*===========================================================================*
 *                              calc_iovec_size                              *
 *===========================================================================*/
static int calc_iovec_size(iovec_dat_t *iov_addr)
{
	int size,i;

	TASKDEBUG("\n");

	size = i = 0;      
	while (i < iov_addr->iod_iovec_s)  {
		if (i >= IOVEC_NR) {
			ec_next_iovec(iov_addr);
			i= 0;
			continue;
        }
		size += iov_addr->iod_iovec[i].iov_size;
		i++;
    }
	
	TASKDEBUG("size=%d\n", size);
	return size;
}

/*===========================================================================*
 *                              ec_nic2user                                  *
 *===========================================================================*/
static void ec_nic2user(eth_card_t *ec_ptr, int nic_addr, 
		iovec_dat_t *iov_addr, vir_bytes offset,vir_bytes count)
{
	/*phys_bytes phys_hw, phys_user;*/
	int bytes, i, r;

	TASKDEBUG("ec_port=%d nic_addr=%X  offset=%X count=%ld\n", 
		ec_ptr->ec_port, nic_addr, offset, (long)count);
		
	/*phys_hw = vir2phys(nic_addr);*/

	i= 0;
	while (count > 0){
		if (i >= IOVEC_NR){
			ec_next_iovec(iov_addr);
			i= 0;
			continue;
		}
		if (offset >= iov_addr->iod_iovec[i].iov_size)	{
			offset -= iov_addr->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iov_addr->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;
		/*
		  phys_user = numap_local(iov_addr->iod_proc_nr,
							iov_addr->iod_iovec[i].iov_addr + offset, bytes);

		  phys_copy(phys_hw, phys_user, (phys_bytes) bytes);
		  */
		if ( (r=M3IPC_vcopy( SELF, nic_addr, iov_addr->iod_proc_nr, 
				iov_addr->iod_iovec[i].iov_addr + offset, bytes )) != OK )
			ERROR_EXIT(r);
		  
		count -= bytes;
		nic_addr += bytes;
		offset += bytes;
	}
}

/*===========================================================================*
 *                              ec_recv                                      *
 *===========================================================================*/
static void ec_recv(eth_card_t *ec_ptr)
{
	vir_bytes length;
	int packet_processed;
	int i;
	unsigned short port;
		
	port = ec_ptr->ec_port;

	TASKDEBUG("port=%d flags=%X\n", port, ec_ptr->flags);

	if ((ec_ptr->flags & ECF_READING)==0){
		ERROR_PRINT(NO_NUM);
		return;
	}
	if ((ec_ptr->flags & ECF_ENABLED)==0){
		ERROR_PRINT(NO_NUM);
		return;
	}
	
	/* we check all the received slots until find a properly received packet */
	packet_processed = FALSE;
	for( i = 0; i < RX_RING_SIZE; i++) {
		if( packet_processed == TRUE)  return;
		ec_ptr->eth_stat.ets_packetR++;
		length = ei_ptr->rx_ring[rx_slot_nr].msg_length;
		if (length > 0){
			TASKDEBUG("i=%d rx_slot_nr=%d length=%d\n", i, rx_slot_nr, length);
//			ec_nic2user(ec_ptr, (int)(ei_ptr->rbuf[rx_slot_nr]),
//				  &ec_ptr->read_iovec, 0, length);
		  
			ec_ptr->read_s = length;
			ec_ptr->flags |= ECF_PACK_RECV;
//			ec_ptr->flags &= ~ECF_READING;
			packet_processed = TRUE;
		}
		/* set up this slot again, and we move to the next slot */
		ei_ptr->rx_ring[rx_slot_nr].buf_length = -ETH_FRAME_LEN;
		ei_ptr->rx_ring[rx_slot_nr].u.addr[3] |= 0x80;
		rx_slot_nr = (++rx_slot_nr) & RX_RING_MOD_MASK;
    }
	return;
}

/*===========================================================================*
 *                              ec_user2nic                                  *
 *===========================================================================*/
static void ec_user2nic(eth_card_t *ec_ptr, iovec_dat_t *iov_addr, 
	vir_bytes offset, int nic_addr, vir_bytes count)
{
	/*phys_bytes phys_hw, phys_user;*/
	int bytes, i, rcode;

	TASKDEBUG("ec_port=%d offset=%X nic_addr=%X count=%ld\n", 
		ec_ptr->ec_port, offset, nic_addr, (long)count);
				
	/* phys_hw = vir2phys(nic_addr);	 */
	i = 0;
	while (count > 0)  {
		if (i >= IOVEC_NR) {
			ec_next_iovec(iov_addr);
			i= 0;
			continue;
        }
		if (offset >= iov_addr->iod_iovec[i].iov_size){
			offset -= iov_addr->iod_iovec[i].iov_size;
			i++;
			continue;
        }
		bytes = iov_addr->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;
		if ( (rcode = M3IPC_vcopy(iov_addr->iod_proc_nr, iov_addr->iod_iovec[i].iov_addr + offset,
				SELF, nic_addr, count )) != OK )
		if(rcode){
			TASKDEBUG("M3IPC_vcopy rcode=%d\n", rcode);
			ERROR_EXIT(rcode);
		}      	
		count -= bytes;
		nic_addr += bytes;
		offset += bytes;
    }
}


/*===========================================================================*
 *                            do_vwrite                                     *
 *===========================================================================*/
static void do_vwrite(message *mp, int from_int, int vectored)
{
	int port, count;
	eth_card_t *ec_ptr;
	int rcode;

/*   m_type       DL_PORT    DL_PROC   DL_COUNT   DL_MODE   DL_ADDR
 * |------------+----------+---------+----------+---------+---------|
 * | DL_WRITE   | port nr  | proc nr | count    | mode    | address |
 * |------------|----------|---------|----------|---------|---------|
*/

	TASKDEBUG("from_int=%X vectored=%X tx_slot_nr=%d \n", 
		from_int, vectored, tx_slot_nr);
	TASKDEBUG( MSG2_FORMAT, MSG2_FIELDS(mp));

	ec_ptr = &ec_table[mp->DL_PORT];
	if(mp->DL_PROC != ec_ptr->ec_owner){
		mo_ptr->m_type = EMOLBADOWNER;
		mo_ptr->m_type= DL_TASK_REPLY;
		mess_reply(mp, mo_ptr);
		return;
	}

	port = mp->DL_PORT;
	count = mp->DL_COUNT;
	ec_ptr->client= mp->DL_PROC;

	if (isstored[tx_slot_nr]==1){
		/* all slots are used, so this message is buffered */
		ec_ptr->sendmsg= *mp;
		ec_ptr->flags |= ECF_SEND_AVAIL;
		reply(ec_ptr, OK, FALSE);
		return;
	}

	/* convert the message to write_iovec */
	if (vectored)  {
		get_userdata(mp->DL_PROC, (vir_bytes) mp->DL_ADDR,
                   (count > IOVEC_NR ? IOVEC_NR : count) *
                   sizeof(iovec_t), ec_ptr->write_iovec.iod_iovec);

		ec_ptr->write_iovec.iod_iovec_s    = count;
		ec_ptr->write_iovec.iod_proc_nr    = mp->DL_PROC;
		ec_ptr->write_iovec.iod_iovec_addr = (vir_bytes) mp->DL_ADDR;

		ec_ptr->tmp_iovec = ec_ptr->write_iovec;
		ec_ptr->write_s = calc_iovec_size(&ec_ptr->tmp_iovec);
    } else   {  
		ec_ptr->write_iovec.iod_iovec[0].iov_addr = (vir_bytes) mp->DL_ADDR;
		ec_ptr->write_iovec.iod_iovec[0].iov_size = mp->DL_COUNT;

		ec_ptr->write_iovec.iod_iovec_s    = 1;
		ec_ptr->write_iovec.iod_proc_nr    = mp->DL_PROC;
		ec_ptr->write_iovec.iod_iovec_addr = 0;

		ec_ptr->write_s = mp->DL_COUNT;
    }
  
	ec_user2nic(ec_ptr, &ec_ptr->write_iovec, 0,
              (int)(ei_ptr->tbuf[tx_slot_nr]), ec_ptr->write_s);
			  
	/* set-up for transmitting, and transmit it if needed. */
	ei_ptr->tx_ring[tx_slot_nr].buf_length = -ec_ptr->write_s;
	ei_ptr->tx_ring[tx_slot_nr].misc = 0x0;
//	ei_ptr->tx_ring[tx_slot_nr].u.base 
//		= virt_to_bus(ei_ptr->tbuf[tx_slot_nr]) & 0xffffff;
		
	isstored[tx_slot_nr]=1;
	tx_slot_nr = (++tx_slot_nr) & TX_RING_MOD_MASK;
	ec_ptr->flags |= ECF_PACK_SEND;

	// to inform the sending thread
	MTX_LOCK(main_mutex);
	COND_SIGNAL(send_barrier[ec_ptr->ec_port]);
	COND_WAIT(smain_barrier[ec_ptr->ec_port],main_mutex);
	// HERE REPORT TO INET 
    reply(ec_ptr, OK, TRUE);
	MTX_UNLOCK(main_mutex);

	/* reply by calling do_int() if this function is called from interrupt. */
//	if (from_int) return;
//	reply(ec_ptr, OK, FALSE);
 
}

/*===========================================================================*
 *                              ec_send                                      *
 *===========================================================================*/
static void ec_send(eth_card_t *ec_ptr)
{
	TASKDEBUG("\n");

	/* from ec_check_ints() or ec_reset(). */
	/* this function proccesses the buffered message. (slot/transmit) */
	if (!(ec_ptr->flags & ECF_SEND_AVAIL))
	  return;

	ec_ptr->flags &= ~ECF_SEND_AVAIL;
	switch(ec_ptr->sendmsg.m_type){
	  case DL_WRITE:  do_vwrite(&ec_ptr->sendmsg, TRUE, FALSE);       break;
	  case DL_WRITEV: do_vwrite(&ec_ptr->sendmsg, TRUE, TRUE);        break;
	  default:
		SYSERR(ec_ptr->sendmsg.m_type);
		ERROR_EXIT(EMOLINVAL);
		break;
	}
}

/*===========================================================================*
 *                              ec_reset                                     *
 *===========================================================================*/
static void ec_reset(eth_card_t *ec_ptr)
{
	/* Stop/start the chip, and clear all RX,TX-slots */
	unsigned short ioaddr = ec_ptr->ec_port;
	int i;

	TASKDEBUG("\n");

//	out_word(ioaddr+LANCE_ADDR, 0x0);
//	(void)in_word(ioaddr+LANCE_ADDR);
//	out_word(ioaddr+LANCE_DATA, 0x4);                      /* stop */
//	out_word(ioaddr+LANCE_DATA, 0x2);                      /* start */
	
	/* purge Tx-ring */
	tx_slot_nr = cur_tx_slot_nr = 0;
	for (i=0; i<TX_RING_SIZE; i++) {
		ei_ptr->tx_ring[i].u.base = 0;
		isstored[i]=0;
	}

	/* re-init Rx-ring */
	rx_slot_nr = 0;
	for (i=0; i<RX_RING_SIZE; i++) {
	  ei_ptr->rx_ring[i].buf_length = -ETH_FRAME_LEN;
	  ei_ptr->rx_ring[i].u.addr[3] |= 0x80;
	}

	/* store a buffered message on the slot if exists */
	ec_send(ec_ptr);
	ec_ptr->flags &= ~ECF_STOPPED;
}

/*===========================================================================*
 *                              do_int                                       *
 *===========================================================================*/
void do_int(eth_card_t *ec_ptr)
{
	TASKDEBUG("if_name=%s flags=0x%8lX\n", ec_ptr->if_name, ec_ptr->flags);
	if (ec_ptr->flags & (ECF_PACK_SEND | ECF_PACK_RECV))
		reply(ec_ptr, OK, TRUE);
}

/*===========================================================================*
 *                            low_level_probe                                     *
 *===========================================================================*/
int low_level_probe(eth_card_t *ec_ptr)
{
	int len;
	int rcode;
	int s, i;
	struct ifreq ifr;
	static char mac_string[3*ETH_ALEN+1];

	TASKDEBUG("%s\n", ec_ptr->if_name);

	len = strlen(ec_ptr->if_name);
	if (len > (IFNAMSIZ-1)) {
		ERROR_RETURN(EMOLNAMETOOLONG);
	}
	s = socket(AF_INET,SOCK_DGRAM,0);
	if (s == -1) {
		ERROR_RETURN(errno);
	}
	memset(&ifr,0,sizeof(ifr));
	strncpy(ifr.ifr_name,ec_ptr->if_name,len);
	if (ioctl(s,SIOCGIFHWADDR,&ifr) == -1) {
		ERROR_PRINT(EMOLBADIOCTL);
		goto err;
	}
	u8_t* hwaddr = (u8_t*)&ifr.ifr_hwaddr.sa_data;
	for(i = 0; i < ETH_ALEN; i++){
		ec_ptr->mac_address.ea_addr[i] = hwaddr[i];
		sprintf(&mac_string[i*3], "%02X:", ec_ptr->mac_address.ea_addr[i]);
	}
	mac_string[ETH_ALEN*3] = 0;
	TASKDEBUG("%s MACADDR %s\n",ec_ptr->if_name, mac_string);
	
	if (ioctl(s,SIOCGIFMTU,&ifr) == -1) {
		ERROR_PRINT(EMOLBADIOCTL);
		goto err;
	}
	ec_ptr->mtu = ifr.ifr_mtu;
	TASKDEBUG("%s MTU %d\n",ec_ptr->port_name, ec_ptr->mtu);
	close(s);
	return OK;
	err:
		rcode = errno;
		close(s);
	return errno;
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/

static char *low_level_input(eth_card_t *ec_ptr)
{
	u16_t length;

	TASKDEBUG("if_name=%s flags=0x%8lX\n", ec_ptr->if_name, ec_ptr->flags);
	
	/* Obtain the size of the packet and put it into the "length"    variable. */
	length = read(ec_ptr->ec_fd, rbuf, ETH_FRAME_LEN);
	TASKDEBUG("if_name=%s Frame received: length=%d\n", ec_ptr->if_name, length);

	_ip_hdr_t *ip_hdr_ptr;
	_icmp_hdr_t *icmp_hdr_ptr;
	_arp_hdr_t *arp_hdr_ptr;
	_eth_hdr_t *eth_hdr_ptr;
	
	// print source and destination MAC ADDRESS
	eth_hdr_ptr = (_eth_hdr_t*)rbuf;
	TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));		
	// print  IP header
	ip_hdr_ptr = (_ip_hdr_t *) (rbuf + ETH_HDRLEN);
	TASKDEBUG(IPHDR_FORMAT,IPHDR_FIELDS(ip_hdr_ptr));
	if(ip_hdr_ptr->proto == IPPROTO_ICMP){
		icmp_hdr_ptr = (_icmp_hdr_t*) (ip_hdr_ptr + IP4_HDRLEN);
		TASKDEBUG(ICMP_FORMAT,ICMP_FIELDS(icmp_hdr_ptr));
	}else{
		arp_hdr_ptr = (_arp_hdr_t *)(ip_hdr_ptr);
		if(arp_hdr_ptr->opcode == htons (ARPOP_REQUEST)){
			TASKDEBUG(ARP_FORMAT,ARP_FIELDS(arp_hdr_ptr));
		}
	}

	/* Interface disabled ? */
	if (!(ec_ptr->flags & ECF_ENABLED)){
		ERROR_PRINT(NO_NUM);
		return(NULL);
	}
		
	if ((ec_ptr->flags & ECF_READING)==0){
		ERROR_PRINT(NO_NUM);
		return(NULL);
	}

	ec_ptr->eth_stat.ets_packetR++;
    ec_ptr->read_s = length;
    ec_ptr->flags |= ECF_PACK_RECV;
//    ec_ptr->flags &= ~ECF_READING;
		
//	do_int(ec_ptr);
	
	return(rbuf);
}


/*-----------------------------------------------------------------------------------*/
/*
 * tapif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static int tapif_input(eth_card_t *ec_ptr)
{
	int ret;
	char *ptr;
	
	TASKDEBUG("port=%d\n", ec_ptr->ec_port);
  
  	/* signal main thread */
	// ATENCION: Deberia haber un tap_mutex que impida 
	// que varias ethX modifiquen las variables globales a la vez
	// solo para probar
//	MTX_LOCK(tap_mutex);
	MTX_LOCK(main_mutex);
    ret =mnx_wakeup(dc_ptr->dc_dcid, eth_ptr->p_endpoint);
	rqst_source |= TAP_REQUEST;
	iface_rqst = ((ec_ptr->ec_port << 4) | ETH_IFACE_READ);
	ptr = low_level_input(ec_ptr);
	COND_WAIT(receive_barrier[ec_ptr->ec_port],main_mutex);	
	MTX_UNLOCK(main_mutex);
//	MTX_UNLOCK(tap_mutex);
	return(OK);
}

/*===========================================================================*
 *                            THREAD send_thread					                         *
 *===========================================================================*/
static void send_thread(void *arg)
{
	eth_card_t *ec_ptr;
	fd_set fdset;
	int rcode, i, ret;
	int fs_ltid;
  	char *tap_addr;
	iovec_dat_t *iov_ptr;

	ec_ptr = (struct netif *)arg;
	
	fs_ltid = syscall(SYS_gettid);
	TASKDEBUG("fs_ltid=%d ec_port=%d\n", fs_ltid, ec_ptr->ec_port);
	
	posix_memalign( (void **) &tap_addr, getpagesize(), ETH_MAX_PACK_SIZE );
	if (tap_addr == NULL) {
		fprintf(stderr, "posix_memalign tap_addr \n");
	}
	
	posix_memalign( (void **) &iov_ptr, getpagesize(), sizeof(iovec_dat_t) );
	if (iov_ptr == NULL) {
		fprintf(stderr, "posix_memalign iov_ptr \n");
	}
	ec_ptr = (struct netif *)arg;
	
	MTX_LOCK(main_mutex);
	while(1) {
		TASKDEBUG("waiting send_barrier[%d]\n", ec_ptr->ec_port);
		COND_WAIT(send_barrier[ec_ptr->ec_port],main_mutex);
		
		TASKDEBUG("Copy IO vector from INET to TAP\n");
		memcpy(iov_ptr, &ec_ptr->write_iovec, sizeof(iovec_dat_t));
		
		TASKDEBUG("iod_iovec_s=%d  iod_proc_nr=%d\n", 
			iov_ptr->iod_iovec_s, iov_ptr->iod_proc_nr);
		for(i = 0; i < IOVEC_NR; i++ ){
			TASKDEBUG("\t i=%d iov_addr=%X iov_size=%d\n",
				i, iov_ptr->iod_iovec[i].iov_addr,  
				iov_ptr->iod_iovec[i].iov_size);
			if(iov_ptr->iod_iovec[i].iov_size == 0 ) break;
			rcode = M3IPC_vcopy(iov_ptr->iod_proc_nr, iov_ptr->iod_iovec[i].iov_addr, 
								SELF, tap_addr, 
								iov_ptr->iod_iovec[i].iov_size);
			if(rcode){
				TASKDEBUG("M3IPC_vcopy rcode=%d\n", rcode);
				break;
			}
			_ip_hdr_t *ip_hdr_ptr;
			_icmp_hdr_t *icmp_hdr_ptr;
			_arp_hdr_t *arp_hdr_ptr;
			_eth_hdr_t *eth_hdr_ptr;
			
			// print source and destination MAC ADDRESS
			eth_hdr_ptr = (_eth_hdr_t*)tap_addr;
			TASKDEBUG(ETHHDR_FORMAT,ETHHDR_FIELDS(eth_hdr_ptr));		
			// print  IP header
			ip_hdr_ptr = (_ip_hdr_t *) (tap_addr + ETH_HDRLEN);
			TASKDEBUG(IPHDR_FORMAT,IPHDR_FIELDS(ip_hdr_ptr));
			if(ip_hdr_ptr->proto == IPPROTO_ICMP){
				icmp_hdr_ptr = (_icmp_hdr_t*) (ip_hdr_ptr + IP4_HDRLEN);
				TASKDEBUG(ICMP_FORMAT,ICMP_FIELDS(icmp_hdr_ptr));
			}else{
				arp_hdr_ptr = (_arp_hdr_t *)(ip_hdr_ptr);
				if(arp_hdr_ptr->opcode == htons (ARPOP_REQUEST)){
					TASKDEBUG(ARP_FORMAT,ARP_FIELDS(arp_hdr_ptr));
				}
			}
	
//			memcpy(&tap_addr[ETH_ALEN],&ec_ptr->mac_address.ea_addr,ETH_ALEN); 
			if(write(ec_ptr->ec_fd, tap_addr, iov_ptr->iod_iovec[i].iov_size) == -1) {
				TASKDEBUG("write error:%d", errno);
			}
			TASKDEBUG("Frame SENT\n");
		}
		if( i == 0) {
			// REPORTAR ERROR DE VECTOR VACIO
			TASKDEBUG("WARNING: vector void!!\n");
			continue;
		}
		
		//*******************************************************************
		// signaling MAIN THREAD that a frame was sent  
		//*******************************************************************
		COND_SIGNAL(smain_barrier[ec_ptr->ec_port]);
	}
}


/*===========================================================================*
 *                          THREAD  receive_thread					                                     *
 *===========================================================================*/
static void receive_thread(void *arg)
{
	eth_card_t *ec_ptr;
	fd_set fdset;
	int rcode;
	int fr_ltid;
	
	ec_ptr = (struct netif *)arg;
	fr_ltid = syscall(SYS_gettid);
	TASKDEBUG("fr_ltid=%d ec_port=%d\n", fr_ltid, ec_ptr->ec_port);

	while(1) {
		FD_ZERO(&fdset);
		FD_SET(ec_ptr->ec_fd, &fdset);

		/* Wait for a packet to arrive. */
		rcode = select(ec_ptr->ec_fd + 1, &fdset, NULL, NULL, NULL);

		if(rcode == -1) ERROR_PRINT(errno);
	
		if ((ec_ptr->flags & ECF_READING)==0){
			TASKDEBUG("port=%d not reading\n", ec_ptr->ec_port);
			low_level_input(ec_ptr);
			continue;
		}
		if ((ec_ptr->flags & ECF_ENABLED)==0){
			TASKDEBUG("port=%d not enabled\n", ec_ptr->ec_port);
			low_level_input(ec_ptr);
			continue;		
		}
		
		if(rcode == 1) { /* Handle incoming packet. */
			tapif_input(ec_ptr);
		}  
	}
}

/*===========================================================================*
 *                            init_rings                                *
 *===========================================================================*/
void init_rings(int ec_nr)
{
	int i; 
	
	TASKDEBUG("ec_nr=%d\n", ec_nr);

	ei_ptr = eth_iface;
	ei_ptr->init_block.mode 		= 0x3;      /* disable Rx and Tx */
	ei_ptr->init_block.filter[0]	= 0x0;
	ei_ptr->init_block.filter[1]	= 0x0;

	for (i=0; i<RX_RING_SIZE; i++) {
		ei_ptr->rx_ring[i].buf_length 	= -ETH_FRAME_LEN;
		ei_ptr->rx_ring[i].u.base 		= ei_ptr->rbuf[i];
		ei_ptr->rx_ring[i].u.addr[3] 	= 0x80;
	}
	/* Preset the transmitting ring headers */
	for (i=0; i<TX_RING_SIZE; i++) {
		ei_ptr->tx_ring[i].u.base = 0;
		isstored[i] = 0;
	}
	ei_ptr->init_block.mode = 0x0;      /* enable Rx and Tx */

  return;
}

/*===========================================================================*
 *                            init_cards                                     *
 *===========================================================================*/
int init_cards(void)
{
	int ec_nr, rcode;
	struct ifreq ifr;

	TASKDEBUG("\n");
	
	for (ec_nr = 0; ec_nr < EC_PORT_NR_MAX; ec_nr++)   {
		sprintf(ec_table[ec_nr].port_name,"eth_card%d",ec_nr);
		sprintf(ec_table[ec_nr].if_name,"tap%1d", ec_nr);
		TASKDEBUG("init card %s on tap %s device %s\n", 
			ec_table[ec_nr].port_name, 
			ec_table[ec_nr].if_name,
			DEVTAP);

		ec_table[ec_nr].ec_fd = open(DEVTAP, O_RDWR);
		if( ec_table[ec_nr].ec_fd < 0){
			ERROR_PRINT(errno);
			ERROR_RETURN(ec_table[ec_nr].ec_fd);
		}
		
		memset(&ifr, 0, sizeof(ifr));
	    strncpy(ifr.ifr_name,ec_table[ec_nr].if_name,strlen(ec_table[ec_nr].if_name));
		ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
		if (ioctl(ec_table[ec_nr].ec_fd, TUNSETIFF, (void *) &ifr) < 0) {
			ERROR_EXIT(errno);
		}
		if ( (rcode = low_level_probe(&ec_table[ec_nr])) != OK)
			ERROR_EXIT(rcode);
	
		/*Create a Thread to deal with received frames */
		TASKDEBUG("Starting %s device receiving thread\n", 
			ec_table[ec_nr].if_name);
		rcode = pthread_create( &eth_rx_th[ec_nr], NULL, receive_thread, &ec_table[ec_nr] );
		if( rcode)ERROR_EXIT(rcode);
		TASKDEBUG("Receiver thread ec_nr=%d thread_id=%d\n",ec_nr, 	eth_rx_th[ec_nr]);		
		rcode = pthread_cond_init(&receive_barrier[ec_nr], NULL);
		if(rcode) ERROR_EXIT(rcode);
		
		/*Create a Thread to deal with sent frames */
		TASKDEBUG("Starting %s device sending thread\n", 
			ec_table[ec_nr].if_name);
		rcode = pthread_create( &eth_tx_th[ec_nr], NULL, send_thread, &ec_table[ec_nr] );
		if( rcode)ERROR_EXIT(rcode);
		TASKDEBUG("Sender thread ec_nr=%d thread_id=%d\n",ec_nr, eth_tx_th[ec_nr]);		

		rcode = pthread_cond_init(&receive_barrier[ec_nr], NULL);
		if(rcode) ERROR_EXIT(rcode);	
		rcode = pthread_cond_init(&send_barrier[ec_nr], NULL);
		if(rcode) ERROR_EXIT(rcode);
		rcode = pthread_cond_init(&smain_barrier[ec_nr], NULL);
		if(rcode) ERROR_EXIT(rcode);
		rcode = pthread_mutex_init(&send_mutex[ec_nr],NULL);
		if(rcode) ERROR_EXIT(rcode);
		rcode = pthread_mutex_init(&smain_mutex[ec_nr],NULL);
		if(rcode) ERROR_EXIT(rcode);		
		init_rings(ec_nr);
		ec_table[ec_nr].ec_owner = NONE;
    }
	return(OK);
}

/*===========================================================================*
 *                            SIGUSR1_handler                                     *
 *===========================================================================*/
static void  SIGUSR1_handler(int signo)
{
	TASKDEBUG("signo=%d\n", signo);
}
/*===========================================================================*
 *				eth_init									     *
 *===========================================================================*/
static void  eth_init(void)
{
	int rcode;

	eth_lpid = syscall(SYS_gettid);
	
	TASKDEBUG("eth_lpid=%lu\n", eth_lpid);

	// bind calling child thread 
//	rcode  = mnx_lclbind(dcid, eth_lpid, ETH_PROC_NR);
	rcode  = mnx_bind(dcid, ETH_PROC_NR);
	if(rcode  < 0 ) ERROR_EXIT(rcode);

	TASKDEBUG("Get the DVS info from SYSTASK\n");
    rcode = sys_getkinfo(&drvs);
	if(rcode) ERROR_EXIT(rcode);
	drvs_ptr = &drvs;
	TASKDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(drvs_ptr));

	TASKDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	TASKDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	TASKDEBUG("Get endpoint info from SYSTASK\n");
	rcode = sys_getproc(&eth, SELF);
	if(rcode) ERROR_EXIT(rcode);
	eth_ptr = &eth;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(eth_ptr));
	
}

/*===========================================================================*
 *                          THREAD inet receiving requests					                        *
 *===========================================================================*/
static void inet2eth_thread(void *arg)
{
	int rcode;
#define 	RECEIVE_TIMEOUT	5000

	eth_init();

	while(TRUE){
		rcode = mnx_receive_T(ANY, mi_ptr, RECEIVE_TIMEOUT);
		TASKDEBUG("rcode=%d\n", rcode);
		if( rcode == EMOLWOKENUP){ // TAP thread has waken me up 	
			MTX_LOCK(main_mutex);				// get globaL lock
		} else if(rcode < 0){  // ATENCION, esto despues hay que darle tratamiento correcto, ahora solo para probar 
			TASKDEBUG("rcode=%d\n", rcode);
			continue;
		} else {
			// A INET request has been received 
			TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mi_ptr));
			MTX_LOCK(main_mutex);
			rqst_source |= INET_REQUEST;
		}
		COND_SIGNAL(main_barrier);			// signal main, something has occurred 
		COND_WAIT(inet2eth_barrier, main_mutex);// wait until all operations end  
		MTX_UNLOCK(main_mutex);
	}
}



/*===========================================================================*
 *				main_init					     *
 *===========================================================================*/
static void main_init()
{
	int rcode;
	
	main_lpid = getpid();
	TASKDEBUG("main_lpid=%d\n", main_lpid);

	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &mi_ptr, getpagesize(), sizeof(message) );
	if (mi_ptr== NULL) {
   		ERROR_EXIT(errno);
	}
	TASKDEBUG("ETH mi_ptr=%p\n",mi_ptr);

	posix_memalign( (void **) &mo_ptr, getpagesize(), sizeof(message) );
	if (mo_ptr== NULL) {
   		ERROR_EXIT(errno);
	}
	TASKDEBUG("ETH mo_ptr=%p\n",mo_ptr);

	rcode = pthread_mutex_init(&main_mutex,NULL);
	if(rcode) ERROR_EXIT(rcode);
	
#ifdef M3IPC_LOCK
	rcode = pthread_mutex_init(&m3ipc_mutex,NULL);
	if(rcode) ERROR_EXIT(rcode);
#endif // M3IPC_LOCK
	
	rcode = pthread_cond_init(&main_barrier, NULL);
	if(rcode) ERROR_EXIT(rcode);

	iface_rqst = ETH_IFACE_NONE;
	if (signal(SIGUSR1,  SIGUSR1_handler) == SIG_ERR)
		ERROR_EXIT(SIG_ERR);
	
	rcode = pthread_cond_init(&inet2eth_barrier, NULL);
	if(rcode) ERROR_EXIT(rcode);

	/*Create a Thread to deal with INET requests */
	TASKDEBUG("Starting INET requests thread\n");
	rcode = pthread_create( &inet2eth_th, NULL, inet2eth_thread, 0 );
	if( rcode)ERROR_EXIT(rcode);
	TASKDEBUG("INET requests thread  thread_id=%d\n",inet2eth_th);
	
	/* Fetch clock ticks */
	clockTicks = sysconf(_SC_CLK_TCK);
	if (clockTicks == -1)	ERROR_EXIT(errno);
	TASKDEBUG("clockTicks =%ld\n",clockTicks );
	
	init_cards();		
}

/*===========================================================================*
 *                              mess_reply                                   *
 *===========================================================================*/
void mess_reply(message *req, message *reply_mess)
{
	int rcode;
	
	TASKDEBUG(MSG3_FORMAT, MSG3_FIELDS(reply_mess) );
  	if ( (rcode = M3IPC_send(req->m_source, reply_mess)) != OK)
   		ERROR_EXIT(rcode);
}

/*===========================================================================*
 *                              reply                                        *
 *===========================================================================*/
void reply(eth_card_t *ec_ptr, int err, int may_block)
{
	message reply, *r_ptr;
	int status,rcode;
	mnx_time_t now;
    molclock_t t[5];

	TASKDEBUG("if_name=%s err=%d may_block=%d flags=0x%8lX\n", 
		ec_ptr->if_name, err, may_block, ec_ptr->flags);
	r_ptr = &reply;
	status = 0;
	if (ec_ptr->flags & ECF_PACK_SEND)
		status |= DL_PACK_SEND;
	if (ec_ptr->flags & ECF_PACK_RECV)
		status |= DL_PACK_RECV;

	rcode=sys_times(eth_ptr->p_endpoint, t);
	if(rcode != OK ) ERROR_EXIT(rcode);
	now = t[4];		/* uptime since boot */
	now /= clockTicks;
	TASKDEBUG("client=%d now =%ld\n", ec_ptr->client, now );
	reply.DL_CLCK = now; 
  
	reply.m_type   = DL_TASK_REPLY;
	reply.DL_PORT  = ec_ptr - ec_table;
	reply.DL_PROC  = ec_ptr->client;
	reply.DL_STAT  = status | ((u32_t) err << 16);
	reply.DL_COUNT = ec_ptr->read_s;

	TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(r_ptr) );
	rcode = M3IPC_send(ec_ptr->client, &reply);
	if (rcode == EMOLLOCKED && may_block)	{
	    TASKDEBUG("EMOLLOCKED!\n");
		return;
	}

	if (rcode < 0)
		ERROR_EXIT(rcode);

	ec_ptr->read_s = 0;
	ec_ptr->flags &= ~(ECF_PACK_SEND | ECF_PACK_RECV);
}

/*===========================================================================*
 *                            do_vread                                     *
 *===========================================================================*/
static void do_vread(message *mp, int vectored)
{
	int port, count, size;
	eth_card_t *ec_ptr;

	port = mp->DL_PORT;
	count = mp->DL_COUNT;
	TASKDEBUG("port=%d count=%d client=%d vectored=%X \n", 
		port, count, mp->DL_PROC, vectored);

	ec_ptr = &ec_table[port];
	ec_ptr->client= mp->DL_PROC;
	if(mp->DL_PROC != ec_ptr->ec_owner){
		mo_ptr->m_type = EMOLBADOWNER;
		mo_ptr->m_type= DL_TASK_REPLY;
		mess_reply(mp, mo_ptr);
		return;
	}
	
	if (vectored)   {
		get_userdata(mp->DL_PROC, (vir_bytes) mp->DL_ADDR,
                   (count > IOVEC_NR ? IOVEC_NR : count) *
                   sizeof(iovec_t), ec_ptr->read_iovec.iod_iovec);
		ec_ptr->read_iovec.iod_iovec_s    = count;
		ec_ptr->read_iovec.iod_proc_nr    = mp->DL_PROC;
		ec_ptr->read_iovec.iod_iovec_addr = (vir_bytes) mp->DL_ADDR; 
		ec_ptr->tmp_iovec = ec_ptr->read_iovec;
		size= calc_iovec_size(&ec_ptr->tmp_iovec);
    } else  {
		ec_ptr->read_iovec.iod_iovec[0].iov_addr = (vir_bytes) mp->DL_ADDR;
		ec_ptr->read_iovec.iod_iovec[0].iov_size = mp->DL_COUNT;
		ec_ptr->read_iovec.iod_iovec_s           = 1;
		ec_ptr->read_iovec.iod_proc_nr           = mp->DL_PROC;
		ec_ptr->read_iovec.iod_iovec_addr        = 0;
		size= count;
    }
	ec_ptr->flags |= ECF_READING;

	ec_recv(ec_ptr);

	if ((ec_ptr->flags & (ECF_READING|ECF_STOPPED)) == (ECF_READING|ECF_STOPPED))
		ec_reset(ec_ptr);
	reply(ec_ptr, OK, FALSE);
}

/*===========================================================================*
 *                            do_init                                     *
  * | DL_INIT    | port nr  | proc nr | mode   | address |
 *===========================================================================*/
static void do_init(message *req)
{
	int port, rcode;
	eth_card_t *ec_ptr;
	ether_addr_t *mac_ptr;
	char *addr_ptr = NULL;
	char *mask_ptr = NULL;
	char *gw_ptr   = NULL;
	char *ptr, *token;
	static char cmd[MNX_PATH_MAX]; 
	
	TASKDEBUG("DL_PORT=%d DL_PROC=%d DL_COUNT=%d\n",
		req->DL_PORT,req->DL_PROC, req->DL_COUNT);
	port = req->DL_PORT;
	if(req->DL_PORT < 0 || req->DL_PORT >= EC_PORT_NR_MAX){
		ERROR_PRINT(EMOLNXIO);
		mo_ptr->m_type = EMOLNXIO;
		mo_ptr->m_type= DL_INIT_REPLY;
		mess_reply(req, mo_ptr);
		return;
	}
	
	ec_ptr= &ec_table[port];
	if(ec_ptr->ec_owner != NONE){
		ERROR_PRINT(EMOLBADOWNER);
		mo_ptr->m_type = EMOLBADOWNER;
		mo_ptr->m_type= DL_INIT_REPLY;
		mess_reply(req, mo_ptr);
		return;
	}
	
	rcode =  M3IPC_vcopy(req->DL_PROC, req->DL_ADDR, SELF, 
		ec_ptr->lwip_args, req->DL_COUNT);
	if(rcode < 0) ERROR_EXIT(rcode);
		
	strcpy(ec_ptr->port_name, "eth_card#0");
	ec_ptr->port_name[9] += port;
	if (ec_ptr->mode == EC_DISABLED) {
		ec_ptr->mode= EC_ENABLED;
		ec_ptr->client = req->DL_PROC;
		ec_ptr->ec_owner = req->DL_PROC;
	}
	
	// READY TO RECEIVE frames and to notify INET/LWIP
	ec_ptr->flags |= (ECF_READING|ECF_ENABLED);

	TASKDEBUG(EC_FORMAT,EC_FIELDS(ec_ptr));
	TASKDEBUG("lwip_args=%s\n", ec_ptr->lwip_args);

	// parse the lwip args
	// dcid=0,name=mol0,addr=172.16.0.2,netmask=255.255.255.0,gw=172.16.0.1
	ptr = ec_ptr->lwip_args;
	do {
		token = strtok(ptr, ",");
		TASKDEBUG("token=>%s<\n", token);
		if( token == NULL) break;
		if( strncmp(token,"addr=",5) == 0){
			addr_ptr = token + 5;
			TASKDEBUG("addr=%s\n", addr_ptr);
		}else if( strncmp(token,"netmask=",8) == 0){
			mask_ptr = token + 8;
			TASKDEBUG("netmask=%s\n", mask_ptr);
		}else if( strncmp(token,"gw=",3) == 0){
			gw_ptr = token + 3;
			TASKDEBUG("gw=%s\n", gw_ptr);
		}		
		ptr = NULL;
	}while(TRUE);
		
	if( addr_ptr == NULL 
	 || mask_ptr == NULL 
	 || gw_ptr   == NULL){
		ERROR_PRINT(EMOLINVAL); 
	 }	
	
	// RUN command IFCONFIG to set the IP ADDRESS  
	sprintf(cmd,IFCONFIG_BIN IFTAP IFCONFIG_ARGS,
            addr_ptr,mask_ptr);
	TASKDEBUG("cmd=%s\n", cmd);
	system(cmd);

	mo_ptr->m_type = DL_INIT_REPLY;
	mo_ptr->m3_i1 = req->DL_PORT;
	mo_ptr->m3_i2 = EC_PORT_NR_MAX;
	mac_ptr = &ec_ptr->mac_address;
	memcpy(mo_ptr->m3_ca1, mac_ptr, ETH_ALEN);
	TASKDEBUG(MAC_FORMAT "\n", MAC_FIELDS(mac_ptr));
	TASKDEBUG(MSG3_FORMAT,MSG3_FIELDS(mo_ptr));
	mess_reply(req, mo_ptr);
}

/*===========================================================================*
 *                              put_userdata                                 *
 *===========================================================================*/
void put_userdata(int user_proc, vir_bytes user_addr,vir_bytes count, void *loc_addr)
{
	int cps;
//	cps = sys_vircopy(SELF, (void *) loc_addr, user_proc, (void *) user_addr, count);
	cps = M3IPC_vcopy(SELF, (void *) loc_addr, user_proc, (void *) user_addr, count);
	if (cps != OK) {
		TASKDEBUG("Warning, sys_vircopy failed: %d\n", cps);
	}
}

/*===========================================================================*
 *                            do_getstat                                     *
 *===========================================================================*/
static void do_getstat(message *req)
{
	int port;
	eth_card_t *ec_ptr;

	TASKDEBUG("port=%d\n", req->DL_PORT );
	port = req->DL_PORT;
	if (port < 0 || port >= EC_PORT_NR_MAX)
		mo_ptr->m_type = EMOLNXIO;
	else	
		mo_ptr->m_type = DL_INIT_REPLY;

	ec_ptr= &ec_table[port];
	ec_ptr->client = req->DL_PROC;
	if(req->DL_PROC != ec_ptr->ec_owner){
		mo_ptr->m_type = EMOLBADOWNER;
		mo_ptr->m_type= DL_TASK_REPLY;
		mess_reply(req, mo_ptr);
		return;
	}
	put_userdata(req->DL_PROC, (vir_bytes) req->DL_ADDR,
               (vir_bytes) sizeof(ec_ptr->eth_stat), &ec_ptr->eth_stat);
	reply(ec_ptr, OK, FALSE);
}

/*===========================================================================*
 *                            do_stop                                     *
 *===========================================================================*/
static void do_stop(message *req)
{
	eth_card_t *ec_ptr;
	
	TASKDEBUG("port=%d\n", req->DL_PORT );
	ec_ptr= &ec_table[req->DL_PORT];
	ec_ptr->client = req->DL_PROC;
	if(req->DL_PROC != ec_ptr->ec_owner){
		mo_ptr->m_type = EMOLBADOWNER;
		mo_ptr->m_type= DL_TASK_REPLY;
		mess_reply(req, mo_ptr);
		return;
	}
}


/*===========================================================================*
 *                            do_getname                                     *
 *===========================================================================*/
static void do_getname(message *req)
{
	eth_card_t *ec_ptr;

	TASKDEBUG("port=%d\n", req->DL_PORT );
	ec_ptr= &ec_table[req->DL_PORT];
	ec_ptr->client = req->DL_PROC;
	if(req->DL_PROC != ec_ptr->ec_owner){
		mo_ptr->m_type = EMOLBADOWNER;
		mo_ptr->m_type= DL_TASK_REPLY;
		mess_reply(req, mo_ptr);
		return;
	}
}

/*===========================================================================*
 *				   main 				    					 *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int ret, iface_nr, iface_op;
	eth_card_t *ec_ptr;

	if ( argc != 2) {
 	        printf( "Usage: %s <dcid> \n", argv[0] );
 	        exit(1);
    }

	dcid = atoi(argv[1]);
	if( dcid < 0 || dcid >= NR_VMS) ERROR_EXIT(EMOLRANGE);

	
	TASKDEBUG("Starting ETH on dcid=%d\n", dcid);
	main_init();
	
	MTX_LOCK(main_mutex);
	while(TRUE){
		TASKDEBUG("waiting on barrier\n");
		COND_WAIT(main_barrier,main_mutex);
		
		// check valid request source 
		if( !(rqst_source & (TAP_REQUEST | INET_REQUEST))) {
			TASKDEBUG("WARNING rqst_source=%X\n", rqst_source);
			continue;
		}
		
		//------------------- TAP IFACE THREAD HAS RECEIVED OR SENT A FRAME --------------
		TASKDEBUG("rqst_source=%X\n", rqst_source);
		if( rqst_source == TAP_REQUEST) {
			if( iface_rqst == ETH_IFACE_NONE) {
				TASKDEBUG("iface_rqst == ETH_IFACE_NONE\n");
				continue;
			}
			iface_nr = (iface_rqst >> 4);
			TASKDEBUG("iface_rqst=%X by iface_nr=%d\n"
				, iface_rqst, iface_nr);
			ec_ptr = &ec_table[iface_nr];
			TASKDEBUG(EC_FORMAT, EC_FIELDS(ec_ptr));
			if(iface_rqst & ETH_IFACE_READ){
				TASKDEBUG("Reply to upper layer about a frame received on card=%d\n", iface_nr);
				ec_ptr->flags |=  ECF_PACK_RECV;
				iface_rqst &= ~(ETH_IFACE_READ);
			}
			if(iface_rqst & ETH_IFACE_WRITE){
				TASKDEBUG("Reply to upper layer about a frame sent on card=%d\n", iface_nr);
				ec_ptr->flags |=  ECF_PACK_SEND;
				iface_rqst &= ~(ETH_IFACE_WRITE);
			}
			iface_rqst = ETH_IFACE_NONE;
			rqst_source &= ~TAP_REQUEST;
//			reply(ec_ptr, OK, TRUE);
			ret = M3IPC_notify(ec_ptr->ec_owner);
			if(ret < 0) ERROR_EXIT(ret);
			COND_SIGNAL(receive_barrier[ec_ptr->ec_port]);
			COND_SIGNAL(inet2eth_barrier);
			if(rqst_source == 0) continue;
			// FALL DOWN, another request
		} 
		//------------------- A REQUEST WAS RECEIVED FROM INET --------------
		TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(mi_ptr));
      	switch(mi_ptr->m_type){
			case DL_WRITE:   do_vwrite(mi_ptr, FALSE, FALSE);   break;
			case DL_WRITEV:  do_vwrite(mi_ptr, FALSE, TRUE);    break;
      		case DL_READ:    do_vread(mi_ptr, FALSE);   	break;
      		case DL_READV:   do_vread(mi_ptr, TRUE);    	break;
      		case DL_INIT:    do_init(mi_ptr);           	break;
      		case DL_GETSTAT: do_getstat(mi_ptr);        	break;
      		case DL_STOP:    do_stop(mi_ptr);           	break;
      		case DL_GETNAME: do_getname(mi_ptr); 			break;
      		default:
				TASKDEBUG("Invalid message type from upper layer: %X\n",mi_ptr->m_type);
				ERROR_EXIT(EMOLBADCALL);
      	}
		rqst_source &= ~INET_REQUEST;
		COND_SIGNAL(inet2eth_barrier);
	}
}

