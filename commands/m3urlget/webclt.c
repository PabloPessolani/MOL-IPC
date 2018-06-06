#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1
#define _MINIX

#define _XOPEN_SOURCE 600 
#define VERSION 23

#define CMDDBG		1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <fcntl.h>
#include <malloc.h>
//#include <termios.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
/* According to POSIX.1-2001 */
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#include "../../kernel/minix/config.h"
#include "../../kernel/minix/const.h"
#include "../../kernel/minix/types.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/limits.h"
#include "../../kernel/minix/type.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/syslib.h"
//#include "../../kernel/minix/u64.h"
#include "../../kernel/minix/dvs_usr.h"
#include "../../kernel/minix/dc_usr.h"
#include "../../kernel/minix/node_usr.h"
#include "../../kernel/minix/proc_usr.h"
#include "../../kernel/minix/proc_sts.h"
#include "../../kernel/minix/com.h"
//#include "../../kernel/minix/proxy_usr.h"
#include "../../kernel/minix/molerrno.h"
#include "../../kernel/minix/endpoint.h"
#include "../../kernel/minix/resource.h"
#include "../../kernel/minix/callnr.h"
#include "../../kernel/minix/ansi.h"
#include "../../kernel/minix/priv.h"
#include "../../kernel/minix/dmap.h"
#include "../../kernel/minix/termios.h"
#include "../../kernel/minix/configfile.h"
#include "../../kernel/minix/ioc_tty.h"
#include "../../kernel/minix/timers.h"
#include "../../kernel/minix/select.h"
#include "../../stub_syscall.h"
#include "../debug.h"
#include "../macros.h"

#include "./webcommon.h"

char command[MNX_PATH_MAX+50];
message webc_msg, *msg_ptr;
int webc_lpid, webc_ep, local_nodeid;

int m3_socket(int domain, int type, int protocol);
int m3_connect(int sockfd, const struct sockaddr *saddr, socklen_t addrlen);
int m3_write(int fd, const void *buf, size_t count);
int m3_read(int fd, const void *buf, size_t count);

pexit(char * msg)
{
	perror(msg);
	exit(1);
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}


print_usage(char *argv0){
	fprintf(stderr,"Usage: %s <filename>\n", argv0 );
}

int main ( int argc, char *argv[] )
{
	int bytes ,sockfd, rcode, out_fd;
	long t_bytes;
	char buffer[WEBMAXBUF];
	static struct sockaddr_in serv_addr;
	double t_start, t_stop, t_total;
	FILE *fp_result;

	if ( argc < 2) {
		print_usage(argv[0]);
 	    exit(1);
    }
	if(strlen(argv[1]) >  MNX_PATH_MAX-1)
		ERROR_EXIT(EMOLNAMETOOLONG);

	out_fd = open(argv[1], O_WRONLY | O_CREAT); 
	if(  out_fd < 0) ERROR_EXIT(errno);
	
	sprintf(command,"GET /%s HTTP/1.0 \r\n\r\n",argv[1]);
	CMDDEBUG("client trying to m3_connect to %s and port %d\n",SVR_ADDR,SVR_PORT);
	if((sockfd = m3_socket(AF_INET, SOCK_STREAM,0)) <0) 
		pexit("m3_socket() failed");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SVR_ADDR);
	serv_addr.sin_port = htons(SVR_PORT);

	/* Connect tot he socket offered by the web server */
	if(m3_connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0) 
		pexit("m3_connect() failed");

	/* Now the sockfd can be used to communicate to the server the GET request */
	CMDDEBUG("Send bytes=%d %s\n",strlen(command), command);
	t_start = dwalltime();
	rcode = m3_write(sockfd, command, strlen(command));
	if( rcode < 0) ERROR_EXIT(errno);
	
	/* This displays the raw HTML file (if index.html) as received by the browser */
#define  STDOUT 1
	t_bytes=0;
	while( (bytes=m3_read(sockfd,buffer,WEBMAXBUF)) > 0){
		write(	out_fd,buffer,bytes);
		t_bytes += bytes;
	}
 	t_stop  = dwalltime();

	close(out_fd);
	t_total = (t_stop-t_start);
	
	fp_result = fopen("results.txt", "a+");
 	fprintf(fp_result, "t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	fprintf(fp_result, "Transfer size= %ld [bytes]\n", t_bytes);
 	fprintf(fp_result, "Throuhput = %f [bytes/s]\n", (double)t_bytes/t_total);
	fclose(fp_result);
	
	mol_exit(OK);
	
	return(OK);
}

/*===========================================================================*
 *				m3_socket					     *
 *===========================================================================*/
int m3_socket(int domain, int type, int protocol)
{
	int rcode; 
	
	webc_lpid = getpid();
	CMDDEBUG("webc_lpid=%d \n", webc_lpid);
	
#define WAIT4BIND_MS	1000
	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		CMDDEBUG("WEBCLT: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			CMDDEBUG("WEBCLT: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	webc_ep = mnx_getep(webc_lpid);
	CMDDEBUG("webc_ep=%d \n", webc_ep);
	
	msg_ptr = &webc_msg;
	msg_ptr->m_type  = WEB_SOCKET;
	rcode = mnx_sendrec(WEB_PROC_NR, msg_ptr);
	if(rcode < 0) {
		ERROR_PRINT(rcode);
		errno=rcode;
		return(-1);
	}
	if(msg_ptr->REP_STATUS < 0){
		ERROR_PRINT(msg_ptr->REP_STATUS);
		errno=msg_ptr->REP_STATUS;
		return(-1);	
	}
	return(msg_ptr->REP_STATUS);	// returns socket FD 		
}

/*===========================================================================*
 *				m3_connect					     *
 *===========================================================================*/
int m3_connect(int sockfd, const struct sockaddr *saddr, socklen_t addrlen)
{
	int rcode;
	struct sockaddr_in *saddri;

	saddri = (struct sockaddr_in *) saddr;
	CMDDEBUG("sockfd=%d \n", sockfd);
	msg_ptr->m_type  = WEB_CONNECT;
	msg_ptr->m2_i1   = sockfd;			// socket FD
	msg_ptr->m2_i2	 = saddri->sin_port; // server PORT
	msg_ptr->m2_l1   = saddri->sin_addr.s_addr; // server IP ADDR
	rcode = mnx_sendrec(WEB_PROC_NR, msg_ptr);
	if(rcode < 0) {
		ERROR_PRINT(rcode);
		errno=rcode;
		return(-1);
	}
	if(msg_ptr->REP_STATUS < 0){
		ERROR_PRINT(msg_ptr->REP_STATUS);
		errno=msg_ptr->REP_STATUS;
		return(-1);	
	}
	return(OK);
}

/*===========================================================================*
 *				m3_write					     *
 *===========================================================================*/
int m3_write(int sockfd, const void *buf, size_t count)
{
	int rcode;
	
	CMDDEBUG("sockfd=%d \n", sockfd);
	msg_ptr->m_type  = WEB_CLTWRITE;
	msg_ptr->m2_i1   = sockfd;
	msg_ptr->m2_i2	 = count; 
	msg_ptr->m2_p1	 = buf; 
	rcode = mnx_sendrec(WEB_PROC_NR, msg_ptr);
	if(rcode < 0) {
		ERROR_PRINT(rcode);
		errno=rcode;
		return(-1);
	}
	if(msg_ptr->REP_STATUS < 0){
		ERROR_PRINT(msg_ptr->REP_STATUS);
		errno=msg_ptr->REP_STATUS;
		return(-1);	
	}
	return(msg_ptr->REP_STATUS);
}

/*===========================================================================*
 *				m3_read					     *
 *===========================================================================*/
int m3_read(int sockfd, const void *buf, size_t count)
{
	int rcode;

	CMDDEBUG("sockfd=%d \n", sockfd);
	msg_ptr->m_type  = WEB_CLTREAD;
	msg_ptr->m2_i1   = sockfd;
	msg_ptr->m2_i2	 = count; 
	msg_ptr->m2_p1	 = buf; 
	rcode = mnx_sendrec(WEB_PROC_NR, msg_ptr);
	if(rcode < 0) {
		ERROR_PRINT(rcode);
		errno=rcode;
		return(-1);
	}
	if(msg_ptr->REP_STATUS < 0){
		ERROR_PRINT(msg_ptr->REP_STATUS);
		errno=msg_ptr->REP_STATUS;
		return(-1);	
	}
	return(msg_ptr->REP_STATUS);
}




