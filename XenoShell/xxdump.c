/*	asm (".globl GC_Interrupts_Enable");
	asm ("GC_Interrupts_Enable:");
	asm ("mfmsr		r3");
	asm ("ori		r4, r3, 0x8000");
	asm ("mtmsr		r4");
	asm ("extrwi	r3, r3, 1, 16");
*/
/*	asm("lis r3,0");
	asm("lis r9,0xCC00");
	asm("sth r3, 0x2000(r9)");
	asm("li r4, 3");
	asm("stw r4, 0x3024(r9)");
	asm("stw r3, 0x3024(r9)");
	asm("nop");
	asm("loop__: b loop__");
*/


/*	0x00060101, 0x4B6A01B0, 0x02F85640, 0x001501E6,
	0x001401E7, 0x4D2B4D6D, 0x4D8A4D4C, 0x00435A4E,
	0x00000000, 0x00435A4E, 0x00000000, 0x013C0144,
	0x113901B1, 0x10010001, 0x00010001, 0x00010001,
	0x00000000, 0x00000000, 0x28500100, 0x1AE771F0,
	0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF,
	0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000,
	0x02800000, 0x000000FF, 0x00FF00FF, 0x00FF00FF	

	0x11F50101, 0x4B6A01B0, 0x02F85640, 0x00010023,
	0x00000024, 0x4D2B4D6D, 0x4D8A4D4C, 0x00435A4E,
	0x00000000, 0x00435A4E, 0x00000000, 0x013C0144,
	0x113901B1, 0x10010001, 0x00010001, 0x00010001,
	0x00000000, 0x00000000, 0x28500100, 0x1AE771F0,
	0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF,
	0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000,
	0x02800000, 0x000000FF, 0x00FF00FF, 0x00FF00FF	*/
	
	/*
int DVD_SetDebugMode()
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xff014d41;
	dvd[3] = 0x54534849;
	dvd[4] = 0x54410200;
	dvd[7] = 1;
	
	DVD_WaitImmediate();

	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xff004456;
	dvd[3] = 0x442d4741;
	dvd[4] = 0x4d450300;
	dvd[7] = 1;
	
	DVD_WaitImmediate();

	if (dvd[0] & 0x04) {
		return 1;
	}
	
	return 0;

}


int DVD_WriteDriveMemDword(u32 dwAddress, u32 dwData)
{
	int nCount = 0;
	
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xFE010100;	

	dvd[3] = dwAddress;
	dvd[4] = 0x00040000;	
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 3;

	DVD_WaitImmediate();

	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = dwData;
	dvd[7] = 1;

	DVD_WaitImmediate();

	return 0;
}
*/

/*	asm("lis r3,1");
	asm("lis r9,0xCC00");
	asm("sth r3, 0x2000(r9)");
	asm("li r4, 2");
	asm("stw r4, 0x3024(r9)");
	asm("stw r3, 0x3024(r9)");
	asm("nop");
	asm("loop__: b loop__");
	
	asm("lis r3, 0x8130");
	asm("ori r3, r3, 0x0000");
//	asm("lwz r3, 0(r3)");
	asm("mtlr r3");
	asm("blr");
*/




#define CODEBASE					0x80010000
#define MEM_WORK					CODEBASE	+ 0x10000	//	0x80020000	0x80080000
#define MEM_FONT					MEM_WORK	+ 0x10000	//	0x80030000	0x80100000	
#define MEM_BACKGROUND				MEM_FONT	+ 0x10000	//	0x80040000	0x81100000

#define MEM_FB						((MEM_BACKGROUND + 0x40000) | 0xC0000000)	// 0xC006C000	0xC0F00000	0xC0500000
#define MEM_FB2						(MEM_FB + 0x500)
#define MEM_VIDEO					((MEM_FB & 0xBFFFFFFF) + 0x170000)			// 0x801DC000	0x80120000	space: 1424000(20mb)



sphere_t sphere[]={	{{SPHERE_XLEFT, 10, 14}, 6, 2,  .92},
					{{28,			12, 5}, 11,  2,  .92},
};
// old:				{{{-5, 8, 9}, 10, 2, .92}};


plane_t plane[]={	{{  0,   1,  -0.3},		0,			TEX_FLOOR,		0.1	},	// floor
					{{  0,   0.3,  0},		120,		TEX_CEILING,	0.0	},		// ceiling 
					{{  0,   -0.3, 0},		120,		TEX_CEILING,	0.0	},		// ceiling 
//					{{  0,	 -.3,  1},		200,		TEX_CEILING,	0.0	},		// back plane
					{{	0.020,   0.4,  1.0},	20,		TEX_TRANS,		0.0	}		// transparent plane
};		
/*									{{0,1,0}, 10, 1, .1}, 
									{{0,1,0}, 150, 1, 0}, d
									{{0,-.4,1}, 200, 0, 0},  
									{{.1,.4,1}, 20, 3, 0,	-15, 10, -13, -5}};*/


				//pVideoBuffer = (u16*) MEM_VIDEO;
				//pVideoBuffer = (u16*) (MEM_VIDEO + (VIDFRAMES-1) * CAPSIZE * 2);
				//pVideoBuffer = (u16*) (MEM_VIDEO + (VIDFRAMES-1) * CAPSIZE * 2);
				//nFrame = 0;
/*
void rayInit(void)
{
	u32 i;
	for (i=0; i<planes; ++i) {

		vec a = { 0, 0, 1};		
		if(i == 0) {
			a.x = 0.5f;
		}
		else {
			a.x = 0;
		}

		cross(&planedyn[i].m1, &a, &plane[i].a);
		cross(&planedyn[i].m2, &planedyn[i].m1, &plane[i].a);
	}
}
*/
#define ABS(x) ((x < 0) ? (-x):(x))

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))

//-----------------------------------------------------------
// more logical in my opinion....
//-----------------------------------------------------------
#define ATMOST(val, limit)		MIN(val, limit)
#define ATLEAST(val, limit)		MAX(val, limit)

/*		if(sphere[0].center.x < 28) {
			sphere[0].center.x += 0.94f;
		}
		else {
			sphere[0].center.x += 0.44f;
			sphere[1].center.x += 0.94f;
		}

		// sphere[1].center.x -= 0.89f;
		// sphere[0].center.z += 0.25f;
		// plane[2].a.x += 0.012;
*/
/*


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
		
//		szIPL [5] = 'J';
//		u8 bJap = (*(unsigned short*)0xCC00206E) & 2;
//		szIPL [5] += 'J' * bJap;
//		u16* pPatch = ((u16*) &szIPL [5]);
//		*pPatch++ = 0x4A;
//		u32* pPatch = ((u32*) &szIPLRevString [5]);
//		*pPatch++ = 0x4A415020;
//		*pPatch++ = 0x20202020;

  if(nCredits == 1) {
				//---------------------------------------------
				// playback prerendered animation
				//---------------------------------------------
				int dwMove = CAPSIZE;
				int nPlayFrame;

				pVideoBuffer = (u16*) MEM_VIDEO;

				for(nPlayFrame = 0; nPlayFrame < (VIDFRAMES * 2)-1; nPlayFrame++) {
					u32 dwDelay = 0x1400000;

					if(nPlayFrame == VIDFRAMES-1) {
						dwMove = -dwMove;
						dwDelay = 0xc0FF0000;
					}

					// clearFB();
					memcpy32(fb + (CAPSTART/2), (u32*) pVideoBuffer , CAPSIZE*2);
					pVideoBuffer += dwMove;
					while(dwDelay--);
				}

				nFrame = 0;
				pVideoBuffer = (u16*) MEM_VIDEO;
				sphere[0].center.x = -25;
				//sphere[1].center.x = 28;
				continue;
			}
			
			  if(nCredits == 0) {
				//---------------------------------------------
				// playback prerendered animation
				//---------------------------------------------
				int nPlayFrame;
				pVideoBuffer = (u16*) MEM_VIDEO;

				for(nPlayFrame = 0; nPlayFrame < VIDFRAMES; nPlayFrame++) {
					// clearFB();
					memcpy32(fb + (CAPSTART/2), (u32*) pVideoBuffer , CAPSIZE*2);
					pVideoBuffer += CAPSIZE;

					u32 dwDelay = 0x1200000;
					while(dwDelay--);
				}

				nFrame = 0;
				sphere[0].center.x = -25;
				//sphere[1].center.x = 28;
				continue;
			}
			
			  if(nCredits == 0) {
				//---------------------------------------------
				// playback prerendered animation
				//---------------------------------------------
				int dwMove = CAPSIZE;
				int nPlayFrame;

				pVideoBuffer = (u16*) MEM_VIDEO;

				for(nPlayFrame = 0; nPlayFrame < VIDFRAMES * 2; nPlayFrame++) {
					u32 dwDelay = 0x1200000;

					if(nPlayFrame == VIDFRAMES) {
						dwMove = -dwMove;
						dwDelay = 0x80FF0000;
					}

					// clearFB();
					memcpy32(fb + (CAPSTART/2), (u32*) pVideoBuffer , CAPSIZE*2);
					pVideoBuffer += dwMove;
					while(dwDelay--);
				}

				nFrame = 0;
				sphere[0].center.x = -25;
				sphere[1].center.x = 28;
			}
*/

/*		#ifdef DBGKEY
			while(1) {

				asm("lis 3, 0xCC00");
				asm("lhz 4, 0x6404(3)");
				asm("lwz 5, 0x6408(3)");

				asm("andi. 5, 4, 0xf7f");
				asm("beq 1f"); 
				asm("lis 4, 0x8140");
				asm("mtlr 4");
				asm("blrl");
				asm("1:");
			}
		#endif
*/

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

