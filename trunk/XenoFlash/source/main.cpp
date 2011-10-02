/*---------------------------------------------------------------------------------

 XenoFlash - Flash updater for XenoGC

 DOL that communicates with the MN102 on the GC DVD drive
 in order to program the XenoGC in-system
 
 Created by the XenoGC Team
 Ported to libOGC by dantheman2865, ... of gc-forever.com

---------------------------------------------------------------------------------*/
#include <stdio.h>
#include <gccore.h>        // Wrapper to include common libogc headers
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvd.h"
 

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

#define FLASHFILE		"../XenoAT/source/XenoAT.bin"
#define DbgPrint	printf

//INCBIN(Flashloader,			"flashloader.bin")
//INCBIN(XenoROM,				FLASHFILE)


extern short g_nConsoleX, g_nConsoleY; 
extern const unsigned long title_Bitmap[110*640];
void CheckDriveState(bool bWaitForXenoUnload = true);

u8 pBuffer[FLASHSIZE];

// 2D Video Globals

GXRModeObj *vmode;		// Graphics Mode Object
u32 *xfb = NULL;		// Framebuffer  
int dvdstatus = 0;

static u8 dvdbuffer[2048] ATTRIBUTE_ALIGN (32);    // One Sector
dvdcmdblk cmdblk;
 
/*---------------------------------------------------------------------------------
 Initialise Video

 Before doing anything in libogc, it's recommended to configure a video
 output.
---------------------------------------------------------------------------------*/
static void Initialise () {
//---------------------------------------------------------------------------------

	VIDEO_Init ();	/*	ALWAYS CALL FIRST IN ANY LIBOGC PROJECT!
						Not only does it initialise the video 
						subsystem, but also sets up the ogc os
					*/
 
	PAD_Init ();	// Initialise pads for input
 
	
	vmode = VIDEO_GetPreferredMode(NULL);
 
	// Let libogc configure the mode
	VIDEO_Configure (vmode);
 

	/*
	
	Now configure the framebuffer. 
	Really a framebuffer is just a chunk of memory
	to hold the display line by line.
	
	*/

	xfb = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));


	// Define a console
	console_init (xfb, 20, 64, vmode->fbWidth, vmode->xfbHeight,
	vmode->fbWidth * 2);
 
    // Clear framebuffer to black
	VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);
	VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);

	/*** Set the framebuffer to be displayed at next VBlank ***/
	VIDEO_SetNextFramebuffer (xfb);
 
	VIDEO_SetBlack (0);
 
	// Update the video for next vblank 
	VIDEO_Flush ();
  
	VIDEO_WaitVSync ();        // Wait for next Vertical Blank

	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync ();
 
}

// Simple Delay based on 60Hz refresh rate (great for debugging drivecode)
void GC_Sleep(u32 dwMiliseconds)
{
	int nVBlanks = (dwMiliseconds / 16);
	while(nVBlanks-- > 0) {
		VIDEO_WaitVSync();
	}
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

		if((dwIntVec != 0xDDDDDDDD) && (dwIntVec != 0) || (PAD_ButtonsDown(0) & PAD_BUTTON_B)) {
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
	VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);

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

int mainold()
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
		
		u32 dwKey = PAD_ButtonsDown(0);

		if(dwKey & PAD_BUTTON_A) {
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

//---------------------------------------------------------------------------
int main () {
//--------------------------------------------------------------------------
	int ret = 0;
	int mounted = 0;
	int *p;
	int j, i;
 
	Initialise ();		// Start video etc
	DVD_Init ();		// And the DVD subsystem

	printf ("libOGC DVD Example\n");
	printf ("Mounting Disc\n");
 
	// This will mount pretty much any disc, original or ISO
	mounted = DVD_Mount ();
	printf ("OK\n");
 
	// Read Sector 0
	ret = DVD_ReadPrio (&cmdblk, dvdbuffer, 2048, 0, 2);

	if (ret <= 0) {
		printf ("Error during read sector 0\n");
		while (1);
	}
 
	printf ("Read %d bytes\n", ret);
	p = (int *) dvdbuffer;
 
	// Identify disc type
	j = 0;

	for (i = 0; i < 512; i++)
		j += p[i];
 
	if (j == 0) {
		printf ("Disc is most probably ISO-9660\n");
	} else {

		if (memcmp (dvdbuffer + 32, "GAMECUBE \"EL TORITO\" BOOTLOADER", 31) == 0) {
			printf ("Disc is gc-linux bootable\n");
		} else {

			if (memcmp (dvdbuffer, "COBRAMB1", 8) == 0) {
				printf ("Disc is multigame image\n");
			} else {
				printf ("Disc is gcm\n%s\n", (char *) dvdbuffer + 32);
			}
		}
	}
 
	while(1) {

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);
		

		if (buttonsDown & PAD_BUTTON_START) {
			void (*reload)() = (void(*)())0x80001800;
			reload();
		}
	}
 
	return 0;
}
