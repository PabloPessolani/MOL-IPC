/* MINIX specific signals. These signals are not used by user proceses, 
 * but meant to inform system processes, like the PM, about system events.
 */
 
typedef unsigned long mnxsigset_t;
typedef  mnxsigset_t molsigset_t;

#define SIGKMESS   	  29	/* new kernel message */
#define SIGKSIG    	  30	/* kernel signal pending */
#define SIGKSTOP      31	/* kernel shutting down */