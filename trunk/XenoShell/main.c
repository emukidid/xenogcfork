
#include "main.h"


#define RELEASE




#define clearFB()	memset32((u32*) MEM_FB2, RGB2YCBR(0,0,0), (640*576)/2);

#define ATTRIBUTE_ALIGN(v)	__attribute__((aligned(v)))

u32 *fb = (u32*) (MEM_FB2 + YBORDEROFFSET);

u32 g_aMBTable[0x50 / 4] ATTRIBUTE_ALIGN(32);
u16 g_nX = 32, g_nY = 0;

volatile long *dvd=(volatile long *)0xCC006000;

extern long GetMSR();
extern void SetMSR(long);

extern void dcache_inv(void *, int);
extern void dcache_flush(void *, int);

static void load_apploader(void* pApploader);
static void memcpy32(u32* pDest, u32* pSrc, u32 dwSize);
static void memset32(u32* pDest, u32 dwVal, u32 dwSize);



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//															EXI / IPL Font stuff
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

volatile u32* ebase = (u32*) 0xCC006800;
int font_offset[256], font_size[256], font_height;


static void exi_select(void)
{
	EXI_SR |= 0x150;
}

static void exi_deselect(void)
{
	EXI_SR &= ~0x100;
}

static void exi_write_word(unsigned long word)
{
	EXI_DATA = word;
	EXI_CR = 0x35;
	EXI_WAIT_EOT;
}

static void exi_read(unsigned char *dst, int len)
{
	while (len)
	{
		int l = len;
		if (l > 4)
			l = 4;
		EXI_DATA = 0;
		EXI_CR = 0x31;
		EXI_WAIT_EOT;
		*(unsigned long*)dst = EXI_DATA;
		dst += 4;
		len -= l;
	}
}

static  void ipl_read(unsigned char *dst, int address, int len)
{
	while (len)	{
		exi_select();
		exi_write_word(address << 6);
		exi_read(dst, 0x100);
		exi_deselect();
		dst += 0x100;
		address += 0x100;
		len -= 0x100;
	}
}

static void ipl_set_config(void)
{
	exi_select();
	exi_write_word(0xc0000000);
	exi_write_word(2 << 24);
	exi_deselect();
}

static void untile(unsigned char *dst, unsigned char *src, int xres, int yres)
{
	// 8x8 tiles
	int x, y;
	int t=0;
	for (y = 0; y < yres; y += 8)
		for (x = 0; x < xres; x += 8)
		{
			t = !t;
			int iy, ix;
			for (iy = 0; iy < 8; ++iy, src+=2)
			{
				unsigned char *d = dst + (y + iy) * xres + x;
				for (ix = 0; ix < 2; ++ix)
				{
					int v = src[ix];
					*d++ = ((v>>6)&3);
					*d++ = ((v>>4)&3);
					*d++ = ((v>>2)&3);
					*d++ = ((v)&3);
				}
			}
		}
}


/* Yay0 decompression */
static  void yay0_decode(void *s, void *d)
{
	int i, j, k, p, q, cnt;

	i = *(unsigned long *)(s + 4);	  // size of decoded data
	j = *(unsigned long *)(s + 8);	  // link table
	k = *(unsigned long *)(s + 12);	 // byte chunks and count modifiers

	q = 0;					// current offset in dest buffer
	cnt = 0;				// mask bit counter
	p = 16;					// current offset in mask table

	unsigned long r22 = 0, r5;
	
	do
	{
		// if all bits are done, get next mask
		if(cnt == 0)
		{
			// read word from mask data block
			r22 = *(unsigned long *)(s + p);
			p += 4;
			cnt = 32;   // bit counter
		}
		// if next bit is set, chunk is non-linked
		if(r22 & 0x80000000)
		{
			// get next byte
			*(unsigned char *)(d + q) = *(unsigned char *)(s + k);
			k++, q++;
		}
		// do copy, otherwise
		else
		{
			// read 16-bit from link table
			int r26 = *(unsigned short *)(s + j);
			j += 2;
			// 'offset'
			int r25 = q - (r26 & 0xfff);
			// 'count'
			int r30 = r26 >> 12;
			if(r30 == 0)
			{
				// get 'count' modifier
				r5 = *(unsigned char *)(s + k);
				k++;
				r30 = r5 + 18;
			}
			else r30 += 2;
			// do block copy
			unsigned char *pt = ((unsigned char*)d) + r25;
			int i;
			for(i=0; i<r30; i++)
			{
				*(unsigned char *)(d + q) = *(unsigned char *)(pt - 1);
				q++, pt++;
			}
		}
		// next bit in mask
		r22 <<= 1;
		cnt--;

	} while(q < i);
}

static  void init_font(void)
{
	ipl_read((unsigned char*)MEM_FONT, 0x1FCF00, 0x3000);
	yay0_decode((void*)MEM_FONT, (void*)MEM_WORK);
	

	struct font_hdr
	{
		unsigned short font_type, first_char, last_char, subst_char, ascent_units, descent_units, widest_char_width,
			leading_space, cell_width, cell_height;
		unsigned long texture_size;
		unsigned short texture_format, texture_columns, texture_rows, texture_width, texture_height, offset_charwidth;
		unsigned long offset_tile, size_tile;
	} *fnt = (void*)MEM_WORK;

	untile((void*)MEM_FONT, (void*)(MEM_WORK + fnt->offset_tile), fnt->texture_width, fnt->texture_height);
	
	int i;
	for (i=0; i<256; ++i) {
		int c = i;
		if ((c < fnt->first_char) || (c > fnt->last_char)) {
			c = fnt->subst_char;
		}
//		else {
			c -= fnt->first_char;
//		}
		font_size[i] = ((unsigned char*)fnt)[fnt->offset_charwidth + c];

		int r = c / fnt->texture_columns;
		c %= fnt->texture_columns;
		
		font_offset[i] = (r * fnt->cell_height) * fnt->texture_width + (c * fnt->cell_width);
	}
	
	font_height = fnt->cell_height;
}
/*
static void blit_char(u8 x, u8 y, unsigned char c)
{
	unsigned char *fnt = ((unsigned char*)MEM_FONT) + font_offset[c];
	int ay, ax;
	for (ay=0; ay<font_height; ++ay) {
		for (ax=0; ax<font_size[c]; ax++) {
			int v0 = fnt[ax];
			bg[(ay+y)*256+((ax+x)&255)] = v0 << 6;
		}
		fnt += 512;
	}
}
*/

static void blit_char(u16 x, u16 y, unsigned char c)
{
	unsigned char *fnt = ((unsigned char*)MEM_FONT) + font_offset[c];
	int ay, ax;
	for (ay=0; ay<font_height; ++ay) {
		for (ax=0; ax<font_size[c]; ax++) {
			u8 v0 = fnt[ax];
			fb[(ay+y)*320+((ax+x))] = (v0 << 21) |  RGB2YCBR(0,0,0);
		}
		fnt += 512;
	}
}

static void cls()
{
	g_nX = 0;
	g_nY = 0;
	clearFB();
}

static void print(const char *string)
{
	if(g_nY >= 460) {
		cls();
	}
	
	while((*string)) {
		if((*string) == '\n') {
			g_nY += 24;
			g_nX = 0;
		}
		else {
			blit_char(g_nX, g_nY, *string);
			g_nX += font_size[*string]-2;
		}
		
		string++;
	}

	if(g_nY >= 460) {
		cls();
	}
}

static void printxy(u16 x, u16 y, const char* szString)
{
	g_nX = x;
	g_nY = y;
	print(szString);
}


static void memcpy32(u32* pDest, u32* pSrc, u32 dwSize)
{
	while(dwSize -=4 ) {
		*pDest++ = *pSrc++;
	}
}

static void memset32(u32* pDest, u32 dwVal, u32 dwSize)
{
	while(dwSize--) {
		*pDest++ = dwVal;
	}
}

static u8 memcmp32(u32* pDest, u32* pSrc, u32 dwSize)
{
	while(dwSize -=4 ) {
		if(*pDest++ != *pSrc++) {
			return 0;
		}
	}

	return 1;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//												Memcard / Dol loading
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


/*	channel	device	freq	offset		Description
	-------------------------------------------------------------
	0 		0 		4 	  				Memory Card (Slot A)
	0 		1 		3 		0x00000000 	Mask ROM
	0 		1 		3 		0x20000000 	Real-Time Clock (RTC)
	0 		1 		3 		0x20000100 	SRAM
	0 		1 				0x20010000 	UART
	1 		0 		4 	  				Memory Card (Slot B)
	2 		0 	  						AD16 (trace step)
	0 		2 	  	  					Serial Port 1
	0 		2 		5 	  				Ethernet Adapter (SP1)
*/



void fn_load_dol_fn_inmem(void *dol)
{
	void (*entrypoint)();

	struct dol_s {
		unsigned long sec_pos[18];
		unsigned long sec_address[18];
		unsigned long sec_size[18];
		unsigned long bss_address, bss_size, entry_point;
	} *d = (struct dol_s*)dol;

	int i;
	
	for (i=0; i<18; ++i) {
		if (!d->sec_size[i])
			continue;
		
		// copy section
		int nCount = d->sec_size[i];
		char *pDest = (char *) (void*)d->sec_address[i], *pSrc = (char *) ((unsigned char*)dol)+d->sec_pos[i];
		while (nCount--)
			*pDest++ = *pSrc++;
	}
	
	// clear BSS
	int nCount = d->bss_size;
	char *pDest = (char *) d->bss_address;
	while (nCount--)
		*pDest++ = 0;//

	entrypoint = (void(*)())d->entry_point;
//	SetMSR((GetMSR() | 2) & ~0x8000);
//	CLI();

	entrypoint();
}



static void ReadMemcardBlockX(u32 dwOffset, void *pData, u32 dwSize)
{
	u8 pCMD[4];
	u32 dwDummy = 0;

//	EXI_SR &= 0x405;
	EXI_SR |= ((1<<0)<<7) | (4 << 4);

 	// read command and block offset 31-8
	pCMD[0] = 0x52;
    pCMD[1] = (dwOffset >> 17) & 0x3F;
    pCMD[2] = (dwOffset >> 9) & 0xFF;
    pCMD[3] = (dwOffset >> 7) & 3;
	exi_write_word(*((u32*) pCMD));
	
	// block offset 7-0
	EXI_DATA = (dwOffset & 0x7F);
	EXI_CR = 0x05;
	EXI_WAIT_EOT;

	// dummy write
//	exi_write_word(*((u32*) pCMD));
	exi_write_word(0);

	// read data
	exi_read((unsigned char*)pData, dwSize);
	EXI_SR &= ~0x80;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//															Debugging stuff
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifndef RELEASE

static void DbgOut4(u8 bValue)
{
	char szText[2];
	szText[1] = 0;
	
	bValue += 0x30;

	if(bValue > 0x39) {
		bValue += 7;
	}

	szText[0] = bValue;
	print(szText);
}

static void DbgOut8(u8 bValue)
{
	u8 bLow = bValue & 0x0F;
	u8 bHigh = bValue >> 4;

	DbgOut4(bHigh);
	DbgOut4(bLow);
}

static void DbgOut32(u32 dwValue)
{
	g_nX = 0;
	print("0x");
	DbgOut8(dwValue >> 24);
	DbgOut8(dwValue >> 16);
	DbgOut8(dwValue >> 8);
	DbgOut8(dwValue & 0xff);
	g_nY += 22;
}

void DbgHexDump(void* ppData, int nBytes)
{
	int nByte = 0;
	int nCurByte = 0;

	u8* pData = ((u8*) ppData);

	print("\n");

	// loop trough lines
	while(nCurByte <= nBytes) {

		// show 16 bytes per line
		for(nByte = 0; nByte < 8; nByte++) {
			DbgOut8(pData[nCurByte++]);
			//print(" ");
		}
		
		print("\n");
	}
}

#else
	#define DbgOut32(x)
	#define DbgHexDump(x,y)
#endif



#define TB_CLOCK  40500000

typedef struct {
	unsigned long l, u;
} tb_t;

#define mftb(rval) ({unsigned long u; do { \
	 asm volatile ("mftbu %0" : "=r" (u)); \
	 asm volatile ("mftb %0" : "=r" ((rval)->l)); \
	 asm volatile ("mftbu %0" : "=r" ((rval)->u)); \
	 } while(u != ((rval)->u)); })

static unsigned long tb_diff_msec(tb_t *end, tb_t *start)
{
	unsigned long upper, lower;
	upper = end->u - start->u;
	if (start->l > end->l)
		upper--;
	lower = end->l - start->l;
	return ((upper*((unsigned long)0x80000000/(TB_CLOCK/2000))) + (lower/(TB_CLOCK/1000)));
}

static void GC_Sleep(u32 dwMiliseconds)
{
	tb_t start, end;
	mftb(&start);
	while (1)
	{
		mftb(&end);
		if (tb_diff_msec(&end, &start) >= dwMiliseconds)
			break;
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//															DVD routines
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define MEM32(_x)	*(u32*)_x				//---------------------------------------------------------------------------------------------------
#define DVD_DISR	MEM32( 0xcc006000 )		// DI Status Register
#define DVD_DISR_TCINT		(1 << 4)		// TCINT - Transfer Complete Interrupt Status
											// read : 0 Interrupt has not been requested / 1 Interrupt has been requested
											// write: write 0 no effect / 1 clear Interrupt
#define DVD_DISR_DEINT		(1 << 2)		// DEINT - Device Error Interrupt Status
											// read : 0 Interrupt has not been requested / 1 Interrupt has been requested
											// write: write 0 no effect / 1 clear Interrupt
#define DVD_DICR_DMA		(1 << 1)		// DMA - 0: immediate mode, 1: DMA mode (*1)
#define DVD_DICR_TSTART		(1 << 0)		// TSTART - transfer start. 
											// write 1: start transfer, read 1: transfer pendin
											//---------------------------------------------------------------------------------------------------
#define DVD_CLEARSTATUS()						DVD_DISR |= ( DVD_DISR_DEINT | DVD_DISR_TCINT );
#define DVD_WAIT_STATUS()						while( ! ( DVD_DISR & ( DVD_DISR_DEINT | DVD_DISR_TCINT)))



static void DVD_Reset()
{
	unsigned long val;
	*(unsigned long*)0xCC006000 = 0x2A;
	*(unsigned long*)0xCC006004 = 0;
	
	val = *(unsigned long*)0xCC003024;
	val &=~4;
	val |= 1;
	*(volatile unsigned long*)0xCC003024 = val;
	val |= 4;
	val |= 1;
	*(volatile unsigned long*)0xCC003024 = val;
}

static int DVD_WaitImmediate()
{
	u32 nCount = 0;

	while ((dvd[7] & 1)) {
		if(++nCount > 0xFFFFFF) {
			return 0;
		}
	}

	return 1;
}

static int DVD_CustomDbgCommand(u32 dwCommand, u32 dwOffset, u32 dwLength, u32* pBuffer)
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = dwCommand;
	dvd[3] = dwOffset;
	dvd[4] = dwLength;
	dvd[5] = (u32) pBuffer;
	dvd[6] = dwLength;
	dvd[7] = 1;

	DVD_WaitImmediate();

	if (dvd[0] & 0x04) {
		return 1;
	}

	return 0;
}


static int DVD_ReadId(void *pDst)
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x40;
	dvd[5] = (u32) pDst;
	dvd[6] = 0x40;
	dvd[7] = 3;

	int nCount = 0;

	while (nCount++ < 0x3ffffff) {
		if (dvd[0] & 0x4)
			return 1;
		if (!dvd[6])
			return 0;
	}
}

static int DVD_Read(void *pDst, u32 dwBytes, u32 dwOffset)
{
	dvd[0] = 0x2E;			// 0x54; 0x2E;	dvd[0] |= 0x10;
	dvd[1] = 0;
	
	dvd[2] = 0xA8000000;
	dvd[3] = dwOffset >> 2;
	dvd[4] = (u32) dwBytes;
	dvd[5] = (u32) pDst;
	dvd[6] = (u32) dwBytes;
	dvd[7] = 3;

	int nCount = 0;
	const int c_nErrorTime = 0xF00000;

	while (nCount++ < c_nErrorTime  + 0x1000) {
		u32 dwStat		= dvd[0];
		u32 dwDMABytes	= dvd[6];

		if (dwStat & 0x4) {
			return 1;
		}
		if (dwDMABytes == 0) {
			return 0;
		}
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//														video mode
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define VI_BASE		(u32*) 0xCC002000
#define VI_BASE2	(u32*) 0xCC002040

/*
static const u32 GC_VI_MODE_IRegs_PAL[32] = {
	0x00050101, 0x4B6A01B0, 0x02F85640, 0x001D0245,
	0x001E0244, 0x4D2B4D6D, 0x4D8A4D4C, 0xC0796A00,
	0x00000000, 0xC0796560, 0x00000000, 0x011500A8,
	0x913901B1, 0x90010001, 0x00010001, 0x00010001,
	0x00000000, 0x00000000, 0x254A10ED, 0x1AE771F0,
	0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF,
	0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000,
	0x02500000, 0x000000FF, 0x00FF00FF, 0x00FF00FF
};

static const u32 GC_VI_MODE_IRegs_NTSC[32] = {
 	0x00060001, 0x476901AD, 0x02EA5140, 0x001501E6,
	0x001401E7, 0x410C410C, 0x40ED40ED, 0xC0881860,
	0x00000000, 0xC0881D00, 0x00000000, 0x00020052,
	0x910701AE, 0x90010001, 0x00010001, 0x00010001,
	0x00000000, 0x00000000, 0x254A10ED, 0x1AE771F0,
	0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF,
	0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000001,
	0x02500000, 0x000000FF, 0x00FF00FF, 0x00FF00FF,
};
//*/

//*
static const u32 GC_VI_MODE_IRegs_PAL[16] = {
	0x11F50101, 0x4B6A01B0, 0x02F85640, 0x00010023,
	0x00000024, 0x4D2B4D6D, 0x4D8A4D4C, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x013C0144,
	0x113901B1, 0x10010001, 0x00010001, 0x00010001
};

static const u32 GC_VI_MODE_IRegs_NTSC[16] = {
	0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018,
	0x00020019, 0x410C410C, 0x40ED40ED, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x110701AE, 0x10010001, 0x00010001, 0x00010001
};


static const u32 GC_VI_MODE_IRegs_Common[16] = {
	0x00000000, 0x00000000, 0x28500100, 0x1AE771F0,
	0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF,
	0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000,
	0x02800000, 0x000000FF, 0x00FF00FF, 0x00FF00FF
};

//*/


static void InitVideo(u32* pData)
{
	int i;

	// Set the VI initialization state.
	for(i = 0; i < 16; i++)
		*(VI_BASE + i) = pData[i];

	for(i = 0; i < 16; i++)
		*(VI_BASE2 + i) = GC_VI_MODE_IRegs_Common[i];

	// fake init video
	R_VIDEO_FRAMEBUFFER_1 =  MEM_FB;
	R_VIDEO_FRAMEBUFFER_2 =  MEM_FB2;
}


#define MBLIST_Y (3 * 24)



static u16 ShowMultibootGames()
{
	u16 nGames = 0;
	u32* pTable = g_aMBTable;
	
	// get table
//	print("\nDVD_Read Table");
	DVD_Read(pTable, 0x50, 0x40);

	// limit to max 16 games
	pTable[(0x50/4)-4] = 0;

	g_nY = MBLIST_Y;

	while(*pTable != 0) {
		u32 dwOffset = 	*pTable++;
		nGames++;

		// print("Set offset");
		DVD_CustomDbgCommand(0x28000000, (dwOffset / 0x400) << 8, 0x20, 0);

		// dcache_flush((void*)szGameName, 0x800);
		// print("DVD_ReadId");
		DVD_ReadId((void*) 0xC0000000);
		DVD_Read((void*) 0xC0800000, 0x40, 0x20);
		*((u32*)0xC0800014) = 0x2e2e2e00;

		g_nX = 21;
		print((char*) 0xC0800000);
		print("\n");
	}

	return nGames;
}

#ifndef RELEASE
static void WaitForKey()
{
	asm("1:");
	asm("lis 3, 0xCC00");
	asm("lhz 4, 0x6404(3)");
	asm("lwz 5, 0x6408(3)");
	asm("andi. 5, 4, 0xf7f");
	asm("beq 1b"); 

	GC_Sleep(300);
}
#endif

void InitSystem( unsigned long VidMode );

//						0xA000 0xBC000 0x12000 0x32000 0x1BE000 0x15C000
#define MEMCARD_BLOCK	512
#define DOLPOS			(0xA000)	
#define DOLSIZE			(500 * 1024)
#define DOLSEARCH_RANGE	0x40000			// (256kb)


// #define SHOWDOLS

int main(void)
{
	memset32((void*)0x80000004, 0, (0x01700000-4)/4);
	*((u32*)0x80000C00) = 0x4c000064;

	ipl_set_config();
	init_font();
	InitSystem(1);
	cls();
	
	if(*((u32*)0x80000000) == 0x2badc0de) {

//		InitVideo((u32*) GC_VI_MODE_IRegs_NTSC);
//		*((u32*)0x80000C00) = 0x4c000064;

		const u32 dwLoadPos = 0x81200000;

		int nOffset = 0;
		u32 dwSize = DOLSIZE;
		int nDolOffset;
		u8* pDest = (u8*) dwLoadPos;

		// switch ebase to memcard 2
		ebase = (u32*) 0xCC006814; 
		#ifdef SHOWDOLS
			print("** scanning for DOLs **\n");
		#endif

/*		while(nOffset < DOLSEARCH_RANGE) {
			ReadMemcardBlockX(nOffset, (void*)pDest, MEMCARD_BLOCK);
			
			// scan for dolphin app file header
			for(nDolOffset = 0;nDolOffset < MEMCARD_BLOCK; nDolOffset += 4) {
				if(memcmp32((u32*) (pDest+nDolOffset), (u32*)"Dolphin Application", 20) == 1) {
	
					#ifdef SHOWDOLS
						// found !!!
						g_nX = 0;
						print(pDest+nDolOffset+0x20);
						print(" ");
						DbgOut32(nOffset+nDolOffset);
					#endif

					pDest -= (nDolOffset-0x100);

					while(dwSize > 0) {
						ReadMemcardBlockX(nOffset, (void*)pDest, MEMCARD_BLOCK);
						pDest += MEMCARD_BLOCK;
						nOffset += MEMCARD_BLOCK;
						dwSize -= MEMCARD_BLOCK;
					}

					#ifdef SHOWDOLS
						WaitForKey();
					#endif

					fn_load_dol_fn_inmem((void *)dwLoadPos);

					cls();
					DbgHexDump(dwLoadPos+0x100, 0x80);
					WaitForKey();
					asm("lis 4, 0x8140");
					asm("mtlr 4");
					asm("blrl");
				
				}
			}

			nOffset += MEMCARD_BLOCK;
		}
	*/

//*		
		nOffset = DOLPOS;
		while(nOffset < DOLPOS+DOLSIZE) {
			ReadMemcardBlockX(nOffset, (void*)pDest, MEMCARD_BLOCK);
			pDest += MEMCARD_BLOCK;
			nOffset += MEMCARD_BLOCK;
		}

		fn_load_dol_fn_inmem((void*)(dwLoadPos));
//*/
	}


	print("XenoGC Shell v1.05\n--------------\n");

//	GC_Sleep(1000);
	while(DVD_ReadId((void*) 0xC0000000)) {
		GC_Sleep(500);
	}

	u16 nNumGames, nGameIndex = 0;

//	*((u32*)0x80000C00) = 0x4c000064;
//	DbgOut32(*((u32*)0x80000C00));


//	GC_Sleep(1000);
//	print("\nSet offset 0");
	DVD_CustomDbgCommand(0x28000000, 0x00, 0x00000000, 0x00000000);

//	GC_Sleep(500);
//	print("\nDVD_ReadId\n");
	DVD_ReadId((void*) 0xC0000000);

//	GC_Sleep(500);
	nNumGames = ShowMultibootGames();

	if(nNumGames == 0) {
		nNumGames = 1;
		g_aMBTable[0] = 0;
	}

	while(1) {
		u32 dwKeys = ((*((volatile u32*)0xCC006404)) >> 16);
		*((volatile u32*)0xCC006408);

		//---------------------------------------------------------
		// move cursor
		//---------------------------------------------------------
		if(dwKeys & PAD_UP) {
			nGameIndex--;
		}
		if(dwKeys & PAD_DOWN) {
			nGameIndex++;
		}
		if(nGameIndex == 0xFFFF) {
			nGameIndex = nNumGames-1;
		}
		if(nGameIndex == nNumGames) {
			nGameIndex = 0;
		}

		GC_Sleep(100);

		//---------------------------------------------------------
		// Start multiboot
		//---------------------------------------------------------
		if(dwKeys & PAD_A) {
			u32 dwMBOffset = g_aMBTable[nGameIndex];
			
			// print("\nSet offset");
			DVD_CustomDbgCommand(0x28000000, (dwMBOffset / 0x400) << 8, 0x00000000, 0x00000000);
			GC_Sleep(500);

			// print("\nDVD_ReadId");
			DVD_ReadId((void*) 0xC0000000);
			load_apploader(0);
		}

	#ifndef RELEASE
		//---------------------------------------------------------
		// dbg return
		//---------------------------------------------------------
		else if(dwKeys & PAD_START) {
			asm("lis 4, 0x8130");
			asm("mtlr 4");
			asm("blrl");
		}
	#endif

		u16 nGame;

		//---------------------------------------------------------
		// show cursor
		//---------------------------------------------------------
		g_nY = MBLIST_Y;
		for( nGame = 0; nGame < nNumGames; nGame++) {
			if(nGame == nGameIndex) {
				print("@\n");
			}
			else {
				print("  \n");
			}

			g_nX = 0;
		}
	}
}


//#define DBGOUT 



#ifdef DBGOUT

#endif


static void report(const char* szMsg, ...) 
{ 
	#ifdef DBGOUT 
//		print("\nreport");
		print(szMsg);
	#endif
}


#define GC_INIT_BASE               	 (0x80000020)
#define GC_INIT_BASE_PTR           	 (u32*)GC_INIT_BASE

void InitSystem( unsigned long VidMode )
{
	*(unsigned short*)0x800030e0 = 6;				// production pads

	static u32 GC_DefaultConfig[64] =
	{
		0x0D15EA5E,0x00000001,0x01800000,0x00000003, //  0.. 3 80000020
		0x00000000,0x816FFFF0,0x817FE8C0,0x00000024, //  4.. 7 80000030
		0x00000000,0x00000000,0x00000000,0x00000000, //  8..11 80000040
		0x00000000,0x00000000,0x00000000,0x00000000, // 12..15 80000050
		0x38A00040,0x7C6802A6,0x9065000C,0x80650008, // 16..19 80000060
		0x64638000,0x7C6803A6,0x38600030,0x7C600124, // 20..23 80000070
		0x4E800020,0x00000000,0x00000000,0x00000000, // 24..27 80000080
		0x00000000,0x00000000,0x00000000,0x00000000, // 28..31 80000090
		0x00000000,0x00000000,0x00000000,0x00000000, // 32..35 800000A0
		0x00000000,0x00000000,0x00000000,0x00000000, // 36..39 800000B0
		0x015D47F8,0xF8248360,0x00000000,0x00000001, // 40..43 800000C0
		0x00000000,0x00000000,0x00000000,0x00000000, // 44..47 800000D0
		0x814B7F50,0x815D47F8,0x00000000,0x81800000, // 48..51 800000E0
		0x01800000,0x817FC8C0,0x09A7EC80,0x1CF7C580  // 52..55 800000F0
	};

	u32 Counter=0;
	u32 *pSrcAddr=GC_DefaultConfig;
	u32 *pDstAddr=GC_INIT_BASE_PTR;

	GC_DefaultConfig[43] = VidMode;

	for(Counter=0; Counter<56; Counter++) {
		pDstAddr[Counter] = pSrcAddr[Counter];
	}

	if( VidMode == 1 )	{
		print("PAL\n");
		InitVideo((u32*)GC_VI_MODE_IRegs_PAL);
	}
	else {
		print("NTSC\n");
		InitVideo((u32*) GC_VI_MODE_IRegs_NTSC);
	}
}

static void load_apploader(void* pApploader)
{
	const int bDBGOUT = 1;

	void (*app_init)(void (*report)(const char *fmt, ...));
	int  (*app_main)(void **dst, int *size, int *offset);
	void *(*app_final)();
	void (*app_entry)(void(**init)(void (*report)(const char *fmt, ...)), int (**main)(), void *(**final)());

    u8 *buffer = (u8*)0xC0100000;

	cls();
	DVD_Read(buffer,0x460,0);
	print("\nLoading...");
//	GC_Sleep(1000);

	InitSystem((buffer[0x45b] == 2) ? 1 : 0);

	DVD_Read(buffer, 0x20, 0x2440);
	DVD_Read((void*)0x81200000, ((*(unsigned long*)(buffer+0x14)) + 31) &~31,0x2460);

	app_entry = (void (*)(void(**)(void (*)(const char*,...)),int (**)(),void *(**)()))(*(unsigned long*)(buffer + 0x10));
    
//	GC_Sleep(500);
	#ifdef DBGOUT 
		print("\n*** calling app_entry() ***");
	#endif
	app_entry(&app_init,( int (**)()) &app_main,&app_final);

	#ifdef DBGOUT 
		DbgOut32((u32) app_init);
		DbgOut32((u32) app_main);
		DbgOut32((u32) app_final);
	#endif

//	GC_Sleep(500);
	#ifdef DBGOUT 
		print("\n*** calling app_init() ***");
	#endif
	app_init((void (*)(const char*,...))report);
	
	#ifdef DBGOUT 
		print("\n*** calling app_main() ***");
	#endif	

	for(;;) {
		void *dst = 0;
		int len = 0,offset = 0;
		int res = app_main(&dst, &len, &offset);

//		print(".");

		#ifdef DBGOUT 
//			WaitForKey();
			DbgOut32((u32) dst);
			DbgOut32((u32) len);
			DbgOut32((u32) offset);
		#endif

		if (!res) break;
		DVD_Read(dst, len, offset);
	}

	#ifdef DBGOUT 
		print("\n*** calling app_final() ***");
	#endif

	void (*entrypoint)() = (void (*)()) app_final();

	#ifdef DBGOUT 
		print("\n*** calling dol entrypoint ***");
	#endif

    entrypoint();
}




//	asm("b SUB_BootFromMC");
	/*	printplane(8, nY, "DVD_Read");
	nY += 24;
	DVD_Read(szGameName, 0x40, 0x20);
	GC_Sleep(500);
	printplane(8, nY, szGameName);
*/



/*	asm("lis 4, 0x8000");
	asm("ori 4, 4, 0x100");
	asm("mtlr 4");
	asm("blrl");
*/

//	print("\nSetDbgMode");
//	DVD_SetDebugMode();
//	print("\nWritemem");
//	DVD_WriteDriveMemDword(0x40d100, 0);

/*	print("\nInit multiboot");
	DVD_CustomDbgCommand(0x27000000, 0x00000000, 0x00000000, 0x00000000);
	GC_Sleep(1000);


*/	

