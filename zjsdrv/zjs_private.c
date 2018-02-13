#pragma warning( disable : 4706 )
#include "zjs_jbig.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "zjs_private.h"
void
error(int fatal, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fatal)
	exit(fatal);
}

/* static int ?here ;cause redefined extern to static(c4211 errror)*/
int
chunk_write_rsvd(unsigned long type, unsigned int rsvd, unsigned long items, unsigned long size, void *fp)
{
    ZJ_HEADER	chunk;
//    int		rc;

    chunk.type = be32(type);
    chunk.items = be32(items);
    chunk.size = be32(sizeof(ZJ_HEADER) + size);
    /*(WORD) ,otherwise C4242 error*/
    chunk.reserved =be16( (WORD)rsvd);
    chunk.signature = 0x5a5a;
    //rc = fwrite(&chunk, 1, sizeof(ZJ_HEADER), fp);
    //if (rc == 0) error(1, "fwrite(1): rc == 0!\n");
	memcpy(fp,&chunk,sizeof(ZJ_HEADER));
	return sizeof(ZJ_HEADER);
}

int
chunk_write(unsigned long type, unsigned long items, unsigned long size,
	    void *fp)
{
    return chunk_write_rsvd(type, 0, items, size, fp);
}

int
item_uint32_write(unsigned short item, unsigned long value, void *fp)
{
    ZJ_ITEM_UINT32 item_uint32;
//    int		rc;

    item_uint32.header.size = be32(sizeof(ZJ_ITEM_UINT32));
    item_uint32.header.item = be16(item);
    item_uint32.header.type = ZJIT_UINT32;
    item_uint32.header.param = 0;
    item_uint32.value = be32(value);
    //rc = fwrite(&item_uint32, 1, sizeof(ZJ_ITEM_UINT32), fp);
    //if (rc == 0) error(1, "fwrite(2): rc == 0!\n");
	memcpy(fp,&item_uint32,sizeof(ZJ_ITEM_UINT32));
	return sizeof(ZJ_ITEM_UINT32);
}

int
item_str_write(unsigned short item, char *str, void *fp)
{
    int			lenpadded;
    ZJ_ITEM_HEADER	hdr;
//    int			rc;
	/*strlen return size_t, otherwise cause C4267 errror*/
    lenpadded = 4 * (((int)strlen(str)+1 + 3) / 4);

    hdr.size = be32(sizeof(hdr) + lenpadded);
    hdr.item = be16(item);
    hdr.type = ZJIT_STRING;
    hdr.param = 0;
	/*
    if (fp)
    {
	rc = fwrite(&hdr, sizeof(hdr), 1, fp);
	if (rc == 0) error(1, "fwrite(3): rc == 0!\n");
	rc = fwrite(str, lenpadded, 1, fp);
	if (rc == 0) error(1, "fwrite(4): rc == 0!\n");
    }
	*/
	memcpy(fp,&hdr,sizeof(hdr));
	memcpy((char *)fp+sizeof(hdr),str,strlen(str));
    return (sizeof(hdr) + lenpadded);
}



void
free_chain(BIE_CHAIN *chain)
{
    BIE_CHAIN	*next;
    next = chain;
    while ((chain = next))
    {
	next = chain->next;
	if (chain->data)
	    free(chain->data);
	free(chain);
    }
}
