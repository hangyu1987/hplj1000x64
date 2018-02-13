// zjs.h

#pragma once
#ifdef __cplusplus 
extern "C" { 
#endif

typedef struct _ZJS_CFG 
{
	unsigned int econoMode;
	unsigned int paperCode;
	unsigned int source;
	unsigned int media;
	unsigned int video_x;
	unsigned int video_y;
	unsigned int raster_x;
	unsigned int raster_y;
	unsigned int res_x;
	unsigned int res_y;
	unsigned int paper_custom_x;
	unsigned int paper_custom_y;
	unsigned int page_count;
}ZJS_CFG;

/*Out Buffer should be >4+16+12*4=68*/
int zjs_start_doc(unsigned char *outbuf,unsigned int *outlen);
/*  16+12*13  */
int zjs_write_page(	unsigned char *outbuf,
	unsigned int *outlen,
	unsigned char *inbuf,
	unsigned int* inlen,
	ZJS_CFG cfg
	);
int zjs_end_doc(unsigned char *outbuf,unsigned int *outlen);


#ifdef __cplusplus 
}
#endif
