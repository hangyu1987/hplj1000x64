#include "zjs_private.h"


typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;



// 短整型大小端互换
#define BigLittleSwap16(A)  ((((UINT16)(A) & 0xff00) >> 8) | (((UINT16)(A) & 0x00ff) << 8))
// 长整型大小端互换

#define BigLittleSwap32(A)  ((((UINT32)(A) & 0xff000000) >> 24) |	(((UINT32)(A) & 0x00ff0000) >> 8) |	(((UINT32)(A) & 0x0000ff00) << 8) |	(((UINT32)(A) & 0x000000ff) << 24))

	// 本机大端返回1，小端返回0
	int checkCPUendian()
{
	union{
		unsigned long int i;
		unsigned char s[4];
	}c;

	c.i = 0x12345678;
	return (0x12 == c.s[0]);
}

// 模拟htonl函数，本机字节序转网络字节序
unsigned long int t_htonl(unsigned long int h)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，转换成大端再返回
	return checkCPUendian() ? h : BigLittleSwap32(h);
}

// 模拟ntohl函数，网络字节序转本机字节序
unsigned long int t_ntohl(unsigned long int n)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，网络数据转换成小端再返回
	return checkCPUendian() ? n : BigLittleSwap32(n);
}

// 模拟htons函数，本机字节序转网络字节序
unsigned short int t_htons(unsigned short int h)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，转换成大端再返回
	return checkCPUendian() ? h : BigLittleSwap16(h);
}

// 模拟ntohs函数，网络字节序转本机字节序
unsigned short int t_ntohs(unsigned short int n)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，网络数据转换成小端再返回
	return checkCPUendian() ? n : BigLittleSwap16(n);
}


uint32_t 	be32(uint32_t dword)
{
	return t_htonl(dword);
}
uint16_t 	be16(uint16_t word)
{
	return t_htons(word);
}