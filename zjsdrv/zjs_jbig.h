/*
 * A linked list of compressed data
 */
#include <stdio.h>

typedef struct _BIE_CHAIN{
    unsigned char	*data;
    size_t		len;
    struct _BIE_CHAIN	*next;
} BIE_CHAIN;


void
	error(int fatal, char *fmt, ...);
void
	free_chain(BIE_CHAIN *chain);

int
	chunk_write_rsvd(unsigned long type, unsigned int rsvd,
	unsigned long items, unsigned long size, void *fp);
int
	chunk_write(unsigned long type, unsigned long items, unsigned long size,
	void *fp);
int
	item_uint32_write(unsigned short item, unsigned long value, void *fp);
int
	item_str_write(unsigned short item, char *str, void *fp);