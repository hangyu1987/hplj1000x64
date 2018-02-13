//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    DDIHook.cpp
//    
//
//  PURPOSE:  Implementation of DDI Hook OEMEndDoc. This function
//          dumps the buffered bitmap data out. 
//


#include "precomp.h"
#include "bitmap.h"
#include "debug.h"
#include "ole2.h"
#include "zjs.h"

//#pragma comment(lib, "zjs.lib")

// This indicates to Prefast that this is a usermode driver file.
__user_driver;

//Forward:
INT GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
BOOL SaveFrame(POEMPDEV pOemPDEV, PDEVOBJ pDevObj, BOOL fClose);
void Bmp1bppToPbm(PBYTE inBuf,DWORD x,DWORD y,PBYTE outBuf);
void rotate_bmp(PBYTE inBuf,DWORD *x,DWORD *y,PBYTE outBuf);

/*
BOOL APIENTRY OEMStartDoc( SURFOBJ *pso, __in PWSTR pwszDocName, DWORD dwJobId )
{
	UNREFERENCED_PARAMETER(pso);
	PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
	POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;
	log(L"OEMStartDoc\n");
	return (pOemPDEV->m_pfnDrvStartDoc)(pso,pwszDocName,dwJobId);
}
*/

/*****************************************************\
The DrvStartPage function is called by GDI when it is
ready to start sending the contents of a physical page
to the driver for rendering.

Note:
- The pso->format covers the printed area, not the whole page

\*****************************************************/
BOOL APIENTRY OEMStartPage( SURFOBJ* pso) {

	log( L"OEMStartPage");

	PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
	POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

	// --- Write the previous page - if exists ---

	
	if (pOemPDEV->pBufStart) {

		if (SaveFrame( pOemPDEV, pDevObj, false))
			pOemPDEV->m_iframe+=1;
		}


	// --- New page ---
	// Initializing private oempdev stuff
	pOemPDEV->bHeadersFilled = FALSE;
	pOemPDEV->bColorTable = FALSE;
	pOemPDEV->cbHeaderOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	pOemPDEV->bmInfoHeader.biHeight = 0;
	pOemPDEV->bmInfoHeader.biSizeImage = 0;
	pOemPDEV->pBufStart = NULL;
	pOemPDEV->dwBufSize = 0;




	return (pOemPDEV->m_pfnDrvStartPage)(pso);
	}

/*****************************************************\
A printer graphics DLL's DrvSendPage function is called
by GDI when it has finished drawing a physical page, so
the driver can send the page to the printer.

\*****************************************************/
BOOL APIENTRY OEMSendPage( SURFOBJ* pso) {

	PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
	POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

	//TODO, or not todo
	UNREFERENCED_PARAMETER(pOemPDEV);

	/* Does not get called with all apps. It does with Office Word, but with
           allmost or all others it doesn't
	   If it does, then it is called before ImageProcessing
	   */
	
	log( L"OEMSendPage");

	return (pOemPDEV->m_pfnDrvSendPage)(pso);
	}



//
// Function prototype is defined in printoem.h
//
BOOL APIENTRY
OEMEndDoc(
    SURFOBJ     *pso,
    FLONG       fl
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvEndDoc.

    DrvEndDoc is called by GDI when it has finished 
    sending a document to the driver for rendering.
    
    Please refer to DDK documentation for more details.

    This particular implementation of OEMEndDoc performs
    the following operations:
    - Dump the bitmap file header
    - Dump the bitmap info header
    - Dump the color table if one exists
    - Dump the buffered bitmap data
    - Free the memory for the data buffers

Arguments:

    pso - Defines the surface object
    flags - A set of flag bits

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
	UNREFERENCED_PARAMETER(fl);

	log( L"OEMEndDoc");

	PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
	POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;

	
	if (pOemPDEV->pBufStart) {
		// --- Write last page ---
		if (SaveFrame( pOemPDEV, pDevObj, true))
			pOemPDEV->m_iframe+=1;

		// --- Free memory for the data buffers ---
		vFreeBuffer(pOemPDEV);
		}

	// --- Punt call back to UNIDRV ---
	return (pOemPDEV->m_pfnDrvEndDoc)(pso, fl);
}


/*******************************************************\
*
* Save Frame
*
\*******************************************************/
BOOL SaveFrame(POEMPDEV pOemPDEV, PDEVOBJ pDevObj, BOOL fClose) {


	UNREFERENCED_PARAMETER(fClose);

	DWORD imageSize=pOemPDEV->bmInfoHeader.biSizeImage;	
	DWORD imageWidth=(DWORD)pOemPDEV->bmInfoHeader.biWidth;
	DWORD imageHeight=(DWORD)pOemPDEV->bmInfoHeader.biHeight;
	BYTE *pbmBuf =new BYTE[imageSize+1024];
	BYTE *jbgBuf =new BYTE[imageSize];
	UINT jbgBufOutlen=0;
	if(!pbmBuf)
		return FALSE;
	if(!jbgBuf)
		return FALSE;



	if(pOemPDEV->m_iframe==0)
	{
		unsigned char buf[128]={0};
		unsigned int len;
		zjs_start_doc(buf,&len);
		OutputDebugStringA("____ZJS_STARTDOC_____");
		DWORD dwWritten=0;
		dwWritten = pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, buf, len);
	}
	log(TEXT("DWORD SIZE: %u"),sizeof(DWORD));
	OutputDebugStringA("___BMP2PBM____");
	if(imageWidth>imageHeight)
		rotate_bmp(pOemPDEV->pBufStart,&imageWidth,&imageHeight,pbmBuf);
	else
		Bmp1bppToPbm(pOemPDEV->pBufStart,imageWidth,imageHeight,pbmBuf);
	OutputDebugStringA("___BMP2PBM___END_");

	OutputDebugStringA("___ZJS_W____");
	ZJS_CFG cfg;
	cfg.econoMode=0;
	if(imageWidth==4092)
	{
		cfg.paperCode=13;
	}
	else
	{
		cfg.paperCode=9;
	}
	
	cfg.video_x=cfg.raster_x=imageWidth;
	cfg.video_y=cfg.raster_y=imageHeight;
	cfg.res_x=600;
	cfg.res_y=600;
	cfg.paper_custom_x=0;
	cfg.paper_custom_y=0;
	cfg.media=1;
	cfg.source=7;
	cfg.page_count=1+pOemPDEV->m_iframe;


	zjs_write_page(jbgBuf,
		&jbgBufOutlen,
		pbmBuf,
		(UINT *)&imageSize,
		cfg);
	OutputDebugStringA("___ZJS_W__END__");
	log(TEXT("____JBG %d\n"),jbgBufOutlen);
	pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, jbgBuf, jbgBufOutlen);
	OutputDebugStringA("____ZJS_WritePage");

	if(fClose)
	{
		BYTE pBuff[128]={0};
		unsigned int len;
		zjs_end_doc(pBuff,&len);
		pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, pBuff, len);
		OutputDebugStringA("____ZJS_ENDDOC");
	}

	if (pbmBuf)
		delete [] pbmBuf;
	if (jbgBuf)
		delete [] jbgBuf;

	if(pOemPDEV->pBufStart)
	{
		LocalFree(pOemPDEV->pBufStart);
		pOemPDEV->pBufStart=NULL;
		pOemPDEV->dwBufSize=0;
	}


	return true;
	}


/*******************************************************\
*
* Landscape to portrait CC90
*
\*******************************************************/

void rotate_bmp(PBYTE inBuf,DWORD *res_x,DWORD *res_y,PBYTE outBuf)
{

	int bplIn,bplOut;
	int x,y;

	BYTE bin2[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	BYTE p[8]={0};

	x=(int)*res_x;
	y=(int)*res_y;
	bplIn=(int)(x+31)/32;bplIn*=4;
	bplOut=(int)(y+7)/8;

	memset(outBuf,0,x*bplOut);
	for(int i=0;/*i<bplIn*/ i<(x+7)/8 ;i++)
	{
		for(int j=0;j<y;j+=8)// read 8 
		{
			for(int k=0;k<8;k++)
			{
				if( y-1-j-k>=0)
					p[k]=~inBuf[(y-1-j-k)*bplIn+i];
				else
					p[k]=0;
			}
			for(int ki=0;ki<8;ki++)
			{
				for(int m=0;m<8;m++)
				{
					int cor=(i*8+ki)*bplOut+j/8;
					if(cor<=x*bplOut)	
						outBuf[cor] |=  p[m]&bin2[ki]?bin2[m] :0;
				}

			}
		}
	}
	*res_x=y;
	*res_y=x;
	OutputDebugStringA("___Rotate___\n");
}


/*******************************************************\
*
* BMP 1BPP to PBM
*
\*******************************************************/

void Bmp1bppToPbm(PBYTE inBuf,DWORD x,DWORD y,PBYTE outBuf)
{
	DWORD i,j;
	DWORD bpl=(x+7)/8;	//pbm pack 8bit
	DWORD bmpBPL=(x+31)/32;bmpBPL*=4;	//bitmap pack 32bit
	for (i=0;i<y;i++)	//printer only accept 32bit pack, this may not need;
	{
		for(j=0;j<bpl;j++)
		{
			outBuf[i*bpl+j]=~(inBuf[i*bmpBPL+j]);
		}
	}
	OutputDebugStringA("----BmpToPbm---and-invert-black/white---");
}


//==============================================================
void log( WCHAR* pFormat, ...) {

   va_list pArg;
   WCHAR* pblah = new WCHAR[1024];

   va_start( pArg, pFormat);
   _vsnwprintf_s( pblah, 1024, 1024, pFormat, pArg);
   va_end(pArg);

	OutputDebugStringW( pblah);
	delete [] pblah;
   }

