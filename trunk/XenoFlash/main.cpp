#include <GCLib.h>
#include <GC_vsprintf.h>
#include <GC_String.h>


#define RELEASE

#define CHUNKSIZE 0x200
#define FLASHSIZE 0x2000

#define CMD_INITFLASH	0x26000000
#define CMD_ERASEFLASH	0x27000000
#define CMD_WRITEFLASH	0x28000000

#define DRIVEMEM_BINARY 0xFF40BBC0

#define IMAGEBASE		0x40D000
#define VERIFY_FLASH	0

#define DVD_FlashErase			(IMAGEBASE + 0x04)
#define DVD_FlashEnable			(IMAGEBASE + 0x07)
#define DVD_WriteFlashBlock		(IMAGEBASE + 0x0a)				
#define DVD_UnloadQcode			(IMAGEBASE + 0x0D)
#define DVD_ReadFlashBlock		(IMAGEBASE + 0x10)

#define FLASHFILE		"../XenoAT/bin/XenoAT.bin"

INCBIN(Flashloader,			"flashloader.bin")
INCBIN(XenoROM,				FLASHFILE)


extern short g_nConsoleX, g_nConsoleY; 
extern const unsigned long title_Bitmap[110*640];
void CheckDriveState(bool bWaitForXenoUnload = true);

u8 pBuffer[FLASHSIZE];

void GC_Startup()
{
	GC_System_Init();

	// init pads
	GC_PAD_Init();
	
	// setup 640x480 pal mode
	GC_Video_Init(VI_PAL50_640x480);
	
	GC_Video_SetFrameBuffer((void*)  0xC0F00000, VIDEO_FRAMEBUFFER_1);
	GC_Video_SetFrameBuffer((void*) (0xC0F00500), VIDEO_FRAMEBUFFER_2);

	// init debug text color
	DEBUG_InitText(RGB2YCBR(255, 255, 255));
	
	GC_SRand(0x2121);
}


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


void UnloadXenoGC_Permanent()
{
	DbgPrint("\nstopping drive...");
	DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
	CheckDriveState(false);
}


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
					//unloaded!
					GC_Sleep(500);
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

bool IsXenoDrivecode()
{
	DVD_SetDebugMode();
	DVD_WaitImmediate();
	
	u32 dwIntVec = DVD_ReadDriveMemDword(0x804c);
	return (dwIntVec == 0x02C64000);
}

bool FlashInit()
{
	DEBUG_InitText(RGB2YCBR(220,0,0));
	DbgPrint("\n\nFlash init......");

	DVD_CallFunc(DVD_FlashEnable);
	CheckDriveState(false);
	DbgPrint("done");

	return true;
}

bool FlashErase()
{
	DbgPrint("\nErasing flash...");
	DVD_CallFunc(DVD_FlashErase);
	GC_Sleep(1000);
	CheckDriveState(false);	
	DbgPrint("done");
	return true;
}

void SetFlashAddress(u32 dwAddress)
{
	dwAddress /= 2;
	u32 dwFlashAddressCmd = (((dwAddress & 0xff) << 8) | ((dwAddress >> 8) & 0xFF));
	DVD_WriteDriveMemDword(DRIVEMEM_BINARY-4, dwFlashAddressCmd);
}

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
}

void InitDrive()
{
	DEBUG_InitText(RGB2YCBR(0,0,220));
	
	// unload xeno if active
	if(IsXenoDrivecode()) {
		UnloadXenoGC_Permanent();
		GC_Sleep(1000);
	}

	// upload the flash programming drivecode
	DbgPrint("\n\nUploading flash-app to drive...");
	CMD_InjectCustomDriveCode();
	DbgPrint("done.");

	// make sure drive is ready
	CheckDriveState(true);

	// enable dbg mode again(!)
	DVD_SetDebugMode();
	DVD_WaitImmediate();
}

int main()
{
	GC_Startup();

	// show title
	DrawTitle();
	
	// assure drive is ready
	InitDrive();

	DEBUG_InitText(RGB2YCBR(0,220,0));
	DbgPrint("\n\n\nset flash switch to [ON] position now");
	DbgPrint("\nthen press A to start flashing.");

	while(true) {
		
		u32 dwKey = GC_PAD_GetCurrentKeys();

		if(dwKey & PAD_A) {
			// programm the chip
			FlashInit();
			FlashErase();
			FlashUpdate();
			DEBUG_InitText(RGB2YCBR(0, 0, 255));
			DbgPrint("\n\nset flash switch back to [OFF]");
			DbgPrint("\nthen then reboot.");
		}
		GC_Video_WaitForVBlank();
	}	

    return 0;
}

