/***************************************************************************
					test_tty.c

This program test basic operation of TTY task.
	- OPEN
	- IO_CTL SET ATTR
	- IO_CTL GET ATTR
	- WRITE
	- READ
It simulate the same operations that FS does.
     TEST_TTY<->TTY
***************************************************************************/

#include "tty.h"
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define WAIT4BIND_MS 1000

int local_nodeid;
drvs_usr_t drvs, *drvs_ptr;
int ttyt_lpid;		
pthread_t main_thread;		
DC_usr_t  vmu, *dc_ptr;
proc_usr_t ttyt_proc, *ttyt_ptr;
proc_usr_t tty_proc, *tty_ptr;
message *m_ptr;

void test_init(void);
void slave_thread(void *arg);


/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int  main( int argc, char *argv[] )
{
	int rcode, lpid, i;
		
	static tty_pseudo_t tty_pse __attribute__((aligned(0x1000)));
	tty_pseudo_t *pse_ptr;
	
	static tty_conf_t tty_cfg __attribute__((aligned(0x1000)));
	tty_conf_t *cfg_ptr;
		
	pthread_t		write_thread;
	static char line_msg[]="line1\nline2\nline3\nline4\nline5\n";
	static char usr_entry[TTY_IN_BYTES];
	
	test_init();

	// ************************ OPEN TTY **************************
	
	TASKDEBUG("TEST OPEN\n");
	m_ptr->m_type	= DEV_OPEN;
	m_ptr->TTY_LINE	= 0;
	m_ptr->IO_ENDPT	= ttyt_ptr->p_endpoint;
	m_ptr->COUNT	= O_NOCTTY;

	sleep(1);
	TASKDEBUG("OPEN " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(TTY_PROC_NR, m_ptr);
	if(rcode) 
		TASKDEBUG("OPEN rcode=%d\n", rcode);
	TASKDEBUG("OPEN " MSG2_FORMAT, MSG2_FIELDS(m_ptr));

	// ************************ DEV_IOCTL  TIOCGETPSE TTY **************************

	
	TASKDEBUG("TEST DEV_IOCTL TIOCGETPSE \n");
	pse_ptr = &tty_pse;
	m_ptr->m_type	= DEV_IOCTL;
	m_ptr->TTY_LINE	= 0;
	m_ptr->IO_ENDPT	= ttyt_ptr->p_endpoint;
	m_ptr->COUNT	= TIOCGETPSE;
	m_ptr->ADDRESS  = &tty_pse;

	sleep(1);
	TASKDEBUG("DEV_IOCTL TIOCGETPSE " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(TTY_PROC_NR, m_ptr);
	if(rcode) 
		TASKDEBUG("DEV_IOCTL TIOCGETPSE rcode=%d\n", rcode);
	TASKDEBUG("DEV_IOCTL TIOCGETPSE " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	TASKDEBUG("DEV_IOCTL TIOCGETPSE " PSEUDO_FORMAT, PSEUDO_FIELDS(pse_ptr));
		
	// ************************ DEV_IOCTL  TIOCGETCFG TTY **************************
	
	TASKDEBUG("TEST DEV_IOCTL TIOCGETCFG \n");
	cfg_ptr = &tty_cfg;
	m_ptr->m_type	= DEV_IOCTL;
	m_ptr->TTY_LINE	= 0;
	m_ptr->IO_ENDPT	= ttyt_ptr->p_endpoint;
	m_ptr->COUNT	= TIOCGETCFG;
	m_ptr->ADDRESS  = &tty_cfg;

	sleep(1);
	TASKDEBUG("DEV_IOCTL TIOCGETCFG " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(TTY_PROC_NR, m_ptr);
	if(rcode) 
		TASKDEBUG("DEV_IOCTL TIOCGETCFG rcode=%d\n", rcode);
	TASKDEBUG("DEV_IOCTL TIOCGETCFG " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	TASKDEBUG("DEV_IOCTL TIOCGETCFG major=%d minor=%d type=%d port=%d endpoint=%d\n",
		 cfg_ptr->major, cfg_ptr->minor, cfg_ptr->type, cfg_ptr->port,cfg_ptr->endpoint);
	
	// ************************ TTY WRITE **************************

	TASKDEBUG("TEST TTY WRITE \n");

	/*Create the VTTY SLAVE Thread */
	TASKDEBUG("Starting thread for minor:%d\n", cfg_ptr->minor);
	
	rcode = pthread_create( &write_thread, NULL, slave_thread, pse_ptr);
	if( rcode)ERROR_EXIT(rcode);
	
	cfg_ptr = &tty_cfg;
	m_ptr->m_type	= DEV_WRITE;
	m_ptr->TTY_LINE	= 0;
	m_ptr->IO_ENDPT	= ttyt_ptr->p_endpoint;
	m_ptr->COUNT	= strlen(line_msg);
	m_ptr->ADDRESS  = line_msg;

	sleep(1);
	TASKDEBUG("TEST TTY WRITE " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	rcode = mnx_sendrec(TTY_PROC_NR, m_ptr);
	if(rcode) 
		TASKDEBUG("TEST TTY WRITE rcode=%d\n", rcode);
	TASKDEBUG("TEST TTY WRITE " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
	sleep(2);
	
	// ************************ TTY READ  **************************

	TASKDEBUG("TEST TTY READ \n");

	cfg_ptr = &tty_cfg;
	for(i= 0 ; i < 5; i++) {
		m_ptr->m_type	= DEV_READ;
		m_ptr->TTY_LINE	= 0;
		m_ptr->IO_ENDPT	= ttyt_ptr->p_endpoint;
		m_ptr->COUNT	= TTY_IN_BYTES; // ONLY FOR TEST
		m_ptr->ADDRESS  = usr_entry;

		sleep(1);	
	
		TASKDEBUG("TEST TTY READ " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
		rcode = mnx_sendrec(TTY_PROC_NR, m_ptr);
		if(rcode) 
			TASKDEBUG("TEST TTY READ rcode=%d\n", rcode);
		TASKDEBUG("TEST TTY READ " MSG2_FORMAT, MSG2_FIELDS(m_ptr));
		TASKDEBUG("TEST TTY READ  usr_entry=%s\n", usr_entry);
	}
	
	sleep(10);
 }
 
 
 
/*===========================================================================*
 *				slave_thread					     *
 *===========================================================================*/
void slave_thread(void *arg)
{
	int fds, rcode, bytes;
	tty_pseudo_t *pse_ptr;
	char buf_in[TTY_IN_BYTES];
	static char buf_out[] = "reply1\nreply2\nreply3\nreply4\nreply5\n";

	static struct termios slave_orig_term_settings; // Saved terminal settings
	static struct termios new_term_settings; // Current terminal settings
	
	pse_ptr = (	tty_pseudo_t *) arg;
	
	TASKDEBUG("%s\n", pse_ptr->sname);
	
	fds = open(pse_ptr->sname, O_RDWR);	
	
	// Save the defaults parameters of the slave side of the PTY
	rcode = tcgetattr(fds, &slave_orig_term_settings);
	
	// Set RAW mode on slave side of PTY
	new_term_settings = slave_orig_term_settings;
	cfmakeraw (&new_term_settings);
	tcsetattr (fds, TCSANOW, &new_term_settings);
	// Make the current process a new session leader
	setsid();
	// As the child is a session leader, set the controlling terminal to be the slave side of the PTY
	// (Mandatory for programs like the shell to make them manage correctly their outputs)
	ioctl(fds, TIOCSCTTY, 1);
	while(TRUE) {
		bytes = read(fds, buf_in, TTY_IN_BYTES);
		if( bytes < 0) {
			ERROR_PRINT(errno);
		}
		TASKDEBUG("bytes=%d buf_in=[%s]\n", bytes, buf_in);
		
		bytes = write(fds, buf_out, strlen(buf_out));
		if( bytes < 0) {
			ERROR_PRINT(errno);
		}
		TASKDEBUG("bytes=%d buf_out=[%s]\n", bytes, buf_out);
	}
}

/*===========================================================================*
 *				test_init					     *
 *===========================================================================*/
void test_init(void)
{
	int rcode;

	ttyt_lpid = getpid();
	TASKDEBUG("ttyt_lpid=%d \n", ttyt_lpid);

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		TASKDEBUG("TTY_TEST: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			TASKDEBUG("TTY_TEST: mnx_wait4bind_T TIMEOUT\n");
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

	TASKDEBUG("Get TTY info from SYSTASK\n");
	rcode = sys_getproc(&tty_proc, TTY_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	tty_ptr = &tty_proc;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(tty_ptr));

	TASKDEBUG("Get TTY_TEST info from SYSTASK\n");
	rcode = sys_getproc(&ttyt_proc, SELF);
	if(rcode) ERROR_EXIT(rcode);
	ttyt_ptr = &ttyt_proc;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(ttyt_ptr));
	
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		ERROR_EXIT(errno);
	}
	TASKDEBUG("TTY_TEST m_ptr=%p\n",m_ptr);


}



	
