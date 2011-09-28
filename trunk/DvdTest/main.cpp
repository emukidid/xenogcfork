
#include <GC_Dvd.h>
#include <GCLib.h>
#include <GC_vsprintf.h>


//#include "card.h"

																					
extern volatile long *dvd;

extern "C" {
	void SUB_BootXenoShell();
}



INCBIN(QCode,		"qcode.bin");
INCBIN(XenoROM,		"XenoGC 1.03.bin")
INCBIN(BinaryImage, "XenoShell.bin");
INCBIN(DolImage,	"gcos.dol");
//INCBIN(DolImage,	"GCOS 1.5.patched.dol");
//INCBIN(DolImage,	"qoobDump.dol");


u8*		g_pBuffer = NULL;
u8		g_aBuffer[(0x800 * 0x40) + 0x100] ATTRIBUTE_ALIGN(32);

u32*	g_pdwCustomCommand = NULL;
u8		g_aCustomDVDCommand[32 + 32];

u32		g_aPrevDvdRegs[10];

int		g_nLbaPreset = 0;
u32		g_dwLbaPresets[3] = { 0x00000000, 0x400000, 0x40D000 };

u32		g_nKey;
u32		nViewOffset			= 0;
u32		g_nLBA				= 0;
u32		dwCmd				= 0;
u32		dwCmdByteSelected	= 0;

u32		g_dwDriveSpeed		= 0;
u32		g_dwDriveSpeedDiff	= 0;

int		g_dwActiveDbgWatch	= 0;

u32		g_dwKeydownTime		= 0;
u8		g_bUpdateDbgWatches	= 0;
u8		g_bShowLowMem		= false;

u32		g_dwDbgVal1			= 0;
u32		g_dwDbgVal2			= 0;

u8		g_bDbgOutMode		= 0;
u16		g_wDbgDumpBytes		= 0;
u16		g_wDbgDumpOffset	= 0;

u8		g_bCheckDRE			= 0;


#define DBG_SHOWWATCH(Nr, Name, Addr, Value)	\
												\
		if(g_dwActiveDbgWatch == Nr)	{\
			DEBUG_Print(210, 40, #Name#Addr );\
			if(g_bUpdateDbgWatches) DEBUG_ShowValueU32(480, 40, Value);	\
		}

#define DBGDUMPENTRY_SIZE	0x34
#define CUSTOMCMD_SELECTRANGE 12

#define DVD_STDCOMMANDS 9
#define DVD_SPCOMMANDS 10


char* g_szDvdCommands[DVD_STDCOMMANDS]  = {	
	"[DVD_CUSTOMCOMMAND]",
	"[DVD_STARTDRIVE 1]",	
	"[DVD_STARTDRIVE 2]",	
	"[DVD_TEST_RANDOM]",
	"[DVD_STOPDRIVE]",
	"[DVD_PATCHSTATUS_READID]",
	"[DVD_PATCHSTATUS_READALL]",
	"[DVD_SET_NOEDC]",	
	"[DVD_CLEAR_HIMEM]",	
};

char* g_szDvdSpecialCommands[DVD_SPCOMMANDS]  = {	

	"[CMD NONE]",	
	"[CMD INIT]",	
	"[CMD STOP MOTOR]",	
	"[CMD STOP  LASER]",
	"[CMD SWAP DISK]",
	"[CMD START MOTOR]", 
	"[CMD START LASER]",
	"[CMD SET BREAKPOINT]",
	"[CMD FAKE RESET]",
	"[CMD FLASH LED]" 
};


u32 GetSectorCheckSum(u8* pSector);
void ShowHexDump(u32* pData, u32 dwSize);
int FlashMain();

void FlushHexDumpBuffer(int nUsage, u32 dwSize = 2048)	/* 0 = normal dump mode, 1 = dbgout dump mode */
{


	if(nUsage == 0) {
		// stop dbgout mode on normal dump command
		g_bDbgOutMode = 0;
	}
	else {
		g_bDbgOutMode = 1;
	}

	GC_Memory_memset(g_pBuffer, 0xBB, dwSize);
	dcache_flush((void*)g_pBuffer, dwSize);
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

void CMD_ReadSector(bool bShowStatus = 1)
{
	if(bShowStatus) {
		DEBUG_PrintStatus( "** Reading sector... **");
	}
	
	FlushHexDumpBuffer(0);
	DVD_Read(g_pBuffer, 2048, g_nLBA * 2048);
	DVD_RequestError();
	GC_Sleep(40);
}

void CMD_ReadDriveMem(u32 dwAddress)
{
	DbgPrintS("** Reading drive mem ... **");
	DVD_ReadDriveMemBlock(dwAddress, (void*)g_pBuffer, 2048);
}

void CMD_ReadGCMem(u32 dwAddress)
{
	DEBUG_PrintStatusNoClear("** Reading Gc mem ... **");
	memcpy((void*)g_pBuffer, (void*) dwAddress, 2048);
}

void CMD_SetDebugMode()
{
	DEBUG_PrintStatus("** SETTING DVD DEBUG MODE **");
	DVD_SetDebugMode();
	DVD_WaitImmediate();
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

void CMD_InjectCustomDriveCode()
{
	u32  dwSize = 0;
	u32* dwAddr = 0;

	DEBUG_PrintStatus("** 4. INJECTING DRIVECODE **");
	CMD_SetDebugMode();

/*	DbgPrint("unloading XenoGC...");
	DVD_CustomDbgCommand(0x25000000, 0, 0, 0);

	DbgPrint("stopping drive...");
	DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
	GC_Sleep(3000);
*/

	// write QCode
	dwAddr = g_pQCode;
	dwSize = BINSIZE(QCode);				
	DVD_WriteDriveMemBlock(0xff40C600, dwAddr,	dwSize);
 	
	// patch interrupt chain vector to 40C600
	DVD_WriteDriveMemDword(0x804c, 0x00c64000);
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


void CMD_DumpDVDDriveMem()
{
	CMD_SetDebugMode();

	DbgPrintS("**  Dumping 64k at 0x%08X to 0x80800000", g_nLBA);
	DVD_ReadDriveMemBlock(g_nLBA, (void *) 0x80800000 , 0x10000);

	char* pStr1 = "'ere qooby qooby";
	char* pStr2 = "highmem here:   ";
	memcpy((void *) 0x807ffff0, pStr1, 16);
	memcpy((void *) 0x80800800, pStr2, 16);
	
/*	DEBUG_PrintStatus("**  Dumping drive memory 1 **");
	DVD_ReadDriveMemBlock(0x8000, (void *) 0x80800000 , 0x800);

	DEBUG_PrintStatus("** Dumping himem may take time... **");
	DVD_ReadDriveMemBlock(0x400000, (void *) 0x80800810, 0x80000);
*/
}

void CMD_ReadApploader()
{
	u32* pDest = (u32*) 0x81200000;

	// read apploader header
	DVD_Read(pDest, 0x20, 0);
	DVD_Read(pDest, 0x20, 0x2440);

	u32 dwApplEntry = pDest[4];
	u32 dwApplSize	= pDest[5];

	DbgPrintS("** Apploader EP: %08x Size: %x **", dwApplEntry, dwApplSize);
	GC_Sleep(2000);

	DVD_Read(pDest, dwApplSize, 0x2460);
}


void CMD_StartDrive(bool bFullCheck, bool bAcceptCopy)
{
	u32 dwCmd = 0xFE110100;

	if(bFullCheck) {
		dwCmd |= 0x00008000;
	}
	if(bAcceptCopy) {
		dwCmd |= 0x00004000;
	}

	DbgPrintS("** Starting drive %08X **", dwCmd);
	FlushHexDumpBuffer(0);

	DVD_CustomDbgCommand(dwCmd, 0, 0);
	GC_Sleep(100);
}


void CMD_ShowDebugBuffer()
{
	DEBUG_PrintStatus("** Reading Dbg buffer **");
	FlushHexDumpBuffer(0, 0x200);
	DVD_ReadDriveMemBlock(0x40ec00, (void*)g_pBuffer, 0x200);
}


void CMD_TestReadSettings()
{
	u16 wStartSetting	= 0x0000;
	u16 wEndSetting		= 0x0100;

	u32 dwLBAStart		= g_nLBA;
	u32 dwLBAEnd		= g_nLBA + 0x8000;

	struct sReadOptTest {
		int nReadErrors;
		bool bReadValid;
		u32 dwCheckTime;
		u32 dwCheckSum;
	};

	memset(g_pBuffer, 0, 0x800);

	sReadOptTest aTest[0x800];

	u32 dwTime;
	int nTest = 0;
	const int c_nLBAStep = 0x40;

	for(u16 wOpt = wStartSetting; wOpt <= wEndSetting; wOpt++) {

		if((wOpt & 0x01) || (wOpt & 0x06)) {
			g_pBuffer[wOpt] = 0xFF;
			continue;
		}
		
		GC_Video_ClearFrameBuffer((g_pFrameBuffer) , RGB2YCBR(44,10,17), 0x280 * 4);
		ShowHexDump((u32*) g_pBuffer, 0xF0);

		
/*		DVD_Read((void*) 0x80800000, 0x800 , 0);
		GC_Sleep(1000);
		DVD_Stop();
		GC_Sleep(3000);
		DVD_Reset();
		GC_Sleep(8000);

//		CMD_SetDebugMode();
//		GC_Sleep(1000);
//		DVD_SetReadOptions(wOpt);
//		GC_Sleep(1000);
		DVD_ReadId((void*) 0x80800000);
//		GC_Sleep(1000);
*/

		int nReadErrors, nReadErrorsPrev = 0;
		u32 dwChecksum, dwChecksumPrev = 0;


		// check twice
		for(int nCount = 0; nCount < 2; nCount++) {

			dwChecksum	= 0;
			nReadErrors = 0;

			u32 dwCheckTime	= GC_GetTime();
			u32 dwBytes = 0;
			
			float fSpeed = 0;

			for(g_nLBA = dwLBAStart; g_nLBA  < dwLBAEnd; g_nLBA  += c_nLBAStep) {

				dwTime		= GC_GetTime() - dwCheckTime;

				if(dwTime>>19 > 0x08) {

					fSpeed		= (float) (dwBytes) / (dwTime>>16);

					dwBytes = 0;
					dwCheckTime = GC_GetTime();
				}


				DbgPrintS("S:%x %05X-%05X Err:%d %d.%d", wOpt, g_nLBA, dwLBAEnd, nReadErrors, fSpeed/10000.f, (int)fSpeed % 10000);
				int nErr = DVD_Read((void*) 0x80800000, 0x800 * c_nLBAStep, g_nLBA * 2048);

				if(nErr) {
					if(((nCount == 0) && ((g_nLBA - dwLBAStart) > 2*c_nLBAStep) && (nReadErrors >= ((g_nLBA - dwLBAStart) / c_nLBAStep)))) {
						DVD_WaitBreak();

						nCount = 1;
						dwChecksum = 99;
						nReadErrors = 0xFE;
						break;
					}

					if(nReadErrors < 0xff) nReadErrors++;
				}
					
				dwBytes += 2048 * c_nLBAStep;

				// calculate sector checksum
				dwChecksum += GetSectorCheckSum((u8*) 0x80800000);

//				if(g_nLBA % 8 == 0) {
//					GC_Video_WaitForVBlank();
//				}

				if(GC_PAD_GetCurrentKeys() & PAD_B) {
					DbgPrintS("Cancel");
					GC_Sleep(1000);
					return;
				}

				if(GC_PAD_GetCurrentKeys() & PAD_R) {
					GC_Sleep(200);
					nCount = 1;
					break;
				}

				if(GC_PAD_GetCurrentKeys() & PAD_X) {
					dwLBAEnd += 0x1000;
					GC_Sleep(50);
				}

				if(GC_PAD_GetCurrentKeys() & PAD_Y) {
					dwLBAEnd -= 0x1000;
					GC_Sleep(50);
				}
			}

			// range done
			if(nCount == 0) {
				dwChecksumPrev = dwChecksum;
				nReadErrorsPrev = nReadErrors;
			}
		}

		nReadErrors = (nReadErrors > nReadErrorsPrev) ? nReadErrors : nReadErrors;

		GC_Video_ClearFrameBuffer((g_pFrameBuffer + 0x26000) , RGB2YCBR(54,20,27), 0x280 * 4);

		bool bChecksum = (dwChecksumPrev == dwChecksum);

		g_pBuffer[wOpt] = nReadErrors;

		if(!bChecksum) {
			if(nReadErrors == 0) {
				g_pBuffer[wOpt] = 0xFC;
			}
		}

		DbgPrintS(	"** S:%X Errs:%d T:%04d %s %04x", wOpt, nReadErrors, dwTime>>16, 
					bChecksum ? "OK": "WRONG", dwChecksum>>16);

//		GC_PAD_WaitForKey(0);
	}
}


/*
00000000 00000020		0		LBA0 		0x20		bytes
00000910 00000020		2440	LBA4+440	0x20		bytes
00000918 000015E0		2460	LBA4+460	0x15E0		bytes
00000918 000015E0
00000918 000015E0

*/

void fn_load_dol_fn_inmem(void *dol, bool bExec)
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

	if(bExec) {
		mtmsr((mfmsr() | 2) & ~0x8000);
		entrypoint();
	}
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
void LoadDol(void *pDol, u32 dwSize, bool bExec = true)
{
	u32* pDolImage = (u32*) pDol;

	u32 dwCodeSrc = (u32) fn_load_dol_fn_inmem;
	u32* pCodeDst = (u32 *) 0x81200000;

	DbgPrintS("Loading dol from %08x size: %x", pDol, dwSize);
//	GC_DBG_PrintHexDump(4, pDol, 0x40, 0);
//	GC_Sleep(2000);

//	fn_load_dol_fn_inmem(pDol, bExec);

	// copy dol loader code to safe buffer area
	memcpy((void *) pCodeDst, (void*) dwCodeSrc, 0x200);
	flush_code(pCodeDst, 0x8000);

//	if(bExec) {
		// run it
//		GC_Video_SetFrameBuffer((void*) 0x000000, VIDEO_FRAMEBUFFER_BOTH);
		void (*pLoadFn)(void*, bool) = (void (*)(void*, bool)) pCodeDst;
		pLoadFn(pDol, bExec);
//	}
}


void CMD_LoadDol()
{
//	LoadBinary();
//	LoadDol((void *) g_pDolImage, g_dwDolImage_Size);
//	fn_load_dol_fn_inmem((void *) g_pDolImage);
}



//asm(".globl GetMSR");
//asm("GetMSR:");
//asm("mfmsr 3");
//asm("blr");
//
//asm(".globl SetMSR");
//asm("SetMSR:");
//asm("mtmsr 3");
//asm("blr");

extern "C" { 
	long GetMSR();
	void SetMSR(long);
}

void nothing() 
{ 
	return; 
}

#define GC_INIT_BASE               	 (0x80000020)
#define GC_INIT_BASE_PTR           	 (u32*)GC_INIT_BASE

void InitSystem( unsigned long VidMode )
{
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

	// read and set:
	//	0x80000000		0x04	Gamecode
	//	0x80000004		0x02	Company
	//	0x80000006		0x01	Disk ID
	//	0x80000007		0x01	Version
	//	0x80000008		0x01	Streaming
	//	0x80000009		0x01	StreamBufSize
	//	0x8000000a		0x0F	padding zeros
	//	0x8000001c	 	0x04	DVD magic word
    DVD_Read((unsigned char *)0x80000000, 0x20, 0);
//	exception_close();

	GC_DefaultConfig[43] = VidMode;

	for(Counter=0; Counter<56; Counter++)
	{
			pDstAddr[Counter] = pSrcAddr[Counter];
	}

	dcache_flush((void*)0x80000000, 0x3100);

	if( VidMode == 1 )	{
//		Set_PAL_IPL_Default_Video();
	}
	else {
//		Set_NTSC_IPL_Default_Video();
	}
}

void load_apploader(void* pApploader)
{
	void (*app_init)(void (*report)(const char *fmt, ...));
	int  (*app_main)(void **dst, int *size, int *offset);
	void *(*app_final)();
	void (*app_entry)(void(**init)(void (*report)(const char *fmt, ...)), int (**main)(), void *(**final)());

    char *buffer = (char*)0xC0100000;


	GC_Video_ClearFrameBuffer(g_pFrameBuffer, COL_LIGHTBLUE);
	
	InitSystem(1);

	DVD_Read(buffer,0x400,0);
	DbgPrint("\nLoading...");

    SetMSR(GetMSR() & ~0x8000);
    SetMSR(GetMSR() | 0x2002);

	memset(buffer,0,0x20);

	DVD_Read(buffer,0x20, 0x2440);
	DVD_Read((void*)0x81200000,((*(unsigned long*)(buffer+0x14)) + 31) &~31,0x2460);

	app_entry = (void (*)(void(**)(void (*)(const char*,...)),int (**)(),void *(**)()))(*(unsigned long*)(buffer + 0x10));
    
	DbgPrint("*** calling app_entry() ***");
	app_entry(&app_init,( int (**)()) &app_main,&app_final);
	DbgPrint("[PTRS] Init:%08X Main:%08X Close:%08X", app_init, app_main, app_final);

	DbgPrint("*** calling app_init() ***");
 	app_init((void (*)(const char*,...))DbgPrintS);
	
	for (;;) {
		void *dst = 0;
		int len = 0,offset = 0;
		DbgPrint("*** calling app_main() ***");
		int res = app_main(&dst, &len, &offset);
		DbgPrint("..");

		if (!res) break;

		DVD_Read(dst, len, offset);
		flush_code(dst,len);
	}


	DbgPrint("*** calling app_final() ***");
	void (*entrypoint)() = (void (*)()) app_final();


	DbgPrint("*** calling dol entrypoint ***");
    entrypoint();
}



void CalcSRAMChecksums(void* pData) 
{
	int i; 
	u16 wChk1 = 0, wChk2 = 0;
	u16 *pSRAM = (u16*) pData;

    for (i = 0; i < 4; ++i) { 
        wChk1 += pSRAM[0x06 + i]; 
        wChk2 += (pSRAM[0x06 + i] ^ 0xFFFF); 
    } 

	pSRAM[0] = wChk1;
	pSRAM[1] = wChk2;
} 

void exi_sram_read(void *data)
{
	unsigned long val;
	exi_select(0, 1, 3);
	val = 0x20000100;
	exi_write(0, &val, 4);
	exi_read(0, data, 64);
	exi_deselect(0);
}

void exi_sram_write(void *data)
{
	unsigned long val;
	exi_select(0, 1, 3);
	val = 0xA0000100;
	exi_write(0, &val, 4);
	exi_write(0, data, 64);
	exi_deselect(0);
}


void CMD_ReadSRAM()
{
	FlushHexDumpBuffer(0);
	exi_sram_read(g_pBuffer);
//	memset(g_pBuffer, 0x00, 64);
//	CalcSRAMChecksums(g_pBuffer);
//	exi_sram_write(g_pBuffer);
}

/*	channel	device	freq	offset		Description
	-------------------------------------------------------------
	0 		0 		4 	  				Memory Card (Slot A)
	0 		1 		3 		0x00000000 	Mask ROM
	0 		1 		3 		0x20000000 	Real-Time Clock (RTC)
	0 		1 		3 		0x20000100 	SRAM
	0 		1 				0x20010000 	UART
	1 		0 		4 	  				Memory Card (Slot B)
	2 		0 	  						AD16 (trace step)
	0 		2 	  	  					Serial Port 1
	0 		2 		5 	  				Ethernet Adapter (SP1)
 */

#define MEMCARD_BLOCK 512
#define CHANNEL	1

void ReadMemcardBlock(u32 dwOffset, void *pData)
{
	u8 pCMD[4];
	u32 dwDummy = 0;

	exi_select(CHANNEL, 0, 4);

 	// read command and block offset 31-8
	pCMD[0] = 0x52;
    pCMD[1] = (dwOffset >> 17) & 0x3F;
    pCMD[2] = (dwOffset >> 9) & 0xFF;
    pCMD[3] = (dwOffset >> 7) & 3;
	exi_write(CHANNEL, pCMD, 4);
	
	// block offset 7-0
	pCMD[0] = (dwOffset & 0x7F);
	exi_write(CHANNEL, pCMD, 1);

	// dummy write
	exi_write(CHANNEL, &dwDummy, 4);

	// read data
	exi_read(CHANNEL, pData, MEMCARD_BLOCK);
	exi_deselect(CHANNEL);
}


void ReadMemcard(u32 dwOffset, int nSize, void *pData)
{
	u8* pDest = (u8*) pData;

	while(nSize > 0) {
		ReadMemcardBlock(dwOffset, pDest);
		dwOffset += MEMCARD_BLOCK;
		pDest+= MEMCARD_BLOCK;
		nSize -= MEMCARD_BLOCK;
	}
}




/* exi */
#define EXI_SR						ebase[0+5]
#define EXI_DMA_MEM					ebase[1+5]
#define EXI_DMA_LEN					ebase[2+5]
#define EXI_CR						ebase[3+5]
#define EXI_DATA					ebase[4+5]
#define EXI_WAIT_EOT				while((EXI_CR)&1);    


volatile u32* ebase = (u32*) 0xCC006800;


static void exi_select(void)
{
	EXI_SR |= 0x150;
}

static void exi_deselect(void)
{
	EXI_SR &= ~0x100;
}

static void exi_write_word(unsigned long word)
{
	EXI_DATA = word;
	EXI_CR = 0x35;
	EXI_WAIT_EOT;
}

static void exi_readx(unsigned char *dst, int len)
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


static void ReadMemcardBlockX(u32 dwOffset, void *pData)
{
	u8 pCMD[4];
	u32 dwDummy = 0;

//	EXI_SR &= 0x405;
	EXI_SR |= ((1<<0)<<7) | (4 << 4);

//	EXI_DATA = 0x52000000;
//	EXI_CR = ((1-1)<<4)|(1<<2)|1;

 	// read command and block offset 31-8
	pCMD[0] = 0x52;
//	exi_write_word(*((u32*) pCMD));
//	exi_write_word(dwOffset);
    
	pCMD[1] = (dwOffset >> 17) & 0x3F;
    pCMD[2] = (dwOffset >> 9) & 0xFF;
    pCMD[3] = (dwOffset >> 7) & 3;
	exi_write_word(*((u32*) pCMD));
	
	// block offset 7-0
//	EXI_DATA = (dwOffset & 0x7F);
//	EXI_CR = 0x35;
//	EXI_WAIT_EOT;

	// dummy write
	exi_write_word(0);

	// read data
//	exi_read(1, (unsigned char*)pData, MEMCARD_BLOCK);
	exi_readx((unsigned char*)pData, MEMCARD_BLOCK);
//	EXI_SR = 0;
	EXI_SR &= ~(((1<<0)<<7) | (4 << 4));
}
/*
void exi_syncX(int channel)
{
	volatile unsigned long *exi = (volatile unsigned long *)0xCC006800;
	while (exi[channel * 5 + 3] & 1);

	if (exi_last_addr)
	{	
		int i;
		unsigned long d;
		d = exi[channel * 5 + 4];
		for (i=0; i<exi_last_len; ++i)
			((unsigned char*)exi_last_addr)[i] = (d >> ((3-i)*8)) & 0xFF;
	}
}
*/
static void WriteMemcardBlockX(u32 dwOffset, void *pData)
{
	u8 pCMD[4];
	u32 dwDummy = 0;

//	EXI_SR &= 0x405;
	EXI_SR |= ((1<<0)<<7) | (4 << 4);

 	// read command and block offset 31-8
	pCMD[0] = 0xF2;
    pCMD[1] = (dwOffset >> 17) & 0x3F;
    pCMD[2] = (dwOffset >> 9) & 0xFF;
    pCMD[3] = (dwOffset >> 7) & 3;
	exi_write_word(*((u32*) pCMD));
	
	// block offset 7-0
	EXI_DATA = (dwOffset & 0x7F);
	EXI_CR = 0x05;
	EXI_WAIT_EOT;

	memset(g_pBuffer, 0x21, 0x80);
	exi_write(1, g_pBuffer, 0x80);

	for(int nBytes = 0; nBytes < 0x80 ; nBytes+=4) {
//		exi_write_word(0x01020304);
//		GC_Sleep(100);
//		exi_sync(1);
	}

	// dummy write
//	exi_write_word(*((u32*) pCMD));
	// read data
//	exi_readx((unsigned char*)pData, MEMCARD_BLOCK);
	EXI_SR = 0;
}




#define DOLPOS		(0xa000)			// 0x12000 0x32000 0x1BE000 0x15C000
#define DOLSIZE		(300 * 1024)

void CMD_ReadMemcard()
{
	FlushHexDumpBuffer(0, MEMCARD_BLOCK);
	
	DEBUG_PrintStatus("** Writing memcard **");
//	WriteMemcardBlockX(DOLPOS, (void*) g_pBuffer);
	
	DEBUG_PrintStatus("** Reading memcard **");
	ReadMemcardBlock(DOLPOS, (void*) g_pBuffer);
	GC_Sleep(100);
	return;

	u8* pCodeDst = (u8*) 0x81400000;//0x81700000;

//	ReadMemcard(DOLPOS, 0x100, (void*) pCodeDst);
//	ReadMemcard(DOLPOS+0x100, DOLSIZE, (void*) (pCodeDst+0x100));
	ReadMemcard(DOLPOS, DOLSIZE, (void*) (pCodeDst));
	flush_code(pCodeDst, DOLSIZE);


//	DEBUG_PrintStatus("** loading binary **");
//	ReadMemcard(DOLPOS+0x620, DOLSIZE, (void*) pCodeDst);
//	flush_code(pCodeDst, DOLSIZE);
//	void (*pLoadFn)() = (void (*)()) pCodeDst;
//	DEBUG_PrintStatus("** running binary **");
//	pLoadFn();
	
	DEBUG_PrintStatus("** loading dol **");
	LoadDol((void*) pCodeDst, DOLSIZE, true);
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
	else if(g_nKey & PAD_DOWN && nViewOffset < 0xf00) {
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


void CMD_BrowseCustomCommands1(int g_nKey)
{
	if(g_nKey & PAD_LEFT) {
		if(g_wDbgDumpOffset >= DBGDUMPENTRY_SIZE) {
			g_wDbgDumpOffset -= DBGDUMPENTRY_SIZE;
			GC_Sleep(40);
		}
	}
	else if(g_nKey & PAD_RIGHT) {
		if((g_wDbgDumpOffset + DBGDUMPENTRY_SIZE) < g_wDbgDumpBytes) {
			g_wDbgDumpOffset += DBGDUMPENTRY_SIZE;
			GC_Sleep(40);
		}
	}
}


void CMD_BrowseCustomCommands2(int g_nKey)
{
	if(g_nKey & PAD_LEFT) {
		dwCmdByteSelected = (dwCmdByteSelected - 1) % (CUSTOMCMD_SELECTRANGE+1);
		GC_Sleep(40);
	}
	else if(g_nKey & PAD_RIGHT) {
		dwCmdByteSelected = (dwCmdByteSelected + 1) % (CUSTOMCMD_SELECTRANGE+1);
		GC_Sleep(40);
	}
	else if(g_nKey & PAD_UP) {
		if(dwCmdByteSelected == 0 && g_aCustomDVDCommand[dwCmdByteSelected + 8] >= (DVD_STDCOMMANDS + DVD_SPCOMMANDS - 1)) {
			return;
		}
		g_aCustomDVDCommand[dwCmdByteSelected + 8]++;
		GC_Sleep(30);
	}
	else if(g_nKey & PAD_DOWN) {
		if(dwCmdByteSelected == 0 && g_aCustomDVDCommand[dwCmdByteSelected + 8] <= 0) {
			return;
		}
		g_aCustomDVDCommand[dwCmdByteSelected + 8]--;
		GC_Sleep(30);
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
	return;

	int nWatch = 0;
	DBG_SHOWWATCH(nWatch++, DBGDUMP,	0x40d800, DVD_ReadDriveMemDword(0x40d800));
	DBG_SHOWWATCH(nWatch++, SCRMBCMD  , 0x40ea00, DVD_ReadDriveMemDword(0x40ea00));
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

	DEBUG_Print(0,  40,		"Lba:");
	DEBUG_Print(0,  60,		"DI[0]");
	DEBUG_Print(0,  80,		"DI[4]");
	DEBUG_Print(0,  100,	"DI[8]");
	
	DEBUG_ShowValueU32(70, 40, g_nLBA);
	
	u32 dwCol = RGB2YCBR(255,255,0);

	for(nCount = 0; nCount < CUSTOMCMD_SELECTRANGE+1; nCount++) {
		// change color every 4 bytes
		if((nCount-1) % 4 == 0) {
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
	
	DEBUG_SetTextColor(GC_Video_RGBToYCbCr(70,44,150));

	int nSpecialCmd = g_aCustomDVDCommand[8];
	
	if(nSpecialCmd >= 0 && nSpecialCmd < DVD_STDCOMMANDS) {
		DEBUG_Print(250, 40, g_szDvdCommands[nSpecialCmd]);
	}
	else if(nSpecialCmd >= DVD_STDCOMMANDS && nSpecialCmd < (DVD_STDCOMMANDS + DVD_SPCOMMANDS)) {
		DEBUG_Print(250, 40, g_szDvdSpecialCommands[nSpecialCmd - DVD_STDCOMMANDS]);
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

//	DVD_RequestError();

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


struct sDvdCpuRegs 
{
	u16 d0, d1, d2, d3;
	u32 a0, a1, a2;

	sDvdCpuRegs() {
	};

	u16 GetD0() const { return BYTESWAP_WORD(d0);};
	u16 GetD1() const { return BYTESWAP_WORD(d1);};
	u16 GetD2() const { return BYTESWAP_WORD(d2);};
	u16 GetD3() const { return BYTESWAP_WORD(d3);};

	u32 GetA0() const { return BITSWAP_DWORD24BIT(a0);};
	u32 GetA1() const { return BITSWAP_DWORD24BIT(a1);};
	u32 GetA2() const { return BITSWAP_DWORD24BIT(a2);};
};


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
void ShowHexDump(u32* pData, u32 dwSize = 0xF0)
{
	if(g_bDbgOutMode == 0) {
		DEBUG_SetTextColor(GC_Video_RGBToYCbCr(255, 255, 255));
		GC_DBG_PrintHexDump(7, g_pBuffer, dwSize, nViewOffset);
	}
	else {
		DEBUG_SetTextColor(GC_Video_RGBToYCbCr(230, 230, 255));

		u8* pData = (u8*) pData;

		g_wDbgDumpBytes = BYTESWAP_WORD(*((u16*) pData));
		pData += 2;
		pData += g_wDbgDumpOffset;

		sDvdCpuRegs* pRegs = (sDvdCpuRegs*) (pData);

		int nLine = 120;
		DEBUG_ShowValueU16Named(200, nLine, g_wDbgDumpOffset / DBGDUMPENTRY_SIZE, "Entry:");
		DEBUG_ShowValueU16Named(0, nLine+=40, g_wDbgDumpBytes, "Size:");
		DEBUG_ShowValueU16Named(0, nLine+=20, pRegs->GetD0(), "d0:");
		DEBUG_ShowValueU16Named(0, nLine+=20, pRegs->GetD1(), "d1:");
		DEBUG_ShowValueU16Named(0, nLine+=20, pRegs->GetD2(), "d2:");
		DEBUG_ShowValueU16Named(0, nLine+=20, pRegs->GetD3(), "d3:");
		DEBUG_ShowValueU32Named(0, nLine+=20, pRegs->GetA0(), "a0:");
		DEBUG_ShowValueU32Named(0, nLine+=20, pRegs->GetA1(), "a1:");
		DEBUG_ShowValueU32Named(0, nLine+=20, pRegs->GetA2(), "a2:");

		pData += sizeof(sDvdCpuRegs);
		GC_DBG_PrintHexDump(19, pData, 3 * 20, nViewOffset);
	}
}



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
	GC_Video_Init(VI_PAL50_640x480);	// VI_PAL50_640x480 VI_PAL60_640x480 VI_NTSC_640x480
	
//	GC_Video_SetFrameBuffer((void*) 0xC0500000, VIDEO_FRAMEBUFFER_BOTH);
	GC_Video_SetFrameBuffer((void*)  0xC0F00000, VIDEO_FRAMEBUFFER_1);
	GC_Video_SetFrameBuffer((void*) (0xC0F00500), VIDEO_FRAMEBUFFER_2);

	GC_Video_ClearFrameBuffer (g_pFrameBuffer, COL_LIGHTBLUE);

	// init debug text color
	DEBUG_InitText(RGB2YCBR(255,0,0));
	
	GC_SRand(0x2121);
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

//	CMD_SetDebugMode();

	g_nLBA		= 0;
	g_pBuffer	=  g_aBuffer;

	// custom dbg command
	g_aCustomDVDCommand[8] = 0x00;
	g_aCustomDVDCommand[9] = 0x26;
}


void TestXY(u32* dwData1, u32* dwData2)
{
	u32 dwXY = 43;

	*dwData1 += 128;
	*dwData2 += 100;

	*dwData1 += dwXY;
	dwXY = *dwData2;
}

u32 GetSectorCheckSum(u8* pSector)
{
	u32 dwCheck = 0x19283845;
	dwCheck = 0;
	
	u32* pCheck = (u32*) pSector;
	
	for(int nDword = 0; nDword < (2048 / 4)-1; nDword++) {
//		dwCheck ^= BYTESWAP_DWORD(*pCheck++);
//		dwCheck <<= 1;
		dwCheck += *pCheck++;
	}

	return dwCheck;
}


bool IsValidSectorCheckSum(u8* pSector, u32 dwCheckSum)
{
	u32 *pPatch = (u32*) pSector;
	return (pPatch[(2048 / 4) - 1] == dwCheckSum);
}

void LoadBinary();

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
//	FlashMain();
	
	Startup();

//	*(unsigned short*)(0xCC002000) = 0x0006;
//	*(unsigned long*)(0xCC00200C) = 0x001501E6;
//	*(unsigned long*)(0xCC002010) = 0x001401E7;
//	CMD_LoadDol();

	// pre-load GCOS 
	LoadDol((void *) g_pDolImage, BINSIZE(DolImage), false);

	while(true) {
		
		g_nKey = GC_PAD_GetCurrentKeys();

		if((g_nKey & 0x0f) == 0) {
			g_dwKeydownTime = 0;
		}
		else {
			g_dwKeydownTime++;
		}

		
		const int c_nLBAStep = 0x40;

		int nErr = 0;
		u32 dwChecksum = 0;
		bool bChecksumError = false;

		if(g_bCheckDRE) {
			nErr = DVD_Read(g_pBuffer, 0x800 * c_nLBAStep, g_nLBA * 2048);
			// calculate sector checksum
//			dwChecksum = GetSectorCheckSum(g_pBuffer);
//			bChecksumError = (IsValidSectorCheckSum(g_pBuffer, dwChecksum) == false);
		}
		
		if(g_bShowLowMem) {
			FlushHexDumpBuffer(0, 0xE0);
			DVD_ReadDriveMemBlock(0x40EC40,	(void*)g_pBuffer, 0xE0);
		}

		if(g_bCheckDRE) {
			if(g_nLBA % 8 == 0) {
				GC_Video_WaitForVBlank();
			}

			GC_Video_ClearFrameBuffer(g_pFrameBuffer, RGB2YCBR(44,10,17), 0x280);
		}
		else {
			GC_Video_WaitForVBlank();

			GC_Video_ClearFrameBuffer(g_pFrameBuffer, COL_LIGHTBLUE);
			GC_Video_ClearFrameBuffer(g_pFrameBuffer, RGB2YCBR(22,10,17), 0x280);
		}

		ShowStats();
		
		if(!g_bCheckDRE) {
			ShowHexDump((u32*) g_pBuffer, 0xFE);
		}
			
		// check for read errors
		if(g_bCheckDRE) {
			if(nErr || bChecksumError) {
				u32 dwKey;
				u32 dwErr = DVD_RequestError();
				
				if(bChecksumError) {
					DbgPrintS("## Wrong sector checksum(0x%08x) @lba: %x ##", dwChecksum, g_nLBA);
				}
				else {
					DbgPrintS("** Read error %x @lba: %x **", dwErr, g_nLBA);
				}
					
				dwKey = GC_PAD_WaitForKey(PAD_A | PAD_B);

				GC_Sleep(160);

				if(dwKey & PAD_B) {
					g_bCheckDRE = 0;
					continue;
				}
			}
				
			if((g_nLBA += c_nLBAStep) >= 0x0AE0B0) {
				g_bCheckDRE = 0;
				DbgPrintS("** Disc check done **");
			}
		}

		//-------------------------------------------------
		// STD COMMANDS
		//-------------------------------------------------
		if(!(g_nKey & PAD_R) && !(g_nKey & PAD_L)) {

			if(g_bDbgOutMode == 0) {
				CMD_BrowseSectors(g_nKey);
			}
			else {
				CMD_BrowseCustomCommands1(g_nKey);
			}

			if(g_nKey & PAD_A) {
				if(g_bCheckDRE) {
					g_bCheckDRE = false;
				}
				CMD_ReadSector();
			}
			if(g_nKey & PAD_B) { 
				if(g_nLBA > 0) {
					FlushHexDumpBuffer(0);
					CMD_ReadDriveMem(g_nLBA);
				}
			}
		
/*			if(g_nKey & PAD_X) {
				DEBUG_PrintStatus("** DVD request status **");
				DVD_RequestError();
				DVD_WaitImmediate();
				GC_Sleep(160);
			}
*/
			if(g_nKey & PAD_START) {
				//DVD_Reset();
				LoadDol((void *) g_pDolImage, BINSIZE(DolImage));
			}

			if(g_nKey & PAD_Z) {

				GC_Sleep(400);
				//LoadBinary();

				memset((void *) 0x81700000, 0, 4300);
				memcpy((void *) 0x81700000, (void *) g_pBinaryImage, BINSIZE(BinaryImage));
				flush_code((void *) 0x81700000, 0x1000);

				u32 dwCheck11 = GetSectorCheckSum((u8*) 0x81700000);
				u32 dwCheck22 = GetSectorCheckSum((u8*) 0x81700000+0x800);
				GC_Video_ClearFrameBuffer(g_pFrameBuffer, COL_LIGHTBLUE);
				DbgPrintS("Checksums org: %08X %08X %d", dwCheck11, dwCheck22, BINSIZE(BinaryImage));
				GC_PAD_WaitForAnyKey();

				DEBUG_PrintStatus("** requesting shell binary **");
				memset((void *) 0x81700000, 0, 4300);
				SUB_BootXenoShell();

				u32 dwCheck1 = GetSectorCheckSum((u8*) 0x81700000);
				u32 dwCheck2 = GetSectorCheckSum((u8*) 0x81700000+0x800);
				GC_Video_ClearFrameBuffer(g_pFrameBuffer, COL_LIGHTBLUE);
				DbgPrintS("Checksums Xeno: %08X %08X", dwCheck1, dwCheck2);

				char* pStr1 = "'ere qooby qooby";
				char* pStr2 = "highmem here:   ";
				memcpy((void *) 0x807ffff0, pStr1, 16);
				memcpy((void *) 0x80800800, pStr2, 16);
				memcpy((void *) 0x80800800, (void *)0x81700000, 4300);
				

				if(GC_PAD_WaitForAnyKey() & PAD_A) {
					flush_code((void *) 0x81700000, 0x1000);

					asm("lis 0,		0x8170");
					asm("ori 0, 0,	0x0000");
					asm("mtlr 0");
					asm("blr");
				}

				//SUB_BootXenoShell();
				// LoadBinary();
				// LoadDol((void *) g_pDolImage2, g_dwDolImage2_Size, true);
			}
		}
		//-------------------------------------------------
		// EXTENDED COMMANDS 1 (holding l)
		//-------------------------------------------------
		else if((g_nKey & PAD_L)) {
			if(g_nKey & PAD_Z) {
				DEBUG_PrintStatus("** Resetting drive **");
				DVD_Reset();
				GC_Sleep(160);
			}
			if(g_nKey & PAD_R) {

				DEBUG_PrintStatus("** Reading Id **");
				FlushHexDumpBuffer(0);
				DVD_ReadId((void*) g_pBuffer);
				GC_Sleep(160);
			}

			if(g_nKey & PAD_X) {
				load_apploader(0);
			}

			if(g_nKey & PAD_Y) {
				CMD_TestReadSettings();
//				g_bShowLowMem = !g_bShowLowMem;
//				GC_Sleep(160);
			}
			if(g_nKey & PAD_B) {
				DEBUG_PrintStatus("** Reading DVD Inquiry **");
				FlushHexDumpBuffer(0);
				DVD_Inquiry((DVD_DRIVEINFO*) g_pBuffer);
			}
			
			// check disk for read errors
			if(g_nKey & PAD_A) {
				g_bCheckDRE ^= 1;
				GC_Sleep(1000);
			}

			 // TEST: load gcos.dol
			if(g_nKey & PAD_START) {
				CMD_ReadMemcard();
//				CMD_ReadSRAM();
				//CMD_DumpDVDDriveMem();
				// g_bUpdateDbgWatches = !g_bUpdateDbgWatches;
				// GC_Sleep(160);
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
				CMD_InjectCustomDriveCode();
				//g_aCustomDVDCommand[8] = DVD_STDCOMMANDS + 6;
				GC_Sleep(160);
			}

			if(g_nKey & PAD_B) {
				FlushHexDumpBuffer(0);
				CMD_ReadDriveMem(0x8000);
			}

			// toggle lba presets
			if(g_nKey & PAD_START) {
				g_nLbaPreset =  (g_nLbaPreset + 1) % (sizeof(g_dwLbaPresets) / 4);
				g_nLBA = g_dwLbaPresets[g_nLbaPreset];
				GC_Sleep(100);
			}

			if(g_nKey & PAD_Z) {
				memset(g_pBuffer, 0x00, 6*1024);
				DVD_WriteDriveMemBlock(0xff40D000, g_pBuffer,	6*1024);
 
				// return to gcos
				/*	asm("lis 0,		0x8140");
					asm("ori 0, 0,	0x0000");
					asm("mtlr 0");
					asm("blr");
				*/
			}

			if(g_nKey & PAD_A) {
				int nSpecialCmd = g_aCustomDVDCommand[8];

				if(nSpecialCmd >= DVD_STDCOMMANDS && nSpecialCmd <= (DVD_STDCOMMANDS + DVD_SPCOMMANDS)) {
					u32 dwCmd = ((nSpecialCmd - DVD_STDCOMMANDS) << 24);
					CMD_ExecuteDriveCodeCommand(dwCmd);
				}
	
				//---------------------------------------------------
				// [DVD_CUSTOMCOMMAND]
				//---------------------------------------------------
				else if(nSpecialCmd == 0) {
					u32 dwCmd1, dwCmd2, dwCmd3;

					memcpy(&dwCmd1, &g_aCustomDVDCommand[9], 4);
					memcpy(&dwCmd2, &g_aCustomDVDCommand[13], 4);
					memcpy(&dwCmd3, &g_aCustomDVDCommand[17], 4);


					FlushHexDumpBuffer(0);
					// memcpy(g_pBuffer, &dwCmd3, 4);
					
					DbgPrintS("*DVDCstCmd: %08X %08X %08X*", dwCmd1, dwCmd2, dwCmd3);
					DVD_CustomDbgCommand(dwCmd1, dwCmd2, 0x20, (u32*) g_pBuffer);
				}

				//---------------------------------------------------
				// [DVD_TEST_STARTDRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 1) {
					CMD_StartDrive(0, 1);
				}
				//---------------------------------------------------
				// [DVD_TEST_STARTDRIVE2]
				//---------------------------------------------------
				else if(nSpecialCmd == 2) {
					CMD_StartDrive(1, 0);
				}
				//---------------------------------------------------
				// [DVD_TEST_RANDOM]
				//---------------------------------------------------
				else if(nSpecialCmd == 3) {
					u32 dwFunc = 0x66756e63;
					memcpy(&g_aCustomDVDCommand[17], &dwFunc, 4);
				}
				//---------------------------------------------------
				// [DVD_STOPDRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 4) {
					DVD_CustomDbgCommand(0xFE114000, 0, 0);
				}
				//---------------------------------------------------
				// [DVD_PATCHSTATUS_READID]
				//---------------------------------------------------
				else if(nSpecialCmd == 5) {
					//---------------------------------------------
					// set status variables to "allow read id"
					//---------------------------------------------
					DbgPrintS("** Setting Status Allow ReadId  **");
					DVD_WriteDriveMemDword(0x8188, 0x03000000);
					DVD_WriteDriveMemDword(0x81a4, 0x06000300);
				}
				//---------------------------------------------------
				// [DVD_PATCHSTATUS_READALL]
				//---------------------------------------------------
				else if(nSpecialCmd == 6) {
					//---------------------------------------------
					// set status variables to "allow read id"
					//---------------------------------------------
					DbgPrintS("** Setting Status Read ALL **");
					DVD_WriteDriveMemDword(0x8188, 0x00000000);
					DVD_WriteDriveMemDword(0x81a4, 0x01000300);
					}
				//---------------------------------------------------
				// [DVD_TEST_DISABLE_SCRAMBLING]
				//---------------------------------------------------
				else if(nSpecialCmd == 7) {
					DVD_WriteDriveMemDword(0x8080, 0x6bda);
					DVD_CustomDbgCommand(0xFE100000, 0x773e165d, 0);
				}
				//---------------------------------------------------
				// [DVD_CLEAR_HIMEM]
				//---------------------------------------------------
				else if(nSpecialCmd == 8) {
					GC_Memory_memset((void*)g_pBuffer, 0xCC, 0xA00);
					DVD_WriteDriveMemBlock(0xff40d000, g_pBuffer, 0xA00);
				}

				GC_Sleep(160);
			}	

			CMD_BrowseCustomCommands2(g_nKey);
		}
	}	
    return 0;
}


void LoadBinary()
{
	u8* pCodeDst = (u8*) 0x81700000;

	// copy dol loader code to safe buffer area
	memcpy((void *) pCodeDst, (void *) g_pBinaryImage, BINSIZE(BinaryImage));
	flush_code(pCodeDst, 0x8000);
	void (*pLoadFn)() = (void (*)()) pCodeDst;

	DbgPrintS("jumping to: %08x", pCodeDst);
	pLoadFn();
}

