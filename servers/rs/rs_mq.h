#define QUEUEBASE	3000

typedef struct {
	int		mtype;
	union {
		struct {
			message mnx_msg;
			char arg_v[_POSIX_ARG_MAX-sizeof(int)-sizeof(message)];
		} mnx;
		char buffer[_POSIX_ARG_MAX-sizeof(int)];
	} mq_u; 
} msgq_buf_t;
