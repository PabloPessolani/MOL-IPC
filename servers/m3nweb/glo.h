/* EXTERN should be extern except for the table file */

int nweb_lpid;		
int nweb_ep;		
dvs_usr_t dvs, *dvs_ptr;
dc_usr_t  dcu, *dc_ptr;
proc_usr_t proc_web, *nweb_ptr;	
int local_nodeid;
unsigned int mandatory;
int listenfd;
struct sockaddr_in cli_addr; /* static = initialised to zeros */
struct sockaddr_in nweb_addr; /* static = initialised to zeros */
struct mnx_stat *fstat_ptr;
char *in_buf;
char *out_buf;

int nweb_port;
char *nweb_name;
char *nweb_rootdir;		// root directory for this server





