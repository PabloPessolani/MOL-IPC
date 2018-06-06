#define RS_BASE_PORT	2000

typedef struct {
	int		mtype;
	union {
		struct {
			message mnx_msg;
			char arg_v[_POSIX_ARG_MAX-sizeof(int)-sizeof(message)];
		} mnx;
		char buffer[_POSIX_ARG_MAX-sizeof(int)];
	} udp_u; 
} udp_buf_t;
