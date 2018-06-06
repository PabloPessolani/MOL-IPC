/* 
		HTTP over M3-IPC WEB SERVER

  *   m_type        m2_i1     m2_i2      m2_i3                m2_p1
 * |------------+----------+---------+----------+---------+---------|
 * |SOCKET|   | 		 |             |            |            |
 * |------------|----------|---------|----------|---------|---------|
 * |CONNECT| socket_fd | svrport  |     |         |            |
 * |------------|----------|---------|----------|---------|---------|
 * |CLTWRITE | socket_fd         |    bytes|             |            | get_addr |
 * |------------|----------|---------|----------|---------|---------|
 * |CLTREAD  | socket_fd        |     bytes|             |            | read_addr |
 * |------------|----------|---------|----------|---------|---------|
 */

 
#define _TABLE
//#define CMDDBG		1
#include "websrv.h"

struct {
  char *ext;
  char *filetype;
} extensions [] = {
  {"gif", "image/gif" },
  {"jpg", "image/jpg" },
  {"jpeg","image/jpeg"},
  {"png", "image/png" },
  {"ico", "image/ico" },
  {"zip", "image/zip" },
  {"gz",  "image/gz"  },
  {"tar", "image/tar" },
  {"htm", "text/html" },
  {"html","text/html" },
  {"txt","text/txt" },
  {0,0} };
  
void web_init(char *cfg_file);
void clear_session(sess_t *s_ptr);
void reset_session(sess_t *s_ptr);
void init_sess_table(void);
void init_web_table(void);
void web_reply(int code, int replyee, int proc_nr, int status);
int do_socket(message *m_ptr);
int do_connect(message *m_ptr);
int do_cltwrite(message *m_ptr);
int do_cltread(message *m_ptr);
int read_file(sess_t *s_ptr);
int web_server(int socket_fd, int rbytes);
int search_web_config(config_t *cfg);
int read_config(char *file_conf);

void usage(char* errmsg, ...) {
	if(errmsg) {
		printf("ERROR: %s\n", errmsg);
	} 
	fprintf(stderr, "Usage: web <config_file>\n");
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int rcode;
	sess_t *s_ptr;

	if ( argc != 2) {
		usage( "No arguments", optarg );
		exit(1);
	}
	
	rqst_ptr = &web_rqst;
	rply_ptr = &web_rply;
	
	web_init(argv[1]);
	
	while (TRUE) {
	
		sess_reset = 0; 
		
		CMDDEBUG("Waiting to receive a request message.\n");
		rcode = mnx_receive(ANY, rqst_ptr);
		if (rcode != 0){
			fprintf(stderr,"receive failed with %d", rcode);
			ERROR_EXIT(rcode);
		}
		CMDDEBUG(MSG2_FORMAT, MSG2_FIELDS(rqst_ptr));

		/* Execute the requested device driver function. */
		switch (rqst_ptr->m_type) {
			case WEB_SOCKET:	
				rcode = do_socket(rqst_ptr);  
				break;
			case WEB_CONNECT:	
				rcode = do_connect(rqst_ptr);  
				break;
			case WEB_CLTWRITE:	
				rcode = do_cltwrite(rqst_ptr);  
				break;
			case WEB_CLTREAD:	
				rcode = do_cltread(rqst_ptr);  
				break;
			default:		
				fprintf(stderr,"Warning, WEB got unexpected request %d from %d\n",
					rqst_ptr->m_type, rqst_ptr->m_source);
				rcode = EMOLNOSYS;
				break;
		}
		web_reply(MOLTASK_REPLY, rqst_ptr->m_source,
						rqst_ptr->IO_ENDPT, rcode);
		if( sess_reset ){
			CMDDEBUG("sess_reset=%d\n",sess_reset)
			s_ptr  = &sess_table[rqst_ptr->m2_i1];
			CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));
			reset_session(s_ptr);
		}
		
  	}
	return(OK);				
}

/*===========================================================================*
 *				web_init					     *
 *===========================================================================*/
 void web_init(char *cfg_file)
 {
 	int rcode;
    config_t *cfg;

 	web_lpid = getpid();
	CMDDEBUG("web_lpid=%d cfg_file=%s\n", web_lpid, cfg_file);
	
#define WAIT4BIND_MS	1000
	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		CMDDEBUG("WEBSRV: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			CMDDEBUG("WEBSRV: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	CMDDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&dvs);
	if(rcode) ERROR_EXIT(rcode);
	dvs_ptr = &dvs;
	CMDDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(dvs_ptr));
	
	CMDDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	CMDDEBUG(VM_USR_FORMAT,VM_USR_FIELDS(dc_ptr));

	CMDDEBUG("Get WEB_PROC_NR info from SYSTASK\n");
	rcode = sys_getproc(&proc_web, WEB_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	web_ptr = &proc_web;
	CMDDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(web_ptr));
	if( TEST_BIT(web_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"WEBSRV task not started\n");
		fflush(stderr);		
		ERROR_EXIT(EMOLNOTBIND);
	}
	
	#define nil ((void*)0)
	cfg= nil;
	CMDDEBUG("cfg_file=%s\n", cfg_file);
	cfg = config_read(cfg_file, CFG_ESCAPED, cfg);
	
#ifdef DYNAMIC_ALLOC 	
	/* alloc dynamic memory for the WEBSRV table */
	CMDDEBUG("Alloc dynamic memory for the WEBSRV  table NR_WEBSRVS=%d\n", NR_WEBSRVS);
	posix_memalign( (void **) &web_table, getpagesize(), sizeof(web_t)*NR_WEBSRVS );
	if (web_table== NULL) {
   		ERROR_EXIT(-errno);
	}
	
	/* alloc dynamic memory for the SESSION table */
	CMDDEBUG("Alloc dynamic memory for the SESSION  table NR_SESSIONS=%d\n", NR_SESSIONS);
	posix_memalign( (void **) &sess_table, getpagesize(), sizeof(sess_t)* NR_SESSIONS);
	if (sess_table== NULL) {
   		ERROR_EXIT(-errno);
	}

	/* alloc dynamic memory for the WEBSRV message  */
	CMDDEBUG("Alloc dynamic memory for the WEBSRV  request \n");
	posix_memalign( (void **) &web_rqst, getpagesize(), sizeof(message) );
	if (web_rqst== NULL) {
   		ERROR_EXIT(-errno);
	}
	CMDDEBUG("Alloc dynamic memory for the WEBSRV  reply \n");
	posix_memalign( (void **) &web_rply, getpagesize(), sizeof(message) );
	if (web_rply== NULL) {
   		ERROR_EXIT(-errno);
	}
#endif // DYNAMIC_ALLOC 	
	
	who_e = who_p = NONE;

	init_web_table();     /* initialize WEB table */
	cfg_web_nr=0; 
	
	CMDDEBUG("before search_web_config\n");	
	rcode = search_web_config(cfg);
	if(rcode) ERROR_EXIT(rcode);
		
	if (rcode || cfg_web_nr==0 ) {
		CMDDEBUG("Configuration error: cfg_web_nr=%d\n", cfg_web_nr);        
		ERROR_EXIT(rcode);
	}else{
		CMDDEBUG("cfg_web_nr=%d\n", cfg_web_nr);        		
	}
}

/*===========================================================================*
 *				clear_session				     *
 *===========================================================================*/
void clear_session(sess_t *s_ptr)
{
	if(s_ptr->clt_ep != NONE && s_ptr->clt_ep != 0)
		CMDDEBUG("clt_ep=%d\n", s_ptr->clt_ep);
	s_ptr->status 		= STS_CLOSED;	
	s_ptr->next			= 0;
	s_ptr->outtotal		= 0;
	s_ptr->filelen		= 0;
	if( s_ptr->file_fd != (-1))
		close(s_ptr->file_fd);	
	s_ptr->file_fd		= (-1);
}


/*===========================================================================*
 *				reset_session				     *
 *===========================================================================*/
void reset_session(sess_t *s_ptr)
{
	if(s_ptr->clt_ep != NONE && s_ptr->clt_ep != 0)
		CMDDEBUG("clt_ep=%d\n", s_ptr->clt_ep);        		
	s_ptr->clt_ep 		= NONE;	
	s_ptr->clt_port		= 0;
	s_ptr->w_ptr 		= NULL;	
	clear_session(s_ptr);
}

/*===========================================================================*
 *				init_sess_table				     *
 *===========================================================================*/
void init_sess_table(void)
{
	/* Initialize session table  */
	sess_t *s_ptr;
	int i;

	CMDDEBUG("NR_SESSIONS=%d\n", NR_SESSIONS);        		

	/* Initialize the terminal lines. */
	for (i=0; i < NR_SESSIONS; i++) {
		s_ptr = &sess_table[i];
		sess_reset = 1;
	}
	CMDDEBUG("\n");        		
}

/*===========================================================================*
 *				init_web_table				     *
 *===========================================================================*/
void init_web_table(void)
{
	/* Initialize web structure and call device initialization routines. */
	web_t *webp;
	int i;

	CMDDEBUG("NR_WEBSRVS=%d\n", NR_WEBSRVS);        		
	/* Initialize the terminal lines. */
	for (i=0; i < NR_WEBSRVS; i++) {
		webp = &web_table[i];
		webp->svr_name 	= NULL;				
		webp->rootdir	= NULL;
		memset(webp->inbuf, 0, WEBMAXBUF);
		memset(webp->outbuf, 0, WEBMAXBUF);
		webp->svr_ep 	= web_ptr->p_endpoint;	
		memset(&webp->svr_addr, 0, sizeof(struct sockaddr_in));
	}
}

/*===========================================================================*
 *				web_reply				     *
 *===========================================================================*/
void web_reply(int code, int replyee, int proc_nr, int status)
{
	int rcode;
	
	/* Send a reply to a process that wanted to read or write data. */

	rply_ptr->m_type = code;
	rply_ptr->REP_ENDPT = proc_nr;
	rply_ptr->REP_STATUS = status;

	CMDDEBUG(MSG2_FORMAT, MSG2_FIELDS(rply_ptr));

	if ((rcode = mnx_send(replyee, rply_ptr)) != OK) {
		fprintf(stderr, "WEBSRV: couldn't reply to %d. rcode=%d\n", 
			replyee, rcode);
		ERROR_EXIT(rcode);
	}
}

/*===========================================================================*
 *				do_socket				     *
 *===========================================================================*/
int do_socket(message *m_ptr)
{
	int rcode, clt_nr;
  	sess_t *s_ptr;

	clt_nr = _ENDPOINT_P(m_ptr->m_source);
	s_ptr  = &sess_table[clt_nr+dc_ptr->dc_nr_tasks];
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));
	
	if( s_ptr->status != STS_CLOSED)
		ERROR_RETURN(EMOLADDRNOTAVAIL);
	
	s_ptr->status = STS_WAIT4CONN;
	s_ptr->clt_ep = m_ptr->m_source;
	rcode = clt_nr+dc_ptr->dc_nr_tasks; // este es el equivalente del FD 
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));

	CMDDEBUG("rcode=%d\n", rcode);
	return(rcode); // return Socket FD 
}

/*===========================================================================*
 *				do_connect				     *
 *===========================================================================*/
int do_connect(message *m_ptr)
{
	int clt_nr, socket_fd, i;
  	sess_t *s_ptr;
	
	clt_nr = _ENDPOINT_P(m_ptr->m_source);
	socket_fd = m_ptr->m2_i1;		// Socket FD
	s_ptr  = &sess_table[clt_nr+dc_ptr->dc_nr_tasks];
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));
	
	// verify socket FD
	if( (clt_nr+dc_ptr->dc_nr_tasks) != socket_fd){
		sess_reset = 1;
		ERROR_RETURN(EMOLBADF);
	}
	
	// check for correct session state 
	if( s_ptr->status != STS_WAIT4CONN){
		sess_reset = 1;
		return(EMOLBADF);
	}
	
	// check for correct web server (pair {IPADDR, PORT})
	for( i = 0; i < NR_WEBSRVS; i++){
		if( (web_table[i].svr_addr.sin_port == m_ptr->m2_i2) && // Server TCP Port
			(web_table[i].svr_addr.sin_addr.s_addr == m_ptr->m2_l1 )){ // Server IP ADDR
			s_ptr->w_ptr = &web_table[i];
			break;
		}
	}
	if( i == NR_WEBSRVS) {
		sess_reset = 1;
		return(EMOLCONNREFUSED);
	}
	CMDDEBUG(WEB_FORMAT, WEB_FIELDS(s_ptr->w_ptr));

	s_ptr->clt_port = m_ptr->m2_i2; // fill session with client port. Not used
	s_ptr->status = STS_WAIT4WRITE;
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));
	return(OK);
}

/*===========================================================================*
 *				do_cltwrite				     *
 *===========================================================================*/
int do_cltwrite(message *m_ptr)
{
	int socket_fd, ibytes, clt_nr, bytes;
	char *get_addr;
  	sess_t *s_ptr;

	clt_nr = _ENDPOINT_P(m_ptr->m_source);
	socket_fd = m_ptr->m2_i1;		// Socket FD
	s_ptr  = &sess_table[clt_nr+dc_ptr->dc_nr_tasks];
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));
	
	// verify socket FD
	if( (clt_nr+dc_ptr->dc_nr_tasks) != socket_fd){
		sess_reset = 1;
		ERROR_RETURN(EMOLBADF);
	}
		
	// check for correct session state 
	if( s_ptr->status != STS_WAIT4WRITE){
		sess_reset = 1;
		return(EMOLBADF);
	}
	
	// check for correct buffer size 
	bytes 	= m_ptr->m2_i2;
	get_addr=  m_ptr->m2_p1;
	if( bytes < 0 || bytes > WEBMAXBUF) { //obtener este parametro del DVS
		sess_reset = 1;
		return(EMOLFBIG);
	}
		
	// copy from client write buffer to server input buffer  	
	ibytes = sys_vircopy(m_ptr->m_source, get_addr, 
					SELF, s_ptr->w_ptr->inbuf, bytes);
	CMDDEBUG("ibytes=%d\n", ibytes);
	if(ibytes == 0) {
		s_ptr->status = STS_WAIT4READ;
		CMDDEBUG("inbuf=%s\n",  s_ptr->w_ptr->inbuf);
	}
	return(ibytes);
}


/*===========================================================================*
 *				do_cltread									     *
 *===========================================================================*/
int do_cltread(message *m_ptr)
{
	int socket_fd, rcode, rbytes, wbytes, clt_nr;
	char *read_addr;
  	sess_t *s_ptr;

	clt_nr = _ENDPOINT_P(m_ptr->m_source);
	socket_fd = m_ptr->m2_i1;
	s_ptr  = &sess_table[socket_fd];
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));

	// verify socket FD
	if( (clt_nr+dc_ptr->dc_nr_tasks) != socket_fd){
		sess_reset = 1;
		ERROR_RETURN(EMOLBADF);
	}
		
	// check for correct session state 	
	if( s_ptr->status != STS_WAIT4READ &&
		s_ptr->status != STS_FILEREAD ){
		sess_reset = 1;
		return(EMOLBADF);
	}

	// check for correct buffer size 
	rbytes 	= m_ptr->m2_i2;
	read_addr=  m_ptr->m2_p1;
	if( rbytes < 0 || rbytes > WEBMAXBUF) {
		sess_reset = 1;
		return(EMOLFBIG);
	}
	
	// call web server 
	wbytes = web_server(socket_fd, rbytes);
	CMDDEBUG("wbytes=%d\n", wbytes);

	if(wbytes > 0 ){
		// copy from server output buffer to client read buffer 
		rcode = sys_vircopy(SELF, s_ptr->w_ptr->outbuf, 
					m_ptr->m_source, read_addr, wbytes);
		CMDDEBUG("rcode=%d\n", rcode);
		if(rcode){
			sess_reset = 1;
			return(rcode);		
		}
		rcode = wbytes;
		// account the total number of bytes sent to client 
		if( s_ptr->status == STS_FILEREAD ){
			s_ptr->outtotal += wbytes;
//			if( s_ptr->outtotal >= s_ptr->filelen)
				// all bytes has been sent - clear session
//				clear_session(s_ptr);
		}else{
			s_ptr->status = STS_FILEREAD;
		}
	}else{
		rcode = wbytes;
		sess_reset = 1;
	}
	CMDDEBUG("rcode=%d\n", rcode);
	return(rcode);
}

/*===========================================================================*
 *				read_file									     *
 *===========================================================================*/
int read_file(sess_t *s_ptr)
{
	int flen, bytes;
	
	CMDDEBUG(SESS_FORMAT, SESS_FIELDS(s_ptr));

	flen = lseek(s_ptr->file_fd, s_ptr->outtotal, SEEK_SET); 
	if( flen < 0) ERROR_RETURN(-errno);
	
	bytes = read(s_ptr->file_fd, 
				(s_ptr->w_ptr->outbuf +  s_ptr->next), 
				(WEBMAXBUF-s_ptr->next));
	if( bytes < 0) ERROR_RETURN(-errno);
	return(bytes);
}

/*===========================================================================*
 *				web_server					     *
 *===========================================================================*/
/* this is a child web server process, so we can exit on errors */
int web_server(int socket_fd, int rbytes)
{
	char *in_ptr, *out_ptr, *fstr;
	int wbytes, fbytes, i, buflen, len, root_len;
  	sess_t *s_ptr;
	double t_start, t_stop, t_total;
	long total_bytes;
	FILE *fp;
	
	CMDDEBUG("socket_fd=%d rbytes=%d\n", socket_fd, rbytes);

	t_start = dwalltime();

	s_ptr  = &sess_table[socket_fd];
	if( s_ptr->status == STS_FILEREAD) { 
		s_ptr->next = 0;
		fbytes = read_file(s_ptr);
		if(fbytes < 0) ERROR_RETURN(fbytes);
		return(fbytes);
	}

	if( s_ptr->status != STS_WAIT4READ) { 
		ERROR_RETURN(EMOLBADF);
	}
	
	in_ptr = s_ptr->w_ptr->inbuf;
	for(i=0;i<rbytes;i++)	/* remove CF and LF characters */
		if(in_ptr[i] == '\r' || in_ptr[i] == '\n')
			in_ptr[i]='*';

	in_ptr = s_ptr->w_ptr->inbuf;
	if( strncmp(in_ptr,"GET ",4) && 
		strncmp(in_ptr,"get ",4) ){
		ERROR_RETURN(EMOLINVAL);
	}

	for(i=4;i<WEBMAXBUF;i++) { /* null terminate after the second space to ignore extra stuff */
		if(in_ptr[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			in_ptr[i] = 0;
			break;
		}
	}

	in_ptr = s_ptr->w_ptr->inbuf;
	for(i=0;i<i-1;i++) {  /* check for illegal parent directory use .. */
		if(in_ptr[i] == '.' && in_ptr[i+1] == '.') {
		ERROR_RETURN(EMOLNOENT);
		}
	}
	
	in_ptr = s_ptr->w_ptr->inbuf;
	if( !strncmp(&in_ptr[0],"GET /\0",6) || 
		!strncmp(&in_ptr[0],"get /\0",6) ) /* convert no filename to index file */
		(void)strcpy(in_ptr,"GET /index.html");
	
	CMDDEBUG("in_ptr=%s\n", in_ptr);

	/* work out the file type and check we support it */
	buflen=strlen(in_ptr);
	fstr = NULL;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&in_ptr[buflen-len], extensions[i].ext, len)) {
		fstr =extensions[i].filetype;
		break;
		}
	}
	if(fstr == NULL) return(EMOLBADF);
	CMDDEBUG("fstr=%s\n", fstr);

	// get memory for the full pathname 
	posix_memalign( (void **) &s_ptr->pathname, getpagesize(), MNX_PATH_MAX);
	if (s_ptr->pathname == NULL) {
   		ERROR_EXIT(-errno);
	}
	
	// root directory len
	root_len = strlen(s_ptr->w_ptr->rootdir);
	// if the last char is an slash, remove is
	if(s_ptr->w_ptr->rootdir[root_len-1] == '/')
		s_ptr->w_ptr->rootdir[root_len-1] = 0;
	
	CMDDEBUG("rootdir=%s\n", s_ptr->w_ptr->rootdir);
	
	// build the complete file pathname 
	sprintf(s_ptr->pathname, "%s%s", s_ptr->w_ptr->rootdir, &in_ptr[4]);
	CMDDEBUG("pathname=%s\n", s_ptr->pathname);

    if(( s_ptr->file_fd = open(s_ptr->pathname,O_RDONLY)) == -1) {  /* open the file for reading */
		free(s_ptr->pathname);
		ERROR_RETURN(-errno);
	}
	free(s_ptr->pathname);
	
	/* Clean the html page buffer */
	out_ptr = s_ptr->w_ptr->outbuf;
	memset(out_ptr,0, WEBMAXBUF);
	
	s_ptr->filelen = (long)lseek(s_ptr->file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
	(void)lseek(s_ptr->file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
 
	(void)sprintf(out_ptr,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", VERSION, s_ptr->filelen, fstr); /* Header + a blank line */
	
	wbytes = strlen(out_ptr);
#ifdef ANULADO	
	s_ptr->next = wbytes;
	if( rbytes < wbytes){
		fbytes = read_file(s_ptr);
		if(fbytes < 0)
			ERROR_RETURN(fbytes);
		else
			wbytes += fbytes;
	}
#endif // ANULADO	
	CMDDEBUG("wbytes=%d\n", wbytes);

	t_stop = dwalltime();
	/*--------- Report statistics  ---------*/
	t_total = (t_stop-t_start);
	fp = fopen("websrv.txt","a+");
	fprintf(fp, "t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
	fprintf(fp, "total_bytes = %ld\n", total_bytes);
	fprintf(fp, "Throuhput = %f [bytes/s]\n", (double)(total_bytes)/t_total);
	fclose(fp);
		
	return(wbytes);
}



