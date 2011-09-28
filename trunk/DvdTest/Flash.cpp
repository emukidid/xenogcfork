
#include <GC_Dvd.h>
#include <GCLib.h>
#include <GC_vsprintf.h>

																					
extern volatile long *dvd;


//INCBIN(QCode, "qcode.bin");


extern u8*	g_pBuffer;
extern u32	g_nKey;

/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«       DVDHack COMMANDS                                                                                         »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/
void CMD_ReadSector(bool bShowStatus);
void CMD_InjectCustomDriveCode();
void Startup();
void CMD_SetDebugMode();

//	DVD_ReadDriveMemBlock(dwAddress, (void*)g_pBuffer, 2048);

/*	#define LOADER_ADDR 0x80a74

	// stack-friendly loading :p
	static u8 pUnloaderCode[] =	{	0xF4,0x74,0x74,0x0a,0x08,		//	F47474A708  MOV	$080a74,a0		# restore original 
									0xF7,0x20,0x4C,0x80,			//	F7204C80    MOV	a0,($804c)		# inthandler
									0xF4,0x74,						//	F47400D040  MOV	QCODEIMGBASE,a0	# jump to drivecode init
									(LOADER_ADDR		& 0xFF),				
									(LOADER_ADDR >> 8	& 0xFF),
									(LOADER_ADDR >> 16	& 0xFF),		
									0xF0,0x00						//	F000        JMP	(a0)
	};
*/



void Flash_Startup()
{
	GC_System_Init();

	// init pads
	GC_PAD_Init();
	
	// setup 640x480 pal mode
	GC_Video_Init(VI_PAL50_640x480);
	
	GC_Video_SetFrameBuffer((void*)  0xC0F00000, VIDEO_FRAMEBUFFER_1);
	GC_Video_SetFrameBuffer((void*) (0xC0F00500), VIDEO_FRAMEBUFFER_2);
	GC_Video_ClearFrameBuffer (g_pFrameBuffer, RGB2YCBR(0,0,0));

	// init debug text color
	DEBUG_InitText(RGB2YCBR(0,0,255));
	
	GC_SRand(0x2121);
}


u32 GetSectorCheckSum(u8* pSector);
bool IsValidSectorCheckSum(u8* pSector, u32 dwCheckSum);

void CheckStatus()
{
	while(true) {

		DVD_SetDebugMode();
		u32 dwStatus = DVD_RequestError();
		u32 dwIntVec = DVD_ReadDriveMemDword(0x804c);
		if(dwIntVec != 0x02C64000 && dwIntVec != 0xDDDDDDDD && dwIntVec != 0 || (GC_PAD_GetCurrentKeys() & PAD_B)) {
			GC_Sleep(500);
			DbgPrint("done!");
			return;
		}

		GC_Sleep(2000);
	}
}

void UnloadXenoGC_Permanent()
{
	DbgPrint("");
	DbgPrint("unloading XenoGC...");
	DVD_CustomDbgCommand(0x25000000, 0, 0, 0);
	DbgPrint("stopping drive...");
	DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
	GC_Sleep(3000);
	CheckStatus();
}


int FlashMain()
{
	Startup();
	Flash_Startup();

	UnloadXenoGC_Permanent();
	GC_Sleep(1000);
	GC_Video_ClearFrameBuffer(g_pFrameBuffer, RGB2YCBR(0,0,0));

	DEBUG_Print(160, 60, "XenoGC flasher v.01");
	DEBUG_InitText(RGB2YCBR(255,0,0));

	DEBUG_Print(0, 80, "Uploading flash-app to drive...");
	CMD_InjectCustomDriveCode();

	DEBUG_Print(0, 120, "set flash switch to on position");
	DEBUG_Print(0, 140, "then press A to start flashing.");

	while(true) {
		
		g_nKey = GC_PAD_GetCurrentKeys();

		if(g_nKey & PAD_A) {
			CMD_SetDebugMode();
			DVD_CustomDbgCommand(0x26000000, 0x00, 0x00, 0x00);
			DEBUG_Print(0, 180, "FLASHING...");
			GC_Sleep(12000);
			DEBUG_Print(0, 200, "DONE!");
		}

		if(g_nKey & PAD_Z) {
			// return to gcos
			asm("lis 0,		0x8140");
			asm("ori 0, 0,	0x0000");
			asm("mtlr 0");
			asm("blr");
		}

		GC_Video_WaitForVBlank();
	}	

    return 0;
}
