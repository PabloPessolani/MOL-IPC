#define ERROR_EXIT(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n",__FILE__, __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
	exit(rcode); \
 }while(0);
 
#define SYSERR(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n",__FILE__, __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
 }while(0)

#define ERROR_PRINT(rcode) SYSERR(rcode)
	 
#define ERROR_RETURN(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n",__FILE__, __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
	return(rcode);\
 }while(0)
   
#define CHECK_P_NR(p_nr)		\
do {\
	if( p_nr < (-dcu.dc_nr_tasks) || p_nr >= dcu.dc_nr_procs) {\
		return(EMOLRANGE);\
	}\
}while(0)

#define MTX_LOCK(x) do{ \
		TASKDEBUG("MTX_LOCK %s \n", #x);\
		pthread_mutex_lock(&x);\
		}while(0)
			
#define MTX_UNLOCK(x) do{ \
		pthread_mutex_unlock(&x);\
		TASKDEBUG("MTX_UNLOCK %s \n", #x);\
		}while(0)	
			
#define COND_WAIT(x,y) do{ \
		TASKDEBUG("COND_WAIT %s %s\n", #x,#y );\
		pthread_cond_wait(&x, &y);\
		}while(0)	
 
#define COND_SIGNAL(x) do{ \
		pthread_cond_signal(&x);\
		TASKDEBUG("COND_SIGNAL %s\n", #x);\
		}while(0)	

	
	
#ifdef ALLOC_LOCAL_TABLE 			
#define ENDPOINT2PTR(ep) 	&proc[_ENDPOINT_P(ep)+dcu.dc_nr_tasks];
#define PROC2PTR(p_nr) 		&proc[p_nr+dcu.dc_nr_tasks];
#else /* ALLOC_LOCAL_TABLE */			
#define ENDPOINT2PTR(ep) 	(proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(ep))
#define PROC2PTR(p_nr) 		(proc_usr_t *) PROC_MAPPED(p_nr)
#endif /* ALLOC_LOCAL_TABLE */
#define PROC2PRIV(p_nr) 	&priv[p_nr+dcu.dc_nr_tasks];
