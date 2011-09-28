
#include "Ray.h"



int font_offset[256], font_size[256], font_height;

volatile u32* ebase = (u32*) 0xCC006800;

u8  *bg = (u8*)		(MEM_BACKGROUND);
u32 *fb = (u32*)	(MEM_FB2 + YBORDEROFFSET);



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

/*
static void writex(int y, const char *string)
{
	u8 x = 8;
	u8 ay;

	while((*string)) {
		if((*string) == '\n') {
			y += 32;
			x = 8;
		}
		else {
			blit_char(x, y, *string);
			x += font_size[*string];
		}
		
		string++;
	}
}
*/

#define printplane(str) _printplane(str)

static void _printplane(const char *string)
{
	u8 ay;
	u8 x = 8;
	u8 y = 8;

	while((*string)) {
		if((*string) == '\n') {
			y += 32;
			x = 8;
		}
		else {
			blit_char(x, y, *string);
			x += font_size[*string];
		}
		
		string++;
	}

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

#define clearFB()	memset32((u32*) MEM_FB, RGB2YCBR(96,0,0), (640*576)/2);
#define clearBG()	memset32((u32*) (MEM_BACKGROUND), 0,  (256*256)/4);

extern const char szDriveVer[];

#ifndef DBGSAVEMEM

	const char szNTSC10[]	= " NTSC Revision 1.0";
	const char szXeno[]		= "  XenoGC v1.03a";
	const char szCredits1[]	= " All your base\n are belong to us";


int main(void)
{
	u32 nX, nY, r, nPlayFrame;
	u32 nFrame = 0, nCredits = 0, dwLFSR = 1;

	int nMove = CAPSIZE;

	float fSpherePos = SPHERE_XLEFT;
	
	// fake init video
	R_VIDEO_FRAMEBUFFER_1 =  MEM_FB;
	R_VIDEO_FRAMEBUFFER_2 =  MEM_FB2;

	u16* pVideoBuffer = (u16*) (MEM_VIDEO - CAPSIZE*2);

	clearFB();

	ipl_set_config();
	init_font();

	// read ipl header
	char* szIPLRevString = (char*) MEM_WORK;
	ipl_read(szIPLRevString, 0x54, 0x100);
	
	// NTSC 1.0 ?
	if(szIPLRevString [1] == 0) {
		szIPLRevString  = (char*) szNTSC10;
	}

	// Japanese IPL ? put 'J' or space ;)
	*((u8*) szIPLRevString) = 0x20 + 0x15 * ((*(unsigned short*)0xCC00206E) & 2);
		
	// RayInit()
	for (r=0; r<planes; ++r) {
		vec a;
		a.x = 0;
		a.z = 1;

		if(r == 0) {
			a.x = 0.5f;
		}

		cross(&planedyn[r].m1, &a, &plane[r].a);
		cross(&planedyn[r].m2, &planedyn[r].m1, &plane[r].a);
	}

	while (1)	{

		//---------------------------------------------
		// render normal frames
		//---------------------------------------------
		if((nFrame % PRECALCFRAMES) == 0) {
			
			char* szCredits;
			clearBG();	

			switch(nCredits) {
				case 0:
					//---------------------------------------------
					// playback prerendered animation
					//---------------------------------------------
					if(nFrame > VIDFRAMES) {

						nMove = -nMove;

						if(nMove < 0) {
							fSpherePos = SPHERE_XRIGHT;
						}
						else {
							fSpherePos = SPHERE_XLEFT;
							u16* pVideoBuffer = (u16*) (MEM_VIDEO - CAPSIZE*2);
						}
						
						for(nPlayFrame = 0; nPlayFrame < VIDFRAMES; nPlayFrame++) {
							pVideoBuffer += nMove;	
							memcpy32(fb + (CAPSTART/2), (u32*) pVideoBuffer , CAPSIZE*2);
							u32 dwDelay = 0x1000000;
							while(dwDelay--);
						}
					}

					szCredits = (char*) szXeno;
					break;
				case 1:
					szCredits = (char*) szIPLRevString;
					break;
				case 2:
					szCredits = (char*) szDriveVer;
					break;
				case 3:
					szCredits = (char*) szCredits1;
					break;
			}

			nCredits++;
			nCredits &= 3;

			printplane(szCredits);
			
			// reset to visible pos
			sphere[0].center.x = fSpherePos;
			
			for (r=0; r < 0x40000; ++r) {
				int ax = dwLFSR % 320;
				int ay = ((dwLFSR / 320) % 512);
				fb[(ay * 320) + ax] = (tracePixel(ax*2,	ay, ANTIALIAS) << 16) | tracePixel(ax*2 + 1, ay, ANTIALIAS);

				dwLFSR <<= 1;
				dwLFSR |= (!!(dwLFSR & (1<<11))) ^ (!!(dwLFSR & (1<<18)));
				dwLFSR &= (1<<18)-1; 
			}
		}

		if(nFrame <= VIDFRAMES) {
			//---------------------------------------------
			// precalculate one animation frame
			//---------------------------------------------
			clearBG();	
			printplane(szCredits1);

			sphere[0].center.x = SPHERE_XRIGHT - ((float) nFrame * (float) SPHERE_XSPEED);

			pVideoBuffer += CAPSIZE;

			for (nY = 0; nY < CAPSIZEY; nY++) {
				for (nX = 0; nX < 640; nX++) {
					pVideoBuffer[(nY * 640) + nX] = tracePixel(nX,	nY + CAPSTARTY, ANTIALIAS);
				}
			}
		}

		nFrame++;

		#ifdef DBGKEY
			asm("lis 3, 0xCC00");
			asm("lhz 4, 0x6404(3)");
			asm("lwz 5, 0x6408(3)");

			asm("andi. 5, 4, 0xf7f");
			asm("beq 1f"); 
			asm("lis 4, 0x8140");
			asm("mtlr 4");
			asm("blrl");
			asm("1:");
		#endif
	}
}


#else
	int main(void)
	{
		return 0;
	}
#endif


