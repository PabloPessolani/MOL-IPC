/*	this file contains the interface of the network software with the file
 *	system.
 *
 * Copyright 1995 Philip Homburg
 *
 * The valid messages and their parameters are:
 * 
 * Requests:
 *
 *    m_type      NDEV_MINOR   NDEV_PROC    NDEV_REF   NDEV_MODE
 * -------------------------------------------------------------
 * | DEV_OPEN    |minor dev  | proc nr   |  fd       |   mode   |
 * |-------------+-----------+-----------+-----------+----------+
 * | DEV_CLOSE   |minor dev  | proc nr   |  fd       |          |
 * |-------------+-----------+-----------+-----------+----------+
 *
 *    m_type      NDEV_MINOR   NDEV_PROC    NDEV_REF   NDEV_COUNT NDEV_BUFFER
 * ---------------------------------------------------------------------------
 * | DEV_READ    |minor dev  | proc nr   |  fd       |  count    | buf ptr   |
 * |-------------+-----------+-----------+-----------+-----------+-----------|
 * | DEV_WRITE   |minor dev  | proc nr   |  fd       |  count    | buf ptr   |
 * |-------------+-----------+-----------+-----------+-----------+-----------|
 *
 *    m_type      NDEV_MINOR   NDEV_PROC    NDEV_REF   NDEV_IOCTL NDEV_BUFFER
 * ---------------------------------------------------------------------------
 * | DEV_IOCTL3  |minor dev  | proc nr   |  fd       |  command  | buf ptr   |
 * |-------------+-----------+-----------+-----------+-----------+-----------|
 *
 *    m_type      NDEV_MINOR   NDEV_PROC    NDEV_REF   NDEV_OPERATION
 * -------------------------------------------------------------------|
 * | DEV_CANCEL  |minor dev  | proc nr   |  fd       | which operation|
 * |-------------+-----------+-----------+-----------+----------------|
 *
 * Replies:
 *
 *    m_type        REP_PROC_NR   REP_STATUS   REP_REF    REP_OPERATION
 * ----------------------------------------------------------------------|
 * | DEVICE_REPLY |   proc nr   |  status    |  fd     | which operation |
 * |--------------+-------------+------------+---------+-----------------|
 */

#include "inet.h"

#include "sys/svrctl.h"
#include "sys/select.h"

#include "proto.h"
#include "generic/type.h"
#include "generic/assert.h"
#include "generic/buf.h"
#include "generic/event.h"
#include "generic/sr.h"
#include "sr_int.h"

#define DEV_CANCEL NW_CANCEL
#define DEVICE_REPLY MOLREVIVE
#define DEV_IOCTL3 DEV_IOCTL
#define NDEV_BUFFER ADDRESS
#define NDEV_COUNT COUNT
#define NDEV_IOCTL REQUEST
#define NDEV_MINOR DEVICE
#define NDEV_PROC IO_ENDPT

sr_fd_t sr_fd_table[FD_NR];

mq_t *repl_queue, *repl_queue_tail;
#ifdef __minix_dcd
cpvec_t cpvec[CPVEC_NR];
#else /* Minix 3 */
struct vir_cp_req vir_cp_req[CPVEC_NR];
#endif

int sr_open(message *m);
void sr_close(message *m);
int sr_rwio(mq_t *m);
int sr_restart_read(sr_fd_t *fdp);
int sr_restart_write(sr_fd_t *fdp);
int sr_restart_ioctl(sr_fd_t *fdp);
int sr_cancel(message *m);
int sr_select(message *m);
void sr_status(message *m);
void sr_reply_(mq_t *m, int reply, int is_revive);
sr_fd_t *sr_getchannel(int minor);
acc_t *sr_get_userdata(int fd, vir_bytes offset,
					vir_bytes count, int for_ioctl);
int sr_put_userdata(int fd, vir_bytes offset,
						acc_t *data, int for_ioctl);
void sr_select_res(int fd, unsigned ops);
int sr_repl_queue(int proc, int ref, int operation);
int walk_queue(sr_fd_t *sr_fd, mq_t **q_head_ptr, 
	mq_t **q_tail_ptr, int type, int proc_nr, int ref, int first_flag);
void process_req_q(mq_t *mq, mq_t *tail, 
							mq_t **tail_ptr);
void sr_event(event_t *evp, ev_arg_t arg);
int cp_u2b(int proc, char *src, acc_t **var_acc_ptr,
								 int size);
int cp_b2u(acc_t *acc_ptr, int proc, char *dest);

void sr_init(void)
{
	int i;

	SVRDEBUG("\n");

	for (i=0; i<FD_NR; i++)
	{
		sr_fd_table[i].srf_flags= SFF_FREE;
		ev_init(&sr_fd_table[i].srf_ioctl_ev);
		ev_init(&sr_fd_table[i].srf_read_ev);
		ev_init(&sr_fd_table[i].srf_write_ev);
	}
	repl_queue= NULL;
}

void sr_rec(mq_t *m)
{
	int result;
	int send_reply, free_mess;
	message *mptr;
	SVRDEBUG("sr_rec m_type=%d\n",m->mq_mess.m_type);

	mptr = &m->mq_mess;
	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(mptr));
	
	if (repl_queue)
	{
		if (m->mq_mess.m_type == DEV_CANCEL)
		{
			result= sr_repl_queue(m->mq_mess.IO_ENDPT, 0, 0);
			if (result)
			{
				mq_free(m);
				return;	/* canceled request in queue */
			}
		}
	}

	switch (m->mq_mess.m_type)	{
		case DEV_OPEN:
			result= sr_open(&m->mq_mess);
			send_reply= 1;
			free_mess= 1;
			break;
		case DEV_CLOSE:
			sr_close(&m->mq_mess);
			result= OK;
			send_reply= 1;
			free_mess= 1;
			break;
		case DEV_READ:
		case DEV_WRITE:
		case DEV_IOCTL3:
			result= sr_rwio(m);
			assert(result == OK || result == SUSPEND);
			send_reply= (result == SUSPEND);
			free_mess= 0;
			break;
		case DEV_CANCEL:
			result= sr_cancel(&m->mq_mess);
			assert(result == OK || result == EMOLINTR);
	//		send_reply = (result == EMOLINTR); MODIFICADO POR PAP
			send_reply = 1;	
			free_mess= 1;
			m->mq_mess.m_type= 0;
			break;
		case DEV_SELECT:
			result= sr_select(&m->mq_mess);
			send_reply= 1;
			free_mess= 1;
			break;
		case DEV_STATUS:
			sr_status(&m->mq_mess);
			send_reply= 0;
			free_mess= 1;
			break;
		default:
			ip_panic(("unknown message, from %d, type %d",
					m->mq_mess.m_source, m->mq_mess.m_type));
	}
	if (send_reply)	{
		sr_reply_(m, result, FALSE /* !is_revive */);
	}
	if (free_mess)
		mq_free(m);
}

void sr_add_minor(int minor, int port,sr_open_t openf,sr_close_t closef,
				sr_read_t readf,sr_write_t writef,sr_ioctl_t ioctlf,
				sr_cancel_t cancelf,sr_select_t selectf)
{
	sr_fd_t *sr_fd;

	SVRDEBUG("minor=%d port=%d\n", minor, port);

	assert (minor>=0 && minor<FD_NR);

	sr_fd= &sr_fd_table[minor];

	assert(!(sr_fd->srf_flags & SFF_INUSE));

	sr_fd->srf_flags= SFF_INUSE | SFF_MINOR;
	sr_fd->srf_port= port;
	sr_fd->srf_open= openf;
	sr_fd->srf_close= closef;
	sr_fd->srf_write= writef;
	sr_fd->srf_read= readf;
	sr_fd->srf_ioctl= ioctlf;
	sr_fd->srf_cancel= cancelf;
	sr_fd->srf_select= selectf;
	// AGREGADO POR PAP 
	sr_fd->srf_ioctl_q = NULL;
	sr_fd->srf_read_q  = NULL;
	sr_fd->srf_write_q = NULL;
	sr_fd->srf_ioctl_q_tail = NULL;
	sr_fd->srf_read_q_tail  = NULL;
	sr_fd->srf_write_q_tail = NULL;
}

int sr_open(message *m)
{
	sr_fd_t *sr_fd;

	int minor= m->NDEV_MINOR;
	int i, fd;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	SVRDEBUG("minor=%d sr_fd_table[minor].srf_flags=%X \n",minor, sr_fd_table[minor].srf_flags);
	
	if (minor<0 || minor>FD_NR)	{
		DBLOCK(1, printf("replying EMOLINVAL\n"));
		return EMOLINVAL;
	}
	if (!(sr_fd_table[minor].srf_flags & SFF_MINOR))	{
		DBLOCK(1, printf("replying ENXIO\n"));
		return ENXIO;
	}
	
	for (i=0; i<FD_NR && (sr_fd_table[i].srf_flags & SFF_INUSE); i++);

	if (i>=FD_NR)	{
		DBLOCK(1, printf("replying ENFILE\n"));
		return ENFILE;
	}

	sr_fd= &sr_fd_table[i];
	*sr_fd= sr_fd_table[minor];
	sr_fd->srf_flags= SFF_INUSE;
	fd= (*sr_fd->srf_open)(sr_fd->srf_port, i, sr_get_userdata,
		sr_put_userdata, 0 /* no put_pkt */, sr_select_res);
	if (fd<0) 	{
		sr_fd->srf_flags= SFF_FREE;
		DBLOCK(1, printf("replying %d\n", fd));
		return fd;
	}
	sr_fd->srf_fd= fd;
	SVRDEBUG("minor=%d i=%d srf_flags=%X srf_fd=%d\n",minor, i,
		sr_fd->srf_flags, sr_fd->srf_fd);

	return i;
}

void sr_close(message *m)
{
	sr_fd_t *sr_fd;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	sr_fd= sr_getchannel(m->NDEV_MINOR);
	assert (sr_fd);

	if (sr_fd->srf_flags & SFF_BUSY)
		ip_panic(("close on busy channel"));

	assert (!(sr_fd->srf_flags & SFF_MINOR));
	(*sr_fd->srf_close)(sr_fd->srf_fd);
	sr_fd->srf_flags= SFF_FREE;
}

int sr_rwio(mq_t *m)
{
	sr_fd_t *sr_fd;
	mq_t **q_head_ptr, **q_tail_ptr;
	int ip_flag, susp_flag, first_flag;
	int r;
	ioreq_t request;
	mnx_size_t size;
	message *m_ptr;

	m_ptr = &m->mq_mess;
	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));

	sr_fd= sr_getchannel(m->mq_mess.NDEV_MINOR);
	assert (sr_fd);

	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	switch(m->mq_mess.m_type) 	{
		case DEV_READ:
			SVRDEBUG("DEV_READ type=%d\n", m->mq_mess.m_type);
			q_head_ptr= &sr_fd->srf_read_q;
			q_tail_ptr= &sr_fd->srf_read_q_tail;
			ip_flag= SFF_READ_IP;
			susp_flag= SFF_READ_SUSP;
			first_flag= SFF_READ_FIRST;
			break;
		case DEV_WRITE:
			SVRDEBUG("DEV_WRITE type=%d\n", m->mq_mess.m_type);
			q_head_ptr= &sr_fd->srf_write_q;
			q_tail_ptr= &sr_fd->srf_write_q_tail;
			ip_flag= SFF_WRITE_IP;
			susp_flag= SFF_WRITE_SUSP;
			first_flag= SFF_WRITE_FIRST;
			break;
		case DEV_IOCTL3:
			SVRDEBUG("DEV_IOCTL3 type=%d\n", m->mq_mess.m_type);
			q_head_ptr= &sr_fd->srf_ioctl_q;
			q_tail_ptr= &sr_fd->srf_ioctl_q_tail;
			ip_flag= SFF_IOCTL_IP;
			susp_flag= SFF_IOCTL_SUSP;
			first_flag= SFF_IOCTL_FIRST;
			break;
		default:
			ip_panic(("illegal case entry"));
	}

	SVRDEBUG("srf_flags=%X ip_flag=%d\n",sr_fd->srf_flags,  ip_flag);
	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	if (sr_fd->srf_flags & ip_flag)	{
		assert(sr_fd->srf_flags & susp_flag);

//		assert(*q_head_ptr); MODIFICADO POR PAP
		if(*q_head_ptr == NULL){
			SVRDEBUG("q_head_ptr == NULL\n");
			*q_head_ptr = m;
		}else{
			SVRDEBUG("q_head_ptr != NULL\n");
			(*q_tail_ptr)->mq_next= m;
		}
		*q_tail_ptr= m;
		(*q_tail_ptr)->mq_next= NULL; // redundantes, se hace en mq.c al allocar 
// fin modificado por PAP 
		ERROR_RETURN(SUSPEND);
	}
	assert(*q_head_ptr == NULL); // MODIFICADO POR PAP 

	*q_tail_ptr= *q_head_ptr= m;
	sr_fd->srf_flags |= ip_flag;
	assert(!(sr_fd->srf_flags & first_flag));
	sr_fd->srf_flags |= first_flag;

	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	switch(m->mq_mess.m_type)	{
		case DEV_READ:
			SVRDEBUG("DEV_READ type=%d\n", m->mq_mess.m_type);
			r= (*sr_fd->srf_read)(sr_fd->srf_fd, 
				m->mq_mess.NDEV_COUNT);
			break;
		case DEV_WRITE:
			SVRDEBUG("DEV_WRITE type=%d\n", m->mq_mess.m_type);
			r= (*sr_fd->srf_write)(sr_fd->srf_fd, 
				m->mq_mess.NDEV_COUNT);
			break;
		case DEV_IOCTL3:
			request= m->mq_mess.NDEV_IOCTL;
			SVRDEBUG("DEV_IOCTL3 request=%X\n", request);
		
			/* There should be a better way to do this... */
			if (request == NWIOQUERYPARAM)
			{
				SVRDEBUG("DEV_IOCTL3 NWIOQUERYPARAM\n");
				r= qp_query(m->mq_mess.NDEV_PROC,
					(vir_bytes)m->mq_mess.NDEV_BUFFER);
				r= sr_put_userdata(sr_fd-sr_fd_table, r, NULL, 1);
				assert(r == OK);
				break;
			}

			/* And now, we continue with our regular program. */
			size= (request >> 16) & _IOCPARM_MASK;
			if (size>MAX_IOCTL_S)
			{
				SVRDEBUG("DEV_IOCTL3 MAX_IOCTL_S request=%X size=%d _IOCPARM_MASK=%d\n"
					,request, size, _IOCPARM_MASK);
				DBLOCK(1, printf("replying EMOLINVAL\n"));
				r= sr_put_userdata(sr_fd-sr_fd_table, EMOLINVAL, 
					NULL, 1);
				assert(r == OK);
				assert(sr_fd->srf_flags & first_flag);
				sr_fd->srf_flags &= ~first_flag;
				return OK;
			}
			SVRDEBUG("srf_fd=%d request=%X\n",sr_fd->srf_fd, request);
			r= (*sr_fd->srf_ioctl)(sr_fd->srf_fd, request);
			break;
		default:
			ip_panic(("illegal case entry"));
	}


	assert(sr_fd->srf_flags & first_flag);
	sr_fd->srf_flags &= ~first_flag;

	assert(r == OK || r == SUSPEND || 
		(printf("r= %d\n", r), 0));
	if (r == SUSPEND)
		sr_fd->srf_flags |= susp_flag;
	else
		mq_free(m);

	SVRDEBUG("r=%d\n",r);
	return r;
}

int sr_restart_read(sr_fd_t *sr_fd)
{
	mq_t *mp;
	int r;

	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	mp= sr_fd->srf_read_q;
	assert(mp);

	if (sr_fd->srf_flags & SFF_READ_IP)	{
		assert(sr_fd->srf_flags & SFF_READ_SUSP);
		return SUSPEND;
	}
	sr_fd->srf_flags |= SFF_READ_IP;

	r= (*sr_fd->srf_read)(sr_fd->srf_fd, 
		mp->mq_mess.NDEV_COUNT);

	assert(r == OK || r == SUSPEND || 
		(printf("r= %d\n", r), 0));
	if (r == SUSPEND)
		sr_fd->srf_flags |= SFF_READ_SUSP;
	return r;
}

int sr_restart_write(sr_fd_t *sr_fd)
{
	mq_t *mp;
	int r;

	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	mp= sr_fd->srf_write_q;
	assert(mp);

	if (sr_fd->srf_flags & SFF_WRITE_IP)	{
		assert(sr_fd->srf_flags & SFF_WRITE_SUSP);
		return SUSPEND;
	}
	sr_fd->srf_flags |= SFF_WRITE_IP;

	r= (*sr_fd->srf_write)(sr_fd->srf_fd, 
		mp->mq_mess.NDEV_COUNT);

	assert(r == OK || r == SUSPEND || 
		(printf("r= %d\n", r), 0));
	if (r == SUSPEND)
		sr_fd->srf_flags |= SFF_WRITE_SUSP;
	return r;
}

int sr_restart_ioctl(sr_fd_t *sr_fd)
{
	mq_t *mp;
	int r;

	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	mp= sr_fd->srf_ioctl_q;
	assert(mp);

	if (sr_fd->srf_flags & SFF_IOCTL_IP)	{
		assert(sr_fd->srf_flags & SFF_IOCTL_SUSP);
		return SUSPEND;
	}
	sr_fd->srf_flags |= SFF_IOCTL_IP;

	r= (*sr_fd->srf_ioctl)(sr_fd->srf_fd, 
		mp->mq_mess.NDEV_COUNT);

	assert(r == OK || r == SUSPEND || 
		(printf("r= %d\n", r), 0));
	if (r == SUSPEND)
		sr_fd->srf_flags |= SFF_IOCTL_SUSP;
	return r;
}

int sr_cancel(message *m)
{
	sr_fd_t *sr_fd;
	int result;
	int proc_nr, ref, operation;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

    result=EMOLINTR;
	proc_nr=  m->NDEV_PROC;
	ref=  0;
	operation= 0;
	sr_fd= sr_getchannel(m->NDEV_MINOR);
	assert (sr_fd);
	SVRDEBUG("srf_fd=%d\n", sr_fd->srf_fd);

	result= walk_queue(sr_fd, &sr_fd->srf_ioctl_q, 
			&sr_fd->srf_ioctl_q_tail, SR_CANCEL_IOCTL,
			proc_nr, ref, SFF_IOCTL_FIRST);
	if (result != EAGAIN)
		ERROR_RETURN(result);

	result= walk_queue(sr_fd, &sr_fd->srf_read_q, 
			&sr_fd->srf_read_q_tail, SR_CANCEL_READ,
			proc_nr, ref, SFF_READ_FIRST);
	if (result != EAGAIN)
		ERROR_RETURN(result);
	
	result= walk_queue(sr_fd, &sr_fd->srf_write_q, 
			&sr_fd->srf_write_q_tail, SR_CANCEL_WRITE,
			proc_nr, ref, SFF_WRITE_FIRST);
	if (result != EAGAIN)
		ERROR_RETURN(result);
	
	SVRDEBUG("request not found: from %d, type %d, MINOR= %d, PROC= %d",
		m->m_source, m->m_type, m->NDEV_MINOR, m->NDEV_PROC);

	ip_panic((
		"request not found: from %d, type %d, MINOR= %d, PROC= %d",
		m->m_source, m->m_type, m->NDEV_MINOR,
		m->NDEV_PROC));
}

int sr_select(message *m)
{
	sr_fd_t *sr_fd;
	mq_t **q_head_ptr, **q_tail_ptr;
	int ip_flag, susp_flag;
	int r, ops;
	unsigned m_ops, i_ops;
	ioreq_t request;
	mnx_size_t size;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	sr_fd= sr_getchannel(m->NDEV_MINOR);
	assert (sr_fd);

	sr_fd->srf_select_proc= m->m_source;

	m_ops= m->IO_ENDPT;
	i_ops= 0;
	if (m_ops & SEL_RD) i_ops |= SR_SELECT_READ;
	if (m_ops & SEL_WR) i_ops |= SR_SELECT_WRITE;
	if (m_ops & SEL_ERR) i_ops |= SR_SELECT_EXCEPTION;
	if (!(m_ops & SEL_NOTIFY)) i_ops |= SR_SELECT_POLL;

	r= (*sr_fd->srf_select)(sr_fd->srf_fd,  i_ops);
	if (r < 0)
		return r;
	m_ops= 0;
	if (r & SR_SELECT_READ) m_ops |= SEL_RD;
	if (r & SR_SELECT_WRITE) m_ops |= SEL_WR;
	if (r & SR_SELECT_EXCEPTION) m_ops |= SEL_ERR;

	return m_ops;
}

void sr_status(m)
message *m;
{
	int fd, result;
	unsigned m_ops;
	sr_fd_t *sr_fd;
	mq_t *mq;

	SVRDEBUG(MSG2_FORMAT, MSG2_FIELDS(m));

	mq= repl_queue;
	if (mq != NULL)	{
		repl_queue= mq->mq_next;

		mq->mq_mess.m_type= DEV_REVIVE;
		result= mnx_send(mq->mq_mess.m_source, &mq->mq_mess);
		if (result != OK)
			ip_panic(("unable to send"));
		mq_free(mq);

		return;
	}

	for (fd=0, sr_fd= sr_fd_table; fd<FD_NR; fd++, sr_fd++)	{
		if ((sr_fd->srf_flags &
			(SFF_SELECT_R|SFF_SELECT_W|SFF_SELECT_X)) == 0)	{
			/* Nothing to report */
			continue;
		}
		if (sr_fd->srf_select_proc != m->m_source)	{
			/* Wrong process */
			continue;
		}

		m_ops= 0;
		if (sr_fd->srf_flags & SFF_SELECT_R) m_ops |= SEL_RD;
		if (sr_fd->srf_flags & SFF_SELECT_W) m_ops |= SEL_WR;
		if (sr_fd->srf_flags & SFF_SELECT_X) m_ops |= SEL_ERR;

		sr_fd->srf_flags &= ~(SFF_SELECT_R|SFF_SELECT_W|SFF_SELECT_X);

		m->m_type= DEV_IO_READY;
		m->DEV_MINOR= fd;
		m->DEV_SEL_OPS= m_ops;

		result= mnx_send(m->m_source, m);
		if (result != OK)
			ip_panic(("unable to send"));
		return;
	}

	m->m_type= DEV_NO_STATUS;
	result= mnx_send(m->m_source, m);
	if (result != OK)
		ip_panic(("unable to send"));
}

int walk_queue(sr_fd_t *sr_fd, mq_t **q_head_ptr,mq_t **q_tail_ptr,
				int type,int proc_nr,int ref,int first_flag)
{
	mq_t *q_ptr_prv, *q_ptr;
	int result;

	SVRDEBUG(SR_FORMAT, SR_FIELDS(sr_fd));

	for(q_ptr_prv= NULL, q_ptr= *q_head_ptr; q_ptr; 
		q_ptr_prv= q_ptr, q_ptr= q_ptr->mq_next) {
		if (q_ptr->mq_mess.NDEV_PROC != proc_nr)
			continue;
		if (!q_ptr_prv)
		{
			SVRDEBUG("srf_flags=%X type=%d\n", sr_fd->srf_flags, type);
			assert(!(sr_fd->srf_flags & first_flag));
			sr_fd->srf_flags |= first_flag;

			result= (*sr_fd->srf_cancel)(sr_fd->srf_fd, type);
			SVRDEBUG("result=%d\n", result);
			assert(result == OK);

			*q_head_ptr= q_ptr->mq_next;
			mq_free(q_ptr);

			SVRDEBUG("srf_flags=%X\n", sr_fd->srf_flags);
			assert(sr_fd->srf_flags & first_flag);
			sr_fd->srf_flags &= ~first_flag;

			ERROR_RETURN(OK);
		}
		q_ptr_prv->mq_next= q_ptr->mq_next;
		mq_free(q_ptr);
		if (!q_ptr_prv->mq_next)
			*q_tail_ptr= q_ptr_prv;
		ERROR_RETURN(EMOLINTR);
	}
	ERROR_RETURN(EAGAIN);
}

sr_fd_t *sr_getchannel(int minor)
{
	sr_fd_t *loc_fd;

	SVRDEBUG("minor=%d\n", minor);

	compare(minor, >=, 0);
	compare(minor, <, FD_NR);

	loc_fd= &sr_fd_table[minor];

	SVRDEBUG("srf_fd=%d srf_port=%d srf_flags=%X %X %X \n", 
		loc_fd->srf_fd,
		loc_fd->srf_port, loc_fd->srf_flags, 
		!(loc_fd->srf_flags & SFF_MINOR),
		(loc_fd->srf_flags & SFF_INUSE));
	
	// MODIFICADO POR PAP
//	assert (!(loc_fd->srf_flags & SFF_MINOR) &&
//		(loc_fd->srf_flags & SFF_INUSE));
	
	assert ((loc_fd->srf_flags & SFF_MINOR) &&
		(loc_fd->srf_flags & SFF_INUSE));

	SVRDEBUG("return srf_fd=%d\n", loc_fd->srf_fd);

	return loc_fd;
}

void sr_reply_(mq_t *mq,int status,int is_revive)
{
	int result, proc, ref,operation;
	message reply, *mp;

	proc= mq->mq_mess.NDEV_PROC;
	ref= 0;
	operation= mq->mq_mess.m_type;

	SVRDEBUG("proc=%d status=%d operation=%X is_revive=%d\n", 
			proc,  status, operation, is_revive);
	if (is_revive)
		mp= &mq->mq_mess;
	else
		mp= &reply;

	mp->m_type= DEVICE_REPLY;
	mp->REP_ENDPT= proc;
	mp->REP_STATUS= status;
	SVRDEBUG("dest=%d " MSG2_FORMAT, mq->mq_mess.m_source, MSG2_FIELDS(mp));
	if (is_revive)	{
		mnx_notify(mq->mq_mess.m_source);
		result= EMOLLOCKED;
	} else
		result= mnx_send(mq->mq_mess.m_source, mp);

	if (result == EMOLLOCKED && is_revive)	{
		mq->mq_next= NULL;
		if (repl_queue)
			repl_queue_tail->mq_next= mq;
		else
			repl_queue= mq;
		repl_queue_tail= mq;
		return;
	}
	if (result != OK)
		ip_panic(("unable to send"));
	if (is_revive)
		mq_free(mq);
}

acc_t *sr_get_userdata (int fd, vir_bytes offset,vir_bytes count,int for_ioctl)
{
	sr_fd_t *loc_fd;
	mq_t **head_ptr, *m, *mq;
	int ip_flag, susp_flag, first_flag;
	int result, suspended, is_revive;
	char *src;
	acc_t *acc;
	event_t *evp;
	ev_arg_t arg;
	message *m_ptr;

	SVRDEBUG("fd=%d offset=%d count=%d for_ioctl=%d\n", 
			fd, offset, count, for_ioctl);

	loc_fd= &sr_fd_table[fd];

	if (for_ioctl)	{
		head_ptr= &loc_fd->srf_ioctl_q;
		evp= &loc_fd->srf_ioctl_ev;
		ip_flag= SFF_IOCTL_IP;
		susp_flag= SFF_IOCTL_SUSP;
		first_flag= SFF_IOCTL_FIRST;
	} else 	{
		head_ptr= &loc_fd->srf_write_q;
		evp= &loc_fd->srf_write_ev;
		ip_flag= SFF_WRITE_IP;
		susp_flag= SFF_WRITE_SUSP;
		first_flag= SFF_WRITE_FIRST;
	}
		
	assert (loc_fd->srf_flags & ip_flag);

	if (!count)	{
		m= *head_ptr;
		mq= m->mq_next;
		*head_ptr= mq;
		result= (int)offset;
		is_revive= !(loc_fd->srf_flags & first_flag);

		m_ptr = &m->mq_mess;
		SVRDEBUG("result=%d is_revive=%d " MSG2_FORMAT, result, is_revive, MSG2_FIELDS(m_ptr));

		sr_reply_(m, result, is_revive);
		suspended= (loc_fd->srf_flags & susp_flag);
		loc_fd->srf_flags &= ~(ip_flag|susp_flag);
		if (suspended) {
			if (mq)	{
				arg.ev_ptr= loc_fd;
				ev_enqueue(evp, sr_event, arg);
			}
		}
		return NULL;
	}

	src= (*head_ptr)->mq_mess.NDEV_BUFFER + offset;
	result= cp_u2b ((*head_ptr)->mq_mess.NDEV_PROC, src, &acc, count);

	return result<0 ? NULL : acc;
}

int sr_put_userdata (int fd, vir_bytes offset,acc_t *data,int for_ioctl)
{
	sr_fd_t *loc_fd;
	mq_t **head_ptr, *m, *mq;
	int ip_flag, susp_flag, first_flag;
	int result, suspended, is_revive;
	char *dst;
	event_t *evp;
	ev_arg_t arg;

	SVRDEBUG("fd=%d offset=%d  for_ioctl=%d\n", 
			fd, offset, for_ioctl);
			
	loc_fd= &sr_fd_table[fd];

	if (for_ioctl) 	{
		head_ptr= &loc_fd->srf_ioctl_q;
		evp= &loc_fd->srf_ioctl_ev;
		ip_flag= SFF_IOCTL_IP;
		susp_flag= SFF_IOCTL_SUSP;
		first_flag= SFF_IOCTL_FIRST;
	} 	else 	{
		head_ptr= &loc_fd->srf_read_q;
		evp= &loc_fd->srf_read_ev;
		ip_flag= SFF_READ_IP;
		susp_flag= SFF_READ_SUSP;
		first_flag= SFF_READ_FIRST;
	}
		
	assert (loc_fd->srf_flags & ip_flag);

	if (!data) 	{
		m= *head_ptr;
		mq= m->mq_next;
		*head_ptr= mq;
		result= (int)offset;
		is_revive= !(loc_fd->srf_flags & first_flag);
		sr_reply_(m, result, is_revive);
		suspended= (loc_fd->srf_flags & susp_flag);
		loc_fd->srf_flags &= ~(ip_flag|susp_flag);
		if (suspended) {
			if (mq) {
				arg.ev_ptr= loc_fd;
				ev_enqueue(evp, sr_event, arg);
			}
		}
		return OK;
	}

	dst= (*head_ptr)->mq_mess.NDEV_BUFFER + offset;
	return cp_b2u (data, (*head_ptr)->mq_mess.NDEV_PROC, dst);
}

void sr_select_res(int fd, unsigned ops)
{
	sr_fd_t *sr_fd;

	sr_fd= &sr_fd_table[fd];

	SVRDEBUG("srf_fd=%d ops=%X\n", sr_fd->srf_fd ,ops);
	
	if (ops & SR_SELECT_READ) sr_fd->srf_flags |= SFF_SELECT_R;
	if (ops & SR_SELECT_WRITE) sr_fd->srf_flags |= SFF_SELECT_W;
	if (ops & SR_SELECT_EXCEPTION) sr_fd->srf_flags |= SFF_SELECT_X;

	mnx_notify(sr_fd->srf_select_proc);
}

void process_req_q(mq_t *mq,mq_t  *tail,mq_t  **tail_ptr)
{
	mq_t *m;
	int result;

	SVRDEBUG("\n");

	for(;mq;) {
		m= mq;
		mq= mq->mq_next;

		result= sr_rwio(m);
		if (result == SUSPEND) 	{
			if (mq)	{
				(*tail_ptr)->mq_next= mq;
				*tail_ptr= tail;
			}
			return;
		}
	}
	return;
}

void sr_event(event_t *evp, ev_arg_t arg)
{
	sr_fd_t *sr_fd;
	int r;

	SVRDEBUG("\n");

	sr_fd= arg.ev_ptr;
	if (evp == &sr_fd->srf_write_ev) {
		while(sr_fd->srf_write_q) {
			r= sr_restart_write(sr_fd);
			if (r == SUSPEND)
				return;
		}
		return;
	}
	if (evp == &sr_fd->srf_read_ev) {
		while(sr_fd->srf_read_q) {
			r= sr_restart_read(sr_fd);
			if (r == SUSPEND)
				return;
		}
		return;
	}
	if (evp == &sr_fd->srf_ioctl_ev) {
		while(sr_fd->srf_ioctl_q) {
			r= sr_restart_ioctl(sr_fd);
			if (r == SUSPEND)
				return;
		}
		return;
	}
	ip_panic(("sr_event: unkown event\n"));
}

int cp_u2b (int proc, char *src,acc_t **var_acc_ptr,int size)
{
	static message mess;
	acc_t *acc;
	int i;
	struct vir_cp_req *vcp_ptr;

	acc= bf_memreq(size);

	SVRDEBUG("proc=%d size=%d \n", proc, size);

	*var_acc_ptr= acc;
	i=0;

	while (acc)
	{
		size= (vir_bytes)acc->acc_length;
		vir_cp_req[i].count= size;
		vir_cp_req[i].src.proc_nr_e = proc;
//		vir_cp_req[i].src.segment = D;
		vir_cp_req[i].src.offset = (vir_bytes) src;
		vir_cp_req[i].dst.proc_nr_e = this_proc;
//		vir_cp_req[i].dst.segment = D;
		vir_cp_req[i].dst.offset = (vir_bytes) ptr2acc_data(acc);
		vcp_ptr= &vir_cp_req[i];
		SVRDEBUG(VIRCP_FORMAT, VIRCP_FIELDS(vcp_ptr));

		src += size;
		acc= acc->acc_next;
		i++;

		if (i == CPVEC_NR || acc == NULL)	{
			mess.m_type= SYS_VIRVCOPY;
			mess.VCP_VEC_SIZE= i;
			mess.VCP_VEC_ADDR= (char *)vir_cp_req;
			if (mnx_sendrec(SYSTASK(local_nodeid), &mess) <0)
				ip_panic(("unable to mnx_sendrec("));
			if (mess.m_type <0)	{
				bf_afree(*var_acc_ptr);
				*var_acc_ptr= 0;
				return mess.m_type;
			}
			i= 0;
		}
	}
	return OK;
}

int cp_b2u (acc_t *acc_ptr, int proc, char *dest)
{
	static message mess;
	message *m_ptr;
	acc_t *acc;
	int i, size;
	struct vir_cp_req *vcp_ptr;

	SVRDEBUG("proc=%d\n", proc);
	m_ptr  =&mess;
	acc= acc_ptr;
	i=0;

	SVRDEBUG("src=%d dst=%d size=%d\n",this_proc, proc, (vir_bytes)acc->acc_length);
	
	while (acc) {
		size= (vir_bytes)acc->acc_length;
		
		if (size) {
			vir_cp_req[i].src.proc_nr_e = this_proc;
//			vir_cp_req[i].src.segment = D;
			vir_cp_req[i].src.offset= (vir_bytes)ptr2acc_data(acc);
			vir_cp_req[i].dst.proc_nr_e = proc;
//			vir_cp_req[i].dst.segment = D;
			vir_cp_req[i].dst.offset= (vir_bytes)dest;
			vir_cp_req[i].count= size;
			vcp_ptr= &vir_cp_req[i];
			SVRDEBUG(VIRCP_FORMAT, VIRCP_FIELDS(vcp_ptr));
			i++;
		}

		dest += size;
		acc= acc->acc_next;

		if (i == CPVEC_NR || acc == NULL){
			mess.m_type= SYS_VIRVCOPY;
			mess.VCP_VEC_SIZE= i;
			mess.VCP_VEC_ADDR= (char *) vir_cp_req;
			SVRDEBUG(MSG1_FORMAT, MSG1_FIELDS(m_ptr));
			if (mnx_sendrec(SYSTASK(local_nodeid), &mess) <0)
				ip_panic(("unable to mnx_sendrec("));
			SVRDEBUG(MSG1_FORMAT, MSG1_FIELDS(m_ptr));
			if (mess.m_type <0)
			{
				bf_afree(acc_ptr);
				return mess.m_type;
			}
			i= 0;
		}
	}
	bf_afree(acc_ptr);
	return OK;
}

int sr_repl_queue(int proc, int ref, int operation)
{
	mq_t *m, *m_cancel, *m_tmp;
	int result;

	SVRDEBUG("proc=%d ref=%d operation=%X\n", proc,  ref, operation);

	m_cancel= NULL;

	for (m= repl_queue; m;)	{
		if (m->mq_mess.REP_ENDPT == proc){
			assert(!m_cancel);
			m_cancel= m;
			m= m->mq_next;
			continue;
		}
		result= mnx_send(m->mq_mess.m_source, &m->mq_mess);
		if (result != OK)
			ip_panic(("unable to send: %d", result));
		m_tmp= m;
		m= m->mq_next;
		mq_free(m_tmp);
	}
	repl_queue= NULL;
	if (m_cancel) {
		result= mnx_send(m_cancel->mq_mess.m_source, &m_cancel->mq_mess);
		if (result != OK)
			ip_panic(("unable to send: %d", result));
		mq_free(m_cancel);
		return 1;
	}
	return 0;
}

/*
 * $PchId: sr.c,v 1.17 2005/06/28 14:26:16 philip Exp $
 */
