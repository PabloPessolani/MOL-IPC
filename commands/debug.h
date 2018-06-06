

#define CMDDBG		1

#ifdef CMDDBG
 #define CMDDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define CMDDEBUG(x, args ...) do {} while(0);
#endif 

#define TESTDBG

#define INFODBG

#ifdef TESTDBG
 #define TESTDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else
#define TESTDEBUG(x, args ...) do {} while(0);
#endif

#ifdef INFODBG
 #define INFODEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__,__FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else
#define INFODEBUG(x, args ...) do {} while(0);
#endif


