


struct proxies_s {
	proxies_usr_t	px_usr;		
	struct proc 	px_sproxy;	/* sender proxy 			*/
	struct proc 	px_rproxy;	/* receiver proxy 			*/
#if LOCK_PROXY_TYPE == USE_PROXY_RWLOCK
	rwlock_t   	px_rwlock;	/* LINUX RW lock to protect this PAIR OF PROXIES */	
#elif LOCK_PROXY_TYPE == USE_PROXY_MUTEX
	struct mutex 	px_mutex;	/* LINUX mutex to protect this PAIR OF PROXIES	*/	
#elif LOCK_PROXY_TYPE == USE_PROXY_RWSEM
	struct rw_semaphore px_rwsem;	/* LINUX RW semaphore to protect this PAIR OF PROXIES	*/
#elif LOCK_PROXY_TYPE == USE_PROXY_RCU
 	spinlock_t px_spinlock;		/* LINUX spinlock to protect this PAIR OF PROXIES */
#endif
};
typedef struct proxies_s proxies_t;







