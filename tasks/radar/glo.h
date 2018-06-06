/* EXTERN should be extern except for the table file */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

/* The parameters of the call are kept here. */
EXTERN message m_in;		/* the input message itself */
EXTERN message m_out;		/* the output message used for reply */
EXTERN message *m_ptr;		/* pointer to message */

EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN dc_usr_t  dcu, *dc_ptr;

EXTERN int local_nodeid;
EXTERN int dcid;
EXTERN char svr_name[MAXPROCNAME];
EXTERN char mbr_name[MAXPROCNAME+10];
EXTERN char spread_group[MAXPROCNAME+10];
EXTERN int svr_ep;		

EXTERN mailbox 		rdr_mbox;		/* SPREAD MAILBOX for RADAR */	
EXTERN char		 	mess_in[MAX_MESSLEN];
EXTERN char		 	mess_out[MAX_MESSLEN];
EXTERN  char    	Spread_name[80];
EXTERN  char    	Private_group[MAX_GROUP_NAME];

EXTERN    membership_info  	memb_info;
EXTERN    vs_set_info      	vssets[MAX_VSSETS];
EXTERN    unsigned int     	my_vsset_index;
EXTERN    int              	num_vs_sets;
EXTERN    char             	members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	  char		   		sp_members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	  int		   		sp_nr_mbrs;

EXTERN		int	FSM_state;		/* Finite State Machine state */
EXTERN 		int primary_mbr;
EXTERN 		int nr_nodes;
EXTERN 		int nr_sync;
EXTERN	  	unsigned long int	bm_nodes;
EXTERN	  	unsigned long int	bm_sync;
EXTERN		int synchronized;


