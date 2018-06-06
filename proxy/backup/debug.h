

#define SVRDBG		1

#if SVRDBG
 #define SVRDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__ ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define SVRDEBUG(x, args ...)
#endif 



