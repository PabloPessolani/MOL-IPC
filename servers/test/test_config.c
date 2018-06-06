/***********************************************
	TEST CONFIGURATION FILE
*  Sample File format
*
machine MYMINIX {
	size		64;
	tokens	32;
	boot_prog	"/usr/src/test/loadvmimg";
	boot_image  "/boot/image/3.1.2H-MHYPERr1875";
	boot_bitmap 0xFFFFFFFF;
	process pm  REAL_PROC_TYPE  BOOT_LOADED  SERVER_LEVEL  DONOT_NTFY ;
	process fs REAL_PROC_TYPE   BOOT_LOADED  SERVER_LEVEL  DONOT_NTFY;
	process tty PROMISCUOUS_PROC_TYPE  BOOT_LOADED  TASK_LEVEL  BOOT_NTFY;
	process at_wini  PROMISCUOUS_PROC_TYPE   EXEC_LOADED   TASK_LEVEL  DONOT_NTFY;
};
machine YOURMINIX {
	size	128;
	tokens	64;
	boot_prog	"/usr/src/test/loadvmimg";
	boot_image  "/boot/image/3.1.2H-MHYPERr1875";
	boot_bitmap 0x0F0F0F0F;
	process pm  REAL_PROC_TYPE  BOOT_LOADED  SERVER_LEVEL  DONOT_NTFY ;
	process fs REAL_PROC_TYPE   BOOT_LOADED  SERVER_LEVEL  DONOT_NTFY;
	process tty	PROMISCUOUS_PROC_TYPE  BOOT_LOADED  TASK_LEVEL  BOOT_NTFY;
	process at_wini  PROMISCUOUS_PROC_TYPE   EXEC_LOADED   TASK_LEVEL  DONOT_NTFY;
};

**************************************************/
#include <sys/types.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <unistd.h>
#include <limits.h>
#include "test.h"

#define  DISABLED_PROC_TYPE		0x0000	/* the process will never run in DCx					*/
#define  REAL_PROC_TYPE			0x0001	/* the process will run in the DCx without changes 			*/
#define  VIRTUAL_PROC_TYPE		0x0002	/* the process will never run but its slot is used to receive messages  */
#define  PROMISCUOUS_PROC_TYPE	0x0004	/* the process is prepared to receive requests from ANY DC	 	*/
#define  MH_TYPE(x)				( x & 0x000F)

#define  BOOT_LOADED			0x0000	/* the process is loaded at DC boot time	*/
#define  EXEC_LOADED			0x0010	/* the process is loaded by EXEC  			*/
#define  MH_LOADED(x)			( x  & 0x0010)

#define  DONOT_NTFY				0x0000	/* the process promiscousus process will NOT be notified of a new DC startup	*/
#define  BOOT_NTFY				0x0020	/* the process promiscousus process will  be notified of a new DC startup	*/
#define  EXEC_NTFY				0x0040	/* the process promiscousus process will  be notified of a new DC startup	*/
#define  MH_NTFY(x)			( x  & 0x0060)

/* This flags will be ORed  with p_misc_flags */
#define  KERNEL_LEVEL			0x0000	/* the process is running at KERNEL LEVEL 	*/
#define  TASK_LEVEL				0x0100	/* the process is running at TASK LEVEL 	*/
#define  SERVER_LEVEL			0x0200	/* the process is running at SERVER LEVEL 	*/
#define  USER_LEVEL				0x0400	/* the process is running at USER LEVEL 	*/
#define  MH_LEVEL(x)			( x & 0x0F00)

#define  PARAVIRTUAL	 		0x1000	/* Only valid for REAL process types 		*/


#define MAXTOKENSIZE	20
#define OK				0
#define EXIT_CODE		1
#define NEXT_CODE		2

#define	TKN_SIZE		0
#define TKN_TOKENS		1
#define TKN_BOOT_PROG	2
#define TKN_BOOT_IMAGE	3
#define TKN_BIT_MAP		4
#define TKN_PROCESS		5

#define nil ((void*)0)

char *cfg_ident[] = {
	"size",
	"tokens",
	"boot_prog",
	"boot_image",
	"boot_bitmap",
	"process",
};
#define NR_IDENT 6

#define MAX_FLAG_LEN 30
struct flag_s {
	char f_name[MAX_FLAG_LEN];
	int f_value;
};
typedef struct flag_s flag_t;	

#define NR_PTYPE	5
flag_t proc_type[] = {
	{"DISABLED_PROC_TYPE",DISABLED_PROC_TYPE},
	{"REAL_PROC_TYPE",REAL_PROC_TYPE},	
	{"VIRTUAL_PROC_TYPE",VIRTUAL_PROC_TYPE},
	{"PROMISCUOUS_PROC_TYPE",PROMISCUOUS_PROC_TYPE},
	{"PARAVIRTUAL_PROC_TYPE",REAL_PROC_TYPE | PARAVIRTUAL }	
};

#define NR_LOAD	2
flag_t proc_load[] = {
	{"BOOT_LOADED",BOOT_LOADED},				
	{"EXEC_LOADED",EXEC_LOADED},			
};

#define NR_LEVEL 4
flag_t proc_level[] = {
	{"KERNEL_LEVEL",KERNEL_LEVEL},			
	{"TASK_LEVEL",TASK_LEVEL},				
	{"SERVER_LEVEL",SERVER_LEVEL},			
	{"USER_LEVEL",USER_LEVEL},				
};

#define NR_NTFY 3
flag_t proc_ntfy[] = {
	{"DONOT_NTFY",DONOT_NTFY},				
	{"BOOT_NTFY",BOOT_NTFY},				
	{"EXEC_NTFY",EXEC_NTFY},	
};

int search_proc(config_t *cfg)
{
	int i, j, rcode;
	unsigned int flags;
	config_t *cfg_lcl;
	
	if( cfg == nil) {
		fprintf(stderr, "No process name at line %d\n", cfg->line);
		return(EXIT_CODE);
	}
	if (config_isatom(cfg)) {
		printf("process=%s\n", cfg->word); 
		cfg = cfg->next;
		if (! config_isatom(cfg)) {
			fprintf(stderr, "Bad argument type at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		cfg_lcl = cfg;
		flags = 0;
		/* 
		* Search for type 
		*/
		for( j = 0; j < NR_PTYPE; j++) {
			if( !strcmp(cfg->word, proc_type[j].f_name)) {
				flags |= proc_type[j].f_value;
				break;
			}
		}
		if( j == NR_PTYPE){
			fprintf(stderr, "No process type defined at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		cfg = cfg->next;
		printf("\t%s\n",proc_type[j].f_name); 	
		if (! config_isatom(cfg)) {
			fprintf(stderr, "Bad argument type at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		/* 
		* Search for load time 
		*/
		for( j = 0; j < NR_LOAD; j++) {
			if( !strcmp(cfg->word, proc_load[j].f_name)) {
				printf("\t%s\n",proc_load[j].f_name); 	
				break;
			}
		}
		if( j == NR_LOAD){
			fprintf(stderr, "No process loading time defined at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		cfg = cfg->next;				
		if (! config_isatom(cfg)) {
			fprintf(stderr, "Bad argument type at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		/* 
		* Search for process level 
		*/
		for( j = 0; j < NR_LEVEL; j++) {
			if( !strcmp(cfg->word, proc_level[j].f_name)) {
				printf("\t%s\n",proc_level[j].f_name); 	
				break;
			}
		}
		if( j == NR_LEVEL){
			fprintf(stderr, "No process level defined at line %d\n", cfg->line);
			return(EXIT_CODE);
		}

		/* 
		* Search for notify  
		*/
		cfg = cfg->next;
		if (! config_isatom(cfg)) {
			fprintf(stderr, "No notify action selected for promiscuous process at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		for( j = 0; j < NR_NTFY; j++) {
			if( !strcmp(cfg->word, proc_ntfy[j].f_name)) {
				printf("\t%s\n",proc_ntfy[j].f_name);  
				break;
			}
		}
		if( j == NR_NTFY){
			fprintf(stderr, "Bad Notify action defined at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
	}else {
		fprintf(stderr, "invalid parameter for process at line %d\n", cfg->line);
		return(EXIT_CODE);
	}
	return(OK);
}

int search_ident(config_t *cfg)
{
	int i, j, rcode;
	
	for( i = 0; cfg!=nil; i++) {
		if (config_isatom(cfg)) {
			printf("search_ident[%d] line=%d word=%s\n",i,cfg->line, cfg->word); 
			for( j = 0; j < NR_IDENT; j++) {
				if( !strcmp(cfg->word, cfg_ident[j])) {
					printf("line[%d] MATCH identifier %s\n", cfg->line, cfg->word); 
					if( cfg->next == nil)
						fprintf(stderr, "Void value found at line %d\n", cfg->line);
					cfg = cfg->next;				
					switch(j){
						case TKN_SIZE:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}					 
							printf("size=%d\n", atoi(cfg->word));
							break;
						case TKN_TOKENS:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							printf("token=%d\n", atoi(cfg->word));
							break;
						case TKN_BOOT_PROG:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							printf("boot_prog=%s\n", cfg->word);
							break;
						case TKN_BOOT_IMAGE:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							printf("boot_image=%s\n", cfg->word);
							break;
						case TKN_BIT_MAP:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							printf("boot_bitmap=%X\n", strtol(cfg->word, (char **)NULL, 16) );
							break;
						case TKN_PROCESS:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							rcode = search_proc(cfg);
							if( rcode) return(rcode);
							break;
						default:
							fprintf(stderr, "Programming Error\n");
							exit(1);
					}
					return(OK);
				}	
			}
			if( j == NR_IDENT)
				fprintf(stderr, "Invaild identifier found at line %d\n", cfg->line);
		}
		cfg = cfg->next;
	}
	return(OK);
}
		
int read_lines(config_t *cfg)
{
	int i, j;
	int rcode;
	for ( i = 0; cfg != nil; i++) {
		printf("read_lines type=%X\n",cfg->flags); 
		rcode = search_ident(cfg->list);
		if( rcode) return(rcode);
		cfg = cfg->next;
	}
	return(OK);
}	

int search_machine(config_t *cfg)
{
	int rcode;
    config_t *name_cfg;
	
    if (cfg != nil) {
		if (config_isatom(cfg)) {
			if( !strcmp(cfg->word, "machine")) {
				printf("TKN_MACHINE ");
				cfg = cfg->next;
				if (cfg != nil) {
					if (config_isatom(cfg)) {
						printf("%s\n", cfg->word);
						name_cfg = cfg;
						cfg = cfg->next;
						if (!config_issub(cfg)) {
							fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n",cfg->word, cfg->line);
							return(EXIT_CODE);
						}
						rcode = read_lines(cfg->list);
						return(rcode);
					}
				}
			}
			fprintf(stderr, "Config error line:%d No machine token found\n", cfg->line);
			return(EXIT_CODE);
		}
		fprintf(stderr, "Config error line:%d No machine name found \n", cfg->line);
		return(EXIT_CODE);
	}
	return(EXIT_CODE);
}

int search_dc_config(config_t *cfg)
{
	int rcode;
	int i;
	
    for( i=0; cfg != nil; i++) {
		if (!config_issub(cfg)) {
			fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n", cfg->word, cfg->line);
			return(EXIT_CODE);
		}
		printf("search_dc_config[%d] line=%d\n",i,cfg->line);
		rcode = search_machine(cfg->list);
		if( rcode == EXIT_CODE)
			return(rcode);
		cfg= cfg->next;
	}
	return(OK);
}



int main(int argc, char **argv)
{
    config_t *cfg;
    int rcode;
	
    if (argc != 2) {
		fprintf(stderr, "One config file name please\n");
		exit(1);
    }

    cfg= nil;
	rcode  = OK;
	cfg= config_read(argv[1], CFG_ESCAPED, cfg);

	rcode = search_dc_config(cfg);
	
}

