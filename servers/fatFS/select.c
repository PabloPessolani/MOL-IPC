/* Implement entry point to select system call.
 *
 * The entry points into this file are
 *   do_select:	       perform the SELECT system call
 *   select_callback:  notify select system of possible fid operation 
 *   select_notified:  low-level entry for device notifying select
 *   select_unsuspend_by_endpt: cancel a blocking select on exiting driver
 * 
 * Changes:
 *   6 june 2005  Created (Ben Gras)
 */

// #define SVRDBG    1

#include "fs.h"
#define MAXSELECTS 25

 struct selectentry {
	struct fproc *requestor;	/* slot is free iff this is NULL */
	int req_endpt;
	fd_set readfds, writefds, errorfds;
	fd_set ready_readfds, ready_writefds, ready_errorfds;
	fd_set *vir_readfds, *vir_writefds, *vir_errorfds;
	struct filp *filps[FD_SETSIZE];
	int type[FD_SETSIZE];
	int nfds, nreadyfds;
	clock_t expiry;
	timer_t timer;	/* if expiry > 0 */
} selecttab[MAXSELECTS];

#define SELFD_FILE	0
#define SELFD_PIPE	1
#define SELFD_TTY	2
#define SELFD_INET	3
#define SELFD_LOG	4
#define SEL_FDS		5

int select_reevaluate(struct filp *fp);

int select_request_file(struct filp *f, int *ops, int block);
int select_match_file (struct filp *f);

int select_request_general(struct filp *f, int *ops, int block);
int select_major_match(int match_major, struct filp *file);

void select_cancel_all(struct selectentry *e);
void select_wakeup(struct selectentry *e, int r);
void select_return(struct selectentry *, int);

/* The Open Group:
 * "The pselect() and select() functions shall support
 * regular files, terminal and pseudo-terminal devices,
 * STREAMS-based files, FIFOs, pipes, and sockets."
 */

struct fdtype {
	int (*select_request)(struct filp *, int *ops, int block);	
	int (*select_match)(struct filp *);
	int select_major;
} fdtypes[SEL_FDS] = {
		/* SELFD_FILE */
	{ select_request_file, select_match_file, 0 },
		/* SELFD_TTY (also PTY) */
	{ select_request_general, NULL, TTY_MAJOR },
		/* SELFD_INET */
	{ select_request_general, NULL, INET_MAJOR },
		/* SELFD_PIPE (pipe(2) pipes and FS FIFOs) */
	// { select_request_pipe, select_match_pipe, 0 }, //Commented for later implementation
		/* SELFD_LOG (/dev/klog) */
	{ select_request_general, NULL, LOG_MAJOR },
};

/*===========================================================================*
 *				select_request_file			     *
 *===========================================================================*/
int select_request_file(struct filp *f, int *ops, int block)
{
	/* output *ops is input *ops */
	return SEL_OK;
}

/*===========================================================================*
 *				select_match_file			     *
 *===========================================================================*/
int select_match_file(struct filp *file)
{
	if (file && file->filp_ino && (file->filp_ino->i_mode & I_REGULAR))
		return 1;
	return 0;
}

/*===========================================================================*
 *				select_request_general			     *
 *===========================================================================*/
int select_request_general(struct filp *f, int *ops, int block)
{
	int rops = *ops;
	if (block) rops |= SEL_NOTIFY;
	*ops = dev_io(DEV_SELECT, f->filp_ino->i_zone[0], rops, NULL, 0, 0, 0);
	if (*ops < 0)
		return SEL_ERR;
	return SEL_OK;
}

/*===========================================================================*
 *				select_major_match			     *
 *===========================================================================*/
int select_major_match(int match_major, struct filp *file)
{
	int major;
	if (!(file && file->filp_ino &&
		(file->filp_ino->i_mode & I_TYPE) == I_CHAR_SPECIAL))
		return 0;
	major = (file->filp_ino->i_zone[0] >> MNX_MAJOR) & BYTE;
	if (major == match_major)
		return 1;
	return 0;
}

/*===========================================================================*
 *				tab2ops					     *
 *===========================================================================*/
int tab2ops(int fid, struct selectentry *e)
{
	return (MOL_FD_ISSET(fid, &e->readfds) ? SEL_RD : 0) |
		(MOL_FD_ISSET(fid, &e->writefds) ? SEL_WR : 0) |
		(MOL_FD_ISSET(fid, &e->errorfds) ? SEL_ERR : 0);
}

/*===========================================================================*
 *				ops2tab					     *
 *===========================================================================*/
void ops2tab(int ops, int fid, struct selectentry *e)
{
	if ((ops & SEL_RD) && e->vir_readfds && MOL_FD_ISSET(fid, &e->readfds)
	        && !MOL_FD_ISSET(fid, &e->ready_readfds)) {
		MOL_FD_SET(fid, &e->ready_readfds);
		e->nreadyfds++;
	}
	if ((ops & SEL_WR) && e->vir_writefds && MOL_FD_ISSET(fid, &e->writefds) 
		&& !MOL_FD_ISSET(fid, &e->ready_writefds)) {
		MOL_FD_SET(fid, &e->ready_writefds);
		e->nreadyfds++;
	}
	if ((ops & SEL_ERR) && e->vir_errorfds && MOL_FD_ISSET(fid, &e->errorfds)
		&& !MOL_FD_ISSET(fid, &e->ready_errorfds)) {
		MOL_FD_SET(fid, &e->ready_errorfds);
		e->nreadyfds++;
	}

	return;
}

/*===========================================================================*
 *				copy_fdsets				     *
 *===========================================================================*/
void copy_fdsets(struct selectentry *e)
{
	if (e->vir_readfds)
		sys_vircopy(SELF, D, (vir_bytes) &e->ready_readfds, e->req_endpt, D, (vir_bytes) e->vir_readfds, sizeof(fd_set));
	if (e->vir_writefds)
		sys_vircopy(SELF, D, (vir_bytes) &e->ready_writefds,e->req_endpt, D, (vir_bytes) e->vir_writefds, sizeof(fd_set));
	if (e->vir_errorfds)
		sys_vircopy(SELF, D, (vir_bytes) &e->ready_errorfds,e->req_endpt, D, (vir_bytes) e->vir_errorfds, sizeof(fd_set));

	return;
}

/*===========================================================================*
 *				do_select				      *
 *===========================================================================*/
int do_select(void)
{
	int r, nfds, is_timeout = 1, nonzero_timeout = 0,
		fid, s, block = 0;
	struct timeval timeout;
	nfds = m_in.SEL_NFDS;

	if (nfds < 0 || nfds > FD_SETSIZE)
		return EINVAL;

	for(s = 0; s < MAXSELECTS; s++)
		if (!selecttab[s].requestor)
			break;

	if (s >= MAXSELECTS)
		return ENOSPC;

	selecttab[s].req_endpt = who_e;
	selecttab[s].nfds = 0;
	selecttab[s].nreadyfds = 0;
	memset(selecttab[s].filps, 0, sizeof(selecttab[s].filps));

	/* defaults */
	MOL_FD_ZERO(&selecttab[s].readfds);
	MOL_FD_ZERO(&selecttab[s].writefds);
	MOL_FD_ZERO(&selecttab[s].errorfds);
	MOL_FD_ZERO(&selecttab[s].ready_readfds);
	MOL_FD_ZERO(&selecttab[s].ready_writefds);
	MOL_FD_ZERO(&selecttab[s].ready_errorfds);

	selecttab[s].vir_readfds = (fd_set *) m_in.SEL_READFDS;
	selecttab[s].vir_writefds = (fd_set *) m_in.SEL_WRITEFDS;
	selecttab[s].vir_errorfds = (fd_set *) m_in.SEL_ERRORFDS;

	/* copy args */
	if (selecttab[s].vir_readfds
	 && (r=sys_vircopy(who_e, D, (vir_bytes) m_in.SEL_READFDS,
		SELF, D, (vir_bytes) &selecttab[s].readfds, sizeof(fd_set))) != OK)
		return r;

	if (selecttab[s].vir_writefds
	 && (r=sys_vircopy(who_e, D, (vir_bytes) m_in.SEL_WRITEFDS,
		SELF, D, (vir_bytes) &selecttab[s].writefds, sizeof(fd_set))) != OK)
		return r;

	if (selecttab[s].vir_errorfds
	 && (r=sys_vircopy(who_e, D, (vir_bytes) m_in.SEL_ERRORFDS,
		SELF, D, (vir_bytes) &selecttab[s].errorfds, sizeof(fd_set))) != OK)
		return r;

	if (!m_in.SEL_TIMEOUT)
		is_timeout = nonzero_timeout = 0;
	else
		if ((r=sys_vircopy(who_e, D, (vir_bytes) m_in.SEL_TIMEOUT,
			SELF, D, (vir_bytes) &timeout, sizeof(timeout))) != OK)
			return r;

	/* No nonsense in the timeval please. */
	if (is_timeout && (timeout.tv_sec < 0 || timeout.tv_usec < 0))
		return EINVAL;

	/* if is_timeout if 0, we block forever. otherwise, if nonzero_timeout
	 * is 0, we do a poll (don't block). otherwise, we block up to the
	 * specified time interval.
	 */
	if (is_timeout && (timeout.tv_sec > 0 || timeout.tv_usec > 0))
		nonzero_timeout = 1;

	if (nonzero_timeout || !is_timeout)
		block = 1;
	else
		block = 0; /* timeout set as (0,0) - this effects a poll */

	/* no timeout set (yet) */
	selecttab[s].expiry = 0;

	for(fid = 0; fid < nfds; fid++) {
		int orig_ops, ops, t, type = -1, r;
		struct filp *filp;
	
		if (!(orig_ops = ops = tab2ops(fid, &selecttab[s])))
			continue;
		if (!(filp = selecttab[s].filps[fid] = get_filp(fid))) {
			select_cancel_all(&selecttab[s]);
			return EBADF;
		}

		for(t = 0; t < SEL_FDS; t++) {
			if (fdtypes[t].select_match) {
			   if (fdtypes[t].select_match(filp)) {
#if DEBUG_SELECT
				printf("select: fd %d is type %d ", fid, t);
#endif
				if (type != -1)
					printf("select: double match\n");
				type = t;
			  }
	 		} else if (select_major_match(fdtypes[t].select_major, filp)) {
				type = t;
			}
		}

		/* Open Group:
		 * "The pselect() and select() functions shall support
		 * regular files, terminal and pseudo-terminal devices,
		 * STREAMS-based files, FIFOs, pipes, and sockets. The
		 * behavior of pselect() and select() on file descriptors
		 * that refer to other types of file is unspecified."
		 *
		 * If all types are implemented, then this is another
		 * type of file and we get to do whatever we want.
		 */
		if (type == -1)
		{
#if DEBUG_SELECT
			printf("do_select: bad type\n");
#endif
			return EBADF;
		}

		selecttab[s].type[fid] = type;

		if ((selecttab[s].filps[fid]->filp_select_ops & ops) != ops) {
			int wantops;
			/* Request the select on this fid.  */
#if DEBUG_SELECT
			printf("%p requesting ops %d -> ",
				selecttab[s].filps[fid],
				selecttab[s].filps[fid]->filp_select_ops);
#endif
			wantops = (selecttab[s].filps[fid]->filp_select_ops |= ops);
#if DEBUG_SELECT
			printf("%d\n", selecttab[s].filps[fid]->filp_select_ops);
#endif
			if ((r = fdtypes[type].select_request(filp,
				&wantops, block)) != SEL_OK) {
				/* error or bogus return code.. backpaddle */
				select_cancel_all(&selecttab[s]);
				printf("select: select_request returned error\n");
				return EINVAL;
			}
			if (wantops) {
				if (wantops & ops) {
					/* operations that were just requested
					 * are ready to go right away
					 */
					ops2tab(wantops, fid, &selecttab[s]);
				}
				/* if there are any other select()s blocking
				 * on these operations of this fp, they can
				 * be awoken too
				 */
				select_callback(filp, ops);
			}
#if DEBUG_SELECT
			printf("select request ok; ops returned %d\n", wantops);
#endif
		} else {
#if DEBUG_SELECT
			printf("select already happening on that filp\n");
#endif
		}

		selecttab[s].nfds = fid+1;
		selecttab[s].filps[fid]->filp_selectors++;

#if DEBUG_SELECT
		printf("[fid %d ops: %d] ", fid, ops);
#endif
	}

	if (selecttab[s].nreadyfds > 0 || !block) {
		/* fid's were found that were ready to go right away, and/or
		 * we were instructed not to block at all. Must return
		 * immediately.
		 */
		copy_fdsets(&selecttab[s]);
		select_cancel_all(&selecttab[s]);
		selecttab[s].requestor = NULL;

		/* Open Group:
		 * "Upon successful completion, the pselect() and select()
		 * functions shall return the total number of bits
		 * set in the bit masks."
		 */
#if DEBUG_SELECT
		printf("returning\n");
#endif

		return selecttab[s].nreadyfds;
	}
#if DEBUG_SELECT
		printf("not returning (%d, %d)\n", selecttab[s].nreadyfds, block);
#endif
 
	/* Convert timeval to ticks and set the timer. If it fails, undo
	 * all, return error.
	 */
	if (is_timeout) {
		int ticks;
		/* Open Group:
		 * "If the requested timeout interval requires a finer
		 * granularity than the implementation supports, the
		 * actual timeout interval shall be rounded up to the next
		 * supported value."
		 */
#define USECPERSEC 1000000
		while(timeout.tv_usec >= USECPERSEC) {
			/* this is to avoid overflow with *HZ below */
			timeout.tv_usec -= USECPERSEC;
			timeout.tv_sec++;
		}
		ticks = timeout.tv_sec * HZ +
			(timeout.tv_usec * HZ + USECPERSEC-1) / USECPERSEC;
		selecttab[s].expiry = ticks;
		fs_set_timer(&selecttab[s].timer, ticks, select_timeout_check, s);
#if DEBUG_SELECT
		printf("%d: blocking %d ticks\n", s, ticks);
#endif
	}

	/* if we're blocking, the table entry is now valid. */
	selecttab[s].requestor = fp;

	/* process now blocked */
	suspend(XSELECT);
	return SUSPEND;
}

/*===========================================================================*
 *				select_cancel_all			     *
 *===========================================================================*/
void select_cancel_all(struct selectentry *e)
{
	int fid;

	for(fid = 0; fid < e->nfds; fid++) {
		struct filp *fp;
		fp = e->filps[fid];
		if (!fp) {
#if DEBUG_SELECT
			printf("[ fid %d/%d NULL ] ", fid, e->nfds);
#endif
			continue;
		}
		if (fp->filp_selectors < 1) {
#if DEBUG_SELECT
			printf("select: %d selectors?!\n", fp->filp_selectors);
#endif
			continue;
		}
		fp->filp_selectors--;
		e->filps[fid] = NULL;
		select_reevaluate(fp);
	}

	if (e->expiry > 0) {
#if DEBUG_SELECT
		printf("cancelling timer %d\n", e - selecttab);
#endif
		fs_cancel_timer(&e->timer); 
		e->expiry = 0;
	}

	return;
}

/*===========================================================================*
 *				select_wakeup				     *
 *===========================================================================*/
void select_wakeup(struct selectentry *e, int r)
{
	revive(e->req_endpt, r);
}

/*===========================================================================*
 *				select_reevaluate			     *
 *===========================================================================*/
int select_reevaluate(struct filp *fp)
{
	int s, remain_ops = 0, fid, type = -1;

	if (!fp) {
		printf("fs: select: reevalute NULL fp\n");
		return 0;
	}

	for(s = 0; s < MAXSELECTS; s++) {
		if (!selecttab[s].requestor)
			continue;
		for(fid = 0; fid < selecttab[s].nfds; fid++)
			if (fp == selecttab[s].filps[fid]) {
				remain_ops |= tab2ops(fid, &selecttab[s]);
				type = selecttab[s].type[fid];
			}
	}

	/* If there are any select()s open that want any operations on
	 * this fid that haven't been satisfied by this callback, then we're
	 * still in the market for it.
	 */
	fp->filp_select_ops = remain_ops;
#if DEBUG_SELECT
	printf("remaining operations on fp are %d\n", fp->filp_select_ops);
#endif

	return remain_ops;
}

/*===========================================================================*
 *				select_return				     *
 *===========================================================================*/
void select_return(struct selectentry *s, int r)
{
	select_cancel_all(s);
	copy_fdsets(s);
	select_wakeup(s, r ? r : s->nreadyfds);
	s->requestor = NULL;
}


 /*===========================================================================*
 *				select_callback			             *
 *===========================================================================*/
int select_callback(struct filp *fp, int ops)
{
	int s, fid, want_ops, type;

	/* We are being notified that file pointer fp is available for
	 * operations 'ops'. We must re-register the select for
	 * operations that we are still interested in, if any.
	 */
	
	want_ops = 0;
	type = -1;
	for(s = 0; s < MAXSELECTS; s++) {
		int wakehim = 0;
		if (!selecttab[s].requestor)
			continue;
		for(fid = 0; fid < selecttab[s].nfds; fid++) {
			if (!selecttab[s].filps[fid])
				continue;
			if (selecttab[s].filps[fid] == fp) {
				int this_want_ops;
				this_want_ops = tab2ops(fid, &selecttab[s]);
				want_ops |= this_want_ops;
				if (this_want_ops & ops) {
					/* this select() has been satisfied. */
					ops2tab(ops, fid, &selecttab[s]);
					wakehim = 1;
				}
				type = selecttab[s].type[fid];
			}
		}
		if (wakehim)
			select_return(&selecttab[s], 0);
	}

	return 0;
}



/*===========================================================================*
 *				select_notified			             *
 *===========================================================================*/
int select_notified(int major, int minor, int selected_ops)
{
	int s, f, t;

#if DEBUG_SELECT
	printf("select callback: %d, %d: %d\n", major, minor, selected_ops);
#endif

	for(t = 0; t < SEL_FDS; t++)
		if (!fdtypes[t].select_match && fdtypes[t].select_major == major)
		    	break;

	if (t >= SEL_FDS) {
#if DEBUG_SELECT
		printf("select callback: no fdtype found for device %d\n", major);
#endif
		return OK;
	}

	/* We have a select callback from major device no.
	 * d, which corresponds to our select type t.
	 */

	for(s = 0; s < MAXSELECTS; s++) {
		int s_minor, ops;
		if (!selecttab[s].requestor)
			continue;
		for(f = 0; f < selecttab[s].nfds; f++) {
			if (!selecttab[s].filps[f] ||
			   !select_major_match(major, selecttab[s].filps[f]))
			   	continue;
			ops = tab2ops(f, &selecttab[s]);
			s_minor =
			(selecttab[s].filps[f]->filp_ino->i_zone[0] >> MNX_MINOR)
				& BYTE;
			if ((s_minor == minor) &&
				(selected_ops & ops)) {
				select_callback(selecttab[s].filps[f], (selected_ops & ops));
			}
		}
	}

	return OK;
}

/*===========================================================================*
 *				init_select  				     *
 *===========================================================================*/
void init_select(void)
{
	int s;

	for(s = 0; s < MAXSELECTS; s++)
		fs_init_timer(&selecttab[s].timer);
}

/*===========================================================================*
 *				select_forget			             *
 *===========================================================================*/
void select_forget(int proc_e)
{
	/* something has happened (e.g. signal delivered that interrupts
	 * select()). totally forget about the select().
	 */
	int s;

	for(s = 0; s < MAXSELECTS; s++) {
		if (selecttab[s].requestor &&
			selecttab[s].req_endpt == proc_e) {
			break;
		}

	}

	if (s >= MAXSELECTS) {
#if DEBUG_SELECT
		printf("select: cancelled select() not found");
#endif
		return;
	}

	select_cancel_all(&selecttab[s]);
	selecttab[s].requestor = NULL;

	return;
}

/*===========================================================================*
 *				select_timeout_check	  	     	     *
 *===========================================================================*/
void select_timeout_check(timer_t *timer)
{
	int s;

	s = tmr_arg(timer)->ta_int;

	if (s < 0 || s >= MAXSELECTS) {
#if DEBUG_SELECT
		printf("select: bogus slot arg to watchdog %d\n", s);
#endif
		return;
	}

	if (!selecttab[s].requestor) {
#if DEBUG_SELECT
		printf("select: no requestor in watchdog\n");
#endif
		return;
	}

	if (selecttab[s].expiry <= 0) {
#if DEBUG_SELECT
		printf("select: strange expiry value in watchdog\n", s);
#endif
		return;
	}

	selecttab[s].expiry = 0;
	select_return(&selecttab[s], 0);

	return;
}

/*===========================================================================*
 *				select_unsuspend_by_endpt  	     	     *
 *===========================================================================*/
void select_unsuspend_by_endpt(int proc_e)
{
	int fid, s;

	for(s = 0; s < MAXSELECTS; s++) {
	  if (!selecttab[s].requestor)
		  continue;
	  for(fid = 0; fid < selecttab[s].nfds; fid++) {
	    int maj;
	    if (!selecttab[s].filps[fid] || !selecttab[s].filps[fid]->filp_ino)
		continue;
	    maj = (selecttab[s].filps[fid]->filp_ino->i_zone[0] >> MNX_MAJOR)&BYTE;
	    if(dmap_driver_match(proc_e, maj)) {
			select_return(&selecttab[s], EMOLAGAIN);
	    }
	  }
	}

	return;
}



