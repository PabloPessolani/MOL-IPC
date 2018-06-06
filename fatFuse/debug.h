

#define SVRDBG	1
#define TESTDBG 1
#define INFODBG 1

#ifdef SVRDBG
 #define SVRDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__ ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define SVRDEBUG(x, args ...) do {} while(0);
#endif 

#ifdef TESTDBG
#define TESTDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__ ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else
#define TESTDEBUG(x, args ...) do {} while(0);
#endif

#ifdef INFODBG
 #define INFODEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else
#define INFODEBUG(x, args ...) do {} while(0);
#endif


