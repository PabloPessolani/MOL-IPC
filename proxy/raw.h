#define 	QUEUEBASE		0x2000
#define		MAX_MQ_ENTRIES  16

enum raw_mq_type {
		RAW_ETH_DONOTUSE =0,
		RAW_ETH_HDR 	= 0x01,	
		RAW_ETH_PAY 	= 0x02,		
		RAW_ETH_ACK 	= 0x04,
		RAW_ETH_ERR 	= 0x08, 	// RAW interface in ERROR
		RAW_ETH_OK 		= 0x10, 	// RAW interface OK  
  };
  
typedef struct {
	int			mtype;
	struct {
		cmd_t	cmd;
		proxy_payload_t pay;
	} m3;
} msgq_buf_t;