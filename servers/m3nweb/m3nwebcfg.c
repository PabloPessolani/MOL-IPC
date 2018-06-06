/***********************************************
	TEST CONFIGURATION FILE
*  Sample File format
# this is a comment 
websrv SERVER1 {
	port			80;
	endpoint		20;
	rootdir		"/websrv/server1";
	ipaddr		"192.168.1.100";
};
**************************************************/

//#define SVRDBG		1

#include "m3nweb.h"

#define NR_IDENT 		3
#define TKN_PORT		0
#define TKN_ENDPOINT	1
#define TKN_ROOTDIR		2


char *cfg_ident[] = {
	"port",
	"endpoint",
	"rootdir",
};

int search_ident(config_t *cfg)
{
	int ident_nr;

	SVRDEBUG("mandatory=%X cfg=%p\n", mandatory, cfg);

	while(cfg!=nil) {
		SVRDEBUG("cfg->line=%d\n",cfg->line); 
		if (config_isatom(cfg)) {
			SVRDEBUG("line=%d word=%s\n",cfg->line, cfg->word); 
			for( ident_nr = 0; ident_nr < NR_IDENT; ident_nr++) {	
//				SVRDEBUG("ident_nr=%d word=>%s< ident=>%s<\n",ident_nr, cfg->word, cfg_ident[ident_nr]); 
				if( !strcmp(cfg->word, cfg_ident[ident_nr])) {
					if( cfg->next == nil)
						fprintf(stderr, "Void value found at line %d\n", cfg->line);
					cfg = cfg->next;				
					switch(ident_nr){
						case TKN_ROOTDIR:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_RETURN(EXIT_CODE);
							}
							SVRDEBUG("\tROOTDIR=%s\n", cfg->word);
							CLR_BIT(mandatory, TKN_ROOTDIR);
							nweb_rootdir = cfg->word;		
							break;
						case TKN_ENDPOINT:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_RETURN(EXIT_CODE);
							}
							SVRDEBUG("\tENDPOINT=%d\n", atoi(cfg->word));
							CLR_BIT(mandatory, TKN_ENDPOINT);					
							nweb_ep =  atoi(cfg->word);
							if(nweb_ep  < 0 || nweb_ep >= dc_ptr->dc_nr_sysprocs)
								ERROR_RETURN(EXIT_CODE);						
							break;
						case TKN_PORT:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_RETURN(EXIT_CODE);
							}
							SVRDEBUG("\tPORT=%d htons(PORT)=%d\n", atoi(cfg->word),htons((atoi(cfg->word))));
							CLR_BIT(mandatory, TKN_PORT);
							nweb_port = atoi(cfg->word);
							nweb_addr.sin_port = htons(nweb_port);
							if( nweb_port < 0 || nweb_port >60000)
								ERROR_RETURN(EXIT_CODE);
							break;																			
						default:
							fprintf(stderr, "Programming Error\n");
							exit(1);
					}
					return(OK);
				}	
			}
			fprintf(stderr, "Invalid identifier found at line %d\n", cfg->line);
			ERROR_RETURN(EXIT_CODE);
		}		
		cfg = cfg->next;
	}
	SVRDEBUG("return OK\n");

	return(OK);	
}
		
int read_lines(config_t *cfg)
{
	int i;
	int rcode;
	SVRDEBUG("\n");
	for ( i = 0; cfg != nil; i++) {
		// printf("read_lines typedef 				=%X\n",cfg->flags); 
		rcode = search_ident(cfg->list);
		if( rcode) ERROR_RETURN(rcode);
		if( cfg == nil) return(OK);
		cfg = cfg->next;
	}
	return(OK);
}	

int search_web_tkn(config_t *cfg)
{
	int rcode;
	
	SVRDEBUG("\n");

    if (cfg != nil) {
		if (config_isatom(cfg)) {
			if( !strcmp(cfg->word, "websrv")) {		
				mandatory =((1 << TKN_PORT) |
							(1 << TKN_ENDPOINT) |
							(1 << TKN_ROOTDIR));
				cfg = cfg->next;
				if (cfg != nil) {
					if (config_isatom(cfg)) {
						SVRDEBUG("websrv: %s\n", cfg->word);
						nweb_name = cfg->word;
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
			fprintf(stderr, "Config error line:%d No websrv token found\n", cfg->line);
			return(EXIT_CODE);
		}
		fprintf(stderr, "Config error line:%d No websrv name found \n", cfg->line);
		return(EXIT_CODE);
	}
	return(EXIT_CODE);
}

/*Searchs for every server configuration and stores in web server table */
int search_web_config(config_t *cfg)
{
	int rcode;
	int i;
	
	SVRDEBUG("\n");

    for( i=0; cfg != nil; i++) {
		if (!config_issub(cfg)) {
			fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n", cfg->word, cfg->line);
			return(EXIT_CODE);
		}
		rcode = search_web_tkn(cfg->list);
		if( rcode ) ERROR_RETURN(rcode);
		cfg= cfg->next;
	}
	return(OK);
}

int read_config(char *file_conf)
{
    config_t *cfg;
	int rcode;
	
	#define nil ((void*)0)
	cfg= nil;
	SVRDEBUG("file_conf=%s\n", file_conf);
	cfg = config_read(file_conf, CFG_ESCAPED, cfg);
	SVRDEBUG("before  search_web_config\n");	
	rcode = search_web_config(cfg);
	if(rcode) ERROR_RETURN(rcode);
	return(rcode );	
}

