/* EXTERN should be extern except in inet.c */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

/* Global variables. */
EXTERN event_t *ev_head;
EXTERN event_t *ev_tail;
EXTERN int clck_call_expire;	/* Call clck_expire_timer from the mainloop */

EXTERN int local_nodeid;
EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN int inet_lpid;		
EXTERN dc_usr_t  dcu, *dc_ptr;

EXTERN long clockTicks;
EXTERN time_t boottime;
EXTERN proc_usr_t eth, *eth_ptr;	
EXTERN proc_usr_t inet, *inet_ptr;	

EXTERN message *m_ptr;

EXTERN int this_proc;


EXTERN struct eth_conf eth_conf[IP_PORT_MAX];
EXTERN struct psip_conf psip_conf[IP_PORT_MAX];
EXTERN struct ip_conf ip_conf[IP_PORT_MAX];
EXTERN struct tcp_conf tcp_conf[IP_PORT_MAX];
EXTERN struct udp_conf udp_conf[IP_PORT_MAX];
EXTERN u8_t iftype[IP_PORT_MAX];	/* Interface in use as? */

EXTERN dev_t ip_dev;

EXTERN int eth_conf_nr;
EXTERN int psip_conf_nr;
EXTERN int ip_conf_nr;
EXTERN int tcp_conf_nr;
EXTERN int udp_conf_nr;

EXTERN int ip_forward_directed_bcast;
EXTERN int ifdefault;

EXTERN tcp_port_t *tcp_port_table;
EXTERN tcp_conn_t tcp_conn_table[TCP_CONN_NR];
EXTERN tcp_fd_t tcp_fd_table[TCP_FD_NR];

EXTERN udp_port_t *udp_port_table;
EXTERN udp_fd_t udp_fd_table[UDP_FD_NR];

EXTERN sr_cancel_t tcp_cancel_f;

EXTERN eth_port_t *eth_port_table;
EXTERN int no_ethWritePort;	/* debug, consistency check */

EXTERN u16_t _tmp;
EXTERN u32_t _tmp_l;

EXTERN int killer_inet;

#ifdef _TABLE
struct devlist devlist[] = {
		{	"/dev/eth",	0600,	ETH_DEV_MINOR	},
		{	"/dev/psip",0600,	PSIP_DEV_MINOR},
		{	"/dev/ip",	0600,	IP_DEV_MINOR	},
		{	"/dev/tcp",	0666,	TCP_DEV_MINOR	},
		{	"/dev/udp",	0666,	UDP_DEV_MINOR	},
	};
#else // _TABLE
extern struct devlist devlist[5];	
#endif // _TABLE

