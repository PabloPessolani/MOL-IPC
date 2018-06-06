
/* COMMANDS 	*/
  enum proxy_cmd{
        CMD_NONE      =  0,	/* NO COMMAND  							*/
		CMD_SEND_MSG,		/*  Send a message to a process 					*/
		CMD_NTFY_MSG,		/* Send a NOTIFY message to remote proces  		*/
		CMD_SNDREC_MSG,		/*  Send a message to a process and wait for reply 		*/
		
		CMD_REPLY_MSG,		/*  Send a REPLY message to a process 				*/
		CMD_COPYIN_DATA,	/* Request and data to copy data to remote process 		*/
		CMD_COPYOUT_RQST,	/* The remote process send to local process the data requested 	*/
		CMD_COPYLCL_RQST,
		
		CMD_COPYRMT_RQST,	/* REQUESTER to SENDER to copy data out to RECEIVER */
		CMD_COPYIN_RQST,	/* SENDER to RECEIVER */
        CMD_HELLO,			/* HELLO COMMAND used by proxies 				*/
		CMD_SHUTDOWN,		/* Exit the waiting loop with error	EMOLINTR */

		CMD_CHECKBIND,		/* Request remote RPROXY if a REMOTE process is binded */
		CMD_LAST_CMD		/* THIS MUST BE THE LAST COMMAND */
  };

#define CMD_SEND_ACK		(CMD_SEND_MSG | CMD_ACKNOWLEDGE) 
#define CMD_NTFY_ACK		(CMD_NTFY_MSG | CMD_ACKNOWLEDGE)
#define CMD_SNDREC_ACK  	(CMD_SNDREC_MSG | CMD_ACKNOWLEDGE)
#define	CMD_REPLY_ACK   	(CMD_REPLY_MSG  | CMD_ACKNOWLEDGE)
#define	CMD_COPYIN_ACK 		(CMD_COPYIN_DATA | CMD_ACKNOWLEDGE)
#define	CMD_COPYOUT_DATA 	(CMD_COPYOUT_RQST | CMD_ACKNOWLEDGE)
#define	CMD_COPYLCL_ACK		(CMD_COPYLCL_RQST | CMD_ACKNOWLEDGE)
#define CMD_COPYRMT_ACK 	(CMD_COPYRMT_RQST | CMD_ACKNOWLEDGE) /* From RECEIVER to REQUESTER */
#define CMD_CHECKBIND_ACK	(CMD_CHECKBIND | CMD_ACKNOWLEDGE) 
  
struct vcopy_s {
	int	v_src;		/* source endpoint		*/
	int	v_dst;		/* destination endpoint		*/
  	int	v_rqtr;		/* requester endpoint		*/
	void 	*v_saddr;	/* virtual address copy from 	*/
  	void 	*v_daddr;	/* virtual address copy to 	*/
  	int	v_bytes;	/* bytes to copy		*/
};
typedef struct vcopy_s vcopy_t;

struct cmd_s {
	int	c_cmd;		
	int c_dcid;		/* DC ID					*/
	int	c_src;		/* source endpoint			*/
	int	c_dst;		/* destination endpoint			*/
	int	c_snode;	/* source node				*/
	int	c_dnode;	/* destination node			*/
	int c_rcode;	/* return code 				*/
  	int c_len;		/* payload len 				*/
  	unsigned long c_flags;			/* generic field for flags filled and controled by proxies not by M3-IPC 	*/
  	unsigned long c_snd_seq;		/* send sequence #  - filled and controled by proxies not by M3-IPC 	*/
  	unsigned long c_ack_seq;		/* acknowledge sequence #  - filled and controled by proxies not by M3-IPC 	*/
	struct timespec c_timestamp;	/* timestamp				*/
	union {
		vcopy_t cu_vcopy;	/* struct used only for remote vcopy 	*/
		message cu_msg;		
	}c_u;
};
typedef struct cmd_s cmd_t;

#define CMD_FORMAT "cmd=0x%X dcid=%d src=%d dst=%d snode=%d dnode=%d rcode=%d len=%d\n" 
#define CMD_FIELDS(p) 	p->c_cmd, p->c_dcid, p->c_src, p->c_dst, p->c_snode \
	, p->c_dnode, p->c_rcode, p->c_len 

#define CMD_XFORMAT "c_flags=0x%lX c_snd_seq=%ld c_ack_seq=%ld\n" 
#define CMD_XFIELDS(p) 	p->c_flags, p->c_snd_seq, p->c_ack_seq
	
#define VCOPY_FORMAT "src=%d dst=%d rqtr=%d saddr=%p daddr=%p bytes=%d \n"
#define VCOPY_FIELDS(p) p->c_u.cu_vcopy.v_src, p->c_u.cu_vcopy.v_dst, p->c_u.cu_vcopy.v_rqtr,\
	 p->c_u.cu_vcopy.v_saddr, p->c_u.cu_vcopy.v_daddr, p->c_u.cu_vcopy.v_bytes
	 
#define	HDR_PAYLOAD_MASK	0x1000	/* it means that thereis another header */

#define HDR_FORMAT 		CMD_FORMAT
#define HDR_FIELDS(p)	CMD_FIELDS(p)





