/**
  * XenoBoot, from Xenoshell custom
  *
  * Originally by cheqmate/adhs
  *
  * Continued by www.gc-forever.com members
  * emu_kidid, ...
  * 
  * Xenoboot by vingt-2
  * https://github.com/vingt-2/xenogcfork
  */

#include "main.h"

/*** global variable definitions ***/

#define HEIGHT_NTSC 480
#define HEIGHT_PAL 576

u32 *fb = (u32*) (MEM_FB2 + YBORDEROFFSET);
u32 vidHeight = HEIGHT_NTSC;
u16 g_nX = 32, g_nY = 0;

/*** set exi base address ***/
volatile u32* ebase = (u32*) 0xCC006800;
int font_offset[256], font_size[256], font_height;

/*** extern function definitions ***/

extern long GetMSR();
extern void SetMSR(long);
extern void dcache_flush_icache_inv(void *, int);

static void print(const char *string);
static void GC_Sleep(u32 dwMiliseconds);

/*** exi / ipl font stuff ***/

static void exi_select(void)
{
	EXI_SR |= 0x150;
}

static void exi_deselect(void)
{
	EXI_SR &= ~0x100;
}


static void memset32(u32* pDest, u32 dwVal, u32 dwSize)
{
	while(dwSize--) {
		*pDest++ = dwVal;
	}
}

static u8 memcmp8(u8* pDest, u8* pSrc, u32 dwSize)
{
	while(dwSize--)
	{
		if(*pDest++ != *pSrc++)
			return 0;
	}
	return 1;
}

static void memcpy8(u8* pDest, u8* pSrc, u32 dwSize)
{
	while(dwSize--) 
	{
		*pDest++ = *pSrc++;
	}
}

static void exi_write_word(u32 word)
{
	EXI_DATA = word;
	EXI_CR = 0x35;
	EXI_WAIT_EOT;
}

static void exi_read(u8 *dst, int len)
{
	while (len)
	{
		int l = len;
		if (l > 4)
			l = 4;
		EXI_DATA = 0;
		EXI_CR = 0x31;
		EXI_WAIT_EOT;
		*(u32*)dst = EXI_DATA;
		dst += 4;
		len -= l;
	}
}

static void ipl_read(u8 *dst, int address, int len)
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

static void untile(u8 *dst, u8 *src, int xres, int yres)
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
				u8 *d = dst + (y + iy) * xres + x;
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

/*** Yay0 decompression ***/

static void yay0_decode(void *s, void *d)
{
	int i, j, k, p, q, cnt;

	i = *(u32 *)(s + 4);	  // size of decoded data
	j = *(u32 *)(s + 8);	  // link table
	k = *(u32 *)(s + 12);	 // byte chunks and count modifiers

	q = 0;					// current offset in dest buffer
	cnt = 0;				// mask bit counter
	p = 16;					// current offset in mask table

	u32 r22 = 0, r5;
	
	do
	{
		// if all bits are done, get next mask
		if(cnt == 0)
		{
			// read word from mask data block
			r22 = *(u32 *)(s + p);
			p += 4;
			cnt = 32;   // bit counter
		}
		// if next bit is set, chunk is non-linked
		if(r22 & 0x80000000)
		{
			// get next byte
			*(u8 *)(d + q) = *(u8 *)(s + k);
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
				r5 = *(u8 *)(s + k);
				k++;
				r30 = r5 + 18;
			}
			else r30 += 2;
			// do block copy
			u8 *pt = ((u8*)d) + r25;
			int i;
			for(i=0; i<r30; i++)
			{
				*(u8 *)(d + q) = *(u8 *)(pt - 1);
				q++, pt++;
			}
		}
		// next bit in mask
		r22 <<= 1;
		cnt--;

	} while(q < i);
}

static void init_font(void)
{
	ipl_read((u8*)MEM_FONT, 0x1FCF00, 0x3000);
	yay0_decode((void*)MEM_FONT, (void*)MEM_WORK);
	
	struct font_hdr
	{
		unsigned short font_type, first_char, last_char, subst_char, ascent_units, descent_units, widest_char_width,
			leading_space, cell_width, cell_height;
		u32 texture_size;
		unsigned short texture_format, texture_columns, texture_rows, texture_width, texture_height, offset_charwidth;
		u32 offset_tile, size_tile;
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
		font_size[i] = ((u8*)fnt)[fnt->offset_charwidth + c];

		int r = c / fnt->texture_columns;
		c %= fnt->texture_columns;
		
		font_offset[i] = (r * fnt->cell_height) * fnt->texture_width + (c * fnt->cell_width);
	}
	
	font_height = fnt->cell_height;
}

/*** framebuffer stuff ***/

static void blit_char(u16 x, u16 y, u8 c)
{
	u8 *fnt = ((u8*)MEM_FONT) + font_offset[c];
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
	memset32((u32*) MEM_FB2, RGB2YCBR(0,0,0), (640*vidHeight)/2);
}

static void print(const char *string)
{
	if(g_nY >= 460) 
	{
		cls();
	}
	
	while((*string)) 
	{
		if((*string) == '\n')
		{
			g_nY += 24;
			g_nX = 0;
		}
		else
		{
			blit_char(g_nX, g_nY, *string);
			g_nX += font_size[(int) *string]-2;
		}
		
		string++;
	}

	if(g_nY >= 460) 
	{
		cls();
	}
}

static u32 tb_diff_msec(tb_t *end, tb_t *start)
{
	u32 upper, lower;
	upper = end->u - start->u;
	if (start->l > end->l)
		upper--;
	lower = end->l - start->l;
	return ((upper*((u32)0x80000000/(TB_CLOCK/2000))) + (lower/(TB_CLOCK/1000)));
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


/*** video modes ***/
static const u32 GC_VI_MODE_IRegs_PAL[16] = 
{
	0x11F50101, 0x4B6A01B0, 0x02F85640, 0x00010023,
	0x00000024, 0x4D2B4D6D, 0x4D8A4D4C, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x013C0144,
	0x113901B1, 0x10010001, 0x00010001, 0x00010001
};

static const u32 GC_VI_MODE_IRegs_NTSC[16] = 
{
	0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018,
	0x00020019, 0x410C410C, 0x40ED40ED, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x110701AE, 0x10010001, 0x00010001, 0x00010001
};


static const u32 GC_VI_MODE_IRegs_Common[16] = 
{
	0x00000000, 0x00000000, 0x28500100, 0x1AE771F0,
	0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF,
	0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000,
	0x02800000, 0x000000FF, 0x00FF00FF, 0x00FF00FF
};

/*** helper functions ***/

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

void InitSystem( u32 VidMode )
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

	for(Counter=0; Counter<56; Counter++) 
	{
		pDstAddr[Counter] = pSrcAddr[Counter];
	}

	if( VidMode == 1 )	
	{
		InitVideo((u32*)GC_VI_MODE_IRegs_PAL);
		vidHeight = HEIGHT_PAL;
	}
	else 
	{
		InitVideo((u32*) GC_VI_MODE_IRegs_NTSC);
		vidHeight = HEIGHT_NTSC;
	}
}

/*** dol loading from memcard ***/

void LoadDolAtAddress(void *dol)
{
	struct dol_s 
	{
		u32 sec_pos[18];
		u32 sec_address[18];
		u32 sec_size[18];
		u32 bss_address, bss_size, entry_point;
	} *d = (struct dol_s*)dol;
	
	int i;
	for (i=0; i<18; ++i) 
	{
		if (!d->sec_size[i])
			continue;
		
		// copy section
		int nCount = d->sec_size[i];
		char *pDest = (char *) (void*)d->sec_address[i], *pSrc = (char *) ((u8*)dol)+d->sec_pos[i];
		while (nCount--)
			*pDest++ = *pSrc++;
		dcache_flush_icache_inv((void*)d->sec_address[i], d->sec_size[i]);
	}
	
	// clear BSS
	int nCount = d->bss_size;
	char *pDest = (char *) d->bss_address;
	while (nCount--)
		*pDest++ = 0;
		
	SetMSR((GetMSR() | 2) & ~0x8000);
	SetMSR(GetMSR() & ~0x8000); // EE
	SetMSR(GetMSR() | 0x2002);  // FP, RI
	
	void (*entrypoint)() = (void(*)())d->entry_point;
	entrypoint();
}

static void EXIReadMemoryCard(u32 dwOffset, void *pData, u32 dwSize)
{
	u32 cmd;
	u8* pCMD = (u8*)&cmd; 

	// Setup to status register for MemCard read (see documentation)
	EXI_SR = 0x4A5;

 	// read command and block offset 31-8
	pCMD[0] = 0x52;
	pCMD[1] = (dwOffset >> 17) & 0x3F;
	pCMD[2] = (dwOffset >> 9) & 0xFF;
	pCMD[3] = (dwOffset >> 7) & 3;

	// Send the first 4 bytes of the command
	exi_write_word(cmd);
	
	// Then send the last byte
	EXI_DATA = (dwOffset & 0x7F);
	EXI_CR = 0x05;
	EXI_WAIT_EOT;

	u8 dummy[4] = { 0, 0, 0, 0 };
	// dummy write
	exi_write_word(0x00000000);

	// read data
	exi_read((u8*)pData, dwSize);
	EXI_SR &= ~0x80;
}

static void ReadMemoryCardBlock(int blockIndex, void* pDest)
{
	for(int i = 0; i < 16; i++)
	{
		EXIReadMemoryCard(0x2000 * blockIndex + 0x200 * i, ((u8*)pDest + 0x200 * i), 0x200);
	}
}

// The first 5 blocks for the memcard header aren't referenced in the block map
// So we the entries actually live in index i - 5
u16 GetNextBlock(u16 blockIndex, Fat* fat)
{
	return fat->blockAllocTable[blockIndex - 5];
}

static void checksum(u16 *buff,u32 len,u16 *cs1,u16 *cs2)
{
	u32 i;
    *cs1 = 0;
	*cs2 = 0; 
	len /= 2;
    for (i = 0; i < len; ++i)
	{ 
        *cs1 += buff[i]; 
        *cs2 += (buff[i] ^ 0xffff); 
    } 
    if (*cs1 == 0xffff) *cs1 = 0; 
    if (*cs2 == 0xffff) *cs2 = 0; 
}

Directory* SelectDirectory(MemCard* memcard)
{
	u16 cs1[2];
	checksum(&memcard->directory1, sizeof(Directory) - sizeof(u32), &cs1[0], &cs1[1]);
	u8 dir1Good = *((u32*)&memcard->directory1.checksum1) == *(u32*)cs1; 

	checksum(&memcard->directory2, sizeof(Directory) - sizeof(u32), &cs1[0], &cs1[1]);
	u8 dir2Good = *((u32*)&memcard->directory2.checksum1) == *(u32*)cs1; 

	if(dir1Good && dir2Good)
	{
		return (memcard->directory1.updateCounter > memcard->directory2.updateCounter) ?
	&memcard->directory1 : &memcard->directory2;
	}

	if(dir1Good)
	{
 		return &memcard->directory1;
	}
	if(dir2Good)
	{
		return &memcard->directory2;
	}

	return 0;
}

Fat* SelectFileTable(MemCard* memcard)
{
	u16 cs[2];
	checksum((u16*)&memcard->fat1 + 2, sizeof(Fat) - 2 * sizeof(u16), &cs[0], &cs[1]);
	u8 bm1Good = *((u32*)&memcard->fat1.checksum1) == *(u32*)cs; 

	checksum((u16*)&memcard->fat2 +  2, sizeof(Fat) - 2 * sizeof(u16), &cs[0], &cs[1]);
	u8 bm2Good = *((u32*)&memcard->fat2.checksum1) == *(u32*)cs; 

	if(bm1Good && bm2Good)
	{
		return (memcard->fat1.updateCounter > memcard->fat2.updateCounter) ?
	&memcard->fat1 : &memcard->fat2;
	}

	if(bm1Good)
	{
 		return &memcard->fat1;
	}
	if(bm2Good)
	{
 		return &memcard->fat2;
	}
	return 0;
}

void BootFromMemcard(u8 slotB)
{
	const u8* pLoadPos = BS_ADDRESS;

	if(slotB) ebase = MEMCARD_B_EXI_REG_BASE;
	else ebase = MEMCARD_A_EXI_REG_BASE;

	// Read the memory card data headers
	for(int i = 0; i < 5; i++)
	{
		ReadMemoryCardBlock(i,pLoadPos + i * MEMCARD_BLOCK_SIZE);	
	}
	MemCard* memCard = (MemCard*)pLoadPos;
	
	// Select proper directory storage and fileAllocTable
	Directory* pDirectory = SelectDirectory(memCard);

	if(!pDirectory) return ;

	Fat* pfileAllocTable = SelectFileTable(memCard);

	if(!pfileAllocTable) return;

	// Copy the block map on the stack as we'll overwrite pLoadPos as soon as we find the dol and copy it there.
	Fat fileAllocTable;
	memcpy8((u8*)&fileAllocTable, (u8*)pfileAllocTable, sizeof(Fat));
	for(int i = 0; i < DIRECTORY_SIZE; i++)
	{
		DirectoryEntry* entry = &(pDirectory->entries[i]);
		if(memcmp8(XENO_DOL_NAME, (u8*)entry->filename, sizeof(XENO_DOL_NAME) - 1)) // exclude \0
		{
			// Swiss uses an extra block for the gci header so we actually start at block + 1
			u16 blockIndex = GetNextBlock(entry->firstBlockIndex, &fileAllocTable);
			u16 fileLength = entry->fileLength;

			int c = 0;
			while(c < fileLength && blockIndex != LAST_BLOCK)
			{
				ReadMemoryCardBlock(blockIndex, pLoadPos + c * MEMCARD_BLOCK_SIZE);	
				c++;
				blockIndex = GetNextBlock(blockIndex, &fileAllocTable);
			}

			LoadDolAtAddress((u8*)pLoadPos);
		}
	}
}

int main(void)
{
	memset32((void*)0x80000004, 0, (0x01700000-4)/4);
	*((u32*)0x80000C00) = 0x4c000064;

	// Try to boot from dol in slot B
	BootFromMemcard(1);
	// Try to boot from dol in slot A
	BootFromMemcard(0);

	// Let's init the video systems to error out we didn't find the dol in either memory card.
	InitSystem(*(u8*)(MEM_TEMP+0x55) == 'P');
	cls();
	ipl_set_config();
	ipl_read((u8*)MEM_TEMP, 0, 256);
	init_font();

	// And show error message
	print("XenoPoop\n\n");
	print("Could not find:\n");
	print(XENO_DOL_NAME);
	print("\n\nTurn off your GC.\n");
	print("Verify dol is in either\n");
	print("Memcard.");
	
	while(1);

	return 0; /*** keep gcc happy ***/
}
