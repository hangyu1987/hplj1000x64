#pragma warning( disable : 4127 )
/*avoid waring if(0)*/
#include "zjs.h"
#include "zjs_private.h"
#include "jbig_ar.h"
#include "jbig.h"
#include "zjs_jbig.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

long JbgOptions[5] =
{
	/* Order */
	JBG_ILEAVE | JBG_SMID,
	/* Options */
	JBG_DELAY_AT | JBG_LRLTWO | JBG_TPDON | JBG_TPBON | JBG_DPON,
	/* L0 */
	128,
	/* MX */
	16,
	/* MY */
	0
};


int zjs_start_doc(unsigned char *outbuf,unsigned int *outlen)
{
	int ret=0;
	int l=0;
	unsigned char *p=outbuf;
//	int nitems=4;
//	int size = nitems * sizeof(ZJ_ITEM_UINT32);
	memcpy(p,"JZJZ",4);
	l+=4,p+=4;
/* if(1) or if(0) cause c2220 error
	if(0)	//from foo2zjs 
	{
		ret=chunk_write(ZJT_START_DOC, 4, 4*sizeof(ZJ_ITEM_UINT32), p);
		l+=ret,p+=ret;
		ret=ret=item_uint32_write(ZJI_DMCOLLATE,0,p);
		l+=ret,p+=ret;
		ret=ret=item_uint32_write(ZJI_DMDUPLEX, 1,p);
		l+=ret,p+=ret;
		ret=ret=item_uint32_write(ZJI_PAGECOUNT,0,p);
		l+=ret,p+=ret;
		ret=ret=item_uint32_write(ZJI_QUANTITY,	1,p);
		l+=ret,p+=ret;
		*outlen=l;
		return l;
	}
*/
/*
	if(1)	//from pbm2zjs
	{
*/
		ret=chunk_write(ZJT_START_DOC, 2, 2*sizeof(ZJ_ITEM_UINT32), p);
		l+=ret,p+=ret;
		ret=ret=item_uint32_write(ZJI_PAGECOUNT,0,p);
		l+=ret,p+=ret;
		ret=ret=item_uint32_write(ZJI_DMDUPLEX, 1,p);
		l+=ret,p+=ret;
		*outlen=l;
		return l;
/*
	}
*/

}

int zjs_end_doc(unsigned char *outbuf,unsigned int *outlen)
{
	int	nitems;
	nitems = 0;
	*outlen=chunk_write(ZJT_END_DOC , nitems, nitems * sizeof(ZJ_ITEM_UINT32), outbuf);
	return *outlen;
}


void
output_jbig(unsigned char *start, size_t len, void *cbarg)
{
	BIE_CHAIN	*current, **root = (BIE_CHAIN **) cbarg;
	int		size = 65536;	// Printer does strange things otherwise.

	if ( (*root) == NULL)
	{
		(*root) = malloc(sizeof(BIE_CHAIN));
		if (!(*root))
			error(1, "Can't allocate space for chain\n");

		(*root)->data = NULL;
		(*root)->next = NULL;
		(*root)->len = 0;
		size = 20;
		if (len != 20)
			error(1, "First chunk must be BIH and 20 bytes long\n");
	}

	current = *root;  
	while (current->next)
		current = current->next;

	while (len > 0)
	{
//		int	amt, left;
		size_t	amt,left;

		if (!current->data)
		{
			current->data = malloc(size);
			if (!current->data)
				error(1, "Can't allocate space for compressed data\n");
		}

		left = size - current->len;
		amt = (len > left) ? left : len;
		memcpy(current->data + current->len, start, amt); 
		current->len += amt;
		len -= amt;
		start += amt;

		if ((int)current->len == size)
		{
			current->next = malloc(sizeof(BIE_CHAIN));
			if (!current->next)
				error(1, "Can't allocate space for chain\n");
			current = current->next;
			current->data = NULL;
			current->next = NULL;
			current->len = 0;
		}
	}
}

int write_plane(BIE_CHAIN **root, unsigned char *p)
{
	BIE_CHAIN	*current = *root;
	BIE_CHAIN	*next;
	int		len, pad_len;
	#define	PADTO		4
	#define ExtraPad	16
	char buf[ExtraPad+4]={0};
	int ret=0;
	int l=0;
	//unsigned char *p=(unsigned char*)fp;

	for (current = *root; current && current->len; current = current->next)
	{
		if (current == *root)
		{
			/*
			chunk_write(ZJT_JBIG_BIH, 0, current->len, fp);
			fwrite(current->data, 1, current->len, fp);
			*/
			ret=chunk_write(ZJT_JBIG_BIH, 0, (unsigned long)current->len, (void *)p);
			p+=ret,l+=ret;
			memcpy(p,current->data,20);	//this should always be 20!
			p+=20,l+=20;

		}
		else
		{
			len = (int)current->len;
			next = current->next;
			if (!next || !next->len)
				//pad_len = ExtraPad + PADTO * ((len+PADTO-1)/PADTO) - len; //bug
				//pad_len=(len%ExtraPad?ExtraPad-len%ExtraPad:0);
				pad_len=16*((len+15)/16)-len;

			else
				pad_len = 0;
			/*
			chunk_write(ZJT_JBIG_BID, 0, len + pad_len, fp);
			fwrite(current->data, 1, len, fp);
			for (i = 0; i < pad_len; i++ )
				putc(0, fp);
			*/
			ret=chunk_write(ZJT_JBIG_BID, 0, len + pad_len, p);
			p+=ret,l+=ret;
			memcpy(p,current->data,len);
			p+=len,l+=len;
			memcpy(p,buf,pad_len);
			p+=pad_len,l+=pad_len;
		}
	}

	free_chain(*root);

	ret=chunk_write(ZJT_END_JBIG, 0,0, p);
	p+=ret,l+=ret;
	return l;
}

int	pbm_page(unsigned char *inbuf, unsigned int w, unsigned int h, void *p)
{
	BIE_CHAIN		*chain = NULL;
	unsigned char	**bitmaps=(unsigned char **)malloc(sizeof(unsigned char *)*1);
	//unsigned char **bitmaps;
	struct jbg_enc_state se;
	 bitmaps[0] = inbuf;
	 jbg_enc_init(&se, w, h, 1, bitmaps, output_jbig, &chain);
	//jbg_enc_init(&se, w, h, 1, &inbuf, output_jbig, &chain);
	 jbg_enc_options(&se, JbgOptions[0], JbgOptions[1], JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	 jbg_enc_out(&se);
	 jbg_enc_free(&se);
	 free(bitmaps);
	  return write_plane(&chain,  p);

}

int zjs_write_page(	unsigned char *outbuf,
	unsigned int *outlen,
	unsigned char *inbuf,
	unsigned int* inlen,
	ZJS_CFG cfg
	)
{//start page
	int ret=0;
	unsigned char *p=outbuf;
	int l=0;
	//UNREFERENCED_PARAMETER(inlen);
	(inlen);
	if(0)
	{
		int nitems=17;
		int size ;
		size= nitems * sizeof(ZJ_ITEM_UINT32);
		ret=chunk_write(ZJT_START_PAGE,	nitems, nitems * sizeof(ZJ_ITEM_UINT32), p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_ECONOMODE, cfg.econoMode, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(22,1, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_VIDEO_X,cfg.video_x,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_VIDEO_Y, cfg.video_y, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_VIDEO_BPP,1, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RASTER_X,cfg.raster_x,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RASTER_Y,cfg.raster_y,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_MINOLTA_CUSTOM_X,cfg.paperCode == 256 ? cfg.paper_custom_x : 0,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_MINOLTA_CUSTOM_Y,cfg.paperCode == 256 ? cfg.paper_custom_y : 0, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_NBIE,                1,           p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RESOLUTION_X,        cfg.res_x,           p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RESOLUTION_Y,        cfg.res_y,           p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMDEFAULTSOURCE,     cfg.source,     p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMCOPIES,            1,         p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMPAPER,             cfg.paperCode,      p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMMEDIATYPE,         cfg.media,      p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_MINOLTA_PAGE_NUMBER,	cfg.page_count,	p);
		p+=ret;l+=ret;
	}

	if(1)
	{
		int nitems=13;
		int size ;
		size= nitems * sizeof(ZJ_ITEM_UINT32);
		ret=chunk_write(ZJT_START_PAGE,	nitems, nitems * sizeof(ZJ_ITEM_UINT32), p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_ECONOMODE, cfg.econoMode, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RET,1, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_VIDEO_Y,cfg.video_y,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_VIDEO_X, cfg.video_x, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_VIDEO_BPP,1, p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RASTER_Y,cfg.raster_y,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RASTER_X,cfg.raster_x,p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_NBIE,                1,           p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RESOLUTION_Y,        cfg.res_y,           p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_RESOLUTION_X,        cfg.res_y,           p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMDEFAULTSOURCE,     cfg.source,     p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMCOPIES,            1,         p);
		p+=ret,l+=ret;
		ret=item_uint32_write(ZJI_DMPAPER,             cfg.paperCode,      p);
		p+=ret,l+=ret;
	}


	// write page
	ret=pbm_page(inbuf,cfg.video_x,cfg.video_y,p);
	p+=ret,l+=ret;

	// end page
	ret=chunk_write(ZJT_END_PAGE, 0, 0, p);
	p+=ret,l+=ret;

	*outlen=l;
	return l;
}
