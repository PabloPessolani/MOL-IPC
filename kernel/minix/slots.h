/* =================================*/
/* SLOT STATUS DESCRIPTOR	             */
/* =================================*/
#ifndef SLOTS_H
#define SLOTS_H

struct slot_s {
	int				s_nr;		/* slot number, for control 			*/
	int				s_endpoint;	/* endpoint number	 			*/
	unsigned int 	s_flags;	/*  slot status flags				*/
	int 			s_owner;	/* Node ID where the process is running 	*/
};
typedef struct slot_s slot_t;

#define SLOTS_FORMAT "s_nr=%d s_endpoint=%d s_flags=%X s_owner=%d\n"
#define SLOTS_FIELDS(p) p->s_nr, p->s_endpoint, p->s_flags, p->s_owner

/* bit for s_flags  */
#define	BIT_SLOT_DONATING	1	/* The owned slot is in been donated to other node 		*/
#define BIT_SLOT_PARTITION	2	/* The slot is owned by other NODE in OTHER partition 	*/

#endif /* SLOTS_H */

