//#define TASKDBG		1
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
#include "data_usr.h"

#include <lz4frame.h>
#define LZ4_HEADER_SIZE 19
#define LZ4_FOOTER_SIZE 4
#define BLOCK_16K	1024 //(16 * 1024) //ver este bloque? 
#define BUFFER_RAW	MAXCOPYBUF

static const LZ4F_preferences_t lz4_preferences = {
	{ LZ4F_max1MB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0, { 0, 0 } },
	0,   /* compression level */
	0,   /* autoflush */
	{ 0, 0, 0, 0 },  /* reserved, must be set to 0 */
};

struct data_desc_s {
	unsigned	*td_payload;	/* uncompressed payload 		*/	
	unsigned 			*td_comp_pl;	/* compressed payload 		*/ 	
	
	LZ4F_errorCode_t td_lz4err;
	size_t			td_maxCsize;		/* Maximum Compressed size */
	size_t			td_maxRsize;		/* Maximum Raw size		 */
	LZ4F_compressionContext_t 	td_lz4Cctx __attribute__((aligned(8))); /* Compression context */
	LZ4F_decompressionContext_t td_lz4Dctx __attribute__((aligned(8))); /* Decompression context */
};
typedef struct data_desc_s data_desc_t;

data_desc_t rdesc;

char *buffer_raw; /*datos crudos previo a comprimir*/
size_t	buff_raw_maxCsize; 
size_t  raw_maxCsize;

void init_compression( data_desc_t *data_ptr ) 
{
	size_t frame_size;

	TASKDEBUG("init_compression\n");

	/*LZ4F_compressBound: provee el tamaño mínimo para dst_buffer de acuerdo al src_buffer, que sirva
	para manejar las peores situaciones. 
	Puede variar según lo que se indique en las preferencias*/
	
	TASKDEBUG("data_ptr->td_maxRsize: %d\n", data_ptr->td_maxRsize);
	
	frame_size = LZ4F_compressBound(data_ptr->td_maxRsize, &lz4_preferences); 
	
	TASKDEBUG("frame_size: %d\n", frame_size);
	
	/*maxCsize: max Compress size*/
	data_ptr->td_maxCsize =  frame_size + LZ4_HEADER_SIZE + LZ4_FOOTER_SIZE; /*size dst_buffer to compression*/
	TASKDEBUG("data_ptr->td_maxCsize: %d\n", data_ptr->td_maxCsize);
	
	/*dst_buffer to compression*/
	posix_memalign( (void**) &data_ptr->td_comp_pl, getpagesize(), data_ptr->td_maxCsize );
	
	if (data_ptr->td_comp_pl== NULL) {
    		fprintf(stderr, "data_ptr->td_comp_pl posix_memalign");
			exit(errno);
  	}
	TASKDEBUG("END_init_compression\n");
}

int compress_payload( data_desc_t *data_ptr )
{
	size_t comp_len;
	
	TASKDEBUG("compress_payload\n");

	/* size_t LZ4F_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, 
				size_t srcSize, const LZ4F_preferences_t* preferencesPtr);
	*/

	/*comprime el srcBuffer completo en una LZ4_frame según las especificaciones v1.5.1
	
	!!!: el tamaño del dstBuffer (dstMaxSize) debe ser suficientemente extenso como para asegurar
	que se pueda terminar la compresión aún en los peores casos (--> LZ4F_compressFrameBound() permite
	obtener el mínimo requerido). Si esto no se respeta devuelve un "errorCode"
	
	Las preferencias pueden ser seteadas por defecto o dejarse NULL
	
	Resultado: cantidad de bytes escritos en dstBuffer
	
	Si hay errores, pueden testearse con LZ4F_isError()*/
	
	comp_len = LZ4F_compressFrame(
				data_ptr->td_comp_pl,	data_ptr->td_maxCsize,
				data_ptr->td_payload,	data_ptr->td_maxRsize, 
				NULL);
	
	
	TASKDEBUG("comp_len: %d - data_ptr->td_comp_pl: %X - data_ptr->td_maxCsize: %d - data_ptr->td_payload: %X - data_ptr->td_td_maxRsize: %d\n",
			comp_len, data_ptr->td_comp_pl,data_ptr->td_maxCsize,data_ptr->td_payload,data_ptr->td_maxRsize);
				
				
	if (LZ4F_isError(comp_len)) {
		fprintf(stderr ,"LZ4F_compressFrame failed: error %zu, %s\n", comp_len, LZ4F_getErrorName(comp_len));
		exit(comp_len);
	}

	TASKDEBUG("COMPRESS_DATA: buffCsize=%d comp_len=%d\n", data_ptr->td_maxCsize, comp_len);
	
	TASKDEBUG("END_compress_payload\n");	
	return(comp_len);
}

void stop_compression( data_desc_t *data_ptr) 
{
	TASKDEBUG("stop_compression\n");
	
	free(data_ptr->td_comp_pl);
	TASKDEBUG("free dst_buffer(td_comp_ln)\n");
}

void init_decompression( data_desc_t *data_ptr) 
{
	LZ4F_errorCode_t lz4_rcode;

	TASKDEBUG("init_decompression\n");

	/*maxRsize = max Raw size*/
	data_ptr->td_maxRsize =  data_ptr->td_maxCsize + LZ4_HEADER_SIZE + LZ4_FOOTER_SIZE;
	TASKDEBUG("data_ptr->td_maxCsize = %d\n", data_ptr->td_maxRsize);
	
	/*dst_buffer to decompression*/
	posix_memalign( (void**) &data_ptr->td_payload, getpagesize(), data_ptr->td_maxRsize ); /*buffer donde quedan los datos descomprimidos*/
	
	if (data_ptr->td_payload== NULL) {
    		fprintf(stderr, "data_ptr->td_payload-posix_memalign\n");
			exit(errno);
  	}
	
	/*LZ4F_createDecompressionContext: crea un objeto que puede ser usado en todas las operaciones de descompresión y otorga un puntero
	a dicho objeto: asignado e inicializado. En nuestro caso: tl_lz4Dctx (puede desactivarse con LZ4F_freeDecompressionContext()
	La versión es LZ4F_VERSION.
	
	Resultado: si es un error que puede ser testeado con LZ4F_isError()	
	*/
	
	lz4_rcode =  LZ4F_createDecompressionContext(&data_ptr->td_lz4Dctx, LZ4F_VERSION);
	
	if (LZ4F_isError(lz4_rcode)) {
		fprintf(stderr ,"LZ4F_createDecompressionContext: error %zu, %s\n", lz4_rcode, LZ4F_getErrorName(lz4_rcode));
		exit(lz4_rcode);
	}
	
	TASKDEBUG("END_init_decompression\n");
}

int decompress_payload( data_desc_t *data_ptr)
{
	LZ4F_errorCode_t lz4_rcode;
	size_t comp_len, raw_len;
	
	TASKDEBUG("Decompress_payload: INPUT td_maxRsize=%d td_maxCsize=%d \n", 
		data_ptr->td_maxRsize,data_ptr->td_maxCsize );

	comp_len = data_ptr->td_maxCsize;
	raw_len  = data_ptr->td_maxRsize;
	
	TASKDEBUG("data_ptr->td_payload: %X - data_ptr->td_comp_pl: %X\n", data_ptr->td_payload,data_ptr->td_comp_pl);
	TASKDEBUG("data_ptr->td_payload: %s - data_ptr->td_comp_pl: %s\n", data_ptr->td_payload,data_ptr->td_comp_pl);
	TASKDEBUG("comp_len=%d raw_len=%d \n", comp_len, raw_len );
	TASKDEBUG("data_ptr->td_lz4Dctx=%d \n", data_ptr->td_lz4Dctx );
	
	/*size_t LZ4F_decompress(LZ4F_decompressionContext_t dctx,
                       void* dstBuffer, size_t* dstSizePtr,
                       const void* srcBuffer, size_t* srcSizePtr,
                       const LZ4F_decompressOptions_t* dOptPtr);*/
	
	/*
	Regenera los datos comprimidos en srcBuffer.
	
	Los datos "descomprimidos" son dejados en dstBuffer. El número total de bytes descomprimidos estará limitado
	por el tamaño del dstBuffer 
	
	El número de bytes leídos desde srcBuffer estará limitado por el tamaño del srcBuffer
	
	Es decir en ambos casos, los datos a descomprimir o a leer deben ser <= que el tamaño máx de dichos buffers.
	
	En gral. el buffer destino no suele ser lo suficientemente grande para contener los datos a descomprimir, por lo que
	como la función decompress no se completa, es llamada nuevamente a partir de (srcBuffer + *srcSizePtr)
	*/
					   
	lz4_rcode = LZ4F_decompress(data_ptr->td_lz4Dctx,
                          data_ptr->td_payload, &raw_len,
                          data_ptr->td_comp_pl, &comp_len,
                          NULL);
						  
	TASKDEBUG("LZA4Post= %d\n", lz4_rcode);					  
	TASKDEBUG("data_ptr->td_payload: %s - data_ptr->td_comp_pl: %s\n", data_ptr->td_payload,data_ptr->td_comp_pl);
						  
	if (LZ4F_isError(lz4_rcode)) {
		TASKDEBUG("LZ4F_decompress failed: error %zu, %s\n", lz4_rcode, LZ4F_getErrorName(lz4_rcode));
		exit(lz4_rcode);
	}
	
	TASKDEBUG("END_decompress: OUTPUT raw_len=%d comp_len=%d\n", raw_len, comp_len);
	return(raw_len);					  
						  
}

void stop_decompression( data_desc_t *data_ptr) 
{
	LZ4F_errorCode_t lz4_rcode;

	TASKDEBUG("Stop_decompression\n");

	lz4_rcode = LZ4F_freeDecompressionContext(data_ptr->td_lz4Dctx);
	if (LZ4F_isError(lz4_rcode)) {
		fprintf(stderr ,"LZ4F_freeDecompressionContext: error %zu", lz4_rcode);
		ERROR_EXIT(lz4_rcode);
	}
	
	free(data_ptr->td_payload);
	TASKDEBUG("free dst_buffer(td_payload)\n");
	
	TASKDEBUG("END_Stop_decompression\n");
}

/*===========================================================================*
 *				lz4_data_cd				     *
 *===========================================================================*/
void lz4_data_cd(in_buffer, inbuffer_size, flag_in)
unsigned *in_buffer; 	/* input: compress or uncompress data case flag_in*/
size_t inbuffer_size;
// SP_message *msg_sp; 
int flag_in;	/* condition: COMP=1  - UNCOMP=0*/
{
/* Main program of lz4_data_cd. */

size_t len;

if ( flag_in == UNCOMP ){
	TASKDEBUG("COMPRESS DATA, UNCOMP =%d\n", UNCOMP);
	
	rdesc.td_payload = in_buffer;
	TASKDEBUG("rdesc.td_payload: %X\n", rdesc.td_payload);
	TASKDEBUG("rdesc.td_payload: %s\n", rdesc.td_payload);
	rdesc.td_maxRsize = inbuffer_size;
	TASKDEBUG("rdesc.td_maxRsize: %d\n", rdesc.td_maxRsize);
		
	init_compression(&rdesc);			
		
	len = compress_payload(&rdesc);

	TASKDEBUG("rdesc.td_comp_pl: %X\n", rdesc.td_comp_pl);
	TASKDEBUG("rdesc.td_comp_pl: %s\n", rdesc.td_comp_pl);
	TASKDEBUG("Bytes compress_payload: %d\n", len);
	
	msg_lz4cd.buf.flag_buff = COMP;
	TASKDEBUG("msg_sp->buf.flag_buff: %d\n", msg_lz4cd.buf.flag_buff);
	msg_lz4cd.buf.buffer_size = len;
	TASKDEBUG("msg_sp->buf.buffer_size: %d\n", msg_lz4cd.buf.buffer_size);
	memcpy(msg_lz4cd.buf.buffer_data, rdesc.td_comp_pl, len); 
	TASKDEBUG("msg_sp->buf.buffer_data: %S\n", msg_lz4cd.buf.buffer_data);
	
	stop_compression(&rdesc);
			
	TASKDEBUG("END - COMPRESS DATA\n");
}
else
{
	if ( flag_in == COMP ){
		TASKDEBUG("UNCOMPRESS DATA\n");
		
		rdesc.td_comp_pl = in_buffer;
		TASKDEBUG("rdesc.td_comp_pl: %X\n", rdesc.td_comp_pl);
		TASKDEBUG("rdesc.td_comp_pl: %s\n", rdesc.td_comp_pl);
		rdesc.td_maxCsize = inbuffer_size;
		TASKDEBUG("rdesc.td_maxCsize: %d\n", rdesc.td_maxCsize);
		
		init_decompression(&rdesc);
		
		TASKDEBUG("DECOMPRESS\n");	
		len = decompress_payload(&rdesc);
	
		msg_lz4cd.buf.flag_buff = UNCOMP;
		TASKDEBUG("msg_sp->buf.flag_buff: %d\n", msg_lz4cd.buf.flag_buff);
		msg_lz4cd.buf.buffer_size = len;
		TASKDEBUG("msg_sp->buf.buffer_size: %d\n", msg_lz4cd.buf.buffer_size);
		memcpy(msg_lz4cd.buf.buffer_data, rdesc.td_payload, len); 
		TASKDEBUG("msg_sp->buf.buffer_data: %S\n", msg_lz4cd.buf.buffer_data);
		
		TASKDEBUG("STOP_DECOM\n");
		stop_decompression(&rdesc);
	
		TASKDEBUG("END - UNCOMPRESS DATA\n");

	}
	else
		fprintf(stderr, "ERROR, Usage COMP or UNCOMP to compress or decompress data.\n");
		// exit(EXIT_FAILURE);
}	

}


