#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <sys/socket.h> 

#include <linux/genetlink.h>

#include "/usr/src/linux/kernel/minix/config.h"
#include "/usr/src/linux/kernel/minix/ipc.h"
#include "/usr/src/linux/kernel/minix/kipc.h"
#include "/usr/src/linux/kernel/minix/moldebug.h"

#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

enum nlexample_msg_types {
   NLEX_CMD_UPD = 0,
   NLEX_CMD_GET,
   NLEX_CMD_MAX
};

enum nlexample_attr {
   NLE_UNSPEC,
   NLE_MYVAR,
   __NLE_MAX,
};
#define NLE_MAX (__NLE_MAX - 1)
#define NLEX_GRP_MYVAR 1


double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

   
void  main ( int argc, char *argv[] )
{
	int ret, i, loops;
	message m;
	double t_start, t_stop, t_total;
	char buf[getpagesize()];
	struct nlmsghdr *nlh;
	struct genlmsghdr *genl;
	struct mnl_socket *nl;
	unsigned int seq, oper, portid, src_pid;

	
  	if (argc != 2) {
    		printf ("usage: %s <loops>\n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);


	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = 0x0001;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	nlh->nlmsg_seq = seq = time(NULL);
	genl = mnl_nlmsg_put_extra_header(nlh, sizeof(struct genlmsghdr));
	genl->cmd = NLEX_CMD_UPD;
	mnl_attr_put_u32(nlh, NLE_MYVAR, 11);

	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */

		nl = mnl_socket_open(NETLINK_GENERIC);
		if (nl == NULL) {
			perror("mnl_socket_open");
			exit(EXIT_FAILURE);
		}
		ret = mnl_socket_bind(nl, 0, 0);
		if (ret == -1) {
			perror("mnl_socket_bind");
			exit(EXIT_FAILURE);
		}
		portid = mnl_socket_get_portid(nl);
		t_start = dwalltime();
		for( i = 0; i < loops; i++) {
			ret = mnl_socket_sendto(nl, nlh, nlh->nlmsg_len);
			if (ret == -1) {
				perror("mnl_socket_send");
				exit(EXIT_FAILURE);
			}

			ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
			while (ret == -1) {
				perror("mnl_socket_recvfrom");
				exit(EXIT_FAILURE);
			}
		}
	     	t_stop  = dwalltime();

		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d\n", loops);
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)loops);
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)(loops*2)/t_total);
			wait(&ret);
	}else{						/* SON = SOURCE		*/

		for( i = 0; i < loops; i++){

		}

	}

 printf("exit \n");
 
 exit(0);
}



