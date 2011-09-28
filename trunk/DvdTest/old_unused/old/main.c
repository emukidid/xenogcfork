
#include<GCLib.h>


#define CUSTOMCMD_SELECTRANGE 12



#define DBG_SHOWWATCH(Nr, Name, Addr, Value)	if(g_dwActiveDbgWatch == Nr) {	DEBUG_Print(210, 40, #Name#Addr ); \
																				if(g_bUpdateDbgWatches) DEBUG_ShowValueU32(480, 40, Value);	\
																				if((g_nKey & PAD_START) && (g_nKey & PAD_R)) { \
																					if(!g_bToggleWatchAddr) g_nLBA = Addr; else g_nLBA = 0;}\
																					g_bToggleWatchAddr = !g_bToggleWatchAddr;};



extern volatile long *dvd;

extern u32* g_pDriveCode, g_pDriveCodeCmd;
extern u32	g_pDriveCodeEnd, g_pDriveCodeCmdEnd;

unsigned char g_pIntChainHook[4] = { 0x02, 0x85, 0x00, 0x00 };


u8*		g_pBuffer = NULL;
u8		g_aBuffer[2048 + 64];

u32*	g_pdwCustomCommand = NULL;
u8		g_aCustomDVDCommand[32 + 32];

u32		g_nKey;
u32		nViewOffset = 0;

u32		g_nLBA				= 0;
u32		dwCmd				= 0;
u32		dwCmdByteSelected	= 0;

u32		g_dwDriveSpeed		= 0;
u32		g_dwDriveSpeedDiff	= 0;

int		g_dwActiveDbgWatch	= 0;

u32		g_dwKeydownTime		= 0;
u8		g_bUpdateDbgWatches	= 0;
u8		g_bToggleWatchAddr	= false;

u32		g_dwDbgVal1			= 0;
u32		g_dwDbgVal2			= 0;

u32		g_dwDriveCodeSize		= 0;
u32		g_dwDriveCodeCmdSize	= 0;

u32		g_aPrevDvdRegs[10];

u8		g_bDbgOutMode = 0;

#define DVD_STDCOMMANDS 5
#define DVD_SPCOMMANDS 10

char* g_szDvdCommands[DVD_STDCOMMANDS]  = {	
	"[DVD_STARTDRIVE]",	
	"[DVD_SET_NOEDC]",
	"[DVD_SET_STD_SCRAMBLING]",	
	"[DVD_UNLOCK_DRIVE]",	
	"[DVD_CLEAR_HIMEM]",	
};


char* g_szDvdSpecialCommands[DVD_SPCOMMANDS]  = {	

	"[CMD NONE]",	
	"[CMD INIT]",	
	"[CMD STOP MOTOR]",	
	"[CMD STOP  LASER]",
	"[CMD START MOTOR]", 
	"[CMD START LASER]",
	"[CMD SET BREAKPOINT]",
	"[CMD FAKE RESET]",
	"[CMD SWAP DISK]",
	"[CMD FLASH LED]" 
};





void GC_Startup()
{
	GC_System_Init();

	// init pads
	GC_PAD_Init();
	
	// setup 640x480 pal mode
	GC_Video_Init(VI_PAL50_640x480); // VI_PAL60_640x480 // VI_NTSC_640x480

	GC_Video_SetFrameBuffer((void *) 0xc0500000, VIDEO_FRAMEBUFFER_BOTH);
	GC_Video_ClearFrameBuffer (g_pFrameBuffer, 0x86936274);
	
	// init debug text color
	DEBUG_InitText(0xFF80FF80);
	
	GC_SRand(0x2121);

	g_pBuffer	=  g_aBuffer +  (64 - ((u32) (g_aBuffer) % 4));
}



void FlushHexDumpBuffer(int nUsage)	/* 0 = normal dump mode, 1 = dbgout dump mode */
{
	if(nUsage == 0) {
		// stop dbgout mode on normal dump command
		g_bDbgOutMode = 0;
	}
	else {
		g_bDbgOutMode = 1;
	}

	GC_Memory_memset((void*)g_pBuffer, 0xBB, 2048);
	dcache_flush((void*)g_pBuffer, 2048);
}


/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«       DVDHack COMMANDS                                                                                         »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/
void CMD_ReadDVDInquiry()
{
	DEBUG_PrintStatus("Reading drive inquiry...");
	
	FlushHexDumpBuffer(0);
	DVD_Inquiry((DVD_DRIVEINFO*) g_pBuffer);
	DVD_RequestError();
}

void CMD_ReadSector()
{
	DEBUG_PrintStatusNoClear( "** Reading sector... **");
	FlushHexDumpBuffer(0);
	DVD_Read(g_pBuffer, 2048, g_nLBA * 2048);
	DVD_RequestError();
}

void CMD_SetDebugMode()
{
	DEBUG_PrintStatus("** SETTING DVD DEBUG MODE **");
	DVD_SetDebugMode();
}

void CMD_HexDumpDriveMem(u32 dwAddress)
{
	DEBUG_PrintStatusNoClear("** Reading drive mem ... **");
	DVD_ReadDriveMemBlock(dwAddress, (void*)g_pBuffer, 2048);
}


void CMD_CleanErrorByte()
{
	DEBUG_PrintStatus("** Cleaning error byte ... **");
	DVD_WriteDriveMemDword(0x818c, 0x00020c00);
}

void CMD_RunAnacondaDrivecode()
{
	DEBUG_PrintStatus("** Running anaconda drivecode **");
	DVD_SetDebugMode();
	DVD_Anaconda_InjectFirmwareHack();
}


void CMD_InjectCustomDriveCode()
{
	DEBUG_PrintStatus("** 4. INJECTING DRIVECODE **");

	// 384 412
	u32  dwSize = g_dwDriveCodeSize;//0x100;//g_dwDriveCodeSize;
	u32* dwAddr = (u32*) &g_pDriveCode;
	DVD_WriteDriveMemBlock(0x8502, dwAddr,	dwSize);

	dwSize = g_dwDriveCodeCmdSize;//0x200;//g_dwDriveCodeCmdSize;
	dwAddr = (u32*) &g_pDriveCodeCmd;
	DVD_WriteDriveMemBlock(0xff40d000, dwAddr,	dwSize);
	DVD_WriteDriveMemBlock(0x804c, g_pIntChainHook, sizeof(g_pIntChainHook));
}

void CMD_ReadDriveSpeed()
{
	u32 dwVal = DVD_ReadDriveMemDword(0x400700)  >> 19;
	g_dwDriveSpeedDiff	+= (dwVal - g_dwDriveSpeed);
	g_dwDriveSpeed		=  dwVal;
}

void CMD_ExecuteDriveCodeCommand(u32 dwCmd)
{
	// set drivecode command byte
	DVD_WriteDriveMemDword(0x8504, dwCmd);
}



/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«       DVDHack Pad Browsing                                                                                     »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/
void CMD_BrowseSectors(int g_nKey)
{
	if(g_nKey & PAD_RIGHT) {

		if(g_dwKeydownTime < 100) {
			g_nLBA += 0x1;
		}
		else {
			g_nLBA += 0x10;
		}
		
		if( !(g_nKey & (PAD_UP | PAD_DOWN))) {
			nViewOffset = 0;
		}
	}
	else if(g_nKey & PAD_LEFT) {

		if(g_dwKeydownTime < 100) {
			g_nLBA -= 0x1;
		}
		else {
			g_nLBA -= 0x10;
		}

		if( !(g_nKey & (PAD_UP | PAD_DOWN))) {
			nViewOffset = 0;
		}
	}

	else if(g_nKey & PAD_UP && nViewOffset > 0) {
		nViewOffset -= 0x10;
	}
	else if(g_nKey & PAD_DOWN && nViewOffset < 0x700) {
		nViewOffset += 0x10;
	}

	if(g_nKey & PAD_X) {

		if(g_dwKeydownTime < 100) {
			g_nLBA += 0x800;
		}
		else if (g_dwKeydownTime < 200) {
			g_nLBA += 0x1000;
		}
		else {
			g_nLBA += 0x10000;
		}

		GC_Sleep(20);
	}
	else if(g_nKey & PAD_Y) {
		if(g_dwKeydownTime < 10) {
			g_nLBA -= 0x800;
		}
		else if (g_dwKeydownTime < 20) {
			g_nLBA -= 0x1000;
		}
		else {
			g_nLBA -= 0x10000;
		}

		GC_Sleep(20);
	}
}


void CMD_BrowseCustomCommands(int g_nKey)
{
	if(g_nKey & PAD_LEFT) {
		dwCmdByteSelected = (dwCmdByteSelected - 1) % CUSTOMCMD_SELECTRANGE;
		GC_Sleep(40);
	}
	else if(g_nKey & PAD_RIGHT) {
		dwCmdByteSelected = (dwCmdByteSelected + 1) % CUSTOMCMD_SELECTRANGE;
		GC_Sleep(40);
	}
	else if(g_nKey & PAD_UP) {
		if(g_aCustomDVDCommand[dwCmdByteSelected + 8] < (DVD_STDCOMMANDS + DVD_SPCOMMANDS - 1)) {
			g_aCustomDVDCommand[dwCmdByteSelected + 8]++;
			GC_Sleep(30);
		}
	}
	else if(g_nKey & PAD_DOWN) {
		if(g_aCustomDVDCommand[dwCmdByteSelected + 8] > 0) {
			g_aCustomDVDCommand[dwCmdByteSelected + 8]--;
			GC_Sleep(30);
		}
	}

	if(g_nKey != 0) {
		g_dwDbgVal1 = dwCmdByteSelected;
	}
}

/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«       DVDHack Debug Watch mechanism                                                                            »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/

void ShowWatches()
{
	g_dwActiveDbgWatch = g_aCustomDVDCommand[9];

	int nWatch = 0;
	DBG_SHOWWATCH(nWatch++, SCRMBCMD  , 0x40EA00, DVD_ReadDriveMemDword(0x40EA00));
	DBG_SHOWWATCH(nWatch++, FREEMEM	 , 0x40d000, DVD_ReadDriveMemDword(0x40d000));
	DBG_SHOWWATCH(nWatch++, ErrByte  , 0x00818E, DVD_ReadDriveMemDword(0x818e));
	DBG_SHOWWATCH(nWatch++, DrvSpeed , 0x400700, g_dwDriveSpeed);
	DBG_SHOWWATCH(nWatch++, DrvSpDLT , 0x400700, g_dwDriveSpeedDiff);
	DBG_SHOWWATCH(nWatch++, DPM1     , 0x40ed0c, DVD_ReadDriveMemDword(0x40ed08));
	DBG_SHOWWATCH(nWatch++, DPM2     , 0x40ed08, DVD_ReadDriveMemDword(0x40ed08));
	DBG_SHOWWATCH(nWatch++, NIN ID   , 0x40C448, DVD_ReadDriveMemDword(0x40ed08));
}


/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«       DVDHack Status Display                                                                                   »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/

void ShowStats()
{
	int nCount	= 0;

	DEBUG_Print(0,  40,		"LBA:");
	DEBUG_Print(0,  60,		"DI[0]");
	DEBUG_Print(0,  80,		"DI[4]");
	DEBUG_Print(0,  100,	"DI[8]");
	
	DEBUG_ShowValueU32(70, 40, g_nLBA);
	
	u32 dwCol = 0xDEADBEEF;

	for(nCount = 0; nCount < CUSTOMCMD_SELECTRANGE; nCount++) {
		// change color every 4 bytes
		if(nCount % 4 == 0) {
			dwCol += 0x40404040;
		}
		
		if(nCount != dwCmdByteSelected) {
			DEBUG_SetTextColor(dwCol);
		}
		// mark selected byte
		else {
			DEBUG_SetTextColor(GC_Video_RGBToYCbCr(255,0,0));
		}

		DEBUG_ShowValueU8(0 + ((nCount) * 32), 20, g_aCustomDVDCommand[nCount + 8]);
	}
	
	DEBUG_SetTextColor(GC_Video_RGBToYCbCr(0,120,0));

	int nSpecialCmd = g_aCustomDVDCommand[8];
	
	if(nSpecialCmd >= 0 && nSpecialCmd < DVD_STDCOMMANDS) {
		DEBUG_Print(0, 0, g_szDvdCommands[nSpecialCmd]);
	}
	else if(nSpecialCmd >= DVD_STDCOMMANDS && nSpecialCmd < (DVD_STDCOMMANDS + DVD_SPCOMMANDS)) {
		DEBUG_Print(0, 0, g_szDvdSpecialCommands[nSpecialCmd - DVD_STDCOMMANDS]);
	}
	else {
		DEBUG_ShowValueU32(0, 0, (g_dwDriveCodeSize << 16) | g_dwDriveCodeCmdSize);
	}

	/*++++++++++++++++++++++++++++++++++++++++++++
		dvd[0]		R_DVD_STATUS_1
		dvd[1]		R_DVD_STATUS_2
		dvd[2]		R_DVD_COMMAND
		dvd[2]		R_DVD_SUBCOMMAND
		dvd[2]		R_DVD_SUBCOMMAND2
		dvd[3]		R_DVD_OFFSET
		dvd[4]		R_DVD_SOURCELENGTH
		dvd[5]		R_DVD_DESTINATIONBUFFER
		dvd[6]		R_DVD_DESTINATIONLENGTH
		dvd[7]		R_DVD_ACTION
		dvd[8]		R_DVD_IMMBUF
		dvd[9]		R_DVD_CONFIG
	 *++++++++++++++++++++++++++++++++++++++++++++*/

	for(nCount = 0; nCount < 10; nCount++) {

		u32 dwReg = dvd[nCount];

		// mark changed registers
		if(g_aPrevDvdRegs[nCount] == dwReg) {
			DEBUG_SetTextColor(0xFF80FF80);
		}
		else {
			DEBUG_SetTextColor(GC_Video_RGBToYCbCr(255,0,0));
		}

		int nX = 90 + ((nCount % 4) * 140);
		int	nY = 60 + ((nCount / 4) * 20);

		DEBUG_ShowValueU32(nX,	nY,	dwReg);

		g_aPrevDvdRegs[nCount] = dwReg;
	}

	DEBUG_ShowValueU32(370, 100, g_dwDbgVal1);
	DEBUG_ShowValueU32(510, 100, g_dwDbgVal2);

	if(g_bUpdateDbgWatches) {
		DEBUG_SetTextColor(GC_Video_RGBToYCbCr(200,0,0));
	}
	
	ShowWatches();
	DEBUG_SetTextColor(0xFF80FF80);
}


typedef struct tDvdCpuRegs 
{
	u16 d0, d1, d2, d3;
	u32 a0, a1, a2;

} sDvdCpuRegs;


/*!	\fn			void ShowHexDump()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void ShowHexDump()
{
	if(g_bDbgOutMode == 0) {
		DEBUG_SetTextColor(GC_Video_RGBToYCbCr(255, 255, 255));
		GC_DBG_PrintHexDump(7, g_pBuffer, 15 * 16, nViewOffset);
	}
	else {
		sDvdCpuRegs* pRegs = (sDvdCpuRegs*) g_pBuffer;

		DEBUG_SetTextColor(GC_Video_RGBToYCbCr(230, 230, 255));

		DEBUG_ShowValueU16Named(10, 140, pRegs->d0, "d0:");
		DEBUG_ShowValueU16Named(10, 160, pRegs->d1, "d1:");
		DEBUG_ShowValueU16Named(10, 180, pRegs->d2, "d2:");
		DEBUG_ShowValueU16Named(10, 200, pRegs->d3, "d3:");
		DEBUG_ShowValueU32Named(10, 220, pRegs->a0, "a0:");
		DEBUG_ShowValueU32Named(10, 240, pRegs->a1, "a1:");
		DEBUG_ShowValueU32Named(10, 260, pRegs->a2, "a2:");

		GC_DBG_PrintHexDump(17, g_pBuffer, 5 * 16, nViewOffset);
	}
}


/*!	\fn			void Startup()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void Startup()
{
	GC_Startup();
	GC_Memory_memset(g_aCustomDVDCommand, 0, 8 * 4);

	CMD_SetDebugMode();

	g_nLBA		= 0;

	// determine drivecode sizes
	g_dwDriveCodeSize		= ((u32) &g_pDriveCodeEnd)		- ((u32) &g_pDriveCode);
	g_dwDriveCodeCmdSize	= ((u32) &g_pDriveCodeCmdEnd)	- ((u32) &g_pDriveCodeCmd);
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
	Startup();

	u32 dwRand3 = 0x1;

	while(true) {
		
		GC_Video_WaitForVBlank();
		GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);

		g_nKey = GC_PAD_GetCurrentKeys();

		if((g_nKey & 0x0f) == 0) {
			g_dwKeydownTime = 0;
		}
		else {
			g_dwKeydownTime++;
		}

		ShowHexDump();
		ShowStats();

		//-------------------------------------------------
		// STD COMMANDS
		//-------------------------------------------------
		if(!(g_nKey & PAD_R) && !(g_nKey & PAD_L)) {

			CMD_BrowseSectors(g_nKey);

			if(g_nKey & PAD_A) {
				CMD_ReadSector();
			}

			if(g_nKey & PAD_B) { 
				if(g_nLBA > 0) {
					FlushHexDumpBuffer(0);
					CMD_HexDumpDriveMem(g_nLBA);
				}
			}
		
			if(g_nKey & PAD_START) {

				DEBUG_PrintStatus("**  Dumping drive memory **");
				DVD_ReadDriveMemBlock(0x400000, (void *) 0x80800000, 0x10000);

//				g_bUpdateDbgWatches = !g_bUpdateDbgWatches;
//				GC_Sleep(160);
			}

			if(g_nKey & PAD_Z) {
				// return to sdload
				asm("lis 3,		0x80d0");
				asm("ori 3, 3,	0x0000");
				asm("mtlr 3");
				asm("blrl");
			}
		}
		//-------------------------------------------------
		// EXTENDED COMMANDS 1 (holding r)
		//-------------------------------------------------
		else if((g_nKey & PAD_L)) {

			if(g_nKey & PAD_Z) {
				DEBUG_PrintStatus("** Resetting drive **");
				DVD_Reset();
			}
			if(g_nKey & PAD_R) {
				DEBUG_PrintStatus("** Reading Id **");
				FlushHexDumpBuffer(0);
				DVD_ReadId((void*) g_pBuffer);
				GC_Sleep(160);
			}
			if(g_nKey & PAD_X) {

				FlushHexDumpBuffer(1);
				CMD_HexDumpDriveMem(0x40d800);

				//if(g_bDbgOutMode == 1) {
				//}

				GC_Sleep(160);
			}
		}
		//-------------------------------------------------
		// EXTENDED COMMANDS 2 (holding r)
		//-------------------------------------------------
		else if((g_nKey & PAD_R)) {

			if(g_nKey & PAD_Y) {
				CMD_SetDebugMode();
			}
			if(g_nKey & PAD_X) {
				CMD_SetDebugMode();
				CMD_InjectCustomDriveCode();
				g_aCustomDVDCommand[8] = DVD_STDCOMMANDS;
				GC_Sleep(160);
			}

			if(g_nKey & PAD_B) {
				FlushHexDumpBuffer(0);
				CMD_HexDumpDriveMem(0x8000);
			}

			if(g_nKey & PAD_A) {
				int nSpecialCmd = g_aCustomDVDCommand[8];

				if(nSpecialCmd >= DVD_STDCOMMANDS && nSpecialCmd <= (DVD_STDCOMMANDS + DVD_SPCOMMANDS)) {
					u32 dwCmd = ((nSpecialCmd - DVD_STDCOMMANDS) << 24);
					CMD_ExecuteDriveCodeCommand(dwCmd);
				}
	
				//---------------------------------------------------
				// [DVD_TEST_STARTDRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 0) {
					DVD_WriteDriveMemDword(0x8188, 0x03000000);		// 0x0301c003
					DVD_WriteDriveMemDword(0x81a4, 0x06000300);
			
					DVD_CustomDbgCommand(0xFE114100, 0, 0);
				}
				//---------------------------------------------------
				// [DVD_TEST_DISABLE_EDC]
				//---------------------------------------------------
				else if(nSpecialCmd == 1) {

					//---------------------------------------------------
					// disable edc
					//---------------------------------------------------
					DVD_WriteDriveMemDword(0x8080, 0x6bda);
					DVD_CustomDbgCommand(0xFE100000, 0x773e165d, 0);
					
					//---------------------------------------------------
					// sets same params as bp scramble patching
					//---------------------------------------------------
					//DVD_WriteDriveMemDword(0x8080, 0x2ab25564);
					//DVD_CustomDbgCommand(0xFE100000, 0x50aca159, 0xa02b2856);
					//DVD_WriteDriveMemDword(0x8080, 0x00);
					
					//---------------------------------------------------
					// set random params
					//---------------------------------------------------
/*					u32 dwRand1 = GC_Rand() + (GC_Rand() << 16); // 0x2484
					u32 dwRand2 = GC_Rand() + (GC_Rand() << 16); // 0x1242
					dwRand3 = GC_Rand() + (GC_Rand() << 16);
					
					DVD_WriteDriveMemDword(0x8000, dwRand1);
					DVD_WriteDriveMemDword(0x8004, dwRand2);
					DVD_WriteDriveMemDword(0x8008, dwRand3);
					
					DVD_WriteDriveMemDword(0x8080, dwRand1);
					DVD_CustomDbgCommand(0xFE100000, dwRand2, dwRand3);
*/
				}
				//---------------------------------------------------
				// [DVD_TEST_DISABLE_SCRAMBLING]
				//---------------------------------------------------
				else if(nSpecialCmd == 2) {
					DVD_CustomDbgCommand(0xFE030000, GC_Rand() + (GC_Rand() << 16), GC_Rand() + (GC_Rand() << 16));
				}
				//---------------------------------------------------
				// [DVD_TEST_UNLOCK_DRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 3) {

					DVD_CustomDbgCommand(0xFE114000, 0, 0);

					// scramble cdbs 40ec62 40ec76 -> 40ec75 || 40ec89 -> 0 means disable scramble

					// DVD_WriteDriveMemDword(0x40ec5c, 0x00002f07);
					// DVD_WriteDriveMemDword(0x40ec74, 0x41000000);
					// DVD_WriteDriveMemDword(0x8050, 0x00800000);
					// DVD_WriteDriveMemDword(0x8054, 0x10400000);
					// DVD_WriteDriveMemDword(0x40ec88, 0x0);
					
					// DVD_CustomDbgCommand(0xFE030000, 0, 0);
					// DVD_CustomDbgCommand(0xFE030000, GC_Rand(), GC_Rand());

					// DVD_CustomDbgCommand(0xFE114106, 0, 0);
					// DVD_CustomDbgCommand(0xFE0f0000, 0x0000000, 0x00000000);
					// DVD_WriteDriveMemDword(0x8188, 0x0201c003);
					// DVD_WriteDriveMemDword(0x8188, 0x0301c003);
					// DVD_WriteDriveMemDword(0x81a4, 0x06000300);
					// DVD_WriteDriveMemDword(0x40ea8c, 0x03000040);
					// DVD_CustomDbgCommand(0xFE030000, 0, 0);
					// DVD_WriteDriveMemDword(0x40ec74, 0x00000000);

					// DVD_WriteDriveMemDword(0x40ea5c, 0x00412c00);
					// DVD_WriteDriveMemDword(0x40ea5c, 0x00380000 + GC_Rand());
					// DVD_WriteDriveMemDword(0x40ed00, 0x00086303);
					// DVD_WriteDriveMemDword(0x40ea8c, 0x03000040);
				}

				//---------------------------------------------------
				// [DVD_CLEAR_HIMEM]
				//---------------------------------------------------
				else if(nSpecialCmd == 4) {
					GC_Memory_memset((void*)g_pBuffer, 0xCC, 2048);
					DVD_WriteDriveMemBlock(0xff40d800, g_pBuffer, 0x200);
				}

				GC_Sleep(160);
			}	

			CMD_BrowseCustomCommands(g_nKey);
		}
	}	
    return 0;
}
























/*

	DVD_WriteDriveMemDword(0x40ec74, 0x00000000);
	DVD_WriteDriveMemDword(0x40ec60, 0x13000000);


*/

/*

  //					DVD_WriteDriveMemBlock(0xff40d000, g_pBuffer, 0x800);
//					DVD_WriteDriveMemDword(0x80040, 0x21212121);
//					DVD_WriteDriveMemDword(0x80800, 0x21212121);
//					DVD_WriteDriveMemDword(0x70040, 0x21212121);
//					DVD_WriteDriveMemDword(0x70800, 0x21212121);

	u8		aBits[32];
	u8		aBitsPrev[16];
	u8		aBitHexDump[32];

	u8* aBitFreq	= &aBits[16];

	u16 dwSpeed1;
	u16 dwSpeed2;
	u16 dwSpeed3;
	
	u16 dwSpeed	= g_dwDriveSpeed >> 3;
	dwSpeed1	= g_dwDriveSpeed;
	dwSpeed2	= dwSpeed;
	DEBUG_ShowValueU16(320, 40, dwSpeed1);
	DEBUG_ShowValueU16(410, 40, dwSpeed2);
	DEBUG_ShowValueU16(500, 40, dwSpeed3);
	dwSpeed = dwSpeed3;
		memcpy(aBitsPrev, aBits, 16);

		for(nCount= 0; nCount < 16; nCount++) {
			
			u8 nBit =  (dwSpeed >> ((15 - (nCount))) & 1);
			aBits[nCount] =  nBit;

			if((aBitsPrev[nCount] != nBit) && (aBitFreq[nCount] < 0xff)) {
				aBitFreq[nCount]	+= nBit;
			}
		}

		for(nCount= 7; nCount >= 0; nCount--) {
			aBitHexDump[nCount]			= BITS2BYTE(aBits[nCount * 2],	aBits[((nCount  * 2)  + 1)]);
			aBitHexDump[nCount + 16]	= BITS2BYTE(aBitFreq[nCount * 2] / 0x2, aBitFreq[((nCount  * 2)  + 1)]  / 0x2);
		}
*/

/*
		DEBUG_PrintStatus("** DOING STD ANACONDA_SWAP in 2 secs **");
		DVD_Anaconda_Delay2();

		DEBUG_PrintStatus("** 1. RESET **");
		DVD_Reset();

		DEBUG_PrintStatus("** 2. DELAY8 **");
		DVD_Anaconda_Delay8();

		DEBUG_PrintStatus("** 3. UNLOCK **");
		DVD_SetDebugMode1();
		DVD_SetDebugMode2();

		DEBUG_PrintStatus("** 4. INJECT **");
		DVD_Anaconda_InjectFirmwareHack();

		DEBUG_PrintStatus("** 5. DELAY2 **");
		DVD_Anaconda_Delay2();

		DEBUG_PrintStatus("** 6. READ_ID **");
		DVD_ReadId((void*) g_pBuffer);

		DEBUG_PrintStatus("** 7. FINISH **");
*/


/*	u32 dwMem		= 0;		
	int dwRead		= 0;
	u32* pBuffer	= (u32 *) g_pBuffer;

	DEBUG_Print(100, 440, "** Reading drive mem ... **");

	for(dwRead = 0; dwRead < 512; dwRead++) {
		u32 dwMem = DVD_ReadDriveMemDword(0x008000 + (dwRead * 4));
		pBuffer[dwRead] = dwMem;

		if(dwMem == 0xBEEFBEEF) {
			break;
		}
	}
*/

//	u8	bHi  = HIBYTE(dwSpeed);
//	u8	bLo  = LOBYTE(dwSpeed);
//	dwSpeed  = BYTES2WORD(bLo, bHi);

/*
				DEBUG_Print(100, 440, "** Dumping drive firmware ... **");
				
				GC_Memory_memset((void*)g_pBuffer, 0xBB, 2048);
				
				u32 dwAddr		= 0x0;
				u32 dwLen		= 64 * 1024;
				u32 dwOffset	= 0;
				u32 dwChecksum	= 0;
				
				u32* pDest		= (u32 *) 0x80800000;	//g_pBuffer;

				*(pDest - 1) = *((u32*) "FWST");

				while(dwOffset < dwLen) {
					u32 dwMem = DVD_ReadDriveMemDword(dwAddr + dwOffset); //0x2BADBEEF
					pDest[dwOffset / 4] = dwMem;
					//dwChecksum	+= dwMem;
					dwOffset	+= 4;

					if(dwMem == 0xBEEFBEEF) {
						break;
					}
				}
				pDest[dwOffset / 4] = *((u32*) "FWEN");
*/

/*
	g_pdwCustomCommand = (u32*) &g_aCustomDVDCommand[0];
	
	g_pdwCustomCommand[0] = 0x2E;
	g_pdwCustomCommand[1] = 0;
	
	g_pdwCustomCommand[2] = 0xA8000000;
	g_pdwCustomCommand[3] = 0x8502;
	g_pdwCustomCommand[4] = (u32) 0xC0000;
	g_pdwCustomCommand[5] = (u32) g_pBuffer;
	g_pdwCustomCommand[6] = (u32) 0x20;
	g_pdwCustomCommand[7] = 3;
*/

//			if(g_nKey & PAD_START) {
//	//			DVD_Reset();
//	//			GC_Sleep(4000);
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(50, 440, "** SENDING DBGMODE COMMAND... **");
//				DVD_SetDebugMode();
//				DEBUG_Print(100, 440, "** DISABLING DVD DESCRAMBLE... **");
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DVD_DisableDescrambling();
//				DVD_ReadId((void*) 0x80000000);
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** OKI                         **");
//
//
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** SWAP!!!                    **");
//				GC_Sleep(4000);
//
//				
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** Reading id...                 ");
//
//				DVD_ReadId((void*) 0x80000000);
//
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** DONE                        **");
//	//			DVD_RequestError();
//			}

			// return to old ipl
//			asm("lis 3,		0x817c"); 

			// return to level3.dol
//			asm("lis 3,		0x8030");
//			asm("ori 3, 3,	0x0100");
//			asm("mtlr 3");
//			asm("blrl");

			// return to ipl
//			asm("lis 3,		0x80d0");
//			asm("ori 3, 3,	0x0000");
//			asm("mtlr 3");
//			asm("blrl");


//		else if(bDoRandomSeeks) {
//			u32 dwRandSector = RAND(0xa05ff);
//			DEBUG_ShowValueU32(350, 60, dwRandSector);
//			DVD_Read(g_pBuffer, 512, dwRandSector * 2048);
//			DVD_RequestError();
//		}


//		if(g_nKey & PAD_CRIGHT) {
//			dwCmd += 0x01000000;
//		}
//		else if(g_nKey & PAD_CLEFT) {
//			dwCmd -= 0x01000000;
//		}
//		else if(g_nKey & PAD_CUP) {
//			dwCmd = ++dwCmd & 0xFF0000FF;
//		}
//		else if(g_nKey & PAD_CDOWN) {
//			dwCmd = --dwCmd & 0xFF0000FF;
//		}


//		if(g_nKey & PAD_START) {
//			DEBUG_Print(50, 440, "** SENDING DBGMODE COMMAND... **");
//			DVD_SetDebugMode();
//			DEBUG_Print(100, 440, "** DISABLING DVD DESCRAMBLE... **");
//			DVD_DisableDescrambling();
//		}

//			DEBUG_Print(100, 440, "sending custom command...");
//			GC_Memory_memset((void*)g_pBuffer, 0xCC, 2048);
//			dcache_flush((void*)g_pBuffer, 2048);
//			DVD_CustomCommand(dwCmd, g_pBuffer);
//			DVD_RequestError();




//			DEBUG_Print(100, 440, "Resetting dvd drive...");
//			DVD_Reset();
//			DEBUG_Print(100, 440, "Reading id...");
//			DVD_ReadId((void*) 0x80000000);
//			DVD_RequestError();
//			DEBUG_Print(100, 440, "done.");

//			DEBUG_Print(100, 440, "dumping leadout....");
//
//			int nSec = 0xa0000;
//			u8* pDest = (u8*) (0x80040000);
//
//			for( ;nSec < 0xa0600; nSec++) {
//				DVD_Read(pDest, 2048, nSec * 2048);
//				pDest += 2048;
//			}
//			DEBUG_Print(100, 440, "done!");

//		else if(g_nKey & PAD_B) { 
//			DEBUG_Print(100, 440, "sending custom command...");
//			GC_Memory_memset((void*)g_pBuffer, 0xCC, 2048);
//			dcache_flush((void*)g_pBuffer, 2048);
//			DVD_CustomCommand(dwCmd, g_pBuffer);
//			DVD_RequestError();
//		}





//	DEBUG_Print(100, 220, "Open and close drive...");
//	while(!DVD_IsCoverOpen()) {
//	}
//	while(DVD_IsCoverOpen()) {
//	}



//		GC_PAD_Read(&sPad1, GC_PAD1);
//		if(sPad1.button & PAD_START) {
//			asm("lis 3, 0x8030");
//			asm("ori 3, 3, 0x0100");
//			asm("mtlr 3");
//			asm("blrl");
//		}



//hotreset:
//lis r3,0
//lis r9,0xCC00
//sth r3, 0x2000(r9)
//li r4, 3
//stw r4, 0x3024(r9)
//stw r3, 0x3024(r9)
//nop
//loop__: b loop__





//asm(".align 4");
//
//asm(".global g_pDriveCode");
//asm("g_pDriveCode:");

// Generated by Hex Workshop
// test.bin - Starting Offset: 0 (0x00000000) Length: 136 (0x00000088)



//#define g_pDriveCode rawData

//asm(".incbin drivecode.bin");
//asm(".global g_pDriveCodeEnd");
//asm("g_pDriveCodeEnd:");

//extern u32 g_pDriveCode;
//extern u32 g_pDriveCodeEnd;
