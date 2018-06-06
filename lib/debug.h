

//#define LIBDBG		1

#ifdef LIBDBG
 #define LIBDEBUG(text, args ...) \
 do { \
     printf(" %s:%s:%u:" \
             text ,__FILE__, __FUNCTION__ ,__LINE__, ## args); \
     fflush(stdout);\
 }while(0);
#else 
#define LIBDEBUG(x, args ...)
#endif 



