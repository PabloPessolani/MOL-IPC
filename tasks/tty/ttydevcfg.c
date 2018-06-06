/***********************************************
	TEST CONFIGURATION FILE
*  Sample File format
*# this is a comment 
tty MY_PSEUDO_TTY {
	major			5;
	minor			0;
	type			TTY_PSEUDO;
	device			"/dev/ttyS0";
};
# this is a comment 
tty MY_TCPIP_TTY {
	major			5;
	minor			1;
	type			TTY_TCPIP;
	device 			"/dev/pty01";
	server          "tcp_server";
	port			3333;
};
# this is a comment 
tty MY_M3IPC_TTY {
	major			5;
	minor			2;
	type			TTY_M3IPC;
	device 			"/dev/pty03";
	endpoint		19;
};
**************************************************/

//#define TASKDBG		1

#include "tty.h"

#define nil ((void*)0)

#define	TKN_MAJOR			0
#define	TKN_MINOR			1
#define TKN_TYPE			2
#define TKN_DEVICE			3
#define TKN_SERVER			4
#define TKN_PORT			5
#define TKN_ENDPOINT		6

#define NR_IDENT 7
char *cfg_ident[] = {
	"major",
	"minor",
	"type",
	"device",
	"server",
	"port",
	"endpoint"
};

#define MAX_FLAG_LEN 30
struct flag_s {
	char f_name[MAX_FLAG_LEN];
	int f_value;
};
typedef struct flag_s flag_t;	

#define NR_TTY_TYPES	3
flag_t img_type[] = {
	{"TTY_PSEUDO",TTY_PSEUDO},
	{"TTY_TCPIP",TTY_TCPIP},	
	{"TTY_M3IPC",TTY_M3IPC}
};

int search_ident(config_t *cfg)
{
	int ident_nr, k,flag_type=0;
	tty_conf_t *cfglcl;

	TASKDEBUG("cfg_tty_nr=%d mandatory=%X cfg=%p\n", cfg_tty_nr, mandatory, cfg);

	cfglcl = &tty_table[cfg_tty_nr].tty_cfg;
	while(cfg!=nil) {
		TASKDEBUG("cfg->line=%d\n",cfg->line); 
		if (config_isatom(cfg)) {
			TASKDEBUG("line=%d word=%s\n",cfg->line, cfg->word); 
			for( ident_nr = 0; ident_nr < NR_IDENT; ident_nr++) {	
//				TASKDEBUG("ident_nr=%d word=>%s< ident=>%s<\n",ident_nr, cfg->word, cfg_ident[ident_nr]); 
				if( !strcmp(cfg->word, cfg_ident[ident_nr])) {
					if( cfg->next == nil)
						fprintf(stderr, "Void value found at line %d\n", cfg->line);
					cfg = cfg->next;				
					switch(ident_nr){
						case TKN_MAJOR:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}					 
							TASKDEBUG("\tMAJOR=%d\n", atoi(cfg->word));
							cfglcl->major = atoi(cfg->word);
							if(cfglcl->major != TTY_PROC_NR) {
								fprintf(stderr, "Invalid major(%d) in line %d\n",cfglcl->major,cfg->line);
								return(EXIT_CODE);								
							}
							CLR_BIT(mandatory, TKN_MAJOR);
							break;
						case TKN_MINOR:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}					 
							TASKDEBUG("\tMINOR=%d\n", atoi(cfg->word));
							#define MAX_MINOR	255
							cfglcl->minor = atoi(cfg->word);
							if((cfglcl->minor > MAX_MINOR) || (cfglcl->minor < 0)) {
								fprintf(stderr, "Invalid minor(%d) in line %d\n",cfglcl->minor,cfg->line);
								return(EXIT_CODE);								
							}
							tty_table[cfg_tty_nr].tty_minor = cfglcl->minor;
							CLR_BIT(mandatory, TKN_MINOR);
							break;							
						case TKN_TYPE:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							for( k = 0; k < NR_TTY_TYPES; k++) {
								if( !strcmp(cfg->word, img_type[k].f_name)) {
									flag_type = img_type[k].f_value;
									break;
								}
							}
							TASKDEBUG("\tTYPE=%s, VALUE(hex)=0x%04x, VALUE(int)=%d\n", 
								cfg->word, flag_type, flag_type);						
							cfglcl->type = flag_type;
							CLR_BIT(mandatory, TKN_MINOR);
							switch(flag_type){
								case TTY_PSEUDO:
									break;
								case TTY_TCPIP:
									SET_BIT(mandatory, TKN_PORT);
									SET_BIT(mandatory, TKN_SERVER);
									break;
								case TTY_M3IPC:
									SET_BIT(mandatory, TKN_ENDPOINT);
									break;
								default:
									ERROR_EXIT(EMOLINVAL);
									break;
							}							
							break;
						case TKN_DEVICE:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							TASKDEBUG("\tDEVICE=%s\n", cfg->word);				
							cfglcl->device = cfg->word;							
							break;
						case TKN_SERVER:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							TASKDEBUG("\tSERVER=%s\n", cfg->word);
							if ( cfglcl->type == TTY_TCPIP ){
								CLR_BIT(mandatory, TKN_SERVER);
							}else{
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);						
								return(EXIT_CODE);
							}	
							cfglcl->server = cfg->word;														
							break;
						case TKN_PORT:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							TASKDEBUG("\tPORT=%d\n", atoi(cfg->word));
							if ( cfglcl->type == TTY_TCPIP){
								if( TEST_BIT(mandatory, TKN_SERVER)){
									fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
									return(EXIT_CODE);
								}
								CLR_BIT(mandatory, TKN_PORT);
							} else {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							} 
							cfglcl->port = atoi(cfg->word);														
							break;
						case TKN_ENDPOINT:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							TASKDEBUG("\tENDPOINT=%d\n", atoi(cfg->word));						
							if ( cfglcl->type == TTY_M3IPC ){
								CLR_BIT(mandatory, TKN_ENDPOINT);
							}else{
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);						
								return(EXIT_CODE);
							}	
							cfglcl->endpoint = atoi(cfg->word);							
							break;																					
						default:
							fprintf(stderr, "Programming Error\n");
							exit(1);
					}
					return(OK);
				}	
			}
			fprintf(stderr, "Invalid identifier found at line %d\n", cfg->line);
			return(EXIT_CODE);
		}		
		cfg = cfg->next;
	}
	TASKDEBUG("return OK\n");

	return(OK);	
}
		
int read_lines(config_t *cfg)
{
	int i;
	int rcode;
	TASKDEBUG("\n");
	for ( i = 0; cfg != nil; i++) {
		// printf("read_lines typedef 				=%X\n",cfg->flags); 
		rcode = search_ident(cfg->list);
		if( rcode) ERROR_RETURN(rcode);
		if( cfg == nil) return(OK);
		cfg = cfg->next;
	}
	return(OK);
}	

int search_tty_tkn(config_t *cfg)
{
	int rcode;
	tty_conf_t *cfglcl;

	TASKDEBUG("\n");
	cfglcl = &tty_table[cfg_tty_nr].tty_cfg;

    if (cfg != nil) {
		if (config_isatom(cfg)) {
			if( !strcmp(cfg->word, "tty")) {		
				mandatory =((1 << TKN_MAJOR) |
							(1 << TKN_MINOR) |
							(1 << TKN_TYPE) |
							(1 << TKN_DEVICE)  );
				cfg = cfg->next;
				if (cfg != nil) {
					if (config_isatom(cfg)) {
						TASKDEBUG("tty: %s\n", cfg->word);
						cfglcl->tty_name = cfg->word;
						cfg = cfg->next;
						if (!config_issub(cfg)) {
							fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n",cfg->word, cfg->line);
							return(EXIT_CODE);
						}
						rcode = read_lines(cfg->list);
						return(rcode);
					}
				}
				if( mandatory != 0){
					fprintf(stderr, "Configuration Error mandatory=0x%X\n", mandatory);
					return(EXIT_CODE);
				}				
			}
			fprintf(stderr, "Config error line:%d No device token found\n", cfg->line);
			return(EXIT_CODE);
		}
		fprintf(stderr, "Config error line:%d No device name found \n", cfg->line);
		return(EXIT_CODE);
	}
	return(EXIT_CODE);
}

/*Searchs for every device configuration and stores in dev_cfg array*/
int search_tty_config(config_t *cfg)
{
	int rcode;
	int i, j;
	TASKDEBUG("\n");

    for( i=0; cfg != nil; i++) {
		if (!config_issub(cfg)) {
			fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n", cfg->word, cfg->line);
			return(EXIT_CODE);
		}
		rcode = search_tty_tkn(cfg->list);
		if( rcode ) ERROR_RETURN(rcode);
		cfg= cfg->next;
		
		/* check for overconfiguration */
		for(  j = 0; j < NR_VTTYS; j++) { 
			if( j == cfg_tty_nr) continue; 
			if( tty_table[j].tty_cfg.minor == tty_table[cfg_tty_nr].tty_cfg.minor  ){
				fprintf(stderr, "Device already configured minor=%d\n", 
					tty_table[cfg_tty_nr].tty_cfg.minor);
				ERROR_EXIT(EMOLINVAL);										
			}
		}
		
		TASKDEBUG("tty_table[%d].tty_cfg.type=%d\n", 
			cfg_tty_nr, tty_table[cfg_tty_nr].tty_cfg.type);
		switch(tty_table[cfg_tty_nr].tty_cfg.type){
			case TTY_PSEUDO:
				break;
			case TTY_TCPIP:
				break;
			case TTY_M3IPC:
				break;
			default:
				ERROR_EXIT(EMOLINVAL);
		}
		cfg_tty_nr++;
	}
	return(OK);
}

int read_config(char *file_conf)
{
    config_t *cfg;
	int rcode;
	
	#define nil ((void*)0)
	cfg= nil;
	TASKDEBUG("file_conf=%s\n", file_conf);
	cfg = config_read(file_conf, CFG_ESCAPED, cfg);
	TASKDEBUG("before  search_tty_config\n");	
	rcode = search_tty_config(cfg);
	if(rcode) ERROR_RETURN(rcode);
	return(rcode );	
}

