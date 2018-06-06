/***********************************************
	TEST CONFIGURATION FILE
*  Sample File format
*
# this is a comment 
# imagen local en archivo (usa read/write)
device MY_FILE_IMG {		
	major			1;
	minor			0;
	type			FILE_IMG;
	filename 		"/home/MoL_Module/mol-ipc/servers/diskImgs/floppy3RWX.img";
	volatile		NO;	
	root_dev		YES;
	buffer_size		4096;
#	compression 	NO;
};
device MY_ETH {		
	major			9;
	minor			0;
	type			FILE_DEV;
};
device MY_IP {		
	major			9;
	minor			1;
	type			FILE_DEV;
};
device MY_TCP {		
	major		9;
	minor			2;
	type			FILE_DEV;
};
device MY_UDP {		
	major		9;
	minor			3;
	type			FILE_DEV;
};

**************************************************/

// #define SVRDBG    1

#define nil ((void*)0)

#include "fs.h"


#define  MOL_FILE_IMG			0x0100	/* FILE image typedef 	*/
#define  MOL_MEMORY_IMG			0x0200	/* MEMORY image typedef	*/
#define  MOL_RDISK_IMG			0x0300	/* RDISK image typedef  */
#define  MOL_NBD_IMG			0x0400	/* NBD image typedef 	*/
#define  MOL_FILE_DEV			0x0500	/* Device Node		 	*/

#define  MOL_NO					0
#define  MOL_YES				1


#define MAXTOKENSIZE		20
#define OK					0
#define EXIT_CODE			1
#define NEXT_CODE			2


#define	TKN_MAJOR			0
#define	TKN_MINOR			1
#define TKN_TYPE			2
#define TKN_FILENAME		3
#define TKN_VOLATILE		4
#define TKN_ROOT_DEV		5
#define TKN_BUFFER_SIZE		6
#define TKN_SERVER			7
#define TKN_PORT			8
#define TKN_COMPRESSION		9


#define nil ((void*)0)

#define NR_IDENT 10
char *cfg_ident[] = {
	"major",
	"minor",
	"type",
	"filename",
	"volatile",
	"root_dev",
	"buffer_size",
	"server",
	"port",
	"compression"
};

#define MAX_FLAG_LEN 30
struct flag_s {
	char f_name[MAX_FLAG_LEN];
	int f_value;
};
typedef struct flag_s flag_t;	

#define NR_ITYPE	5
flag_t img_type[] = {
	{"FILE_IMG",MOL_FILE_IMG},
	{"MEMORY_IMG",MOL_MEMORY_IMG},	
	{"RDISK_IMG",MOL_RDISK_IMG},
	{"NBD_IMG",MOL_NBD_IMG},
	{"FILE_DEV",MOL_FILE_DEV}	
};

#define NR_YESNO	2
flag_t yesNo_type[] = {
	{"YES", MOL_YES},
	{"NO", MOL_NO}
};

int search_ident(config_t *cfg)
{
	int ident_nr, k, flag_yesNo=0, flag_type=0;
	dconf_t *cfglcl;

	SVRDEBUG("cfg_dev_nr=%d mandatory=%X cfg=%p\n", cfg_dev_nr, mandatory, cfg);

	cfglcl = &dmap_tab[cfg_dev_nr].dmap_cfg;
	while(cfg!=nil) {
		SVRDEBUG("cfg->line=%d\n",cfg->line); 
		if (config_isatom(cfg)) {
			SVRDEBUG("line=%d word=%s\n",cfg->line, cfg->word); 
			for( ident_nr = 0; ident_nr < NR_IDENT; ident_nr++) {				
				if( !strcmp(cfg->word, cfg_ident[ident_nr])) {
					// printf("line[%d] MATCH identifier %s\n", cfg->line, cfg->word); 
					if( cfg->next == nil)
						//fprintf(stderr, "Void value found at line %d\n", cfg->line);
						ERROR_PRINT(stderr);
					cfg = cfg->next;				
					switch(ident_nr){
						case TKN_MAJOR:
							if (!config_isatom(cfg)) {
								//fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}					 
							SVRDEBUG("\tmajor=%d\n", atoi(cfg->word));
							cfglcl->major = atoi(cfg->word);
							if((cfglcl->major >= NR_DEVICES) || (cfglcl->major < 0)) {
								//fprintf(stderr, "Invalid major(%d) in line %d\n",cfglcl->major,cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);								
							}
							CLR_BIT(mandatory, TKN_MAJOR);
							break;
						case TKN_MINOR:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}					 
							SVRDEBUG("\tminor=%d\n", atoi(cfg->word));
							cfglcl->minor = atoi(cfg->word);
							#define MAX_MINOR	255
							if((cfglcl->minor > MAX_MINOR) || (cfglcl->minor < 0)) {
								// fprintf(stderr, "Invalid minor(%d) in line %d\n",cfglcl->minor,cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);								
							}
							CLR_BIT(mandatory, TKN_MINOR);
							break;							
						case TKN_TYPE:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}
							for( k = 0; k < NR_ITYPE; k++) {
								if( !strcmp(cfg->word, img_type[k].f_name)) {
									flag_type = img_type[k].f_value;
									break;
								}
							}
							SVRDEBUG("\tTYPE=%s, VALUE(hex)=0x%04x, VALUE(int)=%d\n", 
								cfg->word, flag_type, flag_type);						
							cfglcl->type = flag_type;
							CLR_BIT(mandatory, TKN_MINOR);
							switch(flag_type){
								case MOL_FILE_IMG:
									SET_BIT(mandatory, TKN_FILENAME);
									break;
								case MOL_MEMORY_IMG:
									SET_BIT(mandatory, TKN_FILENAME);
									break;
								case MOL_FILE_DEV:
									break;
								case MOL_RDISK_IMG:
									break;
								case MOL_NBD_IMG:
									SET_BIT(mandatory, TKN_PORT);
									SET_BIT(mandatory, TKN_SERVER);
									break;
								default:
									ERROR_EXIT(EMOLINVAL);
							}							
							break;
						case TKN_FILENAME:
							if (!config_isstring(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}
							SVRDEBUG("\tfilename=%s\n", cfg->word);
							if ( cfglcl->type == MOL_FILE_IMG || 
								 cfglcl->type == MOL_MEMORY_IMG ){
								CLR_BIT(mandatory, TKN_FILENAME);
							}else{
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);								
								ERROR_PRINT(stderr);
								ERROR_EXIT(EMOLINVAL);
							}							
							cfglcl->filename = cfg->word;							
							break;
						case TKN_VOLATILE:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}
							for( k = 0; k < NR_YESNO; k++) {
								if( !strcmp(cfg->word, yesNo_type[k].f_name)) {
									flag_yesNo = yesNo_type[k].f_value;
									break;
								}
							}							
							SVRDEBUG("\tVOLATILE=%s, VALUE=%d\n", cfg->word, flag_yesNo);
							cfglcl->volatile_type = flag_yesNo;							
							break;
						case TKN_ROOT_DEV:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}
							for( k = 0; k < NR_YESNO; k++) {
								if( !strcmp(cfg->word, yesNo_type[k].f_name)) {
									flag_yesNo = yesNo_type[k].f_value;
									break;
								}
							}							
							SVRDEBUG("\tROOT_DEV=%s, VALUE=%d\n", cfg->word, flag_yesNo);
							cfglcl->root_dev = flag_yesNo;							
							break;
						case TKN_BUFFER_SIZE:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}					 
							SVRDEBUG("\tminor=%d\n", atoi(cfg->word));
							cfglcl->buffer_size = atoi(cfg->word);
							break;													
						case TKN_SERVER:
							if (!config_isstring(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}
							SVRDEBUG("\tSERVER=%s\n", cfg->word);
							if ( cfglcl->type == MOL_NBD_IMG ){
								CLR_BIT(mandatory, TKN_SERVER);
							}else{
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);							
								ERROR_PRINT(stderr);
								ERROR_EXIT(EMOLINVAL);
							}	
							cfglcl->server = cfg->word;														
							break;
						case TKN_PORT:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EMOLINVAL);
							}
							SVRDEBUG("\tPORT=%d\n", atoi(cfg->word));
							if ( cfglcl->type == MOL_NBD_IMG){
								if( TEST_BIT(mandatory, TKN_SERVER)){
									// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
									ERROR_PRINT(stderr);
									ERROR_EXIT(EMOLINVAL);	
								}
								CLR_BIT(mandatory, TKN_PORT);
							} else {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EMOLINVAL);								
							} 
							cfglcl->port = atoi(cfg->word);														
							break;
						case TKN_COMPRESSION:
							if (!config_isatom(cfg)) {
								// fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								ERROR_PRINT(stderr);
								ERROR_EXIT(EXIT_CODE);
							}
							for( k = 0; k < NR_YESNO; k++) {
								if( !strcmp(cfg->word, yesNo_type[k].f_name)) {
									flag_yesNo = yesNo_type[k].f_value;
									break;
								}
							}							
							SVRDEBUG("\tVOLATILE=%s, VALUE=%d\n", cfg->word, flag_yesNo);
							cfglcl->volatile_type = flag_yesNo;							
							break;																					
						default:
							// fprintf(stderr, "Programming Error\n");
							ERROR_PRINT(stderr);
							exit(1);
					}
					return(OK);
				}	
			}
			if( ident_nr == NR_IDENT)
				// fprintf(stderr, "Invalid identifier found at line %d\n", cfg->line);
				ERROR_PRINT(stderr);
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
//		if( cfg != nil) return(OK);
		cfg = cfg->next;
	}
	return(OK);
}	

int search_device(config_t *cfg)
{
	int rcode;
	dconf_t *cfglcl;

	SVRDEBUG("\n");
	cfglcl = &dmap_tab[cfg_dev_nr].dmap_cfg;

    if (cfg != nil) {
		if (config_isatom(cfg)) {
			if( !strcmp(cfg->word, "device")) {
				// printf("\n");
				// printf("TKN_DEVICE **************************");
				mandatory =((1 << TKN_MAJOR) |
							(1 << TKN_MINOR) |
							(1 << TKN_TYPE) );
				cfg = cfg->next;
				if (cfg != nil) {
					if (config_isatom(cfg)) {
						SVRDEBUG("DEVICE: %s\n", cfg->word);
						cfglcl->dev_name = cfg->word;
						cfg = cfg->next;
						if (!config_issub(cfg)) {
							// fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n",cfg->word, cfg->line);
							ERROR_PRINT(stderr);
							ERROR_EXIT(EXIT_CODE);
						}
						rcode = read_lines(cfg->list);
						return(rcode);
					}
				}
				if( mandatory != 0){
					// fprintf(stderr, "Configuration Error mandatory=0x%X\n", mandatory);
					ERROR_PRINT(stderr);
					ERROR_EXIT(EMOLINVAL);								
				}				
			}
			// fprintf(stderr, "Config error line:%d No device token found\n", cfg->line);
			ERROR_PRINT(stderr);
			ERROR_EXIT(EXIT_CODE);
		}
		// fprintf(stderr, "Config error line:%d No device name found \n", cfg->line);
		ERROR_PRINT(stderr);
		ERROR_EXIT(EXIT_CODE);
	}
	ERROR_EXIT(EXIT_CODE);
}

/*Searchs for every device configuration and stores in dev_cfg array*/
int search_dev_config(const config_t *cfg)
{
	int rcode;
	int i;
	SVRDEBUG(" \n");

    for( i=0; cfg != nil; i++) {
		if (!config_issub(cfg)) {
			// fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n", cfg->word, cfg->line);
			ERROR_PRINT(stderr);
			ERROR_EXIT(EXIT_CODE);
		}
		// printf("search_dev_config[%d] line=%d\n",i,cfg->line);
		rcode = search_device(cfg->list);
		if( rcode ) ERROR_RETURN(rcode);
		cfg= cfg->next;
		
		/* check for overconfiguration */
		for(  i = 0; i < NR_DEVICES; i++) { 
			if( i == cfg_dev_nr) continue; 
			if( (dmap_tab[i].dmap_cfg.major == dmap_tab[cfg_dev_nr].dmap_cfg.major) 
//			&&	(dmap_tab[i].dmap_cfg.minor == dmap_tab[cfg_dev_nr].dmap_cfg.minor)
				){
				// fprintf(stderr, "Device already configured major=%d minor=%d\n", 
				// 	dmap_tab[cfg_dev_nr].dmap_cfg.major,
				// 	dmap_tab[cfg_dev_nr].dmap_cfg.minor);
				ERROR_PRINT(stderr);				
				ERROR_EXIT(EMOLINVAL);										
			}
		}
		
		/* fills the revers map */
		dmap_rev[dmap_tab[cfg_dev_nr].dmap_cfg.major] = cfg_dev_nr;
		dmap_tab[cfg_dev_nr].dmap_driver= dmap_tab[cfg_dev_nr].dmap_cfg.major;
		dmap_tab[cfg_dev_nr].dmap_flags = 0;
		switch(dmap_tab[cfg_dev_nr].dmap_cfg.type){
			case MOL_FILE_IMG:
				dmap_tab[cfg_dev_nr].dmap_opcl 	= f_gen_opcl;
				dmap_tab[cfg_dev_nr].dmap_io	= f_gen_io;
				break;
			case MOL_MEMORY_IMG:
				dmap_tab[cfg_dev_nr].dmap_opcl 	= v_gen_opcl;
				dmap_tab[cfg_dev_nr].dmap_io	= v_gen_io;
				break;
			case MOL_RDISK_IMG:
				dmap_tab[cfg_dev_nr].dmap_opcl 	= r_gen_opcl;
				dmap_tab[cfg_dev_nr].dmap_io	= r_gen_io;
				break;
			case MOL_NBD_IMG:
				dmap_tab[cfg_dev_nr].dmap_opcl 	= n_gen_opcl;
				dmap_tab[cfg_dev_nr].dmap_io	= n_gen_io;
				break;
			case MOL_FILE_DEV:
				dmap_tab[cfg_dev_nr].dmap_opcl 	= d_gen_opcl;
				dmap_tab[cfg_dev_nr].dmap_io	= d_gen_io;
				break;
			default:
				ERROR_EXIT(EMOLINVAL);
		}
		cfg_dev_nr++;
	}
	return(OK);
}

int count_root_devs()
{
	int i, countRoot = 0;
	dmap_t *d_ptr;
	dconf_t *dc_ptr;
	
	for (i = 0; i < NR_DEVICES; ++i)
	{
		d_ptr = &dmap_tab[i];
		if( d_ptr->dmap_driver != NONE){
			SVRDEBUG(DMAP_FORMAT, DMAP_FIELDS(d_ptr));	
			dc_ptr	= &d_ptr->dmap_cfg;	
			SVRDEBUG(DCONF_FORMAT, DCONF_FIELDS(dc_ptr));
		}
		if(dmap_tab[i].dmap_cfg.root_dev == 1)
		{
			countRoot++;
			SVRDEBUG("\tCountRoot[%d]=%d\n", i, countRoot);
		}		
	}
	for (i = 0; i < NR_DEVICES; ++i){
		if(dmap_rev[i] != (-1))
			SVRDEBUG("dmap_rev[%d]=%d\n", i, dmap_rev[i]);
	}

	return(countRoot);
}

int get_root_major(void )
{
	int i;
	for (i = 0; i < NR_DEVICES; ++i){
		// SVRDEBUG(MOL_CFG_FORMAT1, MOL_CFG_FIELDS1(moldevcfg[i]));		
		if(dmap_tab[i].dmap_cfg.root_dev == 1)	{
			SVRDEBUG("major=%d\n", dmap_tab[i].dmap_cfg.major);
			return(dmap_tab[i].dmap_cfg.major);
		}				
	}	
	ERROR_EXIT(EMOLNOENT);
}

int read_config(char *file_conf)
{
    config_t *cfg;
	cfg = nil;
	SVRDEBUG("file_conf=%s\n", file_conf);
	cfg = config_read(file_conf, CFG_ESCAPED, cfg);
	return(OK);	
}

