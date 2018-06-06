

#define TASKDBG		1

#ifdef TASKDBG
#define TASKDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__, __FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define TASKDEBUG(x, args ...)
#endif 


