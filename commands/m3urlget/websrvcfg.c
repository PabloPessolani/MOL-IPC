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
websrv SERVER2 {
	port			8080;
	endpoint		21;
	rootdir		"/websrv/server2";
	ipaddr		"192.168.1.101";
};
**************************************************/

//#define CMDDBG		1

#include "websrv.h"

#define NR_IDENT 		4
#define TKN_PORT		0
#define TKN_ENDPOINT	1
#define TKN_ROOTDIR		2
#define TKN_IPADDR		3


char *cfg_ident[] = {
	"port",
	"endpoint",
	"rootdir",
	"ipaddr"
};

int search_ident(config_t *cfg)
{
	int ident_nr;
	web_t *cfglcl;

	CMDDEBUG("cfg_web_nr=%d mandatory=%X cfg=%p\n", cfg_web_nr, mandatory, cfg);

	cfglcl = &web_table[cfg_web_nr];
	while(cfg!=nil) {
		CMDDEBUG("cfg->line=%d\n",cfg->line); 
		if (config_isatom(cfg)) {
			CMDDEBUG("line=%d word=%s\n",cfg->line, cfg->word); 
			for( ident_nr = 0; ident_nr < NR_IDENT; ident_nr++) {	
//				CMDDEBUG("ident_nr=%d word=>%s< ident=>%s<\n",ident_nr, cfg->word, cfg_ident[ident_nr]); 
				if( !strcmp(cfg->word, cfg_ident[ident_nr])) {
					if( cfg->next == nil)
						fprintf(stderr, "Void value found at line %d\n", cfg->line);
					cfg = cfg->next;				
					switch(ident_nr){
						case TKN_ROOTDIR:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							CMDDEBUG("\tROOTDIR=%s\n", cfg->word);
							CLR_BIT(mandatory, TKN_ROOTDIR);
							cfglcl->rootdir = cfg->word;														
							break;
						case TKN_IPADDR:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							CMDDEBUG("\tIPADDR=%s\n", cfg->word);
							CLR_BIT(mandatory, TKN_IPADDR);
							cfglcl->svr_addr.sin_addr.s_addr = inet_addr(cfg->word);
							break;
						case TKN_ENDPOINT:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							CMDDEBUG("\tENDPOINT=%d\n", atoi(cfg->word));
							CLR_BIT(mandatory, TKN_ENDPOINT);
							cfglcl->svr_ep =  atoi(cfg->word);												
							break;
						case TKN_PORT:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							CMDDEBUG("\tPORT=%d htons(PORT)=%d\n", atoi(cfg->word),htons((atoi(cfg->word))));
							CLR_BIT(mandatory, TKN_PORT);
							cfglcl->svr_addr.sin_port = htons((atoi(cfg->word)));
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
	CMDDEBUG("return OK\n");

	return(OK);	
}
		
int read_lines(config_t *cfg)
{
	int i;
	int rcode;
	CMDDEBUG("\n");
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
	web_t *cfglcl;

	CMDDEBUG("\n");
	cfglcl = &web_table[cfg_web_nr];

    if (cfg != nil) {
		if (config_isatom(cfg)) {
			if( !strcmp(cfg->word, "websrv")) {		
				mandatory =((1 << TKN_PORT) |
							(1 << TKN_ENDPOINT) |
							(1 << TKN_ROOTDIR));
				cfg = cfg->next;
				if (cfg != nil) {
					if (config_isatom(cfg)) {
						CMDDEBUG("websrv: %s\n", cfg->word);
						cfglcl->svr_name = cfg->word;
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
	int i, j;
	
	CMDDEBUG("\n");

    for( i=0; cfg != nil; i++) {
		if (!config_issub(cfg)) {
			fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n", cfg->word, cfg->line);
			return(EXIT_CODE);
		}
		rcode = search_web_tkn(cfg->list);
		if( rcode ) ERROR_RETURN(rcode);
		cfg= cfg->next;
		
		/* check for overconfiguration */
		for(  j = 0; j < NR_WEBSRVS; j++) { 
			if( j == cfg_web_nr) continue; 
			if( (web_table[j].svr_addr.sin_port == web_table[cfg_web_nr].svr_addr.sin_port) &&
			(web_table[j].svr_addr.sin_addr.s_addr == web_table[cfg_web_nr].svr_addr.sin_addr.s_addr)){
				fprintf(stderr, "websrv already configured at s_addr=0x%X svr_port=%d\n", 
					web_table[cfg_web_nr].svr_addr.sin_addr.s_addr,
					web_table[cfg_web_nr].svr_addr.sin_port);
				ERROR_EXIT(EMOLINVAL);										
			}
		}
		
		CMDDEBUG("web_table[%d]: s_addr=0x%X sin_port=%d \n", cfg_web_nr,
					web_table[cfg_web_nr].svr_addr.sin_addr.s_addr,
					web_table[cfg_web_nr].svr_addr.sin_port);
		cfg_web_nr++;
	}
	return(OK);
}

int read_config(char *file_conf)
{
    config_t *cfg;
	int rcode;
	
	#define nil ((void*)0)
	cfg= nil;
	CMDDEBUG("file_conf=%s\n", file_conf);
	cfg = config_read(file_conf, CFG_ESCAPED, cfg);
	CMDDEBUG("before  search_web_config\n");	
	rcode = search_web_config(cfg);
	if(rcode) ERROR_RETURN(rcode);
	return(rcode );	
}

