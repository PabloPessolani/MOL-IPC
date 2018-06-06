/* This file contains the device dependent part of the drivers for the
*						TTY DEVICE DRIVER
 * The valid messages and their parameters are:
 *   HARD_INT:       output has been completed or input has arrived
 *   SYS_SIG:      e.g., MINIX wants to shutdown; run code to cleanly stop
 *   DEV_READ:       a process wants to read from a terminal
 *   DEV_WRITE:      a process wants to write on a terminal
 *   DEV_IOCTL:      a process wants to change a terminal's parameters
 *   DEV_OPEN:       a tty line has been opened
 *   DEV_CLOSE:      a tty line has been closed
 *   DEV_SELECT:     start select notification request
 *   DEV_STATUS:     TTY wants to know status for SELECT or MOLREVIVE
 *   CANCEL:         terminate a previous incomplete system call immediately
 *
 *    m_type      TTY_LINE   IO_ENDPT    COUNT   TTY_SPEK  TTY_FLAGS  ADDRESS
 * ---------------------------------------------------------------------------
 * | HARD_INT    |         |         |         |         |         |         |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | SYS_SIG     | sig set |         |         |         |         |         |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | DEV_READ    |minor dev| proc nr |  count  |         O_NONBLOCK| buf ptr |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | DEV_WRITE   |minor dev| proc nr |  count  |         |         | buf ptr |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | DEV_IOCTL   |minor dev| proc nr |func code|erase etc|  flags  |         |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | DEV_OPEN    |minor dev| proc nr | O_NOCTTY|         |         |         |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | DEV_CLOSE   |minor dev| proc nr |         |         |         |         |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | DEV_STATUS  |         |         |         |         |         |         |
 * |-------------+---------+---------+---------+---------+---------+---------|
 * | CANCEL      |minor dev| proc nr |         |         |         |         |
 * ---------------------------------------------------------------------------
 *
 */
 
 
#define _TABLE
//#define TASKDBG		1
#include "tty.h"

int init_tty_drivers(void);
int init_tty_m3ipc(void );
int init_tty_tcpip(void );
int init_tty_pseudo(void );
void tty_init(char *cfg_file);
void init_tty_table(void);
int search_tty_config(config_t *cfg);
int tty_devnop(tty_t *tp, int try);
void tty_reply(int code, int replyee, int proc_nr, int status);
void expire_timers(void);

void do_read(tty_t *tp, message *m_ptr);
void do_status(message *m_ptr);
void do_write(tty_t *tp, message *m_ptr);
void do_ioctl(tty_t *tp, message *m_ptr);
void do_open(tty_t *tp, message *m_ptr);
void do_close(tty_t *tp, message *m_ptr);
void do_cancel(tty_t *tp, message *m_ptr);
void do_select(tty_t *tp, message *m_ptr);
int select_retry(struct tty *tp);
int select_try(struct tty *tp, int ops);
void settimer(tty_t *tty_ptr,int enable);
void handle_events(tty_t *tp);
void in_transfer(tty_t *tp);
int in_process(tty_t *tp, char *buf, int count);
void tty_timed_out(moltimer_t *tp);

int tty_icancel(tty_t *tp);
int setattr(tty_t *tp);
int sigchar(tty_t *tp, int sig);

int do_open_pseudo(tty_t *tp);
static void pseudo_out_thread(void *arg);
static void pseudo_in_thread(void *arg);
int pseudo_read(tty_t *tp, int true);
int read_config(char *file_conf);


#define tty_addr(line)	(&tty_table[line])
#define tty_active(tp)	((tp)->tty_devread != NULL)

/* Default attributes. */
mnx_winsize_t winsize_defaults;	/* = all zeroes */
mnx_termios_t termios_defaults = {
  TINPUT_DEF, TOUTPUT_DEF, TCTRL_DEF, TLOCAL_DEF, TSPEED_DEF, TSPEED_DEF,
  {
	TEOF_DEF, TEOL_DEF, TERASE_DEF, TINTR_DEF, TKILL_DEF, TMIN_DEF,
	TQUIT_DEF, TTIME_DEF, TSUSP_DEF, TSTART_DEF, TSTOP_DEF,
	TREPRINT_DEF, TLNEXT_DEF, TDISCARD_DEF,
  },
};


void usage(char* errmsg, ...) {
	if(errmsg) {
		printf("ERROR: %s\n", errmsg);
	} 
	fprintf(stderr, "Usage: tty <config_file>\n");
}

/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int rcode, line;
	tty_t *tp;

	if ( argc != 2) {
		usage( "No arguments", optarg );
		exit(1);
	}
	
	rqst_ptr = &tty_rqst;
	rply_ptr = &tty_rply;
	
	/* Initialize tty_t data structures and */
	/* then fills them with data from 		*/
	/* configuration file					*/
	tty_init(argv[1]);
	
	/* for every configured device initialize*/
	/* the driver (LOCAL, TCPIP, M3IPC		*/
	rcode = init_tty_drivers();
	if (rcode ) {
		TASKDEBUG("init_tty_devices rcode=%d\n", rcode);        
		ERROR_RETURN(rcode);
	}

	while (TRUE) {
	
		TASKDEBUG("Waiting to receive a request message.\n");
		rcode = mnx_receive(ANY, rqst_ptr);
		if (rcode != 0){
			fprintf(stderr,"receive failed with %d", rcode);
			ERROR_EXIT(rcode);
		}
		TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(rqst_ptr));
	
#ifdef ANULADO	
	/* First handle all kernel notification types that the TTY supports. 
	 *  - An alarm went off, expire all timers and handle the events. 
	 *  - A hardware interrupt also is an invitation to check for events. 
	 *  - A new kernel message is available for printing.
	 *  - Reset the console on system shutdown. 
	 * Then see if this message is different from a normal device driver
	 * request and should be handled separately. These extra functions
	 * do not operate on a device, in constrast to the driver requests. 
	 */
		switch (rqst_ptr->m_type) { 
			case SYN_ALARM: 		/* fall through */
				expire_timers();	/* run watchdogs of expired timers */
				continue;			/* contine to check for events */
			case DEV_PING:
				mnx_notify(rqst_ptr->m_source);
				continue;
			case HARD_INT: 		/* hardware interrupt notification */
				if (rqst_ptr->NOTIFY_ARG & kbd_irq_set)
					kbd_interrupt(rqst_ptr);/* fetch chars from keyboard */
		#if NR_RS_LINES > 0
				if (rqst_ptr->NOTIFY_ARG & rs_irq_set)
					rs_interrupt(rqst_ptr);/* serial I/O */
		#endif
				expire_timers();	/* run watchdogs of expired timers */
				continue;		/* contine to check for events */
			case PROC_EVENT: 
				cons_stop();		/* switch to primary console */
				TASKDEBUG("TTY got PROC_EVENT, assuming SIGTERM\n");
				continue;
			case SYS_SIG: 			/* system signal */
				sigset_t sigset = (sigset_t) rqst_ptr->NOTIFY_ARG;
				if (sigismember(&sigset, SIGKMESS)) do_new_kmess(rqst_ptr);
				continue;
			case DIAGNOSTICS: 		/* a server wants to print some */
				do_diagnostics(rqst_ptr);
				continue;
			case GET_KMESS:
				do_get_kmess(rqst_ptr);
				continue;
			case FKEY_CONTROL:		/* (un)register a fkey observer */
				do_fkey_ctl(rqst_ptr);
				continue;
			default:			/* should be a driver request */
				;			/* do nothing; end switch */
		}

		/* Only device requests should get to this point. All requests, 
		 * except DEV_STATUS, have a minor device number. Check this
		 * exception and get the minor device number otherwise.
		 */
		if (rqst_ptr->m_type == DEV_STATUS) {
			do_status(rqst_ptr);
			continue;
		}
#endif // ANULADO	
		
		line = rqst_ptr->TTY_LINE;
		tp = tty_addr(line);

		/* If the device doesn't exist or is not configured return ENXIO. */
		if (tp == NULL || !tty_active(tp)) {
			fprintf(stderr, "Warning, TTY got illegal request %d from %d\n",
				rqst_ptr->m_type, rqst_ptr->m_source);
			if (rqst_ptr->m_source != LOG_PROC_NR){
				tty_reply(MOLTASK_REPLY, rqst_ptr->m_source,
							rqst_ptr->IO_ENDPT, ENXIO);
			}
			continue;
		}

		/* Execute the requested device driver function. */
		switch (rqst_ptr->m_type) {
			case DEV_READ:	 do_read(tp, rqst_ptr);	  break;
			case DEV_WRITE:	 do_write(tp, rqst_ptr);  break;
			case DEV_IOCTL:	 do_ioctl(tp, rqst_ptr);  break;
			case DEV_OPEN:	 do_open(tp, rqst_ptr);	  break;
			case DEV_CLOSE:	 do_close(tp, rqst_ptr);  break;
			case DEV_SELECT: do_select(tp, rqst_ptr); break;
			case CANCEL:	 do_cancel(tp, rqst_ptr); break;
			default:		
				fprintf(stderr,"Warning, TTY got unexpected request %d from %d\n",
					rqst_ptr->m_type, rqst_ptr->m_source);
				tty_reply(MOLTASK_REPLY, rqst_ptr->m_source,
								rqst_ptr->IO_ENDPT, EINVAL);
				break;
		}
  	}
	return(OK);				
}

/*===========================================================================*
 *				tty_init					     *
 *===========================================================================*/
 void tty_init(char *cfg_file)
 {
 	int rcode;
    config_t *cfg;

 	tty_lpid = getpid();
	TASKDEBUG("tty_lpid=%d cfg_file=%s\n", tty_lpid, cfg_file);
	
#define WAIT4BIND_MS	1000
	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		TASKDEBUG("TTY: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			TASKDEBUG("TTY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	TASKDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&drvs);
	if(rcode) ERROR_EXIT(rcode);
	drvs_ptr = &drvs;
	TASKDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(drvs_ptr));
	
	TASKDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	TASKDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	TASKDEBUG("Get TTY_PROC_NR info from SYSTASK\n");
	rcode = sys_getproc(&proc_tty, TTY_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	tty_ptr = &proc_tty;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(tty_ptr));
	if( TEST_BIT(tty_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"TTY task not started\n");
		fflush(stderr);		
		ERROR_EXIT(EMOLNOTBIND);
	}
	
	#define nil ((void*)0)
	cfg= nil;
	TASKDEBUG("cfg_file=%s\n", cfg_file);
	cfg = config_read(cfg_file, CFG_ESCAPED, cfg);
	
#ifdef DYNAMIC_ALLOC 	
	/* alloc dynamic memory for the TTY table */
	TASKDEBUG("Alloc dynamic memory for the TTY  table NR_VTTYS=%d\n", NR_VTTYS);
	posix_memalign( (void **) &tty_table, getpagesize(), sizeof(tty_t) );
	if (tty_table== NULL) {
   		ERROR_EXIT(errno);
	}

	/* alloc dynamic memory for the TTY message  */
	TASKDEBUG("Alloc dynamic memory for the TTY  request \n");
	posix_memalign( (void **) &tty_rqst, getpagesize(), sizeof(message) );
	if (tty_rqst== NULL) {
   		ERROR_EXIT(errno);
	}
	TASKDEBUG("Alloc dynamic memory for the TTY  reply \n");
	posix_memalign( (void **) &tty_rply, getpagesize(), sizeof(message) );
	if (tty_rply== NULL) {
   		ERROR_EXIT(errno);
	}
#endif // DYNAMIC_ALLOC 	
	
	who_e = who_p = NONE;

	init_tty_table();     /* initialize device table and map boot driver */
	cfg_tty_nr=0; 
	
	TASKDEBUG("before  search_tty_config\n");	
	rcode = search_tty_config(cfg);
	if(rcode) ERROR_EXIT(rcode);
		
	if (rcode || cfg_tty_nr==0 ) {
		TASKDEBUG("Configuration error: cfg_tty_nr=%d\n", cfg_tty_nr);        
		ERROR_EXIT(rcode);
	}else{
		TASKDEBUG("cfg_tty_nr=%d\n", cfg_tty_nr);        		
	}
}

/*===========================================================================*
 *				init_tty_table				     *
 *===========================================================================*/
void init_tty_table(void)
{

	/* Initialize tty structure and call device initialization routines. */

	register tty_t *tp;
	int i;

	TASKDEBUG("NR_VTTYS=%d\n", NR_VTTYS);        		

	/* Initialize the terminal lines. */
	for (i=0; i < NR_VTTYS; i++) {
		tp = tty_addr(i);
		tp->tty_minor 	= TTY_NO_MINOR;
		tp->tty_index 	= i;
		tp->tty_intail 	= tp->tty_inhead = tp->tty_inbuf;
		tp->tty_min 	= 1;
		tp->tty_termios = termios_defaults;
		tp->tty_icancel = tp->tty_ocancel 
						= tp->tty_ioctl 
						= tp->tty_close 
						= tty_devnop;
		tp->tty_eotct		= (-1);	// means that no accounting was done on input buffer
		tp->tty_cfg.type 	= TTY_NONE;				
		tp->tty_cfg.minor 	= TTY_NO_MINOR;	
		tp->tty_pseudo.master 	= TTY_NO_MINOR;	
		tp->tty_pseudo.slave 	= TTY_NO_MINOR;	
		strcpy(tp->tty_pseudo.sname,"NO_PSEUDO_NAME");	
	}
	TASKDEBUG("\n");        		

}

/*===========================================================================*
 *				init_tty_drivers			     *
 *===========================================================================*/
int init_tty_drivers(void)
{

	tty_t *tp;
	int i, rcode = OK;
	unsigned int bm_drivers;
	
	TASKDEBUG("\n");        		

	bm_drivers = 0; // bitmap of configured devices 
	for (i=0; i < NR_VTTYS; i++) {
		tp = tty_addr(i);
		if( tp->tty_minor != TTY_NO_MINOR);
			SET_BIT(bm_drivers, tp->tty_cfg.type);
	}

	TASKDEBUG("bm_drivers=%X\n", bm_drivers);        		
	
	if( TEST_BIT(bm_drivers, TTY_PSEUDO))
		rcode = init_tty_pseudo();
	if( TEST_BIT(bm_drivers, TTY_TCPIP))
		rcode = init_tty_tcpip();
	if( TEST_BIT(bm_drivers, TTY_M3IPC))
		rcode = init_tty_m3ipc();
	if(rcode) ERROR_RETURN(rcode);
	return(OK);
}


/*===========================================================================*
 *				init_tty_pseudo			     *
 *===========================================================================*/
int init_tty_pseudo(void)
{
	int i, tty_nr;
	tty_t *tp;

	TASKDEBUG("\n");        		

	for (i=0 ,tty_nr=0; i < NR_VTTYS; i++) {
		tp = tty_addr(i);
		if( tp->tty_minor != TTY_NO_MINOR);
			if(tp->tty_cfg.type == TTY_PSEUDO){
				// HERE CALL THE INITIALIZATION FUNCTION FOR DEVICE TYPE
				tp->tty_devread = pseudo_read; 	
				tp->tty_pseudo.in_count= 0;
				tp->tty_pseudo.in_next = tp->tty_pseudo.in_buf;
				tty_nr++;
			}
	}
	assert( tty_nr > 0 );
	TASKDEBUG("LOCAL tty_nr=%d\n", tty_nr);        		
	return(OK);
}

/*===========================================================================*
 *				init_tty_tcpip			     *
 *===========================================================================*/
int init_tty_tcpip(void )
{
	int i, tty_nr;
	tty_t *tp;

	TASKDEBUG("\n");        		

	for (i=0 ,tty_nr=0; i < NR_VTTYS; i++) {
		tp = tty_addr(i);
		if( tp->tty_minor != TTY_NO_MINOR);
			if(tp->tty_cfg.type == TTY_TCPIP){
				// HERE CALL THE INITIALIZATION FUNCTION FOR DEVICE TYPE
				tp->tty_devread = tty_devnop; // TEST ONLY
				tty_nr++;
			}
	}
	assert( tty_nr > 0 );
	TASKDEBUG("TCPIP tty_nr=%d\n", tty_nr);        		
	return(OK);
}

/*===========================================================================*
 *				init_tty_m3ipc			     *
 *===========================================================================*/
int init_tty_m3ipc(void )
{
	int i, tty_nr;
	tty_t *tp;

	TASKDEBUG("\n");        		

	for (i=0 ,tty_nr=0; i < NR_VTTYS; i++) {
		tp = tty_addr(i);
		if( tp->tty_minor != TTY_NO_MINOR);
			if(tp->tty_cfg.type == TTY_M3IPC){
				// HERE CALL THE INITIALIZATION FUNCTION FOR DEVICE TYPE
				tp->tty_devread = tty_devnop; // TEST ONLY		
				tty_nr++;
			}
	}
	assert( tty_nr > 0 );
	TASKDEBUG("M3-IPC tty_nr=%d\n", tty_nr);        		
	return(OK);
}

/*===========================================================================*
 *				sigchar				     *
 *===========================================================================*/
int sigchar(tty_t *tp, int sig)
{
  /* Some functions need not be implemented at the device level. */
  	TASKDEBUG("\n"); 
	return(OK);
}


/*===========================================================================*
 *				setattr				     *
 *===========================================================================*/
int setattr(tty_t *tp)
{
  /* Some functions need not be implemented at the device level. */
  	TASKDEBUG("\n"); 
	return(OK);
}

/*===========================================================================*
 *				tty_icancel				     *
 *===========================================================================*/
int tty_icancel(tty_t *tp)
{
  /* Some functions need not be implemented at the device level. */
  	TASKDEBUG("\n"); 
	return(OK);
}

/*===========================================================================*
 *				tty_devnop				     *
 *===========================================================================*/
int tty_devnop(tty_t *tp, int try)
{
  /* Some functions need not be implemented at the device level. */
  	TASKDEBUG("\n"); 
	return(OK);
}


/*===========================================================================*
 *				expire_timers			    	     *
 *===========================================================================*/
 void expire_timers(void)
{
	/* A synchronous alarm message was received. Check if there are any expired 
	 * timers. Possibly set the event flag and reschedule another alarm.  
	 */
	molclock_t now;				/* current time */
	int rcode;

	int sys_getuptime(molclock_t *ticks);

	/* Get the current time to compare the timers against. */
	if ((rcode=sys_getuptime(&now)) != OK){
		fprintf(stderr,"sys_getuptime't get uptime from clock. rcode=%d", rcode);
		ERROR_EXIT(rcode);
	}
	
	tty_next_timeout = tty_timers->tmr_exp_time;
	if ((rcode=sys_setalarm(tty_next_timeout, 1)) != OK){
		fprintf(stderr, "Couldn't set synchronous alarm. rcode=%d", rcode);
		ERROR_EXIT(rcode);
	}			
}

/*===========================================================================*
 *				tty_reply				     *
 * int code;		MOLTASK_REPLY or MOLREVIVE 
 * int replyee;	destination address for the reply 
 * int proc_nr;	to whom should the reply go? 
 *  int status;	reply code 
 *===========================================================================*/
void tty_reply(int code, int replyee, int proc_nr, int status)
{
	int rcode;
	
	/* Send a reply to a process that wanted to read or write data. */

	rply_ptr->m_type = code;
	rply_ptr->REP_ENDPT = proc_nr;
	rply_ptr->REP_STATUS = status;

	TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(rply_ptr));

	if ((rcode = mnx_send(replyee, rply_ptr)) != OK) {
		fprintf(stderr, "TTY: couldn't reply to %d. rcode=%d\n", 
			replyee, rcode);
		ERROR_EXIT(rcode);
	}
}


/*===========================================================================*
 *				do_status				     *
 *===========================================================================*/
void do_status(message *m_ptr)
{
	struct tty *tp;
	int event_found;
	int status;
	int ops, i;

	TASKDEBUG("\n");

	  /* Check for select or revive events on any of the ttys. If we found an, 
	   * event return a single status message for it. The FS will make another 
	   * call to see if there is more.
	   */
	event_found = 0;
  	for (i=0; i < NR_VTTYS; i++) {
		tp = tty_addr(i);
		if ((ops = select_try(tp, tp->tty_select_ops)) && 
				tp->tty_select_proc == m_ptr->m_source) {

			/* I/O for a selected minor device is ready. */
			rply_ptr->m_type = DEV_IO_READY;
			rply_ptr->DEV_MINOR = tp->tty_minor;
			rply_ptr->DEV_SEL_OPS = ops;

			tp->tty_select_ops &= ~ops;	/* unmark select event */
			event_found = 1;
			break;
		} else if (tp->tty_inrevived && tp->tty_incaller == m_ptr->m_source) {
			
			/* Suspended request finished. Send a MOLREVIVE. */
			rply_ptr->m_type = DEV_REVIVE;
			rply_ptr->REP_ENDPT = tp->tty_inproc;
			rply_ptr->REP_STATUS = tp->tty_incum;

			tp->tty_inleft = tp->tty_incum = 0;
			tp->tty_inrevived = 0;		/* unmark revive event */
			tp->tty_inrepcode = 0;
			event_found = 1;
			break;
		} else if (tp->tty_outrevived && tp->tty_outcaller == m_ptr->m_source) {
			
			/* Suspended request finished. Send a MOLREVIVE. */
			rply_ptr->m_type = DEV_REVIVE;
			rply_ptr->REP_ENDPT = tp->tty_outproc;
			rply_ptr->REP_STATUS = tp->tty_outcum;

			tp->tty_outcum = 0;
			tp->tty_outrevived = 0;		/* unmark revive event */
			tp->tty_inrepcode = 0;
			event_found = 1;
			break;
		}
  }

	if (! event_found) {
		/* No events of interest were found. Return an empty message. */
		rply_ptr->m_type = DEV_NO_STATUS;
	}

	/* Almost done. Send back the reply message to the caller. */
	TASKDEBUG(MSG2_FORMAT, MSG2_FIELDS(rply_ptr));
	if ((status = mnx_send(m_ptr->m_source, rply_ptr)) != OK) {
		fprintf(stderr, "Send in do_status failed, status=%d \n", status);
		ERROR_EXIT(status);
	}
}

/*===========================================================================*
 *				do_read					     *
 *===========================================================================*/
void do_read(tty_t *tp, message *m_ptr)
{
	/* A process wants to read from a terminal. */
	int rcode;

	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));

	 /* Check if there is already a process hanging in a read, check if the
	 * parameters are correct, do I/O.
	 */
	if (tp->tty_inleft > 0) {
		TASKDEBUG("do_read: EMOLIO\n");
		rcode = EMOLIO;
	} else if (m_ptr->COUNT <= 0) {
		TASKDEBUG("do_read: EMOLINVAL\n");
		rcode = EMOLINVAL;
	} else {
		/* Copy information from the message to the tty struct. */
		tp->tty_inrepcode 	= MOLTASK_REPLY;
		tp->tty_incaller 	= m_ptr->m_source;
		tp->tty_inproc 		= m_ptr->IO_ENDPT;
		tp->tty_in_vir 		= (vir_bytes) m_ptr->ADDRESS;
		tp->tty_inleft 		= m_ptr->COUNT;

		if (!(tp->tty_termios.c_lflag & ICANON)
			&& tp->tty_termios.c_cc[VTIME] > 0) {
			if (tp->tty_termios.c_cc[VMIN] == 0) {
				/* MIN & TIME specify a read timer that finishes the
				 * read in TIME/10 seconds if no bytes are available.
				 */
				settimer(tp, TRUE);
				tp->tty_min = 1;
			} else {
				/* MIN & TIME specify an inter-byte timer that may
				 * have to be cancelled if there are no bytes yet.
				 */
				if (tp->tty_eotct == 0) {
					settimer(tp, FALSE);
					tp->tty_min = tp->tty_termios.c_cc[VMIN];
				}
			}
		}
		
		tp->tty_devread(tp,TRUE);
		
		/* Anything waiting in the input buffer? Clear it out... */
		in_transfer(tp);
		
		/* ...then go back for more. */
//		handle_events(tp);
		if (tp->tty_inleft == 0)  {
			if (tp->tty_select_ops)
				select_retry(tp);
			return;			/* already done */
		}

		/* There were no bytes in the input queue available, so either suspend
		 * the caller or break off the read if nonblocking.
		 */
		if (m_ptr->TTY_FLAGS & O_NONBLOCK) {
			rcode = EMOLAGAIN;				/* cancel the read */
			tp->tty_inleft = tp->tty_incum = 0;
		} else {
			rcode = SUSPEND;				/* suspend the caller */
			tp->tty_inrepcode = MOLREVIVE;
		}
	}
	TASKDEBUG("do_read: replying %d\n", rcode);
 	tty_reply(MOLTASK_REPLY, m_ptr->m_source, m_ptr->IO_ENDPT, rcode);
	if (tp->tty_select_ops)
		select_retry(tp);
}

/*===========================================================================*
 *				do_write				     *
 *===========================================================================*/
void do_write(tty_t *tp, message *m_ptr)
{
	/* A process wants to write on a terminal. */
	int rcode;
  
	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));

  /* Check if there is already a process hanging in a write, check if the
   * parameters are correct, do I/O.
   */
	if (tp->tty_outleft > 0) {
		rcode = EMOLIO;
	} else if (m_ptr->COUNT <= 0 || m_ptr->COUNT > TTY_OUT_BYTES ) {
		rcode = EMOLINVAL;
	} else 	{
	
		MTX_LOCK(tp->tty_Omutex);
		
		/* Copy message parameters to the tty structure. */
		tp->tty_outrepcode 	= MOLTASK_REPLY;
		tp->tty_outcaller 	= m_ptr->m_source;
		tp->tty_outproc 	= m_ptr->IO_ENDPT;
		tp->tty_out_vir 	= (vir_bytes) m_ptr->ADDRESS;
		tp->tty_outleft		= m_ptr->COUNT;

		// copy output data from userspace to tty output buffer
		rcode = sys_vircopy(m_ptr->IO_ENDPT, m_ptr->ADDRESS,
							SELF,  tp->tty_outbuf,
							m_ptr->COUNT);

		COND_SIGNAL(tp->tty_Obarrier);				// signal OUTPUT Thread
		COND_WAIT(tp->tty_Mbarrier, tp->tty_Omutex); // wait for the reply 

		/* None or not all the bytes could be written, so either suspend the
		 * caller or break off the write if nonblocking.
		 */
		if (m_ptr->TTY_FLAGS & O_NONBLOCK) {		/* cancel the write */
			rcode = tp->tty_outcum > 0 ? tp->tty_outcum : EAGAIN;
			tp->tty_outleft = tp->tty_outcum = 0;
		} else {
			rcode = SUSPEND;				/* suspend the caller */
			tp->tty_outrepcode = MOLREVIVE;
		}
	}
	tty_reply(MOLTASK_REPLY, m_ptr->m_source, m_ptr->IO_ENDPT, rcode);
}

/*===========================================================================*
 *				do_ioctl				     *
 *===========================================================================*/
void do_ioctl(tty_t *tp, message *m_ptr)
{
/* Perform an IOCTL on this terminal. Posix termios calls are handled
 * by the IOCTL system call
 */
	tty_conf_t *cfg_ptr;
	tty_pseudo_t *pse_ptr;

  int rcode = OK;
  union {
	int i;
#if ENABLE_SRCCOMPAT
	struct sgttyb sg;
	struct tchars tc;
#endif
  } param;
  size_t size;

  	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));

  /* Size of the ioctl parameter. */
  switch (m_ptr->TTY_REQUEST) {
    case TCGETS:        /* Posix tcgetattr function */
    case TCSETS:        /* Posix tcsetattr function, TCSANOW option */ 
    case TCSETSW:       /* Posix tcsetattr function, TCSADRAIN option */
    case TCSETSF:	/* Posix tcsetattr function, TCSAFLUSH option */
        size = sizeof(struct mnx_termios_s);
        break;

    case TCSBRK:        /* Posix tcsendbreak function */
    case TCFLOW:        /* Posix tcflow function */
    case TCFLSH:        /* Posix tcflush function */
    case TIOCGPGRP:     /* Posix tcgetpgrp function */
    case TIOCSPGRP:	/* Posix tcsetpgrp function */
        size = sizeof(int);
        break;

    case TIOCGWINSZ:    /* get window size (not Posix) */
    case TIOCSWINSZ:	/* set window size (not Posix) */
        size = sizeof(mnx_winsize_t);
        break;

#if ENABLE_SRCCOMPAT
    case TIOCGETP:      /* BSD-style get terminal properties */
    case TIOCSETP:	/* BSD-style set terminal properties */
	size = sizeof(struct sgttyb);
	break;

    case TIOCGETC:      /* BSD-style get terminal special characters */ 
    case TIOCSETC:	/* BSD-style get terminal special characters */ 
	size = sizeof(struct tchars);
	break;
#endif
	case TIOCGETCFG:		// MOL VTTY configuration 
		size = sizeof(struct tty_conf_s);
		break;

	case TIOCGETPSE:		// MOL VTTY PSEudo terminal 
		size = sizeof(struct tty_pseudo_s);
		break;
		
    case TCDRAIN:	/* Posix tcdrain function -- no parameter */
    default:		size = 0;
  }

  rcode = OK;
  switch (m_ptr->TTY_REQUEST) {
    case TCGETS:
	/* Get the termios attributes. */
	rcode = sys_vircopy(SELF, (void *) &tp->tty_termios,
		m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS, 
		(vir_bytes) size);
	break;

    case TCSETSW:
    case TCSETSF:
    case TCDRAIN:
	if (tp->tty_outleft > 0) {
		/* Wait for all ongoing output processing to finish. */
		tp->tty_iocaller = m_ptr->m_source;
		tp->tty_ioproc = m_ptr->IO_ENDPT;
		tp->tty_ioreq = m_ptr->REQUEST;
		tp->tty_iovir = (vir_bytes) m_ptr->ADDRESS;
		rcode = SUSPEND;
		break;
	}
	if (m_ptr->TTY_REQUEST == TCDRAIN) break;
	if (m_ptr->TTY_REQUEST == TCSETSF) tty_icancel(tp);
	/*FALL THROUGH*/
    case TCSETS:
	/* Set the termios attributes. */
	rcode = sys_vircopy( m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		SELF,(void *) &tp->tty_termios, (vir_bytes) size);
	if (rcode != OK) break;
	setattr(tp);
	break;

    case TCFLSH:
	rcode = sys_vircopy( m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		SELF, (void *) &param.i, (vir_bytes) size);
	if (rcode != OK) break;
	switch (param.i) {
	    case TCIFLUSH:	tty_icancel(tp);		 	    break;
	    case TCOFLUSH:	(*tp->tty_ocancel)(tp, 0);		    break;
	    case TCIOFLUSH:	tty_icancel(tp); (*tp->tty_ocancel)(tp, 0); break;
	    default:		rcode = EINVAL;
	}
	break;

    case TCFLOW:
	rcode = sys_vircopy( m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		SELF, (void *) &param.i, (vir_bytes) size);
	if (rcode != OK) break;
	switch (param.i) {
	    case TCOOFF:
	    case TCOON:
		tp->tty_inhibited = (param.i == TCOOFF);
		tp->tty_events = 1;
		break;
	    case TCIOFF:
		(*tp->tty_echo)(tp, tp->tty_termios.c_cc[VSTOP]);
		break;
	    case TCION:
		(*tp->tty_echo)(tp, tp->tty_termios.c_cc[VSTART]);
		break;
	    default:
		rcode = EINVAL;
	}
	break;

    case TCSBRK:
	if (tp->tty_break != NULL) (*tp->tty_break)(tp,0);
	break;

    case TIOCGWINSZ:
	rcode = sys_vircopy(SELF, (void *) &tp->tty_winsize,
		m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS, 
		(vir_bytes) size);
	break;

    case TIOCSWINSZ:
	rcode = sys_vircopy( m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		SELF, (void *) &tp->tty_winsize, (vir_bytes) size);
	sigchar(tp, SIGWINCH);
	break;

#if ENABLE_SRCCOMPAT
    case TIOCGETP:
	compat_getp(tp, &param.sg);
	rcode = sys_vircopy(SELF, (void *) &param.sg,
		m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		(vir_bytes) size);
	break;

    case TIOCSETP:
	rcode = sys_vircopy( m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		SELF, (void *) &param.sg, (vir_bytes) size);
	if (rcode != OK) break;
	compat_setp(tp, &param.sg);
	break;

    case TIOCGETC:
	compat_getc(tp, &param.tc);
	rcode = sys_vircopy(SELF, (void *) &param.tc,
		m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS, 
		(vir_bytes) size);
	break;

    case TIOCSETC:
	rcode = sys_vircopy( m_ptr->IO_ENDPT, (void *) m_ptr->ADDRESS,
		SELF, (void *) &param.tc, (vir_bytes) size);
	if (rcode != OK) break;
	compat_setc(tp, &param.tc);
	break;
#endif
	

	case TIOCGETCFG:			// MOL VTTY configuration 
		cfg_ptr = &tp->tty_cfg;
		TASKDEBUG(TCONF_FORMAT, TCONF_FIELDS(cfg_ptr));
		rcode = sys_vircopy( SELF, (void *) &tp->tty_cfg, 
						m_ptr->m_source, (void *) m_ptr->ADDRESS,
							(vir_bytes) size);
		if (rcode != OK) 
			ERROR_PRINT(rcode);
		break;

	case TIOCGETPSE:			// MOL VTTY pseudoterminal 
		pse_ptr = &tp->tty_pseudo;
		TASKDEBUG(PSEUDO_FORMAT, PSEUDO_FIELDS(pse_ptr));
		rcode = sys_vircopy( SELF, (void *) &tp->tty_pseudo, 
							m_ptr->m_source, (void *) m_ptr->ADDRESS,
							(vir_bytes) size);
		if (rcode != OK) 
			ERROR_PRINT(rcode);
		break;		
/* These Posix functions are allowed to fail if _POSIX_JOB_CONTROL is 
 * not defined.
 */
    case TIOCGPGRP:     
    case TIOCSPGRP:	
    default:
#if ENABLE_BINCOMPAT
	do_ioctl_compat(tp, m_ptr);
	return;
#else
	rcode = ENOTTY;
#endif
  }

  /* Send the reply. */
  tty_reply(MOLTASK_REPLY, m_ptr->m_source, m_ptr->IO_ENDPT, rcode);
}

/*===========================================================================*
 *				do_open					     *
 *===========================================================================*/
void do_open(tty_t *tp, message *m_ptr)
{
	/* A tty line has been opened.  Make it the callers controlling tty if
	* O_NOCTTY is *not* set and it is not the log device.  1 is returned if
	* the tty is made the controlling tty, otherwise OK or an error code.
	*/
	int rcode = OK;

  	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));
	
	if(tp->tty_openct == 0) {
		switch(tp->tty_cfg.type){
			case TTY_PSEUDO:
				rcode = do_open_pseudo(tp);
				break;
			case TTY_TCPIP: 
				break;
			case TTY_M3IPC: 
				break;		
			default:
				ERROR_EXIT(EMOLINVAL);
				break;
		}
		
		if (!(m_ptr->COUNT & O_NOCTTY)) {
			tp->tty_pgrp = m_ptr->IO_ENDPT;
			rcode = 1;
		}
	}	
	tp->tty_openct++;
  
	tty_reply(MOLTASK_REPLY, m_ptr->m_source, m_ptr->IO_ENDPT, rcode);
}

/*===========================================================================*
 *				do_open_pseudo				     *
 *===========================================================================*/
int do_open_pseudo(tty_t *tp)
{
	int rcode, fdm, fds;
	tty_pseudo_t *tps;
	
	tps = &tp->tty_pseudo;
	
	// set the minor number 
	tp->tty_minor = tp->tty_cfg.minor;
	
	fdm = posix_openpt(O_RDWR); 
	if (fdm < 0) { 
		fprintf(stderr, "Error %d on posix_openpt()\n", errno); 
		ERROR_RETURN(errno); 
	} 
	
	rcode = grantpt(fdm); 
	if (rcode != 0) { 
		fprintf(stderr, "Error %d on grantpt()\n", errno); 
		ERROR_RETURN(errno); 
	} 

	rcode = unlockpt(fdm); 
	if (rcode != 0) { 
		fprintf(stderr, "Error %d on unlockpt()\n", errno); 
		ERROR_RETURN(errno); 
	} 
 
	//Open the slave side ot the PTY
	fds=open(ptsname(fdm),O_RDWR);
	if (fds < 0) { 
		fprintf(stderr, "Error %d open slave\n", errno); 
		ERROR_RETURN(errno); 
	} 
	
	tps->master = fdm;
	tps->slave  = fds;
	strncpy(tps->sname, ptsname(fdm), MNX_PATH_MAX-1);
  	TASKDEBUG(PSEUDO_FORMAT,PSEUDO_FIELDS(tps));

	//Close the slave side of the PTY
	close(fds);
	
	rcode = pthread_cond_init(&tp->tty_Ibarrier, NULL);
	if(rcode) ERROR_EXIT(rcode);

	rcode = pthread_cond_init(&tp->tty_Obarrier, NULL);
	if(rcode) ERROR_EXIT(rcode);
	
	rcode = pthread_cond_init(&tp->tty_Mbarrier, NULL);
	if(rcode) ERROR_EXIT(rcode);
	
	rcode = pthread_mutex_init(&tp->tty_Imutex,NULL);
	if(rcode) ERROR_EXIT(rcode);		

	rcode = pthread_mutex_init(&tp->tty_Omutex,NULL);
	if(rcode) ERROR_EXIT(rcode);	
	
	tp->tty_outleft = tp->tty_outcum = 0;
	tp->tty_incount = tp->tty_inleft = tp->tty_incum = 0; 
	tp->tty_inhead 	= tp->tty_intail = tp->tty_inbuf;
	tp->tty_devread = pseudo_read;
	
	/*Create Thread to deal with INPUT  for this VTTY */
	TASKDEBUG("Starting %s INPUT thread for minor %d\n", 
		tp->tty_cfg.tty_name,tp->tty_minor); 
	rcode = pthread_create( &tp->tty_in_thread, NULL, pseudo_in_thread, (void *) tp);
	if( rcode)ERROR_EXIT(rcode);

	/*Create Thread to deal with OUTPUT  for this VTTY */
	TASKDEBUG("Starting %s OUTPUT thread for minor %d\n", 
		tp->tty_cfg.tty_name,tp->tty_minor); 
	rcode = pthread_create( &tp->tty_out_thread, NULL, pseudo_out_thread, (void *) tp);
	if( rcode)ERROR_EXIT(rcode);

	return(OK); 

}

/*===========================================================================*
 *				pseudo_in_thread				     *
 *===========================================================================*/
static void pseudo_in_thread(void *arg)
{
	tty_t *tp;
	int rcode, count;
	char *in_ptr;
	tty_pseudo_t *pse_ptr;
	
	tp = (tty_t *) arg;
	pse_ptr = &tp->tty_pseudo;
	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));
	TASKDEBUG(PSEUDO_FORMAT,PSEUDO_FIELDS(pse_ptr));

	MTX_LOCK(tp->tty_Imutex);
	
	while(TRUE){
		
		TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));
		// wait for main thread data read request 
		COND_WAIT(tp->tty_Ibarrier ,tp->tty_Imutex);
		
		MTX_UNLOCK(tp->tty_Imutex);
		count = read(tp->tty_pseudo.master, pse_ptr->in_buf, TTY_IN_BYTES);
		MTX_LOCK(tp->tty_Imutex);

		if(count < 0 ) {
			ERROR_PRINT(count);
			MTX_UNLOCK(tp->tty_Imutex);
			ERROR_EXIT(errno);
		}
		pse_ptr->in_count = count;
		pse_ptr->in_next  = pse_ptr->in_buf; // next char from the start of buffer
		
		// Signal main thread, new data read (or none)
		COND_SIGNAL(tp->tty_Mbarrier);
		TASKDEBUG("in_count=%d in_buf=%s\n", pse_ptr->in_count, pse_ptr->in_buf);
	}
}

/*===========================================================================*
 *				pseudo_out_thread				     *
 *===========================================================================*/
static void pseudo_out_thread(void *arg)
{
	tty_t *tp;
	int rcode, bytes, count;
	char *out_ptr;
	static char out_buf[TTY_OUT_BYTES];

	tp = (tty_t *) arg;
	out_ptr = tp->tty_outbuf;
	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));
	
	MTX_LOCK(tp->tty_Omutex);
	while(TRUE){
		TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));
		COND_WAIT(tp->tty_Obarrier ,tp->tty_Omutex);
		if( tp->tty_outleft == 0){ // there aren't any character for output 
			TASKDEBUG("WARNING minor=%d tty_outleft=%d\n", 
				tp->tty_minor, tp->tty_outleft);
			continue;
		}
		// the are some characters for output 
		if( tp->tty_outleft > TTY_OUT_BYTES)
			ERROR_EXIT(EMOLINVAL);
		memcpy(out_buf,  tp->tty_outbuf, tp->tty_outleft);
		count = tp->tty_outleft;
		TASKDEBUG("out_buf=%s count=%d\n", out_ptr, count);
		tp->tty_outcum = 0;
		tp->tty_outleft= 0;
		
		MTX_UNLOCK(tp->tty_Omutex);
		do {
			bytes = write(tp->tty_pseudo.master, out_buf , count);
			if(bytes < 0 ) 
				ERROR_PRINT(errno);
			count -= bytes;
		}while(count > 0);
		MTX_LOCK(tp->tty_Omutex);
	
		COND_SIGNAL(tp->tty_Mbarrier);
	}

}

/*===========================================================================*
 *				do_close				     *
 *===========================================================================*/
void do_close(tty_t *tp, message *m_ptr)
{
/* A tty line has been closed.  Clean up the line if it is the last close. */

	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));
#ifdef ANULADO
  if (m_ptr->TTY_LINE != LOG_MINOR && --tp->tty_openct == 0) {
	tp->tty_pgrp = 0;
	tty_icancel(tp);
	(*tp->tty_ocancel)(tp, 0);
	(*tp->tty_close)(tp, 0);
	tp->tty_termios = termios_defaults;
	tp->tty_winsize = winsize_defaults;
	setattr(tp);
  }
#endif // ANULADO
  tty_reply(MOLTASK_REPLY, m_ptr->m_source, m_ptr->IO_ENDPT, OK);
}

/*===========================================================================*
 *				do_cancel				     *
 *===========================================================================*/
void do_cancel(tty_t *tp, message *m_ptr)
{
/* A signal has been sent to a process that is hanging trying to read or write.
 * The pending read or write must be finished off immediately.
 */

  int proc_nr;
  int mode;

  	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));

  /* Check the parameters carefully, to avoid cancelling twice. */
  proc_nr = m_ptr->IO_ENDPT;
  mode = m_ptr->COUNT;
  
#ifdef ANULADO
  if ((mode & R_BIT) && tp->tty_inleft != 0 && proc_nr == tp->tty_inproc) {
	/* Process was reading when killed.  Clean up input. */
	tty_icancel(tp);
	tp->tty_inleft = tp->tty_incum = 0;
  }
  if ((mode & W_BIT) && tp->tty_outleft != 0 && proc_nr == tp->tty_outproc) {
	/* Process was writing when killed.  Clean up output. */
	(*tp->tty_ocancel)(tp, 0);
	tp->tty_outleft = tp->tty_outcum = 0;
  }
  if (tp->tty_ioreq != 0 && proc_nr == tp->tty_ioproc) {
	/* Process was waiting for output to drain. */
	tp->tty_ioreq = 0;
  }
  tp->tty_events = 1;
  
#endif // ANULADO 
  tty_reply(MOLTASK_REPLY, m_ptr->m_source, proc_nr, EMOLINTR);
}

/*===========================================================================*
 *				do_select				     *
 *===========================================================================*/
void do_select(tty_t *tp, message *m_ptr)
{
	int ops, ready_ops = 0, watch;

	TASKDEBUG(TTY_FORMAT,TTY_FIELDS(tp));

	ops = m_ptr->IO_ENDPT & (SEL_RD|SEL_WR|SEL_ERR);
	watch = (m_ptr->IO_ENDPT & SEL_NOTIFY) ? 1 : 0;

	ready_ops = select_try(tp, ops);

	if (!ready_ops && ops && watch) {
		tp->tty_select_ops |= ops;
		tp->tty_select_proc = m_ptr->m_source;
	}

    tty_reply(MOLTASK_REPLY, m_ptr->m_source, m_ptr->IO_ENDPT, ready_ops);

    return;
}


/*===========================================================================*
 *				settimer				     *
 *===========================================================================*/
void settimer(tty_t *tp,int enable)
{
	molclock_t now;				/* current time */
	molclock_t exp_time;
	int rcode;

	TASKDEBUG("enable=%d " TTY_FORMAT,enable, TTY_FIELDS(tp));

	/* Get the current time to calculate the timeout time. */
		/* Get the current time to compare the timers against. */
	if ((rcode=sys_getuptime(&now)) != OK){
		fprintf(stderr,"sys_getuptime't get uptime from clock.rcode=%d", rcode);
		ERROR_EXIT(rcode);
	}

	if (enable) {
		exp_time = now + tp->tty_termios.c_cc[VTIME] * TENTH_SECOND_RATE;
		/* Set a new timer for enabling the TTY events flags. */
		tmrs_settimer(&tty_timers, (moltimer_t *) &tp->tty_tmr, 
 		exp_time, tty_timed_out, NULL);  
	} else {
		/* Remove the timer from the active and expired lists. */
		tmrs_clrtimer(&tty_timers, &tp->tty_tmr, NULL);
	}
  
  /* Now check if a new alarm must be scheduled. This happens when the front
   * of the timers queue was disabled or reinserted at another position, or
   * when a new timer was added to the front.
   */
  if (tty_timers == NULL) tty_next_timeout = TMR_NEVER;
  else if (tty_timers->tmr_exp_time != tty_next_timeout) { 
  	tty_next_timeout = tty_timers->tmr_exp_time;
  	if ((rcode=sys_setalarm(tty_next_timeout, 1)) != OK){
 		fprintf(stderr,"Couldn't set synchronous alarm. rcode=%d\n", rcode);
		ERROR_EXIT(rcode);
	}
  }
}

 
int select_try(struct tty *tp, int ops)
{
	int ready_ops = 0;

	TASKDEBUG("ops=%d " TTY_FORMAT,ops, TTY_FIELDS(tp));

	/* Special case. If line is hung up, no operations will block.
	 * (and it can be seen as an exceptional condition.)
	 */
	if (tp->tty_termios.c_ospeed == B0) {
		ready_ops |= ops;
	}

	if (ops & SEL_RD) {
		/* will i/o not block on read? */
		if (tp->tty_inleft > 0) {
			ready_ops |= SEL_RD;	/* EIO - no blocking */
		} else if (tp->tty_incount > 0) {
			/* Is a regular read possible? tty_incount
			 * says there is data. But a read will only succeed
			 * in canonical mode if a newline has been seen.
			 */
			if (!(tp->tty_termios.c_lflag & ICANON) ||
				tp->tty_eotct > 0) {
				ready_ops |= SEL_RD;
			}
		}
	}

	if (ops & SEL_WR)  {
  		if (tp->tty_outleft > 0)  ready_ops |= SEL_WR;
		else if ((*tp->tty_devwrite)(tp, 1)) ready_ops |= SEL_WR;
	}
	return ready_ops;
}

int select_retry(struct tty *tp)
{
	TASKDEBUG(TTY_FORMAT, TTY_FIELDS(tp));

  	if (tp->tty_select_ops && select_try(tp, tp->tty_select_ops))
		mnx_notify(tp->tty_select_proc);
	return OK;
}


/*===========================================================================*
 *				handle_events				     *
 *===========================================================================*/
void handle_events(tty_t *tp)
{
/* Handle any events pending on a TTY.  These events are usually device
 * interrupts.
 *
 * Two kinds of events are prominent:
 *	- a character has been received from the console or an RS232 line.
 *	- an RS232 line has completed a write request (on behalf of a user).
 * The interrupt handler may delay the interrupt message at its discretion
 * to avoid swamping the TTY task.  Messages may be overwritten when the
 * lines are fast or when there are races between different lines, input
 * and output, because MINIX only provides single buffering for interrupt
 * messages (in proc.c).  This is handled by explicitly checking each line
 * for fresh input and completed output on each interrupt.
 */
 	TASKDEBUG(TTY_FORMAT, TTY_FIELDS(tp));

#ifdef ANULADO
  do {
	tp->tty_events = 0;

	/* Read input and perform input processing. */
	(*tp->tty_devread)(tp, 0);

	/* Perform output processing and write output. */
	(*tp->tty_devwrite)(tp, 0);

	/* Ioctl waiting for some event? */
	if (tp->tty_ioreq != 0) dev_ioctl(tp);
  } while (tp->tty_events);

  /* Transfer characters from the input queue to a waiting process. */
  in_transfer(tp);

  /* Reply if enough bytes are available. */
  if (tp->tty_incum >= tp->tty_min && tp->tty_inleft > 0) {
	if (tp->tty_inrepcode == MOLREVIVE) {
		notify(tp->tty_incaller);
		tp->tty_inrevived = 1;
	} else {
		tty_reply(tp->tty_inrepcode, tp->tty_incaller, 
			tp->tty_inproc, tp->tty_incum);
		tp->tty_inleft = tp->tty_incum = 0;
	}
  }
  if (tp->tty_select_ops)
  {
  	select_retry(tp);
  }
#if NR_VTTYS > 0
  if (ispty(tp))
  	select_retry_pty(tp);
#endif
#endif //  ANULADO

}

/*===========================================================================*
 *				pseudo_read				     *
 *===========================================================================*/
int pseudo_read(tty_t *tp, int true)
{
	u16_t *tty_ptr;
	char *next_ptr;
	u16_t ch;
	int i;
	
 	TASKDEBUG(TTYIN_FORMAT, TTYIN_FIELDS(tp));
	
	if (tp->tty_inleft > 0   // there is a need of data 
	&& tp->tty_eotct < tp->tty_min){ // but no IN_EOT in buffer 

		// first,  count IN_EOT characters in buffer
		TASKDEBUG("first,  count IN_EOT characters in buffer\n")
		if(tp->tty_eotct == (-1)){
			tp->tty_eotct = 0;
			tty_ptr = tp->tty_inhead;
			for(i=0; i < tp->tty_incount; i++){
				ch = *tty_ptr;
				if( ch == '\n' || ch == '\r'){
					*tty_ptr = (ch | IN_EOT);
					tp->tty_eotct++;
				}
				tty_ptr++;
				if( tty_ptr > (tp->tty_inbuf + TTY_IN_BYTES)){
					tty_ptr = tp->tty_inbuf;
				}
			} 
		}
		
		// copy from tty_pseudo input buffer to tty input buffer 
		TASKDEBUG(TTYIN_FORMAT, TTYIN_FIELDS(tp));
		while (tp->tty_eotct < tp->tty_min){
			tty_ptr = tp->tty_inhead;
			next_ptr = tp->tty_pseudo.in_next;
			while ( tp->tty_incount < TTY_IN_BYTES && tp->tty_pseudo.in_count > 0 ){
				ch = *next_ptr;
				if( ch == '\n' || ch == '\r'){
					*tty_ptr = (ch | IN_EOT);
					tp->tty_eotct++;
				}else{
					*tty_ptr = ch;
				}
				tty_ptr++;
				next_ptr++;
				if( tty_ptr > (tp->tty_inbuf + TTY_IN_BYTES)){
					tty_ptr = tp->tty_inbuf;
				}
				tp->tty_incount++;
				tp->tty_pseudo.in_count--;
			}
	
			//update TTY pointers
			tp->tty_inhead = tty_ptr;
			tp->tty_pseudo.in_next = next_ptr;
		
			TASKDEBUG(TTYIN_FORMAT, TTYIN_FIELDS(tp));
			if( tp->tty_eotct < tp->tty_min 	// there is not any IN_EOT char in TTY buffer 
			 && tp->tty_pseudo.in_count == 0){ 	// pseudo buffer is empty
				MTX_LOCK(tp->tty_Imutex);
				// Signal input thread 
				COND_SIGNAL(tp->tty_Ibarrier);	// signal pseudo input thread to get data
				// wait for input 
				COND_WAIT(tp->tty_Mbarrier, tp->tty_Imutex);				
				MTX_UNLOCK(tp->tty_Imutex);
				if( tp->tty_pseudo.in_count == 0) break;
			}	
		}
	}
	return(OK);
}

/*===========================================================================*
 *				in_transfer				     *
 *===========================================================================*/
void in_transfer(tty_t *tp)
{
/* Transfer bytes from the input queue to a process reading from a terminal. */

  int ch;
  int count;
  static char buf[64], *bp;

   	TASKDEBUG(TTY_FORMAT, TTY_FIELDS(tp));

	/* Anything to do? */
	if (tp->tty_inleft == 0 || tp->tty_eotct < tp->tty_min){
		return;
	}
	
	bp = buf;
	while (tp->tty_inleft > 0 && tp->tty_eotct > 0) {
		ch = *tp->tty_intail;

		if (!(ch & IN_EOF)) {
			/* One character to be delivered to the user. */
			*bp = ch & IN_CHAR;
			tp->tty_inleft--;
			if (++bp == bufend(buf)) {
				/* Temp buffer full, copy to user space. */
				sys_vircopy(SELF, (void *) buf, 
					tp->tty_inproc,(void *) tp->tty_in_vir,
					buflen(buf));
				tp->tty_in_vir += buflen(buf);
				tp->tty_incum += buflen(buf);
				bp = buf;
			}
		}

		/* Remove the character from the input queue. */
		if (++tp->tty_intail == bufend(tp->tty_inbuf))
			tp->tty_intail = tp->tty_inbuf;
		tp->tty_incount--;
		if (ch & IN_EOT) {
			tp->tty_eotct--;
			/* Don't read past a line break in canonical mode. */
			if (tp->tty_termios.c_lflag & ICANON) tp->tty_inleft = 0;
		}
	}

   	TASKDEBUG("buf=%s\n", buf);
	if (bp > buf) {
		/* Leftover characters in the buffer. */
		count = bp - buf;
		sys_vircopy(SELF, (void *) buf, 
			tp->tty_inproc, (void *)  tp->tty_in_vir, count);
		tp->tty_in_vir += count;
		tp->tty_incum += count;
	}

	/* Usually reply to the reader, possibly even if incum == 0 (EOF). */
	if (tp->tty_inleft == 0) {
		if (tp->tty_inrepcode == MOLREVIVE) {
			mnx_notify(tp->tty_incaller);
			tp->tty_inrevived = 1;
		} else {
			tty_reply(tp->tty_inrepcode, tp->tty_incaller, 
				tp->tty_inproc, tp->tty_incum);
			tp->tty_inleft = tp->tty_incum = 0;
		}
	}
   	TASKDEBUG(TTYIN_FORMAT, TTYIN_FIELDS(tp));
}

/*===========================================================================*
 *				in_process				     *
 *===========================================================================*/
int in_process(tty_t *tp, char *buf, int count)
{
/* Characters have just been typed in.  Process, save, and echo them.  Return
 * the number of characters processed.
 */

  int ch, sig, ct=0;
  int timeset = FALSE;

   	TASKDEBUG("count=%d " TTY_FORMAT, count, TTY_FIELDS(tp));

#ifdef ANULADO

  for (ct = 0; ct < count; ct++) {
	/* Take one character. */
	ch = *buf++ & BYTE;

	/* Strip to seven bits? */
	if (tp->tty_termios.c_iflag & ISTRIP) ch &= 0x7F;

	/* Input extensions? */
	if (tp->tty_termios.c_lflag & IEXTEN) {

		/* Previous character was a character escape? */
		if (tp->tty_escaped) {
			tp->tty_escaped = NOT_ESCAPED;
			ch |= IN_ESC;	/* protect character */
		}

		/* LNEXT (^V) to escape the next character? */
		if (ch == tp->tty_termios.c_cc[VLNEXT]) {
			tp->tty_escaped = ESCAPED;
			rawecho(tp, '^');
			rawecho(tp, '\b');
			continue;	/* do not store the escape */
		}

		/* REPRINT (^R) to reprint echoed characters? */
		if (ch == tp->tty_termios.c_cc[VREPRINT]) {
			reprint(tp);
			continue;
		}
	}

	/* _POSIX_VDISABLE is a normal character value, so better escape it. */
	if (ch == _POSIX_VDISABLE) ch |= IN_ESC;

	/* Map CR to LF, ignore CR, or map LF to CR. */
	if (ch == '\r') {
		if (tp->tty_termios.c_iflag & IGNCR) continue;
		if (tp->tty_termios.c_iflag & ICRNL) ch = '\n';
	} else
	if (ch == '\n') {
		if (tp->tty_termios.c_iflag & INLCR) ch = '\r';
	}

	/* Canonical mode? */
	if (tp->tty_termios.c_lflag & ICANON) {

		/* Erase processing (rub out of last character). */
		if (ch == tp->tty_termios.c_cc[VERASE]) {
			(void) back_over(tp);
			if (!(tp->tty_termios.c_lflag & ECHOE)) {
				(void) tty_echo(tp, ch);
			}
			continue;
		}

		/* Kill processing (remove current line). */
		if (ch == tp->tty_termios.c_cc[VKILL]) {
			while (back_over(tp)) {}
			if (!(tp->tty_termios.c_lflag & ECHOE)) {
				(void) tty_echo(tp, ch);
				if (tp->tty_termios.c_lflag & ECHOK)
					rawecho(tp, '\n');
			}
			continue;
		}

		/* EOF (^D) means end-of-file, an invisible "line break". */
		if (ch == tp->tty_termios.c_cc[VEOF]) ch |= IN_EOT | IN_EOF;

		/* The line may be returned to the user after an LF. */
		if (ch == '\n') ch |= IN_EOT;

		/* Same thing with EOL, whatever it may be. */
		if (ch == tp->tty_termios.c_cc[VEOL]) ch |= IN_EOT;
	}

	/* Start/stop input control? */
	if (tp->tty_termios.c_iflag & IXON) {

		/* Output stops on STOP (^S). */
		if (ch == tp->tty_termios.c_cc[VSTOP]) {
			tp->tty_inhibited = STOPPED;
			tp->tty_events = 1;
			continue;
		}

		/* Output restarts on START (^Q) or any character if IXANY. */
		if (tp->tty_inhibited) {
			if (ch == tp->tty_termios.c_cc[VSTART]
					|| (tp->tty_termios.c_iflag & IXANY)) {
				tp->tty_inhibited = RUNNING;
				tp->tty_events = 1;
				if (ch == tp->tty_termios.c_cc[VSTART])
					continue;
			}
		}
	}

	if (tp->tty_termios.c_lflag & ISIG) {
		/* Check for INTR (^?) and QUIT (^\) characters. */
		if (ch == tp->tty_termios.c_cc[VINTR]
					|| ch == tp->tty_termios.c_cc[VQUIT]) {
			sig = SIGINT;
			if (ch == tp->tty_termios.c_cc[VQUIT]) sig = SIGQUIT;
			sigchar(tp, sig);
			(void) tty_echo(tp, ch);
			continue;
		}
	}

	/* Is there space in the input buffer? */
	if (tp->tty_incount == buflen(tp->tty_inbuf)) {
		/* No space; discard in canonical mode, keep in raw mode. */
		if (tp->tty_termios.c_lflag & ICANON) continue;
		break;
	}

	if (!(tp->tty_termios.c_lflag & ICANON)) {
		/* In raw mode all characters are "line breaks". */
		ch |= IN_EOT;

		/* Start an inter-byte timer? */
		if (!timeset && tp->tty_termios.c_cc[VMIN] > 0
				&& tp->tty_termios.c_cc[VTIME] > 0) {
			settimer(tp, TRUE);
			timeset = TRUE;
		}
	}

	/* Perform the intricate function of echoing. */
	if (tp->tty_termios.c_lflag & (ECHO|ECHONL)) ch = tty_echo(tp, ch);

	/* Save the character in the input queue. */
	*tp->tty_inhead++ = ch;
	if (tp->tty_inhead == bufend(tp->tty_inbuf))
		tp->tty_inhead = tp->tty_inbuf;
	tp->tty_incount++;
	if (ch & IN_EOT) tp->tty_eotct++;

	/* Try to finish input if the queue threatens to overflow. */
	if (tp->tty_incount == buflen(tp->tty_inbuf)) in_transfer(tp);
  }
#endif // ANULADO

  return ct;
}

/*===========================================================================*
 *				tty_timed_out				     *
 *===========================================================================*/
void tty_timed_out(moltimer_t *tp)
{
/* This timer has expired. Set the events flag, to force processing. */
  tty_t *tty_ptr;
  tty_ptr = &tty_table[tmr_arg(tp)->ta_int];
  tty_ptr->tty_min = 0;			/* force read to succeed */
  tty_ptr->tty_events = 1;		
}

