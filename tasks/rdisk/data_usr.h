// #include "../../kernel/minix/config.h"
// #include "../../kernel/minix/types.h"
// #include "../../kernel/minix/const.h"
// #include "../../kernel/minix/timers.h"
// #include "../../kernel/minix/type.h"
// #include "../../kernel/minix/ipc.h"

typedef union {
	message pay_msg;		/* Minix message		*/
	unsigned 	pay_data[MAXCOPYBUF];	/* buffer space to copy data	*/  
} data_payload_t;

// #define TIME_FORMAT "TIMESTAMP sec=%ld nsec=%ld\n"
// #define TIME_FIELDS(p) p->tv_sec, p->tv_nsec

// struct proxies_usr_s {
	// unsigned int	px_id; 		/* The number of pair of proxies	*/
// unsigned long int	px_flags; 	/* The status of the pair of proxies 	*/
	// char		px_name[MAXPROXYNAME];

// };
// typedef struct proxies_usr_s proxies_usr_t;

// #define PX_USR_FORMAT 		"px_id=%d px_flags=%d px_name=%s\n" 
// #define PX_USR_FIELDS(p) 	p->px_id, p->px_flags, p->px_name

/*For rdisk/tests*/
#define BUFFER 4096
int flag_buff;
char buffer[BUFFER];

size_t len, buffer_size;

