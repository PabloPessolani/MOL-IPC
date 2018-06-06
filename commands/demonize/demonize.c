#define _TABLE
#include "demonize.h"

extern char *optarg;
extern int optind, opterr, optopt;

int mol_deamon_exit(int lnx_pid, int status);

dc_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid;

proc_usr_t deamon_usr, *deamon_ptr;
int dcid, nodeid, deamon_ep, deamon_nr, bind_type;
int thrower_lpid, thrower_pid, thrower_ep;
int deamon_lpid, mnxpid, deamon_nodeid;
int deamon_sts, deamon_lsts; 
char *const deamon_args;
proc_usr_t pm, *pm_ptr;
proc_usr_t rs, *rs_ptr;
int deamon_sd;
struct sockaddr_in rs_addr;
struct hostent *rs_host;

udp_buf_t udpbuf_in, udpbuf_out;
char  *cmd_line;
char  tty_name[MNX_PATH_MAX];

#define FORK_WAIT_MS 1000
#define	INIT_PID 		1

/*===========================================================================*
 *				check_PM					     *
 * Check for PM alive *
 *===========================================================================*/
int check_PM(void)
{
	int rcode; 

	CMDDEBUG("\n");
	pm_ptr = &pm;
	rcode = sys_getproc(pm_ptr,PM_PROC_NR);
	if (pm_ptr->p_rts_flags == SLOT_FREE){
		ERROR_EXIT(EMOLDEADSRCDST);
	} 
	CMDDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(pm_ptr));
	return (rcode);
}	

/*===========================================================================*
 *				check_RS					     *
 * Check for RS alive *
 *===========================================================================*/
int check_RS(void)
{
	int rcode; 

	CMDDEBUG("\n");
	rs_ptr = &rs;
	rcode = sys_getproc(rs_ptr,RS_PROC_NR);
	if (rs_ptr->p_rts_flags == SLOT_FREE){
		ERROR_EXIT(EMOLDEADSRCDST);
	} 
	CMDDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(rs_ptr));
	return (rcode);
}	

/*===========================================================================*
 *				get_dvs_params				     *
 *===========================================================================*/
void get_dvs_params(void)
{
	CMDDEBUG("\n");
	local_nodeid = mnx_getdvsinfo(&dvs);
	CMDDEBUG("local_nodeid=%d\n",local_nodeid);
	if( local_nodeid < DVS_NO_INIT) ERROR_EXIT(local_nodeid);
	dvs_ptr = &dvs;
	CMDDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));

}

/*===========================================================================*
 *				get_dc_params				     *
 *===========================================================================*/
void get_dc_params(int dcid)
{
	int rcode;

	CMDDEBUG("dcid=%d\n", dcid);
	if ( dcid < 0 || dcid >= dvs.d_nr_dcs) {
 	        printf( "Invalid dcid [0-%d]\n", dvs.d_nr_dcs );
 	        ERROR_EXIT(EMOLBADDCID);
	}
	rcode = mnx_getdcinfo(dcid, &dcu);
	if( rcode ) ERROR_EXIT(rcode);
	dc_ptr = &dcu;
	CMDDEBUG(DC_USR_FORMAT, DC_USR_FIELDS(dc_ptr));
}

print_usage(char *argv0){
	fprintf(stderr,"Usage: %s -<l|r|b> <hostname> <execnodeid> <dcid> <endpoint> <mpid> \"<command> <args...>\" \n", argv0 );
	fprintf(stderr,"<node> means in which node the command will be executed (referred to PM) \n");
	fprintf(stderr,"\t\t l: local bind \n");
	fprintf(stderr,"\t\t r: replica bind \n");
	fprintf(stderr,"\t\t b: backup bind \n");
	fprintf(stderr,"<hostname> of the RS process\n");
	fprintf(stderr,"<execnodeid> the node where to execute\n");
	fprintf(stderr,"<dcid>: DC ID for the process\n");
	fprintf(stderr,"<endpoint>: Endpoint to allocate for the process\n");
	fprintf(stderr,"for LOCAL operation: <mpid> MINIX PID desired to allocate. \n");
	fprintf(stderr,"\t (0) means that PM allocates any one\n");	
	fprintf(stderr,"If a <commmand> is not supplied, STDIN is read to get it\n");
	ERROR_EXIT(EMOLINVAL);	
}

/*===========================================================================*
 *				main					     					*
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int rcode, opt, i, arg_len, bytes, len;
	char *arg_ptr, *iptr, *optr;
	message *rqst_ptr,*reply_ptr;
    int bytes_to_read = _POSIX_ARG_MAX;
	int nbytes, was_space, fd_tty;
	
	if ( argc != 7 && argc != 8 ) {
		CMDDEBUG("argc=%d\n", argc);
		for( i = 0; i < argc ; i++) {
			CMDDEBUG("argv[%d]=%s\n", i, argv[i]);
		}
		print_usage("argv[0]");
 	    exit(1);
    }

	bind_type = SELF_BIND;
    while ((opt = getopt(argc, argv, "lrb")) != -1) {
        switch (opt) {
			case 'l':
				if( bind_type != SELF_BIND)
					print_usage(argv[0]);
				bind_type = LCL_BIND;
				break;
			case 'r':
				if( bind_type != SELF_BIND)
					print_usage(argv[0]);
				bind_type = REPLICA_BIND;
				break;
			case 'b':
				if( bind_type != SELF_BIND)
					print_usage(argv[0]);
				bind_type = BKUP_BIND;
				break;
			default: /* '?' */
				print_usage(argv[0]);
				break;
        }
    }
	
	CMDDEBUG("getopt bind_type=%d\n", bind_type);
	if( bind_type == SELF_BIND ) {
		print_usage(argv[0]);
	}
		
		
	get_dvs_params();
	deamon_nodeid = atoi(argv[3]);
	CMDDEBUG("deamon_nodeid=%d\n", deamon_nodeid);
	dcid = atoi(argv[4]);
	CMDDEBUG("dcid=%d\n", dcid);
	get_dc_params(dcid);
	check_PM();
	check_RS();
			
	deamon_ep = atoi(argv[5]);
	CMDDEBUG("deamon_ep=%d\n", deamon_ep);
	deamon_nr = _ENDPOINT_P(deamon_ep);
	if ( deamon_nr < 0 || deamon_nr >= (dc_ptr->dc_nr_sysprocs-dc_ptr->dc_nr_tasks) ) {
 	    fprintf(stderr, "Invalid <endpoint> [0-%d]\n", (dc_ptr->dc_nr_sysprocs-dc_ptr->dc_nr_tasks-1));
		ERROR_EXIT(EMOLBADPROC);
	}

	CMDDEBUG("deamon_nodeid=%d\n", deamon_nodeid);
	if( deamon_nodeid < 0 || deamon_nodeid >= dvs.d_nr_nodes) {
 	    fprintf(stderr, "Invalid <node> [0-%d]\n", (dvs.d_nr_nodes-1));
		ERROR_EXIT(EMOLBADNODEID)
	}

	if( !TEST_BIT(dcu.dc_nodes, deamon_nodeid)) {
 	    fprintf(stderr, "DC %d cannot run on node %d\n", dcid, deamon_nodeid);
		ERROR_EXIT(EMOLDCNODE);
	}

	mnxpid = atoi(argv[6]);
	CMDDEBUG("mnxpid=%d\n", mnxpid);
	nodeid = deamon_nodeid; 
	CMDDEBUG("nodeid=%d\n", nodeid);

	if( pm_ptr->p_nodeid ==  deamon_nodeid) { //  LOCAL OPERATION 		
		if ( mnxpid < 0 || mnxpid >= NR_PIDS ) { /* NR_PIDS from servers/pm/const.h */
			fprintf(stderr, "Invalid mnxpid=%d [0-%d]\n",mnxpid, (NR_PIDS-1));
			ERROR_EXIT(EMOLBADPID);
		}
	}else{		// REMOTE OPERATION 
		if( nodeid < 0 || nodeid >= dvs.d_nr_nodes) {
			fprintf(stderr, "Invalid <nodeid> [0-%d]\n", (dvs.d_nr_nodes-1));
			ERROR_EXIT(EMOLBADNODEID)
		}
		if( !TEST_BIT(dcu.dc_nodes, nodeid)) {
			fprintf(stderr, "DC %d cannot run on node %d\n", dcid, nodeid);
			ERROR_EXIT(EMOLDCNODE);
		}		
	}
			
	rcode = udp_init(argv[2]);
	if( rcode) {	
		CMDDEBUG("udp_init rcode=%d\n",rcode);
		ERROR_EXIT(rcode);
	}
	
	/* Dump the argv[]  into a copy buffer */
	CMDDEBUG("argc=%d \n",argc);

	arg_ptr = udpbuf_in.udp_u.mnx.arg_v;
	bzero(arg_ptr, _POSIX_ARG_MAX-sizeof(int)-sizeof(message));
	arg_len = 0;
	if( argc == 7) { //means open an input line 
//		if() {
//			dup2(fileno(someotherfile), STDOUT_FILENO);
//			dup2(fileno(somethirdopenfile), STDERR_FILENO);
//		}
		puts("Please enter a command:\n");
		nbytes = getline(&arg_ptr, &bytes_to_read, stdin);
		arg_ptr[nbytes] = 0; // Replace LF
		--nbytes;
	} else {
		strcpy(arg_ptr,argv[7]);
		nbytes = strlen(arg_ptr);
	}

	CMDDEBUG("nbytes=%d strlen=%d >%s<\n",nbytes, strlen(arg_ptr), arg_ptr);
#ifdef  TOKENIZE
	arg_len = 0;
	iptr = optr = arg_ptr;
	// convert a string into a sequence of strings (spaces replaced by zeros)
	for (was_space = TRUE; *iptr != 0; iptr++) {
		if (isspace(*iptr)) {
			if(was_space == FALSE){
				*optr = '\0';
				arg_len++;
				optr++;
			}
			was_space = TRUE;
		} else {
			was_space = FALSE;
			optr++;
			arg_len++;
		}
	}
	arg_ptr =  optr;
	arg_len++;

	// set a zero at the end of the sequence of strings 
	arg_ptr++;
	*arg_ptr = '\0';	
	arg_len++;
#else // TOKENIZE
	arg_len = nbytes;
#endif // TOKENIZE

	CMDDEBUG("arg_len=%d\n",arg_len);
	CMDDEBUG("arg_v= >%s<\n", udpbuf_in.udp_u.mnx.arg_v);

	/* build the request message to insert into the RS input message queue */
	rqst_ptr 	= &udpbuf_in.udp_u.mnx.mnx_msg;
	reply_ptr 	= &udpbuf_out.udp_u.mnx.mnx_msg;
	udpbuf_in.mtype 		= RS_UP;
	rqst_ptr->m_type 		= RS_UP;
	rqst_ptr->M7_ENDPT1		= deamon_ep;	// m7_i1	
	rqst_ptr->M7_BIND_TYPE 	= bind_type;	// m7_i2	
	if( bind_type == RMT_BIND || bind_type == BKUP_BIND)
		rqst_ptr->M7_NODEID		= nodeid;	// m7_i3		
	else
		rqst_ptr->M7_MNXPID = mnxpid;		// m7_i3
	rqst_ptr->M7_LEN		= (char * ) arg_len; // m7_i4
	rqst_ptr->M7_RUNNODE	= (int) deamon_nodeid; 	
	CMDDEBUG(MSG7_FORMAT, MSG7_FIELDS(rqst_ptr));
	bytes = arg_len + sizeof(message) + sizeof(int) + 1;
	
	/* rs_mq_in is the RS input message queue */
	CMDDEBUG("Sending request to RS bytes=%d\n",bytes);
    bytes = sendto(deamon_sd, &udpbuf_in, bytes, 0, (struct sockaddr*) &rs_addr, sizeof(rs_addr));
	if( bytes < 0) {
		CMDDEBUG("msgsnd errno=%d\n",errno);
		ERROR_EXIT(errno);
	}
		
	/* rs_mq_in is the RS output message queue */
	CMDDEBUG("Receiving reply from RS\n");
	len = sizeof(struct sockaddr_in);
   	bytes = recvfrom(deamon_sd, &udpbuf_out, sizeof(udp_buf_t), 
					0, (struct sockaddr*) &rs_addr, &len );
	if( bytes < 0) {
		CMDDEBUG("msgrcv errno=%d\n",errno);
		ERROR_EXIT(errno);
	}
	CMDDEBUG(MSG7_FORMAT, MSG7_FIELDS(reply_ptr));

	CMDDEBUG("m_type=0x%X bytes=%d\n", reply_ptr->m_type, bytes);	
	exit(0);
}

/*===========================================================================*
 *				udp_init					     *
 *===========================================================================*/
int udp_init( char *host_ptr)
{
	int rs_port, rcode, i;
    char rs_ipaddr[INET_ADDRSTRLEN+1];

	rs_port = RS_BASE_PORT + (dcid * dvs_ptr->d_nr_nodes) + local_nodeid;
	
	CMDDEBUG("RS may be listening on host %s at port=%d\n",  host_ptr, rs_port);    

    rs_addr.sin_family = AF_INET;  
    rs_addr.sin_port = htons(rs_port);  

    rs_host = gethostbyname(host_ptr);
	if( rs_host == NULL) ERROR_EXIT(errno);
	
	for( i =0; rs_host->h_addr_list[i] != NULL; i++) {
		CMDDEBUG("RS host address %i: %s\n", i, 
			inet_ntoa( *( struct in_addr*)(rs_host->h_addr_list[i])));
	}
	
    if((inet_pton(AF_INET,	
		inet_ntoa( *( struct in_addr*)(rs_host->h_addr_list[0])), 
		(struct sockaddr*) &rs_addr.sin_addr)) <= 0)
    	ERROR_RETURN(errno);

    inet_ntop(AF_INET, (struct sockaddr*) &rs_addr.sin_addr, rs_ipaddr, INET_ADDRSTRLEN);
	CMDDEBUG("RS is running on  %s at IP=%s\n", host_ptr, rs_ipaddr);    

    if ( (deamon_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
       	ERROR_EXIT(errno)
    }
	
	rcode = connect(deamon_sd, (struct sockaddr *) &rs_addr, sizeof(rs_addr));
    if (rcode != 0) ERROR_RETURN(errno);
    return(OK);

	return(OK);
}
