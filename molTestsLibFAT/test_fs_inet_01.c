
#define  MOL_USERSPACE	1
#define nil 0

#include "testlib.h"

char *buf_ptr;

// VARIABLES GLOBALES - NUEVO BINDING - START
#define WAIT4BIND_MS 1000
int dcid, clt_ep, clt_nr, clt_lpid;
int local_nodeid;
drvs_usr_t drvs, *drvs_ptr;
proc_usr_t fs, *fs_ptr;
proc_usr_t clt, *clt_ptr;
VM_usr_t  vmu, *dc_ptr;

#define LCL_TAP0_IP		"172.16.1.4"	
#define RMT_TAP0_IP		"172.16.1.9"	
#define BR0_TAP0_IP		"172.16.1.3"	
#define MASK_TAP0_IP	"255.255.255.0"	

/*===========================================================================*
 *				init_m3ipc					     *
 *===========================================================================*/
void init_m3ipc(void)
{
	int rcode;

	clt_lpid = getpid();
	do {
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("CLIENT mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("CLIENT mnx_wait4bind_T TIMEOUT\n");
			continue ;
		} else if ( rcode < 0)
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);

	SVRDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&drvs);
	if (rcode) ERROR_EXIT(rcode);
	drvs_ptr = &drvs;
	SVRDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(drvs_ptr));

	SVRDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if (rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	SVRDEBUG(VM_USR_FORMAT, VM_USR_FIELDS(dc_ptr));

	SVRDEBUG("Get FS info from SYSTASK\n");
	rcode = sys_getproc(&fs, FS_PROC_NR);
	if (rcode) ERROR_EXIT(rcode);
	fs_ptr = &fs;
	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(fs_ptr));
	if ( TEST_BIT(fs_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr, "FS not started\n");
		fflush(stderr);
		ERROR_EXIT(EMOLNOTBIND);
	}

	SVRDEBUG("Get Client info from SYSTASK\n");
	rcode = sys_getproc(&clt, SELF);
	if (rcode) ERROR_EXIT(rcode);
	clt_ptr = &clt;
	SVRDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(clt_ptr));

}

void mol_dirfile(const char *dname)
{
	DIR *ddir;
	struct dirent *dp;
	printf("mol_dirfile\t");
	printf(dname);
	ddir = mol_opendir(dname);
	if (ddir != 0) {
		// printf("ddir distinto de null!!!!");
		while ((dp = mol_readdir(ddir)) != 0)
		{
			printf("%s\n", dp->d_name);
			SVRDEBUG("e->d_name=%s\n",dp->d_name);
		}
		mol_closedir(ddir);
	}
	printf("\n\n");
}

void  main( int argc, char *argv[])
{
	int dcid;
	int clt_pid, svr_pid;
	int clt_nr, rcode;
	off_t pos;
	unsigned position;
	int i, fd;
	double t_start, t_stop, t_total;
	int buf_size, loops, bytes;
	FILE *fp_lcl;
	//Para STAT
	struct mnx_stat bufferStat;
	int retStat;
	//Para FCNTL
	int accmode, retFcntl;
	//Para OPENDIR
	DIR *dp;
	//Para READDIR
	struct dirent * de;
	char *user_path, *user_path0;
	//Para CLOSEDIR
	int resp_cd;
	struct nwio_ipconf *ipcf_ptr;

	init_m3ipc();

	SVRDEBUG("(%d)Starting %s in dcid=%d endpoint=%d\n",
				clt_lpid, argv[0], dc_ptr->dc_dcid, clt_ep);

	   
	/*---------------------- BUFFER ALIGNMENT-------------------*/
	rcode = posix_memalign( (void**) &buf_ptr, getpagesize(), buf_size);
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}
	SVRDEBUG("CLIENT buf_ptr address=%X\n", buf_ptr);

	SVRDEBUG("Alloc dynamic memory for user_path\n");
	posix_memalign( (void**) &user_path, getpagesize(), MNX_PATH_MAX);
 	if(user_path == NULL) ERROR_EXIT(errno);
	SVRDEBUG("CLIENT user_path=%X \n", user_path);
	
	SVRDEBUG("Alloc dynamic memory for user_path0\n");
	posix_memalign( (void**) &user_path0, getpagesize(), MNX_PATH_MAX);
 	if(user_path0 == NULL) ERROR_EXIT(errno);
	SVRDEBUG("CLIENT user_path0=%X \n", user_path0);
	
	/*----------------------MKDIR /dev -------------------*/
#define DEV_DIR		"/dev"
#define	DEV_MODE 	0755
	strcpy(user_path, DEV_DIR);
	fd = mol_mkdir(user_path, DEV_MODE);
	if( fd < 0 ) {
		fprintf(stderr,"mol_mkdir fd=%d errno=%d\n",fd, errno);
		exit(1);
	}
	SVRDEBUG("mol_mkdir fd=%d\n",fd);

	/*---------------------- MKNOD /dev/eth -------------------*/
#define INET_DEV_MAYOR 	INET_PROC_NR
#define ETH_NODE	"/dev/eth"
	strcpy(user_path, ETH_NODE);
	SVRDEBUG("mol_mknod %s mode=%o dev=%X \n",
		ETH_NODE, (I_CHAR_SPECIAL | 0666) , (INET_DEV_MAYOR *256) + ETH_DEV_MINOR);
		
	rcode = mol_mknod(user_path, I_CHAR_SPECIAL | 0666 , (INET_DEV_MAYOR *256) + ETH_DEV_MINOR);
	if( rcode != 0 ) {
		fprintf(stderr,"mol_mknod rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	SVRDEBUG("mol_mknod rcode=%d\n",rcode);	


	/*---------------------- MKNOD /dev/ip -------------------*/
#define IP_NODE		"/dev/ip"
	strcpy(user_path, IP_NODE);
	SVRDEBUG("mol_mknod %s mode=%o dev=%X \n",
		IP_NODE, (I_CHAR_SPECIAL | 0666) , (INET_DEV_MAYOR *256) + IP_DEV_MINOR);
		
	rcode = mol_mknod(user_path, I_CHAR_SPECIAL | 0666 , (INET_DEV_MAYOR *256) + IP_DEV_MINOR);
	if( rcode != 0 ) {
		fprintf(stderr,"mol_mknod rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	SVRDEBUG("mol_mknod rcode=%d\n",rcode);

	
	/*---------------------- LINK /dev/eth /dev/eth0 -------------------*/
#define ETH0_NODE	"/dev/eth0"
	strcpy(user_path, ETH_NODE);
	strcpy(user_path0, ETH0_NODE);
	rcode = mol_link(user_path, user_path0);
	if( rcode != 0 ) {
		fprintf(stderr,"mol_link rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	SVRDEBUG("mol_link rcode=%d\n",rcode);
	
	/*---------------------- LINK /dev/ip /dev/ip0 -------------------*/
#define IP0_NODE		"/dev/ip0"
	strcpy(user_path, IP_NODE);
	strcpy(user_path0, IP0_NODE);
	rcode = mol_link(user_path, user_path0);
	if( rcode != 0 ) {
		fprintf(stderr,"mol_link rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	SVRDEBUG("mol_link rcode=%d\n",rcode);
	
	
	strcpy(user_path, "/");
	mol_dirfile(user_path);
	
	strcpy(user_path, DEV_DIR);
	mol_dirfile(user_path);

	/*---------------------- OPEN /dev/ip -------------------*/
	int ip_fd; 
	
	strcpy(user_path, IP_NODE);
	ip_fd = mol_open(user_path, O_RDWR);
	if( ip_fd != 0 ) {
		fprintf(stderr,"mol_open user_path=%s ip_fd=%d errno=%d\n",
			user_path, ip_fd, errno);
		exit(1);
	}
	SVRDEBUG("ip_fd=%d\n",ip_fd);
	
	// ************************ IOCTL SET IP CONFIG  **************************
	
	ipcf_ptr = &user_path;
	ipcf_ptr->nwic_flags= (NWIC_IPADDR_SET | NWIC_NETMASK_SET |  NWIC_MTU_SET);
	ipcf_ptr->nwic_ipaddr	= inet_addr(LCL_TAP0_IP);
	ipcf_ptr->nwic_netmask = inet_addr(MASK_TAP0_IP);
	ipcf_ptr->nwic_mtu = 1490;

	SVRDEBUG("NWIOSIPCONF " IPCONF_FORMAT, IPCONF_FIELDS(ipcf_ptr));
	
	rcode = mol_ioctl(ip_fd, NWIOSIPCONF, ipcf_ptr);	
	if( rcode != 0 ) {
		fprintf(stderr,"mol_ioctl rcode=%d errno=%d\n",rcode, errno);
		exit(1);
	}
	SVRDEBUG("mol_ioctl rcode=%d\n",rcode);	
	
	exit(0);
 }

