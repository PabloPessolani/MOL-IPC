/***********************************************
	TEST CONFIGURATION FILE
*  Sample File format *

device MY_FILE_IMG {
	mayor			3
	minor			0;
	type			FILE_IMAGE 
	image_file 		"/usr/src/test/minix3.img";
	replicated		YES
};

device MY_MEMORY_IMG {
	mayor			3
	minor			1;
	type			MEMORY_IMAGE 
	image_file 		"/usr/src/test/minix3.img";
	volatile		YES
	replicated		NO
};

device MY_MEMORY_IMG {
	mayor			3
	minor			2;
	type			NBD_IMAGE 
	image_file 		"/usr/src/test/minix3.img";
	replicated		NO
};
**************************************************/

#define TASKDEBG		1
#define  MOL_USERSPACE	1


#define OPER_NAME 0
#define OPER_OPEN 1
#define OPER_NOP 2
#define OPER_IOCTL 3
#define OPER_PREPARE 4
#define OPER_TRANSF 5
#define OPER_CLEAN 6
#define OPER_GEOM 7
#define OPER_SIG 8
#define OPER_ALARM 9
#define OPER_CANC 10
#define OPER_SEL 11


#include "../rdisk/rdisk.h"
#include "../debug.h"
//#include "../const.h"
#include "../../kernel/minix/configfile.h"
 
#define MAXTOKENSIZE	20
#define OK				0
#define EXIT_CODE		1
#define NEXT_CODE		2

#define FILE_IMAGE		0
#define MEMORY_IMAGE	1
#define NBD_IMAGE		2

#define YES				1
#define NO				0

#define	TKN_MAJOR		0
#define TKN_MINOR		1
#define TKN_TYPE		2
#define TKN_IMAGE_FILE	3
#define TKN_VOLATILE	4
#define TKN_BUFFER		5
#define TKN_REPLICATED	6

#define nil ((void*)0)

char *cfg_ident[] = {
	"major",
	"minor",
	"type",
	"image_file",
	"volatile",
	"buffer",
	"replicated",
};
#define NR_IDENT 7

#define MAX_FLAG_LEN 30
struct flag_s {
	char f_name[MAX_FLAG_LEN];
	int f_value;
};
typedef struct flag_s flag_t;	

#define NR_DTYPE	3
flag_t dev_type[] = {
	{"FILE_IMAGE",FILE_IMAGE},
	{"MEMORY_IMAGE",MEMORY_IMAGE},	
	{"NBD_IMAGE",NBD_IMAGE},
};

#define NR_DBIT	2
flag_t dev_bit[] = {
	{"YES",YES},
	{"NO",NO},	
};

int minor_dev=0;

int bit_flag;

int search_type(config_t *cfg)
{
	int j;
	unsigned int flags;
	config_t *cfg_lcl;
	
	if( cfg == nil) {
		fprintf(stderr, "No type at line %d\n", cfg->line);
		return(EXIT_CODE);
	}
	if (config_isatom(cfg)) {
		TASKDEBUG("type=%s\n", cfg->word); 
		// cfg = cfg->next;
		
		if (! config_isatom(cfg)) {
			fprintf(stderr, "Bad argument type at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		cfg_lcl = cfg;
		flags = 0;
		/* 
		* Search for type 
		*/
		for( j = 0; j < NR_DTYPE; j++) {
			if( !strcmp(cfg->word, dev_type[j].f_name)) {
				flags |= dev_type[j].f_value;
				break;
			}
		}
		if( j == NR_DTYPE){
			fprintf(stderr, "No device type defined at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		// cfg = cfg->next;
		if (!config_isatom(cfg)) {
			fprintf(stderr, "Bad argument type at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		
	}
	return(OK);
}

int search_bit(config_t *cfg)
{
	int j;
	unsigned int flags;
	config_t *cfg_lcl;
	
	if( cfg == nil) {
		fprintf(stderr, "No Bit OK at line %d\n", cfg->line);
		return(EXIT_CODE);
	}
	if (config_isatom(cfg)) {
		TASKDEBUG("Bit=%s\n", cfg->word); 
		// cfg = cfg->next;
		
		if (! config_isatom(cfg)) {
			fprintf(stderr, "Bad argument bit OK at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		cfg_lcl = cfg;
		flags = 0;
		/* 
		* Search for bit: YES / NO
		*/
		for( j = 0; j < NR_DBIT; j++) {
			if( !strcmp(cfg->word, dev_bit[j].f_name)) {
				flags |= dev_bit[j].f_value;
				break;
			}
		}
		if( j == NR_DBIT){
			fprintf(stderr, "No bit OK defined at line %d\n", cfg->line);
			return(EXIT_CODE);
		}
		// cfg = cfg->next;
		if (!config_isatom(cfg)) {
			fprintf(stderr, "Bad argument bit OK at line %d\n", cfg->line);
			return(EXIT_CODE);
		}

		if ( !strcmp(cfg->word, dev_bit[0].f_name)){
			bit_flag = YES;
			TASKDEBUG("dev_bit[0].f_name=%s, bit_flag=%d\n", dev_bit[0].f_name, bit_flag);
			}
		else{
			bit_flag = NO;
			TASKDEBUG("dev_bit[0].f_name=%s, bit_flag=%d\n", dev_bit[0].f_name, bit_flag);
		}
			
	}
		
	return(OK);
}

int search_ident(config_t *cfg)
{
	int i, j, rcode;
	
	for( i = 0; cfg!=nil; i++) {
		if (config_isatom(cfg)) {
			TASKDEBUG("search_ident[%d] line=%d word=%s\n",i,cfg->line, cfg->word); 
			for( j = 0; j < NR_IDENT; j++) {
				if( !strcmp(cfg->word, cfg_ident[j])) {
					TASKDEBUG("line[%d] MATCH identifier %s\n", cfg->line, cfg->word); 
					if( cfg->next == nil)
						fprintf(stderr, "Void value found at line %d\n", cfg->line);
					cfg = cfg->next;				
					switch(j){
						case TKN_MAJOR:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}					 
							TASKDEBUG("major=%d\n", atoi(cfg->word));
							break;
						case TKN_MINOR:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							TASKDEBUG("minor=%d\n", atoi(cfg->word));
							minor_dev=atoi(cfg->word);
							TASKDEBUG("minor_dev=%d\n", minor_dev);
							
							if ((minor_dev < 0) || (minor_dev >= NR_DEVS)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								fprintf(stderr, "Minor number %d, is not 0<= >NR_DEVS(%d)\n", minor_dev,NR_DEVS);
								minor_dev = -1;
								}
							break;
						case TKN_TYPE:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							rcode = search_type(cfg);
							if( rcode) return(rcode);
							break;
						case TKN_IMAGE_FILE:
							if (!config_isstring(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							TASKDEBUG("image_file=%s\n", cfg->word);
							
							if (minor_dev == -1){
								fprintf(stderr, "Check the minor device number for %s in the configure file\n", cfg->word);
							}
							else{
								devvec[minor_dev].img_ptr=(cfg->word);
								devvec[minor_dev].available = 1;
								TASKDEBUG("devvec[%d].img_ptr=%s\n", minor_dev,devvec[minor_dev].img_ptr);
								TASKDEBUG("devvec[%d].available=%d\n", minor_dev,devvec[minor_dev].available);
							}
							break;
						case TKN_VOLATILE:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							rcode = search_bit(cfg);
							if( rcode) return(rcode);
							 break;
						 case TKN_REPLICATED:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}
							rcode = search_bit(cfg);
							
							TASKDEBUG("bit_flag=%d\n", bit_flag);

							if( rcode) return(rcode);
							break;
						case TKN_BUFFER:
							if (!config_isatom(cfg)) {
								fprintf(stderr, "Invalid value found at line %d\n", cfg->line);
								return(EXIT_CODE);
							}					 
							TASKDEBUG("buffer=%u\n", atoi(cfg->word));
							
							devvec[minor_dev].buff_size = (( atoi(cfg->word) < 0 || atoi(cfg->word)) >= BUFF_SIZE)?BUFF_SIZE:atoi(cfg->word);
							TASKDEBUG("devvec[%d].buff_size=%d\n", minor_dev,devvec[minor_dev].buff_size);
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
		TASKDEBUG("próximo cfg\n");
		cfg = cfg->next;
	}
	return(OK);
}
		
int read_lines(config_t *cfg)
{
	int i;
	int rcode;
	for ( i = 0; cfg != nil; i++) {
		TASKDEBUG("read_lines type=%X\n",cfg->flags); 
		rcode = search_ident(cfg->list);
		if( rcode) ERROR_RETURN(rcode);
		if( cfg == nil)return(OK);
		cfg = cfg->next;
	}
	return(OK);
}	

int search_device_tkn(config_t *cfg)
{
	int rcode;
    config_t *name_cfg;
	
    if (cfg != nil) {
		if (config_isatom(cfg)) {
			if( !strcmp(cfg->word, "device")) {
				cfg = cfg->next;
				TASKDEBUG("TKN_DEVICE  ");
				if (cfg != nil) {
					if (config_isatom(cfg)) {
						TASKDEBUG("%s\n", cfg->word);
						name_cfg = cfg;
						cfg = cfg->next;
						if (!config_issub(cfg)) {
							fprintf(stderr, "Cell at \"%s\", line %u is not a sublist\n",cfg->word, cfg->line);
							return(EXIT_CODE);
						}
						rcode = read_lines(cfg->list);
						if(rcode) return(EXIT_CODE);
						return(OK);
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
		TASKDEBUG("search_dc_config[%d] line=%d\n",i,cfg->line);
		rcode = search_device_tkn(cfg->list);
		if( rcode == EXIT_CODE)
			return(rcode);
		count_availables++;
		cfg= cfg->next;
	}
	return(OK);
}

 
/*===========================================================================*
 *				test_config				     *
 *===========================================================================*/
void test_config(char *f_conf)	/* config file name. */
{
/* Main program of test_config. */
config_t *cfg;
int rcode;

cfg = nil;
rcode  = OK;
cfg = config_read(f_conf, CFG_ESCAPED, cfg);

rcode = search_dc_config(cfg);


}

