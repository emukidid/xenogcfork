/*---------------------------------------------------------------------------------

 XenoFlash - Flash updater for XenoGC

 DOL that communicates with the MN102 on the GC DVD drive
 in order to program the XenoGC in-system

 Created by the XenoGC Team
 Ported to libOGC by dantheman2865, emu_kidid, infact, and megalomaniac of gc-forever.com

---------------------------------------------------------------------------------*/
#include <stdio.h>
#include <gccore.h>        // Wrapper to include common libogc headers
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <ogc/lwp_watchdog.h>

#include "title_bmp.h"

extern "C" {
	#include "dvd.h"
}

/*** some variables ***/

#define CHUNKSIZE 0x200
#define FLASHSIZE 0x2000

#define CMD_INITFLASH	0x26000000
#define CMD_ERASEFLASH	0x27000000
#define CMD_WRITEFLASH	0x28000000

#define DRIVEMEM_BINARY 0xFF40BBC0

#define IMAGEBASE	0x40D000
#define VERIFY_FLASH	0

#define DVD_FlashErase		(IMAGEBASE + 0x04)
#define DVD_FlashEnable		(IMAGEBASE + 0x07)
#define DVD_WriteFlashBlock	(IMAGEBASE + 0x0a)	
#define DVD_UnloadQcode		(IMAGEBASE + 0x0D)
#define DVD_ReadFlashBlock	(IMAGEBASE + 0x10)

// 2D Video Globals
GXRModeObj *vmode;	// Graphics Mode Object
u32 *xfb = NULL;	// Framebuffer
int dvdstatus = 0;      //  << is this even needed ?? remove later


/*** included binaries ***/
#include "flashloader_bin.h"
#include "XenoAT_bin.h"

/*** banner ***/
extern const unsigned char title_Bitmap[];
static unsigned char title_banner [TITLE_WIDTH * TITLE_HEIGHT * 2] ATTRIBUTE_ALIGN (32);


void CheckDriveState(bool bWaitForXenoUnload = true);
u8 pBuffer[FLASHSIZE];
//static u8 dvdbuffer[2048] ATTRIBUTE_ALIGN (32);    // One Sector
//dvdcmdblk cmdblk;
/*---------------------------------------------------------------------------------
 Initialise Video

 Before doing anything in libogc, it's recommended to configure a video
 output.
---------------------------------------------------------------------------------*/
static void Initialise () {
//---------------------------------------------------------------------------------

   VIDEO_Init ();
   PAD_Init ();
   vmode = VIDEO_GetPreferredMode(NULL);
 
   // Let libogc configure the mode
   VIDEO_Configure (vmode);
   xfb = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));

   // Define a console
   console_init (xfb, 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);
 
   // Clear framebuffer to black
   VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);

   // Set the framebuffer to be displayed at next VBlank
   VIDEO_SetNextFramebuffer (xfb);
   VIDEO_SetBlack (0);
 
   // Update the video for next vblank 
   VIDEO_Flush ();
   VIDEO_WaitVSync ();
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
   dwAddr = (u32 *) flashloader_bin;
   dwSize = flashloader_bin_size;
   DVD_WriteDriveMemBlock(0xff40D000, dwAddr, dwSize);

   printf("..");
   DVD_CallFunc(DVD_UnloadQcode);
   printf("..");

   // patch interrupt chain vector to 40D000
   DVD_WriteDriveMemDword(0x804c, 0x00D04000);
}

/*  Dont need this, moved this single call function below in InitDrive()
void UnloadXenoGC_Permanent()
{
	printf("\nstopping drive...\n");

#if IS_PORTED
	DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
	CheckDriveState(false);
#endif
}
*/

void CheckDriveState(bool bWaitForXenoUnload)
{

	int nTry = 0;
//	printf("Check Drive State");
	while(true) {

		GC_Sleep(100);
//	printf(".");
		DVD_SetDebugMode();
//	printf(".");
		u32 dwStatus = DVD_RequestError();
//	printf(".");
		u32 dwIntVec = DVD_ReadDriveMemDword(0x804c);
//	printf(".");
	
	
	
//	printf("DVD Error: %li\n", dwStatus);
//	printf("ReadDrive 0x804c:           %li\n", dwIntVec);
//	u32 dwIntVec8 = DVD_ReadDriveMemDword(0x40D800);
//	printf("ReadDrive 0x40D800    AFTER: %li\n", dwIntVec8);
//		GC_Sleep(100);
		
//		DVD_SetDebugMode();
//		u32 dwStatus1 = DVD_RequestError();

//	printf("DVD Error: %li\n", dwStatus1);
//	u32 dwIntVec8 = DVD_ReadDriveMemDword(0x804c);		// MUST READ before leaving...dont know why but it works...
//	printf("ReadDrive 0x804c      AFTER: %li\n", dwIntVec8);
//	u32 dwIntVec9 = DVD_ReadDriveMemDword(0x804c);
//	printf("ReadDrive 0x804c      AFTER: %li\n", dwIntVec9);
//	u32 dwIntVec9 = DVD_ReadDriveMemBlock(0x40D000, pBuffer, CHUNKSIZE);
//	printf("ReadDrive 0x40D800    AFTER: %li\n", dwIntVec9);
	
//	u32 dwIntVec10 = DVD_ReadDriveMemDword(0x40D000);
//	printf("ReadDrive 0x40D000:         %li\n", dwIntVec10);

		if(((dwIntVec != 0xDDDDDDDD) && (dwIntVec != 0)) || (PAD_ButtonsDown(0) & PAD_BUTTON_B)) {
			if(bWaitForXenoUnload) {
				if(dwIntVec != 0x02C64000) {
//				  printf("unloaded!\n");
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

		if(++nTry > 5) {//120) {
			printf("error!\n\n");
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

bool helper()
{
   DVD_CallFunc(DVD_FlashEnable);
   CheckDriveState(false);
   DVD_CallFunc(DVD_FlashErase);
   GC_Sleep(1000);
   CheckDriveState(false);	
   return true;
}

bool FlashInit()
{
   //set red bg color
   printf("\x1b[31mFlash init...");

   DVD_CallFunc(DVD_FlashEnable);
   CheckDriveState(false);
	printf(".");
   return true;
}

bool FlashErase()
{
   printf("Erasing flash...");
   DVD_CallFunc(DVD_FlashErase);
   GC_Sleep(1000);
   CheckDriveState(false);	

   printf("done\n");
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
   printf("FLASHING........[                ]");

   //move cursor 17 chars left
   printf("\x1b[17D");


   u32 dwTime = gettime() >> 15;

   u8* pROM = (u8*) XenoAT_bin;


   for(u32 dwAddress = 0; dwAddress < FLASHSIZE; dwAddress += CHUNKSIZE) {

      // set next address to flash to
      SetFlashAddress(dwAddress);

      // write one chunk
      DVD_WriteDriveMemBlock(DRIVEMEM_BINARY, pROM, CHUNKSIZE);
      DVD_CallFunc(DVD_WriteFlashBlock);
      CheckDriveState(false);

      #if VERIFY_FLASH == 1
         // verify chunk
         //printf("verify one chunk\n");
         DVD_CallFunc(DVD_ReadFlashBlock);
         //printf("Set to ReadFlashBlock chunk\n");
         CheckDriveState(false);
         DVD_ReadDriveMemBlock(0x40D800, pBuffer, CHUNKSIZE);
         //printf("ReadDriveMemBlock\n");
         if(memcmp(pBuffer, pROM, CHUNKSIZE)) {
            printf("X");
         }
      #endif

      if(dwAddress % (FLASHSIZE/16) == 0) {
         printf("*");
      }

      pROM += CHUNKSIZE;
   }

   dwTime = (gettime() >> 15) - dwTime;
   printf("\n\x1b[32mDONE!");
   printf("   \x1b[37mTime: %d MS\n", (int)dwTime);

   return true;
}

/****************************************************************************
* unpacks the title banner
****************************************************************************/
static void
unpack_banner (void)
{
   unsigned long inbytes, outbytes;

   inbytes = TITLE_COMPRESSED;
   outbytes = TITLE_SIZE;

   uncompress (title_banner, &outbytes, title_Bitmap, inbytes);
}

void DrawTitle()
{
   int y, x, j;
   int offset;
   int *bb = (int *) title_banner;

   offset = (12 * 320); /*** start 10 lines from top ***/

   VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);

   for (y = 0, j = 0; y < TITLE_HEIGHT; y++)
   {
      for (x = 0; x < (TITLE_WIDTH >> 1); x++)
         xfb[offset + x] = bb[j++];

         offset += 320;
   }

   //set white color, console position
   printf("\x1b[37m\x1b[6;40f");
   printf("openXenoGC flash updater");// 0.01\n");

   VIDEO_Flush ();
   VIDEO_WaitVSync ();
}

void InitDrive()
{
// printf("\x1b[34m");  //set blue color  << too damn dark
   printf("\x1b[36m");  //set blue cyan   << better

   // unload xeno if active
   if(IsXenoDrivecode()) {
      printf("stopping drive...\n");
      DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
      CheckDriveState(false);
   }

   // upload the flash programming drivecode
   printf("Uploading flash-app to drive...");
   CMD_InjectCustomDriveCode();
   printf("done");
   
   // make sure drive is ready
   printf("Wait for XenoUnload...");
   CheckDriveState(true);

   // enable dbg mode again(!)
   DVD_SetDebugMode();
   DVD_WaitImmediate();
}

//---------------------------------------------------------------------------
int main ()
//--------------------------------------------------------------------------
{
   Initialise ();

   // show title
   unpack_banner();
   DrawTitle();
   printf("\n\nXenoGC flash v1.03a-v1\n");


   // assure drive is ready
   InitDrive();
   printf("READY!!\n\n");

   //set green color
   printf("\x1b[32m");
   printf("Set flash switch to \x1b[31m[ON]\x1b[32m position now\n");
   printf("then press A to start flashing.\n\n");

   while(1) {

      VIDEO_WaitVSync();
      PAD_ScanPads();

      int buttonsDown = PAD_ButtonsDown(0);
/*
      if(buttonsDown & PAD_BUTTON_B) {
         // flash init, not really needed
         //    used for initial testing to help get code working
         //    remove this later
         FlashInit();
      }
*/
      if(buttonsDown & PAD_BUTTON_A) {
         // program the chip
         //   after programming complete, turn switch off and check led status
         //   if led is lit, then done, else turn switch on and flash again 
         FlashInit();
         // FlashErase();
         FlashUpdate();

         //set blue color, black background
         printf("\x1b[36m \x1b[40m");
         printf("\n\nSet flash switch back to [OFF] then reboot.\n\n");
         // while (1) VIDEO_WaitVSync(); /*** loop till death ***/
      }

      if(buttonsDown & PAD_BUTTON_Y) {
         // erase the chip
         //   need to erase twice then turn xeno switch off
         //   check led status 
         //   if no leds lit, then turn switch on and flash
         //   if led is lit, then erase was not performed
         //   turn swithc on and try erase again
         FlashInit();
         GC_Sleep(1000);
         FlashErase();
      }
/*
      if(buttonsDown & PAD_BUTTON_X) {
         //-----------------------------------------------
         // read back flash data
         //-----------------------------------------------
         FlashInit();
         printf("reading flash...\n");
         DVD_CallFunc(DVD_ReadFlashBlock);
         CheckDriveState(false);
         printf("\ndone\n\n");
      } */
   }

   return 0;
}
