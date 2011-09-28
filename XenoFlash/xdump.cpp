	#---------------------------------------------#
	#- replace the current irq handler with ours -#
	#---------------------------------------------#
#	mov	v_irqh, a0
#	mov	a0, (FwIrqVec)
#	sub d0, d0
#	mov d0, (0x40d100)


/*	
	mov FwCDB0, a0
	movbu (a0), d0							
	movb d0, (wCBDBackup)	
					
	mov 0x2B, d0						# and replace with fake value
	movb d0, (a0)						# to detect new commands

	#-----------------------------#
 	# call the original handler   #
	#-----------------------------#
	jsrabsOrg FwIntHandler
	
	#==================================================================#
 	# Check for new command											   #
	#==================================================================#
	mov FwCDB0, a0
	movbu (a0), d0
	cmp 0x2B, d0
	beq NoCDB

 OnCDB:
 	jmp SUB_OnNewCDBRecieved

 NoCDB:
 	movbu (wCBDBackup), d0					# restore original value
	movb d0, (a0)
	rts
*/


/***********************************************************************/
 SUB_OnNewCDBRecieved:
/***********************************************************************/
/*
	#==================================================================#
 	# Check for our custom commands									   #
	#==================================================================#

	#----------------------------------------------------------
	# 	Init flash
	#	CDB0 26 00 00 00
	#----------------------------------------------------------
	 cmp CMD_INITFLASH, d0				
	 bne 1f
	 jsr FwCLI
	 jsr patchNiceCMD
	 jsr SUB_FlashEnable
	 jsr FwSTI
	 rts

	#----------------------------------------------------------
	# 	Erase flash
	#	CDB0 27 00 00 00 
	#----------------------------------------------------------
1:	 cmp CMD_ERASEFLASH, d0				
	 bne 1f
	 jsr FwCLI
	 jsr patchNiceCMD
	 jsr SUB_FlashErase
	 jsr FwSTI
	 rts
	 
	#----------------------------------------------------------
	# 	Write flash
	#	CDB0 26 00 ADDR ADDR
	#----------------------------------------------------------
1:	 cmp CMD_WRITEFLASH, d0		
 	 bne 1f
	 jsr FwCLI						# disable interrupts
	 jsr patchNiceCMD
	 jsr SUB_WriteFlashBlock		# write the block
  	 jsr FwSTI						# enable interrupts
	 rts
	 
	#----------------------------------------------------------
	# 	If a custom command was executed, patch the cdb to
	#	some real command to return valid status
	#----------------------------------------------------------
 patchNiceCMD:

	mov FwCDB0, a0
	mov 0xE0, d0					# bad boi!
	movb d0, (a0)

#	mov d0, (a0)
#	sub d0, d0
#	mov d0, (2, a0) 
 1:	rts
*/

/*	 jsr SUB_UnloadQcode
	 jsr SUB_ClearDataSegs
	 bra SUB_DVDShortReset
	 jmpabsOrg 0x80000
*/


/*!	\fn			void UnloadXenoGC_NoReset()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void UnloadXenoGC_NoReset()
{
	u8 pUnloaderCode[] = {  0x80, 0x00,                 // 40D090   8000            mov $00,D0				# disable breakpoints  
							0xC4, 0xDA, 0xFC,           // 40D092   C4DAFC          movb    D0,($FCDA)      # 
							0xF4,0x74,0x74,0x0a,0x08,   // 40D095   F47474A708      mov 0x80a74,a0          # restore original handler
							0xF7,0x20,0x4C,0x80,        //          F7204C80        mov a0,($804c)          # 
							0xFE, 0x00					//			FE				rts
	};

	DbgPrint("\ndisabling XenoGC no-spindown...");
	DVD_SetDebugMode();
	DVD_WriteDriveMemBlock(0x00008600, pUnloaderCode, sizeof(pUnloaderCode));
 	DVD_WriteDriveMemDword(0x40D100, 0x44444444);
	DVD_CustomDbgCommand(0xFE120000, 0x8600, 0x66756e63);
	CheckDriveState();
}


/*!	\fn			void UnloadXenoGC_NextTime()
 *	
 *				
 *	
 *	\param		none
 *	
 *	\return		none
 *	
 *	\note		
 */
void UnloadXenoGC_NextTime()
{
	DbgPrint("\ndisabling XenoGC...");
	DVD_SetDebugMode();
	DVD_WriteDriveMemDword(0x40D100, 0x44444444);
//	DVD_Reset();
//	GC_Sleep(6000);
	GC_Sleep(1000);
	DbgPrint("done!");
}


void TestWritemem()
{
	DVD_SetDebugMode();
	DVD_WaitImmediate();
	
	u32 dwCheck = CalcChecksum(g_pXenoROM, FLASHSIZE);
	DbgPrint("\nChecksum original: %x", dwCheck);

	DbgPrint("\nReading memory...");
	DVD_ReadDriveMemBlock(0xFF40D800, pBuffer, FLASHSIZE);
//	dcache_inv(pBuffer, FLASHSIZE);
	dwCheck = CalcChecksum(pBuffer, FLASHSIZE);
	DbgPrint("\nChecksum memory before: %x", dwCheck);

	DbgPrint("\nWriting memory...");
	DVD_WriteDriveMemBlock(0xFF40D800, g_pXenoROM, FLASHSIZE);
//	GC_Sleep(8000);
	DVD_WaitImmediate();
	DbgPrint("done");

	DbgPrint("\nReading back...");
	DVD_ReadDriveMemBlock(0xFF40D800, pBuffer, FLASHSIZE);
//	dcache_inv(pBuffer, FLASHSIZE);

	dwCheck = CalcChecksum(pBuffer, FLASHSIZE);
	DbgPrint("\nChecksum read back: %x", dwCheck);
}


#==============================================================================================
# Interrupt handler 
#==============================================================================================
main_irq_handler:

	mov FwCDB0, a0							# backup CDB byte and replace with fake value
	mov (a0), d0							# to detect new commands
	mov d0, (wCBDBackup)					
	mov 0x2BAD, d0
	mov d0, (a0)

	#-----------------------------#
 	# call the original handler   #
	#-----------------------------#
	jsrabsOrg FwIntHandler

	#==================================================================#
 	# Check for new command											   #
	#==================================================================#
	mov FwCDB0, a0
	mov (a0), d0
#	cmp 0x2BAD, d0
#	bne SUB_OnNewCDBRecieved

	mov (wCBDBackup), d0					# restore original value
	mov d0, (a0)
 	rts


	








bool FlashTest()
{
/*	DbgPrint("\nNow writing flash...");
	memset(pBuffer, 0x44, CHUNKSIZE);
	DVD_WriteDriveMemBlock(0x40C000, pBuffer, CHUNKSIZE);
	GC_Sleep(1000);
	DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x0080);
	CheckDriveState(false);	
	DbgPrint("done");

	memset(pBuffer, 0xCC, CHUNKSIZE);
	pBuffer[0] = 0x00;
	pBuffer[1] = 0x11;
	pBuffer[2] = 0x22;
	pBuffer[3] = 0x33;
	DVD_WriteDriveMemBlock(0x40C000, pBuffer, CHUNKSIZE);
	GC_Sleep(1000);
	DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x0510);
	CheckDriveState(false);
	DbgPrint("done");
*/
	
	DbgPrint("\nFLASHING........[        ]");
	g_nConsoleX -= 16*9;

	int nX = 1;

//	for(u32 dwAddress = 0x00; dwAddress < FLASHSIZE; dwAddress += CHUNKSIZE) {

//		dcache_inv(g_pXenoROM, 0x2000);

//		memset(g_pXenoROM, 0x21, 0x1000);

		SetFlashAddress(0);
		DVD_WriteDriveMemBlock(0xFF40D800, g_pXenoROM, 0x400);
		DVD_CustomDbgCommand(CMD_WRITEFLASH);
		CheckDriveState(false);
		DbgPrint("*");

		SetFlashAddress(0x400);
		DVD_WriteDriveMemBlock(0xFF40D800, g_pXenoROM+0x100, 0x400);
		DVD_CustomDbgCommand(CMD_WRITEFLASH);
		CheckDriveState(false);
		DbgPrint("*");

		SetFlashAddress(0x800);
		DVD_WriteDriveMemBlock(0xFF40D800, g_pXenoROM+0x200, 0x400);
		DVD_CustomDbgCommand(CMD_WRITEFLASH);
		CheckDriveState(false);
		DbgPrint("*");
		
		SetFlashAddress(0xC00);
		DVD_WriteDriveMemBlock(0xFF40D800, g_pXenoROM+0x300, 0x400);
		DVD_CustomDbgCommand(CMD_WRITEFLASH);
		CheckDriveState(false);
		DbgPrint("*");
		
		//	DVD_WaitImmediate();

/*		DVD_WriteDriveMemBlock(0xFF40D800, g_pXenoROM + (0x800/4), 0x800);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x000);
		CheckDriveState(false);
		DbgPrint("*");
*/
//		DbgPrint("Mem write done");

/*
//		memset(pBuffer2, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40DA00, g_pXenoROM + (0x1000 / 4), CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x800);	
		GC_Sleep(30000);
		CheckDriveState(false);
		DbgPrint("*");



//		memset(pBuffer3, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40C000, pBuffer3, CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x400);
		GC_Sleep(8000);
//		CheckDriveState(false);
		DbgPrint("*");



//		memset(pBuffer4, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40C000, pBuffer4, CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x600);	
		GC_Sleep(8000);
//		CheckDriveState(false);
		DbgPrint("*");

		memset(pBuffer5, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40C000, pBuffer5, CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0x800);	
		CheckDriveState(false);
		DbgPrint("*");
		memset(pBuffer6, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40C000, pBuffer6, CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0xA00);	
		CheckDriveState(false);
		DbgPrint("*");
		memset(pBuffer7, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40C000, pBuffer7, CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0xC00);	
		CheckDriveState(false);
		DbgPrint("*");
		memset(pBuffer8, nX++, 0x10);
		DVD_WriteDriveMemBlock(0x40C000, pBuffer8, CHUNKSIZE);
		DVD_CustomDbgCommand(CMD_WRITEFLASH | 0xE00);	
		CheckDriveState(false);
		DbgPrint("*");
		*/
		
//		pROM += CHUNKSIZE;
//	}

	DbgPrint("\n\nDONE!");
	return true;

	
/*	u32 dwAddress = 0xC00;

	memset(pROM, 4, 0x10);
	DVD_WriteDriveMemBlock(0x40C000, pROM, CHUNKSIZE);
	CheckDriveState(false);
	DVD_WriteDriveMemBlock(0x40C000, pROM, CHUNKSIZE);
	CheckDriveState(false);
	DVD_WriteDriveMemBlock(0x40C000, pROM, CHUNKSIZE);

	DVD_CustomDbgCommand(CMD_WRITEFLASH | (dwAddress / 2));
	CheckDriveState(false);
	DbgPrint("\n\nDONE!");
*/
	return true;
}


/*	memcpy(pBuffer1, g_pXenoROM, CHUNKSIZE);
	memcpy(pBuffer2, g_pXenoROM+0x100, CHUNKSIZE);
	memcpy(pBuffer3, g_pXenoROM+0x200, CHUNKSIZE);
	memcpy(pBuffer4, g_pXenoROM+0x300, CHUNKSIZE);
	memcpy(pBuffer5, g_pXenoROM+0x400, CHUNKSIZE);
	memcpy(pBuffer6, g_pXenoROM+0x500, CHUNKSIZE);
	memcpy(pBuffer7, g_pXenoROM+0x600, CHUNKSIZE);
	memcpy(pBuffer8, g_pXenoROM+0x700, CHUNKSIZE);
*/	


/*

void UnloadXenoGC_Permanent()
{
	DbgPrint("\n\nunloading XenoGC...");
	DVD_CustomDbgCommand(0x25000000, 0, 0, 0);
	DbgPrint("\nstopping drive...");
	DVD_CustomDbgCommand(0xE3000000, 0, 0, 0);
	GC_Sleep(3000);
	CheckDriveState();
}

*/



	