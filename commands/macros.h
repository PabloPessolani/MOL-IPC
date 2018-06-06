#define ERROR_EXIT(rcode) \
 do { \
     	printf("ERROR: %s:%s:%u: rcode=%d\n",__FILE__ , __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
	exit(rcode); \
 }while(0);

 #define ERROR_PRINT(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n",__FILE__ , __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
 }while(0)
	 
#define SYSERR(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n", __FILE__ , __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
 }while(0)

#define ERROR_RETURN(rcode) \
 do { \
     	fprintf(stderr,"ERROR: %s:%s:%u: rcode=%d\n", __FILE__ , __FUNCTION__ ,__LINE__,rcode); \
	fflush(stderr);\
	return(rcode);\
 }while(0)
   
#define CHECK_P_NR(p_nr)		\
do {\
	if( p_nr < (-dcu.dc_nr_tasks) || p_nr >= dcu.dc_nr_procs) {\
		return(EMOLRANGE);\
	}\
}while(0)

#define ENDPOINT2PTR(ep) 	&proc[_ENDPOINT_P(ep)+dcu.dc_nr_tasks];
#define PROC2PTR(p_nr) 		&proc[p_nr+dcu.dc_nr_tasks];
#define PROC2PRIV(p_nr) 	&priv[p_nr+dcu.dc_nr_tasks];
