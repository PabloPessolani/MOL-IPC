#define 	QUEUEBASE		0x2000
#define		MAX_MQ_ENTRIES  16

enum raw_mq_type {
		RAW_MQ_DONOTUSE =0,
		RAW_MQ_HDR 	= 0x01,	
		RAW_MQ_PAY 	= 0x02,		
		RAW_MQ_ACK 	= 0x04,
		RAW_MQ_ERR 	= 0x08, 	// RAW interface in ERROR
		RAW_MQ_OK 	= 0x10, 	// RAW interface OK  
  };
  
typedef struct {
	int			mtype;
	struct {
		cmd_t	cmd;
		proxy_payload_t pay;
	} m3;
} msgq_buf_t;