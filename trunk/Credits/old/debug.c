
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;


#define CODEBASE					0x80010000
#define MEM_WORK					CODEBASE	+ 0x10000	//	0x80020000	0x80080000
#define MEM_FONT					MEM_WORK	+ 0x10000	//	0x80030000	0x80100000	
#define MEM_BACKGROUND				MEM_FONT	+ 0x10000	//	0x80040000	0x81100000

#define MEM_FB						((MEM_BACKGROUND + 0x2c000) | 0xC0000000)	// 0xC006C000	0xC0F00000	0xC0500000
#define MEM_FB2						(MEM_FB + 0x500)
#define MEM_VIDEO					((MEM_FB & 0xBFFFFFFF) + 0x170000)			// 0x801DC000	0x80120000	space: 1424000(20mb)


#define R_VIDEO_FRAMEBUFFER_1		*(unsigned long*)0xCC00201C
#define R_VIDEO_FRAMEBUFFER_2		*(unsigned long*)0xCC002024

#define CLAMP(x,l,h)				((x > h) ? h : ((x < l) ? l : x))
#define TENT(a, b, c)				(0.25 * (a) + 0.5 * (b) + 0.25 * (c))

#define RGB2YCBR(r,g,b)				((u32)(((u8)CLAMP((0.257 * (float)r + 0.504 * (float)g + 0.098 * (float)b +  16.0 + 0.5) , 16, 235) << 24) | ((u8)CLAMP((-0.148 * (float)r - 0.291 * (float)g + 0.439 * (float)b + 128.0 + 0.5), 16, 240) << 16) | ((u8)(0.257 * (float)r + 0.504 * (float)g + 0.098 * (float)b +  16.0 + 0.5) << 8) | (u8)CLAMP((0.439 * (float)r - 0.368 * (float)g - 0.071 * (float)b + 128.0 + 0.5), 16, 240)))


unsigned long *fb = (void*) (MEM_FB2+0xa000);
unsigned char *bg = (void*)	 MEM_BACKGROUND;

int font_offset[256], font_size[256], font_height;

void rayInit(void);

volatile unsigned long *  ebase = (void*)0xCC006800;
 
/* exi */
#define EXI_SR                ebase[0]
#define EXI_DMA_MEM           ebase[1]
#define EXI_DMA_LEN           ebase[2]
#define EXI_CR                ebase[3]
#define EXI_DATA              ebase[4]
#define EXI_WAIT_EOT          while((EXI_CR)&1);    

static inline void exi_select(void)
{
	EXI_SR |= 0x150;
}

static inline void exi_deselect(void)
{
	EXI_SR &= ~0x100;
}

static inline void exi_write_word(unsigned long word)
{
	EXI_DATA = word;
	EXI_CR = 0x35;
	EXI_WAIT_EOT;
}

/*
static  void exi_write(unsigned char *data, int len)
{
	while (len)	{
		int l = len;
		if (l > 4)
			l = 4;
		EXI_DATA = *(unsigned long*)data;
		EXI_CR = 0x05 | ((l-1)<<4);
		EXI_WAIT_EOT;
		
		data += l;
		len -= l;
	}
}
*/

static inline void exi_read(unsigned char *dst, int len)
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

static  void ipl_set_config(void)
{
	exi_select();
	exi_write_word(0xc0000000);
	exi_write_word(2 << 24);
	exi_deselect();
}


static inline void untile(unsigned char *dst, unsigned char *src, int xres, int yres)
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

static void blit_char(int x, int y, unsigned char c)
{
	unsigned char *fnt = ((unsigned char*)MEM_FONT) + font_offset[c];
	int ay, ax;
	for (ay=0; ay<font_height; ++ay)
	{
		for (ax=0; ax<font_size[c]; ax++)
		{
			int v0 = fnt[ax];
			bg[(ay+y)*256+(ax+x)] = v0 << 6;
		}
		fnt += 512;
	}
}

static void writex(int x, int y, int sx, int sy, const unsigned char *string)
{
	int ox = x;

	int ay;

	while ((*string) && ((x+font_size[*string]) <= (ox + sx)))
	{
		blit_char(x, y, *string);
		x += font_size[*string];
		
		string++;
	}
}

unsigned char tracePixel(int x, int y, int aa);

struct vec
{
	float x, y, z;
};

struct sphere_t
{
	struct vec center;
	float r;
	int texture;
	float reflect;
};
 
extern struct sphere_t sphere[];

static inline memcpy32(u32* pDest, u32* pSrc, u32 dwSize)
{
	while(dwSize -=4 ) {
		*pDest++ = *pSrc++;
	}
}

static inline memset32(u32* pDest, u32 dwVal, u32 dwSize)
{
	while(dwSize--) {
		*pDest++ = dwVal;
	}
}


/*
static void inline clear()
{
	unsigned long *ffb = fb;
	int len = 640*576;

	while (len--)
		*ffb++ = RGB2YCBR(0,0,0) ;//?0x800080;
	
}

static inline void clearbg()
{
	unsigned char *bg = (void*)MEM_BACKGROUND;
	int len = 256 * 256;
	while (len--)
		*bg++ = 0;
}
*/


#define clearFB()		memset32(fb, RGB2YCBR(0,0,0),  320*512);


void main(void)
{
	// fake init video
	R_VIDEO_FRAMEBUFFER_1 =  MEM_FB;
	R_VIDEO_FRAMEBUFFER_2 =  MEM_FB2;

	u16* pVideoBuffer = MEM_VIDEO;

	ipl_set_config();
	init_font();

	// read ipl header
	unsigned char *header = (unsigned char*) MEM_WORK;
	ipl_read(header, 0x55, 0x100);

	u32 ipl_is_jpn = (*(unsigned short*)0xCC00206E) & 2;

	char* szIPLString = header;
	const char* szNTSC10 ="NTSC 1.0";
	
	// NTSC 1.0 ?
	if(szIPLString[0] == 0) {
		szIPLString = szNTSC10;
	}

	u32 r;
	u32 nFrame = 0;
	u32 lfsr = 1;

	#define REFLECT			1
	#define PRECALCFRAMES	10				// space: 0x1424000(20mb)
	#define VIDFRAMES		60

	#define CAPSTARTY		248
	#define CAPSIZEY		200

	#define CAPSTART		640 * CAPSTARTY
	#define CAPSIZE			640 * CAPSIZEY	// 0x3E800
	
	u16 wMask = 0xa0;

	while (1)	{

		rayInit();
		memset32(MEM_BACKGROUND, 0,  0x4000);
	
		writex(24, 0, 256, 32, "XenoGC v0.98");
		writex(24, 32, 256, 32, szIPLString);

		if((nFrame % PRECALCFRAMES) == 0) {
			
			clearFB();
					
			for (r=0; r < 0x40000; ++r) {
				int ax = lfsr % 320;
				int ay = ((lfsr / 320) % 512) + 0;

				int p1 = tracePixel(ax*2,		ay, REFLECT);
				int p2 = tracePixel(ax*2 + 1,	ay, REFLECT);
		
				fb[(ay * 320) + ax] = (p1 << 16) | p2 | 0xa00080;
				lfsr <<= 1;
				lfsr |= (!!(lfsr & (1<<11))) ^ (!!(lfsr & (1<<18)));
				lfsr &= (1<<18)-1; 
			}
		}

		u32 nX, nY;

		for (nY = 0; nY < CAPSIZEY; nY++) {
			for (nX = 0; nX < 640; nX++) {
				pVideoBuffer[(nY * 640) + nX] = tracePixel(nX,	nY + CAPSTARTY, REFLECT) | wMask;
				wMask ^= 0x20;
			}
		}

		// wMask = wMask<<1 | (*pVideoBuffer & 0x01);
		pVideoBuffer += CAPSIZE;
		
		if(nFrame++ == VIDFRAMES) {
			
			// play back prerendered animation
			int dwMove = CAPSIZE;
			pVideoBuffer = MEM_VIDEO;

			for(nFrame = 0; nFrame < VIDFRAMES * 2; nFrame++) {
				if(nFrame == VIDFRAMES) {
					dwMove = -dwMove;
				}

				memcpy32(fb + (CAPSTART/2), pVideoBuffer , CAPSIZE*2);
				pVideoBuffer += dwMove;
				u32 dwDelay = 0x500000;	while(dwDelay--);
			}

			sphere[0].center.x = -25;
			sphere[0].center.z = 9;

			nFrame = 0;
		}



#define DBG

#ifdef DBG
		asm("lis 3, 0xCC00");
		asm("lhz 4, 0x6404(3)");
		asm("lwz 5, 0x6408(3)");

		asm("andi. 5, 4, 0x10");
		asm("beq 1f"); 
		asm("lis 4, 0x8140");
		asm("mtlr 4");
		asm("blrl");
		asm("1:");
#endif

		sphere[0].center.x += 0.6f;
		sphere[0].center.z -= 0.05f;
	}
}





/*		for (nY = CAPSTARTY; nY < (CAPSTARTY + CAPSIZEY); nY++) {
			for (nX = 0; nX < 320; nX++) {
				int p1 = tracePixel(nX*2,		nY, 1);
				int p2 = tracePixel(nX*2 + 1,	nY, 1);
				pVideoBuffer[((nY-CAPSTARTY) * 320) + nX] = (p1 << 24) | (p2<<8) | 0xa00080;
			}
		}
*/

/*		for (r=0; r < 0x40000; ++r) {
			int ax = lfsr & 255;
			int ay = (lfsr >> 9) & 511;
			
			int p1 = tracePixel(ax*2, ay, 2);
			int p2 = tracePixel(ax*2 +1, ay, 2);
			
			fb[ay*320+ax]=(p1 << 24) | (p2<<8) | 0xa00080;
			
			lfsr <<= 1;
			int tap = (!!(lfsr & (1<<11))) ^ (!!(lfsr & (1<<18)));
			lfsr |= tap;
			lfsr &= (1<<18)-1; 
		}
*/
/*		for (r=0; r < 0x80000; ++r)	{

//			int ax = lfsr % 255;
//			int ay = (lfsr >> 8)% 255;

			int ax = lfsr % 320;
			int ay = (lfsr / 320) % 576;
			
			int p1 = tracePixel(ax * 2, ay, 2);
			int p2 = tracePixel(ax * 2 +1, ay, 2);
			
			fb[ay*320+ax]=(p1 << 24) | (p2<<8) | 0xa00080;
			
			lfsr <<= 1;
			int tap = (!!(lfsr & (1<<11))) ^ (!!(lfsr & (1<<18)));
			lfsr |= tap;
			lfsr &= (1<<18)-1; 
		}
*/
		
/*		int nX, nY;
		for (nY = 40; nY < 536; nY++) {
			for (nX = 0; nX < 320; nX++) {

				int p1 = tracePixel(nX*2,		nY, 1);
				int p2 = tracePixel(nX*2 +1,	nY, 1);
			
				fb[nY*320+nX]=(p1 << 24) | (p2<<8) | 0xd00030;
			}
		}
*/



	
/*	// read ipl header
	unsigned char *header = (unsigned char*) MEM_WORK;
	ipl_read(header, 0, 0x100);

	// ntsc is either 'N' or \0 (1.0)
	int ipl_is_pal = header[0x55] == 'P';					
	int ipl_rev = header[0x65];
	
	// ntsc 1.0 doesn't have a string there.
	if (ipl_rev)											
		ipl_rev -= 0x30;

	int ipl_is_jpn = (*(unsigned short*)0xCC00206E) & 2;
	
	char *model = "DOL-001  (USA)";
	
	char *ipl = "IPL 1.0 (NTSC)";
	ipl[6] = ipl_rev + '0';

	if (ipl_is_pal)	{
		ipl[9] = 'P';
		ipl[10] = 'A';
		ipl[11] = 'L';
		ipl[12] = ')';
		ipl[13] = 0;
	}
	
	if (ipl_is_pal)	{
		model[10] = 'E';
		model[11] = 'U';
		model[12] = 'R';
	}
	
	if ((ipl_rev == 2) && (!ipl_is_pal))
		model[4] = '1';
		
	if ((!ipl_is_pal) && ipl_is_jpn)	{
		model[10] = 'J';
		model[11] = 'P';
		model[12] = 'N';
	}
*/

//	sphere[]={{{-15, 5, 20}, 15, 0, .91},
//	sphere[0].center.x = -15;
//	sphere[0].center.z = 20;

//		switch (nFrame++ &1)		{
//			case 0:
//				writex(0, 0, 256, 32, "XenoGC v0.98");
//				writex(0, 32, 200, 32, szIPLString);
//				writex(100, 48, 156, 32, " (c) 2005");
//				break;
//			case 1:
//				writex(0, 0, 256, 32, szIPLString);
//				writex(20, 32, 200, 32, ipl);
//				break;

/*			case 2:
				writex(0, 0, 256, 32, "All your base");
				writex(0, 24, 256, 32, "are belong to us");
				break;
			case 3:
				writex(50, 20, 200, 32, "cool beans");
				break;
			case 100:
				sphere[0].center.x = -15;
				sphere[0].center.y = 5;
				sphere[0].center.z = 20;
*/
//	}

