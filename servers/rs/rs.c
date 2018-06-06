
#define SVRDBG		1

#define _TABLE
#include "rs.h"

dc_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid, nodeid;
priv_usr_t *kpriv, *kv;
proc_usr_t pm, *pm_ptr;
int deamon_lsts; 

proc_usr_t deamon_usr;
proc_usr_t *deamon_ptr;
	
int rs_lpid, rs_ep, vmid;
message m_in, m_out;
int deamon_type, deamon_ep, deamon_nodeid;
int mnxpid, deamon_lpid, deamon_nr;
udp_buf_t udpbuf_in, udpbuf_out;

struct sockaddr_in rmtclient_addr, rmtserver_addr;
int    rs_sd;

void be_a_daemon(void);

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int rcode, bytes, len;
	message *mptr;
	char *ptr;
	
	if ( argc != 2) {
 	        SVRDEBUG( "Usage: %s <vmid> \n", argv[0] );
 	        exit(1);
    	}

	vmid = atoi(argv[1]);
	
	rs_init(vmid);
	
	rcode = udp_init();
	if( rcode) {	
		SVRDEBUG("udp_init rcode=%d\n",rcode);
		goto rs_exit;
	}
	
	// MAIN LOOP 
	len = sizeof(struct sockaddr_in);
	while(TRUE) {
		SVRDEBUG("Receiving command line \n");
		bzero( &udpbuf_in, sizeof(udp_buf_t));
		bytes = recvfrom(rs_sd, &udpbuf_in, sizeof(udp_buf_t), 0, 
					(struct sockaddr*) &rmtclient_addr, &len );
		if( bytes < 0) {
			SVRDEBUG("recvfrom errno=%d\n",errno);
			close(rs_sd);
			break;
		}
		SVRDEBUG("bytes=%d mtype=0x%X\n",bytes, udpbuf_in.mtype);
		mptr = &udpbuf_in.udp_u.mnx.mnx_msg;
		SVRDEBUG(MSG7_FORMAT, MSG7_FIELDS(mptr));
		
		rcode = check_PM();
		if(rcode == OK) {
			switch(udpbuf_in.mtype) {
				case RS_UP:
					rcode = deamonize();	
					// demonize return de MINIX PID > 0
					SVRDEBUG("deamonize rcode=%d\n",rcode);
					rcode = MOLTASK_REPLY;
					break;
				default:
					rcode = EMOLNOSYS;
					ERROR_PRINT(rcode);
					break;
			}
		}
		udpbuf_out.mtype = rcode; /* WARNING!! message type, must be > 0 */
		SVRDEBUG("Sending reply rcode=%d\n",rcode);
		bytes = sendto(rs_sd, &udpbuf_out, bytes, 0, (struct sockaddr*) &rmtclient_addr, sizeof(struct sockaddr_in));
		if( bytes < 0) {
			SVRDEBUG("msgsnd errno=%d\n",errno);
			goto rs_exit;
		}
	}
	close(rs_sd);
	
rs_exit:
	SVRDEBUG("RS SERVER %d EXITING rcode=%d\n",rs_lpid, rcode);
	exit(0);
}

/*===========================================================================*
 *				check_PM					     *
 * Check for PM alive *
 *===========================================================================*/
int check_PM(void)
{
	int rcode; 

	pm_ptr = &pm;
	rcode = sys_getproc(pm_ptr,PM_PROC_NR);
	if (pm_ptr->p_rts_flags == SLOT_FREE){
		rcode = EMOLDEADSRCDST;
	} 
	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(pm_ptr));
	return (rcode);
}	
/*===========================================================================*
 *				rs_init					     *
 *===========================================================================*/
int rs_init(int vmid)
{
  	int i, rcode, tab_len;
	char *sig_ptr;
	message *mptr;
	static proc_usr_t rrs, *rrs_ptr;	
	static char mess_sigs[] = { SIGTERM, SIGHUP, SIGABRT, SIGQUIT };

	SVRDEBUG("RS: address of m_in=%p m_out=%p\n",&m_in, &m_out);
	
	rs_lpid = getpid();
	mptr = &m_in;

	/* Bind RS to the kernel */
	SVRDEBUG("Binding process %d to DC%d with rs_nr=%d\n",rs_lpid,vmid,RS_PROC_NR);
	rs_ep = mnx_bind(vmid, RS_PROC_NR);
	
	if (rs_ep != RS_PROC_NR) {
		if(rs_ep == EMOLSLOTUSED){
			rrs_ptr = &rrs;
			rcode = mnx_getprocinfo(vmid, RS_PROC_NR, rrs_ptr);
			if(rcode < 0) ERROR_EXIT(rcode);
			SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(rrs_ptr));
			if( (TEST_BIT(rrs_ptr->p_rts_flags, BIT_REMOTE)) 
			 && (!TEST_BIT(rrs_ptr->p_misc_flags , MIS_BIT_RMTBACKUP))) {
				rcode = mnx_migr_start(vmid, RS_PROC_NR);
				if(rcode < 0) ERROR_EXIT(rcode);
				rcode = mnx_migr_commit(rs_lpid, vmid, RS_PROC_NR, local_nodeid);
				if(rcode < 0) ERROR_EXIT(rcode);
			}else{
				ERROR_EXIT(EMOLSLOTUSED);
			}
		} else {
			if(rs_ep < 0) ERROR_EXIT(rs_ep);
			ERROR_EXIT(EMOLBADPROC);
		}
	}
	
	SVRDEBUG("Get the DVS info from SYSTASK\n");
    rcode = sys_getkinfo(&dvs);
	if(rcode < 0) ERROR_RETURN(rcode);
	dvs_ptr = &dvs;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(dvs_ptr));

	SVRDEBUG("Get the DC info from SYSTASK\n");
	sys_getmachine(&dcu);
	if(rcode < 0 ) ERROR_RETURN(rcode);
	dc_ptr = &dcu;
	SVRDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	/* Register into SYSTASK (as an autofork) */
	SVRDEBUG("Register RS into SYSTASK rs_lpid=%d\n",rs_lpid);
	rs_ep = sys_bindproc(RS_PROC_NR, rs_lpid, SELF_BIND);
	if(rs_ep < 0) ERROR_RETURN(rs_ep);
	
	rcode = sys_rsetpname(rs_ep,  program_invocation_short_name, local_nodeid);
	if(rcode < 0) ERROR_RETURN(rcode);
			
	rcode = mol_bindproc(rs_ep, rs_ep, rs_lpid, LCL_BIND);
	if(rcode < 0) ERROR_RETURN(rcode);
		
	/* change PRIVILEGES of RS */
//	SVRDEBUG("change PRIVILEGES of RS\n");
//	rcode = sys_privctl(rs_ep, SERVER_PRIV);
//	if(rcode < 0) ERROR_RETURN(rcode);
	return(OK);
}

/*===========================================================================*
 *				udp_init					     *
 *===========================================================================*/
int udp_init( void)
{
	/* receiving message queue */
	int rs_port, ret; 
    struct sockaddr_in servaddr;
    int optval = 1;

	rs_port = RS_BASE_PORT + (vmid * dvs_ptr->d_nr_nodes) + local_nodeid;

	SVRDEBUG("RS: unning at port=%d\n", rs_port);

    // Create server socket.
    if ( (rs_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        ERROR_EXIT(errno);

    if( (ret = setsockopt(rs_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0)
       	ERROR_EXIT(errno);

    // Bind (attach) this process to the server socket.
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(rs_port);
   	ret = bind(rs_sd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(ret < 0) ERROR_EXIT(errno);

	SVRDEBUG("RS: is bound to port=%d socket=%d\n", rs_port, rs_sd);
	
	return(OK);
}

/*===========================================================================*
 *				deamonize     *
 * returns the MINIX PID of the started deamon (rcode > 0)
 * or an error code (rcode <= 0)
 *===========================================================================*/
int deamonize(void)
{
	message *rqst_ptr;
	int arg_len, rcode, i, child_pid, len;
	char *ptr;
static char *arg_v[MNX_MAX_ARGS];

	rqst_ptr= &udpbuf_in.udp_u.mnx.mnx_msg;
	SVRDEBUG(MSG7_FORMAT, MSG7_FIELDS(rqst_ptr));

	deamon_nodeid	= (int) rqst_ptr->M7_RUNNODE;
	deamon_type 	= rqst_ptr->M7_BIND_TYPE;
	deamon_ep 		= rqst_ptr->M7_ENDPT1;
	nodeid 			= rqst_ptr->M7_NODEID;	
	mnxpid 			= rqst_ptr->M7_MNXPID;
	arg_len			= rqst_ptr->M7_LEN;
		
	SVRDEBUG("deamon_nodeid=%d deamon_type=%d deamon_ep=%d\n",
		deamon_nodeid, deamon_type, deamon_ep);
	SVRDEBUG("nodeid=%d mnxpid=%d%d\n",
		nodeid, mnxpid);
	SVRDEBUG("arg_v=%s arg_len=%d\n", udpbuf_in.udp_u.mnx.arg_v, arg_len);

	deamon_nr 	= _ENDPOINT_P(deamon_ep);
	// Check for valid deamon type 
	if( deamon_type < LCL_BIND || deamon_type > MAX_BIND_TYPE){
		rcode = EMOLINVAL;
		ERROR_RETURN(rcode);
	}

	SVRDEBUG("deamon_ep=%d\n", deamon_ep);
	deamon_nr = _ENDPOINT_P(deamon_ep);
	if ( deamon_nr < 0 || deamon_nr >= (dc_ptr->dc_nr_sysprocs-dc_ptr->dc_nr_tasks) ) {
 	    fprintf(stderr, "Invalid <endpoint> [0-%d]\n", (dc_ptr->dc_nr_sysprocs-dc_ptr->dc_nr_tasks-1));
		ERROR_RETURN(EMOLBADPROC);
	}

	SVRDEBUG("deamon_nodeid=%d\n", deamon_nodeid);
	if( deamon_nodeid < 0 || deamon_nodeid >= dvs.d_nr_nodes) {
 	    fprintf(stderr, "Invalid <node> [0-%d]\n", (dvs.d_nr_nodes-1));
		ERROR_RETURN(EMOLBADNODEID);
	}

	if( !TEST_BIT(dcu.dc_nodes, deamon_nodeid)) {
 	    fprintf(stderr, "DC %d cannot run on node %d\n", vmid, deamon_nodeid);
		ERROR_RETURN(EMOLDCNODE);
	}

	if( pm_ptr->p_nodeid ==  deamon_nodeid) { //  LOCAL OPERATION 		
		SVRDEBUG("mnxpid=%d\n", mnxpid);
		if ( mnxpid < 0 || mnxpid >= NR_PIDS ) { /* NR_PIDS from servers/pm/const.h */
			fprintf(stderr, "Invalid mnxpid=%d [0-%d]\n",mnxpid, (NR_PIDS-1));
			ERROR_RETURN(EMOLBADPID);
		}
	}else{		// REMOTE OPERATION 
		SVRDEBUG("nodeid=%d\n", nodeid);
		if( nodeid < 0 || nodeid >= dvs.d_nr_nodes) {
			fprintf(stderr, "Invalid <nodeid> [0-%d]\n", (dvs.d_nr_nodes-1));
			ERROR_RETURN(EMOLBADNODEID);
		}
		if( !TEST_BIT(dcu.dc_nodes, nodeid)) {
			fprintf(stderr, "DC %d cannot run on node %d\n", vmid, nodeid);
			ERROR_RETURN(EMOLDCNODE);
		}		
	}
		
	/*---------------------------------------------------*/
	/*		REMOTE EXECUTION			*/
	/*---------------------------------------------------*/
	if( deamon_nodeid != pm_ptr->p_nodeid){
		SVRDEBUG("deamon_nodeid:%d  deamon_ep=%d cmd:%s arg_len=%d bytes\n", 
			deamon_nodeid, deamon_ep, udpbuf_in.udp_u.mnx.arg_v, arg_len);
		mnxpid = mol_rexec(deamon_nodeid, deamon_type, deamon_ep, udpbuf_in.udp_u.mnx.arg_v, arg_len);
		SVRDEBUG("mol_rexec mnxpid:%d\n", mnxpid);
		if( mnxpid < 0) ERROR_RETURN(errno);
		return(mnxpid);
	}
	
	/*---------------------------------------------------*/
	/*		LOCAL EXECUTION			*/
	/*---------------------------------------------------*/
	if ((child_pid = fork()) == 0) {     /* CHILD  */	
		be_a_daemon();
		deamon_lpid = getpid();
		SVRDEBUG("mnxpid:%d deamon_type=%d\n", mnxpid, deamon_type);
		switch(deamon_type){
			case LCL_BIND:
				rcode = mnx_lclbind(dc_ptr->dc_dcid,deamon_lpid,deamon_ep); 
				break;
			case REPLICA_BIND:
				rcode = mnx_replbind(dc_ptr->dc_dcid,deamon_lpid,deamon_ep); 
				break;
			case BKUP_BIND:
				rcode = mnx_bkupbind(dc_ptr->dc_dcid,deamon_lpid,deamon_ep,deamon_nodeid);
				break;
			default:
				SVRDEBUG("NEVER HERE!!\n");
				rcode = EMOLINVAL;
				break;
		}
//		rcode = sys_bindproc(deamon_ep, deamon_lpid, deamon_type);
		if(rcode < 0) ERROR_RETURN(rcode);

		/* Bind to PM  */
		SVRDEBUG("CHILD:  Bind to PM \n");
		if( deamon_type != BKUP_BIND ) {
			rcode = mol_bindproc(deamon_ep, mnxpid, deamon_lpid, deamon_type);
			if( rcode < 0) 
				ERROR_RETURN(rcode);			
			SVRDEBUG("Demonized process PID=%d\n",rcode);
		}
		
		SVRDEBUG("CHILD: sys_getproc \n");
		deamon_ptr = &deamon_usr;
		rcode = sys_getproc(deamon_ptr,PM_PROC_NR);
		if(rcode < 0) ERROR_RETURN(rcode);
		SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(deamon_ptr));
			
		ptr = udpbuf_in.udp_u.mnx.arg_v;
		SVRDEBUG("CHILD: arg_len=%d/%d execvpe >%s< \n", arg_len,
					strlen(ptr), ptr);

		i = 0;
		len = 0;
		do {
			arg_v[i] = strtok(ptr, " ");
			SVRDEBUG("CHILD: arg_v[%d]=%s\n", i, arg_v[i]);
			if( arg_v[i] == NULL) break;
			i++;
			ptr = NULL;
		}while(TRUE);

		if(i >= MNX_MAX_ARGS)
			ERROR_RETURN(EMOL2BIG);
		
		rcode = mol_setpname(deamon_ep, basename(arg_v[0]));
		if( rcode < 0) {
			mol_exit(rcode);
			ERROR_RETURN(rcode);
		}
		
		rcode = execvpe(arg_v[0], arg_v, NULL);
		
		/* this code only executes if execvp() failure */
		SVRDEBUG("CHILD: execvpe rcode=%d errno=%d\n", rcode, errno);
		SVRDEBUG("CHILD: execvpe %s\n", *strerror(errno));
		ERROR_RETURN(errno);
	}

	SVRDEBUG("PARENT waiting child exiting\n");
	wait(&child_pid);
	SVRDEBUG("exiting child=%d\n", child_pid);

	return(OK);
}
	
void be_a_daemon(void)
{
	int i,lfp;
	char str[10];
	
	SVRDEBUG("\n");

	if(getppid()==1) return; /* already a daemon */
	i=fork();
	if (i<0) exit(1); /* fork error */
	if (i>0) exit(0); /* parent exits */
	
	SVRDEBUG("pid=%d\n", getpid());

	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
//	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
//	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */
	umask(027); /* set newly created file permissions */
//	chdir(RUNNING_DIR); /* change running directory */
//	lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);
//	if (lfp<0) exit(1); /* can not open */
//	if (lockf(lfp,F_TLOCK,0)<0) exit(0); /* can not lock */
	/* first instance continues */
//	sprintf(str,"%d\n",getpid());
//	write(lfp,str,strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,SIG_IGN); /* catch hangup signal */
	signal(SIGTERM,SIG_IGN); /* catch kill signal */
}	
			