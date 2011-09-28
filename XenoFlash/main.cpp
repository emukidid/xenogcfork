

#include <GCLib.h>
#include <GC_vsprintf.h>
#include <GC_String.h>


//#define RELEASE

#define CHUNKSIZE 0x200
#define FLASHSIZE 0x2000

#define CMD_INITFLASH	0x26000000
#define CMD_ERASEFLASH	0x27000000
#define CMD_WRITEFLASH	0x28000000

#define DRIVEMEM_BINARY 0xFF40BBC0	// 0xFF40BBC0 //0xFF40D800

#define IMAGEBASE		0x40D000


#define VERIFY_FLASH	0

#define DVD_FlashErase			(IMAGEBASE + 0x04)
#define DVD_FlashEnable			(IMAGEBASE + 0x07)
#define DVD_WriteFlashBlock		(IMAGEBASE + 0x0a)				
#define DVD_UnloadQcode			(IMAGEBASE + 0x0D)
#define DVD_ReadFlashBlock		(IMAGEBASE + 0x10)



//#define FLASHFILE		"XenoAT.1.03a.v2.bin"
#define FLASHFILE		"../XenoAT/bin/XenoAT.bin"
//#define FLASHFILE		"XenoAT.bin"
//#define FLASHFILE		"XenoGC 1.01.bin"
//#define FLASHFILE		"XenoGC.txt.bin"


INCBIN(Flashloader,			"flashloader.bin")
INCBIN(XenoROM,				FLASHFILE)

#ifndef RELEASE
	INCBIN(GCOs15,			"gcos.dol")
#endif
//INCBIN(DvdTest,		"DvdTest.dol")






extern short g_nConsoleX, g_nConsoleY; 
extern const unsigned long title_Bitmap[110*640];
void CheckDriveState(bool bWaitForXenoUnload = true);

u8 pBuffer[FLASHSIZE];


/*!	\fn			void GC_Startup()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void GC_Startup()
{
	GC_System_Init();

	// init pads
	GC_PAD_Init();
	
	// setup 640x480 pal mode
	GC_Video_Init(VI_PAL50_640x480);
	
	GC_Video_SetFrameBuffer((void*)  0xC0F00000, VIDEO_FRAMEBUFFER_1);
	GC_Video_SetFrameBuffer((void*) (0xC0F00500), VIDEO_FRAMEBUFFER_2);
//	GC_Video_ClearFrameBuffer (g_pFrameBuffer, RGB2YCBR(0,0,0));

	// init debug text color
	DEBUG_InitText(RGB2YCBR(255, 255, 255));
	
	GC_SRand(0x2121);
}




/*!	\fn			void LoadBinary(void* pSource, u32 dwDest, u32 dwSize)
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void LoadBinary(void* pSource, u32 dwDest, u32 dwSize)
{
	u8* pCodeDst = (u8*) dwDest;

	// copy dol loader code to safe buffer area
	memcpy((void *) pCodeDst, pSource, dwSize);
	flush_code(pCodeDst, 0x8000);
	void (*pLoadFn)() = (void (*)()) pCodeDst;

	DbgPrint("\njumping to: %08x", pCodeDst);
	pLoadFn();
}


/*!	\fn			void fn_load_dol_fn_inmem(void *dol)
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
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
	mtmsr((mfmsr() | 2) & ~0x8000);
	entrypoint();
}


/*!	\fn			void load_dol_fn_inmem(void *dol, int size)
 *	
 *				simplyfied version of above function, doesnt use subroutines
 *				so it can be copied to another mem location. needed when
 *				uploading a dol that overlaps gcos itself
 *
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void LoadDol(void *pDol, u32 dwSize)
{
	u32* pDolImage = (u32*) pDol;

	u32 dwCodeSrc = (u32) fn_load_dol_fn_inmem;
	u32* pCodeDst = (u32 *) 0x81200000;

	DbgPrint("\nLoading dol from %08x size: %x", pDol, dwSize);
//	fn_load_dol_fn_inmem(pDol);

	// copy dol loader code to safe buffer area
	memcpy((void *) pCodeDst, (void*) dwCodeSrc, 0x200);
	flush_code(pCodeDst, 0x8000);

	// run it
	void (*pLoadFn)(void*) = (void (*)(void*)) pCodeDst;

	DbgPrint("\njumping to: %08x", pCodeDst);
	GC_Sleep(100);
	pLoadFn(pDol);
}


/*!	\fn			u32 CalcChecksum(void* pDataIn, u32 dwSize)
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
u32 CalcChecksum(void* pDataIn, u32 dwSize)
{
	u32 dwChecksum = 0;
	u32 *pData = (u32*) pDataIn;

	for(int nDwords = 0; nDwords < dwSize / 4; nDwords++) {
		dwChecksum += pData[nDwords];
	}

	return dwChecksum;
}


/*!	\fn			void CMD_InjectCustomDriveCode()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void CMD_InjectCustomDriveCode()
{
	u32  dwSize = 0;
	u32* dwAddr = 0;

	// write flashloader
	dwAddr = g_pFlashloader;
	dwSize = BINSIZE(Flashloader);				
	DVD_WriteDriveMemBlock(0xff40D000, dwAddr,	dwSize);
	DVD_CallFunc(DVD_UnloadQcode);

	// patch interrupt chain vector to 40D000
	DVD_WriteDriveMemDword(0x804c, 0x00D04000);
}


/*!	\fn			void UnloadXenoGC_Permanent()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void UnloadXenoGC_Permanent()
{
	DbgPrint("\nstopping drive...");
	DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
	CheckDriveState(false);

//	DbgPrint("\nunloading XenoGC....");
//	DVD_CustomDbgCommand(0x25000000, 0, 0, 0);
//	GC_Sleep(12000);
//	CheckDriveState();
}


/*!	\fn			void CheckDriveState(bool bWaitForXenoUnload = true)
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void CheckDriveState(bool bWaitForXenoUnload)
{
	int nTry = 0;

	while(true) {

		GC_Sleep(100);
		
		DVD_SetDebugMode();
		u32 dwStatus = DVD_RequestError();
		u32 dwIntVec = DVD_ReadDriveMemDword(0x804c);

		if(dwIntVec != 0xDDDDDDDD && dwIntVec != 0 || (GC_PAD_GetCurrentKeys() & PAD_B)) {
			if(bWaitForXenoUnload) {
				if(dwIntVec != 0x02C64000) {
					GC_Sleep(500);
					//DbgPrint("unloaded!");
					return;
				}
			}
			else {
				GC_Sleep(100);
				return;
			}
		}

		if(++nTry > 120) {
			DbgPrint("error!");
			return;
		}
	}
}

/*!	\fn			bool IsXenoDrivecode()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
bool IsXenoDrivecode()
{
	DVD_SetDebugMode();
	DVD_WaitImmediate();
	
	u32 dwIntVec = DVD_ReadDriveMemDword(0x804c);
	return (dwIntVec == 0x02C64000);
}


/*!	\fn			bool FlashInit()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
bool FlashInit()
{
	DEBUG_InitText(RGB2YCBR(220,0,0));
	DbgPrint("\n\nFlash init......");

	DVD_CallFunc(DVD_FlashEnable);
	CheckDriveState(false);
	DbgPrint("done");

	return true;
}


/*!	\fn			bool FlashErase()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
bool FlashErase()
{
	DbgPrint("\nErasing flash...");
	DVD_CallFunc(DVD_FlashErase);
	GC_Sleep(1000);
	CheckDriveState(false);	
	DbgPrint("done");
	return true;
}


/*!	\fn			void SetFlashAddress(u32 dwAddress)
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void SetFlashAddress(u32 dwAddress)
{
	dwAddress /= 2;
	u32 dwFlashAddressCmd = (((dwAddress & 0xff) << 8) | ((dwAddress >> 8) & 0xFF));
	DVD_WriteDriveMemDword(DRIVEMEM_BINARY-4, dwFlashAddressCmd);
}


/*!	\fn			bool FlashUpdate()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
bool FlashUpdate()
{
	DbgPrint("\nFLASHING........[                ]");
	
	g_nConsoleX -= 16*17;
	u32 dwTime = GC_GetTime() >> 15;

	u8* pROM = (u8*) g_pXenoROM;
	
	for(u32 dwAddress = 0; dwAddress < FLASHSIZE; dwAddress += CHUNKSIZE) {

		// set next address to flash to
		SetFlashAddress(dwAddress);

		// write one chunk
		DVD_WriteDriveMemBlock(DRIVEMEM_BINARY, pROM, CHUNKSIZE);
		DVD_CallFunc(DVD_WriteFlashBlock);
		CheckDriveState(false);

		#if VERIFY_FLASH == 1
			// verify chunk
			DVD_CallFunc(DVD_ReadFlashBlock);
			CheckDriveState(false);
			DVD_ReadDriveMemBlock(0x40D800, pBuffer, CHUNKSIZE);

			if(memcmp(pBuffer, pROM, CHUNKSIZE)) {
				DbgPrint("X");
			}
		#endif

		if(dwAddress % (FLASHSIZE/16) == 0) {
			DbgPrint("*");
		}

		pROM += CHUNKSIZE;
	}

	DbgPrint("\nDONE!");
	dwTime = (GC_GetTime() >> 15) - dwTime;
	DbgPrint("   Time: %d MS", dwTime);

	return true;
}


/*!	\fn			void DrawTitle()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void DrawTitle()
{
	DEBUG_ResetConsole();
	GC_Video_ClearFrameBuffer(g_pFrameBuffer, RGB2YCBR(0, 0, 0));

	u32* pDst = (u32*) g_pFrameBuffer + 640*20;
	u32* pSrc = (u32*) title_Bitmap;

	for(int nCount = 0; nCount < 640 * 110 / 2; nCount++) {
		*pDst++ = *pSrc++;
	}

	DEBUG_InitText(RGB2YCBR(200, 200, 255));
	DbgPrint("\n\n\n\n\n\n\n\n");
	DbgPrint("\nXenoGC flash update 1.04 beta");
//	DbgPrint(FLASHFILE);
}

/*!	\fn			void InitDrive()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void InitDrive()
{
	DEBUG_InitText(RGB2YCBR(0,0,220));
	
/*	u32 dwErr = DVD_RequestError();
	DbgPrint("\nInitialising drive (state: %x)", dwErr);

	if(dwErr != 0) {
		DbgPrint("\nresetting drive");
		DVD_Reset();
		CheckDriveState(false);
	}
*/  
	//-----------------------------------------------
	// make sure drive is ready
	//-----------------------------------------------
//	CheckDriveState(false);

	//-----------------------------------------------
	// unlock drive
	//-----------------------------------------------
//	DVD_SetDebugMode();
//	DVD_WaitImmediate();

	//-----------------------------------------------
	// unload xeno if active
	//-----------------------------------------------
	if(IsXenoDrivecode()) {
		UnloadXenoGC_Permanent();
		GC_Sleep(1000);
	}

	//-----------------------------------------------
	// upload the flash programming drivecode
	//-----------------------------------------------
	DbgPrint("\n\nUploading flash-app to drive...");
	CMD_InjectCustomDriveCode();
	DbgPrint("done.");

	// TODO: wait for drive ready here

	//-----------------------------------------------
	// make sure drive is ready
	//-----------------------------------------------
//	GC_Sleep(2000);
//	DbgPrint("CheckDriveState.");
	CheckDriveState(true);
	
	//-----------------------------------------------
	// enable dbg mode again(!)
	//-----------------------------------------------
//	DbgPrint("DVD_SetDebugMode.");
	DVD_SetDebugMode();
//	DbgPrint("DVD_WaitImmediate.");
	DVD_WaitImmediate();
}


/*!	\fn			int main()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
int main()
{
	GC_Startup();

	//-----------------------------------------------
	// show title
	//-----------------------------------------------
	DrawTitle();
		
	//-----------------------------------------------
	// assure drive is ready
	//-----------------------------------------------
	InitDrive();

	DEBUG_InitText(RGB2YCBR(0,220,0));
	DbgPrint("\n\n\nset flash switch to [ON] position now");
	DbgPrint("\nthen press A to start flashing.");

	while(true) {
		
		u32 dwKey = GC_PAD_GetCurrentKeys();

		if(dwKey & PAD_A) {
			//-----------------------------------------------
			// programm the chip
			//-----------------------------------------------
			FlashInit();
			FlashErase();
			FlashUpdate();
			DEBUG_InitText(RGB2YCBR(0, 0, 255));
			DbgPrint("\n\nset flash switch back to [OFF]");
			DbgPrint("\nthen then reboot.");
		}

#ifndef RELEASE
		if(dwKey & PAD_X) {
			//-----------------------------------------------
			// read back flash data
			//-----------------------------------------------
			FlashInit();
			DbgPrint("\nreading flash...");
			DVD_CallFunc(DVD_ReadFlashBlock);
			CheckDriveState(false);
			DbgPrint("done");
		}

		if(dwKey & PAD_Y) {
			DVD_Reset();
			DrawTitle();
			CheckDriveState(false);
			InitDrive();
		}
#endif
		if(dwKey & PAD_Z) {
			FlashInit();
			FlashErase();
		}

#ifndef RELEASE
		if(dwKey & PAD_START) {
			
			//DVD_Reset();

			// return to gcos
			LoadDol(g_pGCOs15, BINSIZE(GCOs15));

			asm("lis 0,		0x8140");
			asm("ori 0, 0,	0x0000");
			asm("mtlr 0");
			asm("blr");

		}
#endif
		GC_Video_WaitForVBlank();
	}	

    return 0;
}

