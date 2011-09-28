
/*		unsigned long PROGRAM_ENABLE = 0xAC53FFFFL;
		unsigned long CHIP_ERASE = 0xAC80FFFFL;
		unsigned long READ_DEVICE = 0x30FF00FFL;
		unsigned long READ_PROG_LO = 0x200000FFL;
		unsigned long READ_PROG_HI = 0x280000FFL;
		unsigned long WRITE_PROG_LO = 0x40000000L;
		unsigned long WRITE_PROG_HI = 0x48000000L;
		unsigned long LOAD_PAGE_LO = 0x40000000L;
		unsigned long LOAD_PAGE_HI = 0x48000000L;
		unsigned long WRITE_PAGE = 0x4C0000FFL;
		unsigned long READ_EEPROM = 0xA00000FFL;
		unsigned long WRITE_EEPROM = 0xC0000000L;	*/	
		
	jsr DBGInit

	mov 200, d0
	jsr SUB_Sleep
	
	mov 0xAC, d0
	jsr Sub_SendByteSerial
	mov 0x80, d0
	jsr Sub_SendByteSerial
	mov 0xFF, d0
	jsr Sub_SendByteSerial
	mov 0xFF, d0
	jsr Sub_SendByteSerial	
	
	mov 0xAC, d0
	jsr Sub_SendByteSerial
	mov 0x53, d0
	jsr Sub_SendByteSerial
	mov 0xFF, d0
	jsr Sub_SendByteSerial
	mov 0xFF, d0
	jsr Sub_SendByteSerial	

	mov 200, d0
	jsr SUB_Sleep

	mov 0xAC, d0
	jsr Sub_SendByteSerial
	mov 0x80, d0
	jsr Sub_SendByteSerial
	mov 0xFF, d0
	jsr Sub_SendByteSerial
	mov 0xFF, d0
	jsr Sub_SendByteSerial	

	
	mov	0x40D000, a0						# clear the bss section
	movb (0x8089), d0
	mov 0x100, d1
	jsrabs FwMemset


#		mov (0x8068), d0
#		mov d0, (0x40D0D0)
#		mov d0, (0x40D0D2)
#		mov d0, (0x40D0D4)

/*	.long 0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018	# NTSC Normal
	.long 0x00020019, 0x410C410C, 0x40ED40ED, 0x80100000
	.long 0x00000000, 0x80100000, 0x00000000, 0x00000000
	.long 0x110701AE
*/	
/*	.long 0x11F50101, 0x4B6A01B0, 0x02F85640, 0x00010023	# PAL Normal
	.long 0x00000024, 0x4D2B4D6D, 0x4D8A4D4C, 0x00000000
	.long 0x00000000, 0x00000000, 0x00000000, 0x013C0144
	.long 0x113901B1
*/


/*	#=======================================================================================================
1:	andi. r5, r4, PAD_Z					# Z-Key pressed ?		-> bypass drivecode
	beq 1f								# 
	lis r0, 0x2600						
	bl SUB_SendCustomCommand
#	lis r5, 0x8130
#	mtlr r5
	#=======================================================================================================
*/
	.if DBGMODE == 1
 1:		cmp CMD_TOGGLE_ECMA, d0				# set orgdisc flag
		bne 1f
		movbu (bConfig), d0
		xor 0x01, d0
		movb d0, (bConfig)
		mov (dwUsrSectorOffset), d0
		xor 0x0a, d0
		mov d0, (dwUsrSectorOffset)
		bra patchNiceCMD			
 1:		cmp CMD_SETREADSTATUS, d0			# set status for readid
		bne 1f
		jsr SUB_SetReadIdStatus
	.endif
	#================================================================
#===================================	
 BP1Handler:
#===================================
	bitclr REG_ADBCTRL 1	
	mov (REG_ADB1), a1
	bra 1f

#===================================
 BP0Handler:
#===================================
	bitclr REG_ADBCTRL 0	
	mov (REG_ADB0), a1

	#- ack the unknown opcode interrupt -#
1:	bitclr REG_UNICR, 0

	

/*	#----------------------------------------------------------------------
	# get the address of the according code replacement into a1
	#----------------------------------------------------------------------
	cmp FwBP0Addr_ReadECMA, a1
	bne 1f
	mov BP0_ReadECMA, a1
	bra PatchStack
1:	cmp FwBP1Addr_AuthPlusR, a1
	bne 1f
	mov BP1_AuthPlusR, a1
	bra PatchStack
1:	cmp FwBP1Addr_AuthDMI, a1
	bne 1f
	mov BP1_AuthDMI, a1
	bra PatchStack

	#- Panic! we dont recognise the break address. just rts then
	rts
*/

#	mov 0x2222, d0								# set credits request tag
#	mov d0, (0x40D100)

#	mov (FwCDB0), a0
#	cmp 0x0101FE, a0
#	bne 1f

/*	#=================================================================================================================
	# TEST: patch some IPL stings ;)
	#=================================================================================================================
		GetRelocAddr r3, str1
		lis r4, 0x8139
		ori r4, r4, 0x2c10
		li r5, 6
		bl SUB_Memcpy	
	str1:
		.ascii "Cool beans or whut???"
		.byte 0
		.byte 0
		.byte 0
*/
	#=================================================================================================================



	#=================================================================================================================
/*	lis r3, 0x8130
	lis r4, 0x8081
	lis r5, 0x0008
	bl SUB_Memcpy
	li32 r0, 0x27657265					# Set GCOs Memdump tag
	li32 r4, 0x807FFFF0			
	stw	 r0, 0(r4)
*/
	#=================================================================================================================



#==============================================================================#
 OnApploaderClose:								
#==============================================================================#	

	mflr r0								# push lr
	stwu r0, -0x04(sp)

#----------------------------------------------------------------------------------------
	lis r3, ApplCloseAddr@h				# get saved original ApplClose address
	lwz r0, ApplCloseAddr@L(r3)
	mtlr r0	
	blrl								# call AppClose
#----------------------------------------------------------------------------------------

#	subi sp, sp, 0x78					# push all gp registers
#	stmw r2, 0(sp)
	stwu r3, -0x04(sp)
	
	lis r3, 0xCC00						# read PAD0
	lhz r4, 0x6404(r3)
	lwz r5, 0x6408(r3)

	andi. r5, r4, PAD_L					# L-Key pressed ?	-> disable audiofix
	beq 1f								# 
	lis r0, 0x2100						
	bl SUB_SendCustomCommand
	
1:	andi. r5, r4, PAD_R					# R-Key pressed ?	-> disable dreFix
	beq 1f								# 
	lis r0, 0x2200						
	bl SUB_SendCustomCommand

1:	andi. r5, r4, PAD_Y					# Y-Key pressed ?	-> force NTSC
	beq 1f								# 
	li r0, 0x01
	GetRelocAddr r5, dwRegionCode		# set game region
	stw r0, 0(r5)

1:	andi. r5, r4, PAD_X					# X-Key pressed ?	-> force PAL
	beq 1f								# 
	li r0, 0x02							
	GetRelocAddr r5, dwRegionCode		
	stw r0, 0(r5)

	.if CREDITS == 1
1:		andi. r5, r4, PAD_START			# L-Key pressed ?	-> disable audiofix
		beq noCredits					# 
		
		lis r0, 0xE300
		bl SUB_SendCustomCommand

		lis r0, 0x2500
		bl SUB_SendCustomCommand
		
		li32 r3, 3000
		bl SUB_Delay
	
 		bl SUB_DVDReadMemBlock

		li r0, 0x02						# set pal region
		GetRelocAddr r5, dwRegionCode	
		stw r0, 0(r5)
		bl SUB_PatchVideoMode
#		li32 r4, 0xcc00204c
	 	li32 r0, 0x28500100				# disable 3dmode
		stw  r0, 0x00(r4)

		lis r3, 0x8001
		stw r3, 0x00(sp)

#		lwz r0, 0x78(sp)
#		mtlr 

#		stw r3, 0x78(sp)

#		lwzu r0, 0x78(sp)				# pop lr
#		mtlr r0
#		blr		

	 noCredits:
	.endif

1:	bl SUB_PatchVideoMode				# switch videoMode

#	lis r5, ApplCloseAddr@h				# get saved original ApplClose address
#	lwz r3, ApplCloseAddr@L(r5)
#	mtlr r3	
	
#	lmw r2, 0(sp)						# pop all gp registers & lr					
#	addi sp, sp, 0x78

#	blrl								# call AppClose

	lwz r3, 0(sp)						# pop lr
	lwzu r0, 4(sp)						# pop lr
	addi sp, sp, 0x08
	mtlr r0
	blr									# return to IPL


/*
SET(0x842bc, FixState) 
SET(0x878f8, RestartMotor) 
SET(0x8ae80, DIO_Continue) 
 
SET(0x8d4d9, Init) 
 
#define OFFSET_BREAK_DIO      0xae60 
#define OFFSET_0C             0x8196 
 
#define OFFSET_DEBUG          0x818e 
 
#define DVDP_FIX_OFFSET 0x9db6 
 
#define OFFSET_CB0            0x80b0 
 
#define OFFSET_COUNT_TICK     0x8080 
#define OFFSET_COUNT_YYY      0x8198 

/*		lis		r4 , 0xcc00					# DVDReset
		ori		r4 , r4 , 0x3024
		lwz		r5 , 0(r4)
		rlwinm	r5 , r5 , 0 , 30 , 28
		ori		r5 , r5 , 1
		stw		r5 , 0(r4)
		lwz		r5 , 0(r4)
		ori		r5 , r5 , 4
		ori		r5 , r5 , 1
		stw		r5 , 0(r4)
*/

	tbzAbs ADBCTRL, 0, 1f
	bra BP0Handler
1:	tbzAbs ADBCTRL, 1, 1f
	bra BP1Handler
1:	

	#=======================================================
	.if DBGMODE == 1
		jsr DBGUpdate
	.endif
	#=======================================================

	sub d0, d0
#	movb d0, (0xFFFA)					# P7MODE
#	movb d0, (0xFFFB)					# P7MODE
	movb d0, (0xFFFC)					# P7MODE
	movb d0, (0xFFFD)					# P7MODE
	
#	movb (P8OUT), d0
#	or 1<<4, d0

#	mov 0xfc, d0
	mov 3, d0
	movb d0,(P8OUT)
#	movb d0,(P7OUT)

	
 /*	READOPTBYTE_0x8082:
	--------------------------------
	0x0001	Bit0	prevents read error 0x1003 at 86FDB	 (retry ?)
	0x0002	Bit1	prevents read error 0x1004
	0x0004	Bit2	together with Bit1  disables edc check (06)
	0x0008	Bit3	disables on-the-fly timeout protection (CP2) (ernie&bert)
	0x0010	Bit4
	0x0020	Bit5
	0x0040	Bit6	mario kart: very slow video, skips CP_0 on llread
	0x0080	Bit7	causes audible 'focus-clicking' worse error correction
	0x0100	Bit9	waits on some ll state for readdone
	0x0200	Bit9	RE3: read error on boot. fast err detection even though err 
					table is patched
 */  
	#================================================================
	.if DBGMODE == 1
 1:		cmp CMD_TOGGLE_ECMA, d0				# set orgdisc flag
		bne 1f
		movbu (bConfig), d0
		xor 0x01, d0
		movb d0, (bConfig)
		mov (dwUsrSectorOffset), d0
		xor 0x0a, d0
		mov d0, (dwUsrSectorOffset)
		bra patchNiceCMD			
 1:		cmp CMD_SETREADSTATUS, d0			# set status for readid
		bne 1f
		jsr SUB_SetReadIdStatus
	.endif
	#================================================================

	/*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		~ 0x40EAD0	logintime
		~ PROTOPT (cactus-1)
		~ bit0 0x01: 0x40 from hlecmd
		~ bit2 0x04: 0x80 from hlecmd
		~ bit5 0x20: (?) happens to good boy
		~ DRV6: 1010
		~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	
SUB_UndoReadOptions:
			
	sub d0, d0
	mov d0, (FwReadCfg)
		
	mov 0x2c, d0					
	mov	0x40ea5c, a0
	movb d0, (2, a0)
	mov	1, d0
	jsrabs FwSendCtrlCommand		

	mov 0x05, d0
	movb d0, (0x40ec14)
	rts
#END

/*		mov 0x10f, d0					# readOpt byte
		mov d0, (FwReadCfg)
#		mov 0x00, d0
#		jsrabsOrg 0x8B0EB				# set_c_40050_0
		mov	0x03, d0					# LLCMD: 00 30 03/00 00 10
		mov	0x40ea52, a0
		movb d0, (2, a0)
		mov	0x05, d0
		jsrabs FwSendCtrlCommand	
		mov 0x08, d0					# LLCMD: 00 38 2c/08
		mov	0x40ea5c, a0
		movb d0, (2, a0)
		mov	1, d0
		jsrabs FwSendCtrlCommand		
*/





/*	.long 0x11F50101, 0x4B6A01B0, 0x02F85640, 0x00010023	# PAL Normal
	.long 0x00000024, 0x4D2B4D6D, 0x4D8A4D4C, 0x00000000
	.long 0x00000000, 0x00000000, 0x00000000, 0x013C0144
	.long 0x113901B1

	.long 0x00060101, 0x4B6A01B0, 0x02F85640, 0x001501E6	# PAL Black
	.long 0x001401E7, 0x4D2B4D6D, 0x4D8A4D4C, 0x00435A4E
	.long 0x00000000, 0x00435A4E, 0x00000000, 0x013C0144
	.long 0x113901B1

	.long 0x11F50101, 0x4B6A01B0, 0x02F85640, 0x00010023	# PAL GCOs
	.long 0x00000024, 0x4D2B4D6D, 0x4D8A4D4C, 0x00435A4E
	.long 0x00000000, 0x00435A4E, 0x00000000, 0x013C0144
	.long 0x113901B1

	.long 0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018	# PAL 60Hz
	.long 0x00020019, 0x410C410C, 0x40ED40ED, 0x00000000
	.long 0x00000000, 0x00000000, 0x00000000, 0x00050176
	.long 0x110701AE

	.long 0x6aac017c, 0x850001a4, 0x060000f0, 0x00180019	# EURGB60_INT / MPAL_INT
	.long 0x00030002, 0x100f0e0d, 0x02060205, 0x02040207
	.long 020d01ad

	.long 0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018	# NTSC Normal
	.long 0x00020019, 0x410C410C, 0x40ED40ED, 0x80100000
	.long 0x00000000, 0x80100000, 0x00000000, 0x00000000
	.long 0x110701AE

	.long 0x00060001, 0x476901AD, 0x02EA5140, 0x001501E6	# NTSC Black
	.long 0x001401E7, 0x410C410C, 0x40ED40ED, 0x80100000
	.long 0x00000000, 0x80100000, 0x00000000, 0x00000000
	.long 0x110701AE

	.long 0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018	# NTSC GcOS
	.long 0x00020019, 0x410C410C, 0x40ED40ED, 0x00435A4E
	.long 0x00000000, 0x00435A4E, 0x00000000, 0x00000000
	.long 0x110701AE
*/

/*
00000000 00060001 476901AD 02EA5140 001501E6 ....Gi....Q@....
00000010 001401E7 410C410C 40ED40ED 00800040 ....A.A.@.@....@
00000020 00000000 008004E0 00000000 00020062 ...............b
00000030 910701AE 90010001 00010001 00010001 ................
00000040 00000000 00000000 254A10ED 1AE771F0 ........%J....q.
00000050 0DB4A574 00C1188E C4C0CBE2 FCECDECF ...t............
00000060 13130F08 00080C0F 00FF0000 00000000 ................
00000070 02500000 000000FF 00FF00FF 00FF00FF .P..............
*/

/*
00000000 0006 0001 4769 01AD 02EA 5140 0015 01E6 ....Gi....Q@....
00000010 0014 01E7 410C 410D 40ED 40ED 0088 1860 ....A.A.@.@....`
00000020 0000 0000 0088 1D00 0000 0000 0002 01CE ................
00000030 9107 01AE 9001 0001 0001 0001 0001 0001 ................
00000040 0000 0000 0000 0000 254A 10ED 1AE7 71F0 ........%J....q.
00000050 0DB4 A574 00C1 188E C4C0 CBE2 FCEC DECF ...t............
00000060 1313 0F08 0008 0C0F 00FF 0000 0000 0002 ................
00000070 0250 0000 0000 00FF 00FF 00FF 00FF 00FF .P..............

   vidModeNTSC:
	.long 0x00060001, 0x476901AD, 0x02EA5140, 0x001501E6	# NTSC US
	.long 0x001401E7, 0x410C410C, 0x40ED40ED, 0x00800040
	.long 0x00000000, 0x008004E0, 0x00000000, 0x00020062
	.long 0x910701AE

  vidModeNTSC:
	.long 0x00060001, 0x476901AD, 0x02EA5140, 0x001501E6	# NTSC JAP
	.long 0x001401E7, 0x410C410D, 0x40ED40ED, 0x00881860
	.long 0x00000000, 0x00881D00, 0x00000000, 0x000201CE
	.long 0x910701AE

*/

	mov	0x01, d0			
	mov	d0, (0x40ec48)
	mov 0x40, d0
	movb d0, (0x01, a1)
	
	jsrabsOrg 0x898a3
	mov	d0,d2
	mov 0x00, d0
	movb d0, (FwDriveState)

	mov 0x40d800, a0				# clear dbgout mem
	sub d0, d0
	mov 0x00, d0
	mov 0x020, d1
	jsrabs FwMemset

	mov 0x40d800, a0
	mov 0x22, d0
	mov d0,(a0)
	mov d0,(2,a0)

*/
/*	movb d0, (0x40EB52)
	movb d0, (0x40EB53)
#	movb d0, (0x81F2)
	mov 0x02, d0
	mov d0, (0x40EB44)
	# enable rx and tx
	mov 0xC080, d1
	mov d1, (SC0CTR)
	P80_HI									# request to send
*/


	mov FwTimer2,	a0
	movbu (0, a0),	d1
	and 0xF8, d0
	
	movbu (bLastTime), d0
	sub d1, d0
	movb d1, (bLastTime)
	cmp 0, d0
	beq noTick
	jsr SUB_LogDiscCheckState
 noTick:



/***********************************************************************/
 Sub_SendByteSerial:		# (d0 = byte)
/***********************************************************************/
.if DBGOUTPUT == 1
		xor 0xAA, d0
		mov d0, d3

		mov 0x00, d0		# set both data and clock pins low
		movb d0, (P8OUT)	
		mov 1, d0			# wait a bit
		jsr SUB_Sleep
		
		mov 0x03, d0		# set both data and clock pins high
		movb d0, (P8OUT)	# to indicate startbit
		mov 3, d0			# wait some more
		jsr SUB_Sleep

		mov 8, d2			# loop 8 times
  bitloop:
		mov d3, d0			# set next databit to P80, clear clockbit (P81)
		and 0x01, d0		
		mov d0, (P8OUT)
		mov 3, d0			# sleep a little
		jsr SUB_Sleep

		mov 0x02, d0		# set clockbit high, data low
		movb d0, (P8OUT)
		mov 3 d0			# take a nap :p
		jsr SUB_Sleep
		
		ror d3				# rotate input bits
		sub 01, d2
		bne bitloop

		mov 0x00, d0		# set both pins low again
		movb d0, (P8OUT)	
.endif
		rts
#END

/***********************************************************************/
 SUB_LogDiscCheckState:
/***********************************************************************/
	
	movb (bLastLoginByte), d1
	movb (0x40ead2), d0
#	movb (0x8069), d0
	cmp d0, d1
	beq noLoginByte
	movb d0, (bLastLoginByte)

	jsr SUB_DbgOutByte
	jsr Sub_FlashLED_P81

	jsr SUB_WaitForP80Low

	mov 2000, d0
	jsr SUB_Sleep

noLoginByte:
	rts
#END
		

/*		lis r3, 0x8000
		lis r4, 0x8080
		li  r5, 0x3000 / 4
		bl SUB_Memcpy

		lis r3, 0x8130
		lis r4, 0x8081
		li  r5, 0x3000 / 4
		bl SUB_Memcpy

		li32 r0, 0x27657265				# Set GCOs Memdump tag
		li32 r4, 0x807FFFF0			
		stw	 r0, 0(r4)
*/
/*
#=======================================================================
 SUB_FillVideoMem:					# R3: color
#=======================================================================
videofun:
	lis         r4, 0xC0F0
	lis         r9, 0xCC00
	stw         r4, 0x201c(r9)
	stw         r4, 0x2024(r9)
	li32		r5, 0x0004b000
	mtctr		r5
 loopf:
	stwu		r3, 4(r4)
	bdnz loopf
	blr
*/

#	li32	r0, 0x27657265				# Set GCOs Memdump tag
#	li32 	r4, 0x807FFFF0			
#	stw		r0, 0(r4)





	#		li32 r3, 0x86936274
#		bl SUB_FillVideoMem
	
/*	li		r5, 19						# counter
	mtctr	r5
	GetRelocAddr r5, vidModeCommon-4	# get commom table address

 VidModeLoop2:
	lwzu	r0, 4(r5)					# copy second part
	stwu	r0, 4(r4)
	bdnz VidModeLoop2
*/	


/*	.long 0x00060101, 0x4B6A01B0, 0x02F85640, 0x001501E6	# PAL Black
	.long 0x001401E7, 0x4D2B4D6D, 0x4D8A4D4C, 0x00435A4E
	.long 0x00000000, 0x00435A4E, 0x00000000, 0x013C0144
	.long 0x113901B1
*/

/*
vidModeCommon:	
	.long			  0x10010001, 0x00010001, 0x00010001
	.long 0x00000000, 0x00000000, 0x28500100, 0x1AE771F0
	.long 0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF
	.long 0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000
	.long 0x02800000, 0x000000FF, 0x00FF00FF, 0x00FF00FF
*/
	.if REGASSERT == 0
		lis r3, ApplMainPtr@h				# get ApplMain Ptr
		lwz r3, ApplMainPtr@l(r3)			# get ApplMain Vec

		lwz r9, 0(r3)	
		lis r5, ApplMainAddr@h				# make a backup
		stw r9, ApplMainAddr@l(r5)
		GetRelocAddr r5, OnApploaderMain	# replace with our hook
		stw r5, 0(r3)

		lis r3, ApplClosePtr@h				# get ApplClose Ptr
		lwz r3, ApplClosePtr@l(r3)			# get ApplClose Vec
		lwz r9, 0(r3)		
		lis r5, ApplCloseAddr@h				# make a backup
		stw r9, ApplCloseAddr@l(r5)
		GetRelocAddr r5, OnApploaderClose	# replace with our hook
		stw r5, 0(r3)
	.else


#	mov	(FwTimer2), d0				# avoid spindown when drive idles for too long
#	mov	d0, (FwIdleTimeout)

#==================================================#
#	Sets status bytes to "need readid"
#==================================================#
 SUB_SetReadIdStatus:
	mov 6, d0
	mov d0, (FwCmdState1)
	mov 3, d0
	mov d0, (FwCmdState2)
	rts
 #END

	
/***********************************************************************/
 SUB_PatchLBA0CountryCodes:		/* (a0) ptr to sector data  */
/***********************************************************************/
#	mov 0x02, d0								# patch region code in lba 0
#	movb d0, (LBA0 + 0x440 + 0x1B, a0)
	rts
#END

#	tbzAbs bConfig, 3, Regionpatch
#	rts
#
# Regionpatch:


#	mov a1, (0x40d900 + 0x04)
#	mov d1, (0x40d900 + 0x08)
#	mov d2, (0x40d900 + 0x0A)
#	mov (0x40d900 + 0x04), a1				# pop regs
#	mov (0x40d900 + 0x08), d1
#	mov (0x40d900 + 0x0A), d2


/*	#- get the bp address that caused the bp in a1 -#
	movb (REG_ADBCTRL), d0
	and	ADB1ACK, d0
	bne	1f
	bitclr REG_ADBCTRL 0		# on BP0
	mov (REG_ADB0), a1
	bra 2f	
1:	bitclr REG_ADBCTRL 1		# on BP1
	mov (REG_ADB1), a1
2:
*/



/*	#=======================================================
	 .if SKIP_PROTECTION == 1
		mov FwDriveState, a0
		movb (-1, a0), d0
		and 0xfb, d0				# drive4,8:	bit2
		and 0xf7, d0				# drive6:	bit3
		movb d0, (-1, a0)
	 .endif
	#=======================================================
*/

/*	mov	FwDriveState, a0			# cactus		
	mov	2, d0
	bset d0, (a0)
*/	
#	tbnzAbs FwDriveState, 1, 1f
#	bitset FwDriveState, 1
#	bitset FwDriveState, 2
#	bitset FwDriveState, 3
#	bset d0, (a0)

#	mov	FwDriveState, a1					# if (cactus byte != e0) set bit 1
#	movbu (a1), d1
#	cmp	0xe0, d1			
##	beq	1f
#	or	0x02, d1			
#	movb d1, (a1)
#1:	mov	0x400024, a1


	#--------------------------------------------#
	#- check for presence of qoob drivecode		-#
	#--------------------------------------------#
	mov (REG_ADB0), a0
	cmp 0, a0
	beq 1f

#	mov 0xad2b, d0
#	mov d0, (0x40d820)

	sub d0, d0
	movb d0, (REG_ADBCTRL)

	mov 0x40d0a0, a0
	mov 0x80a74, a0
	mov a0, (FwIrqVec)
	jmp (a0)

#	mov 0x40d096, a0
#	mov 0x80a74, a0
#	jmp (a0)




1:	cmp CMD_DISABLEDREFIX, d0
	bne 1f
	sub d0, d0
	movb d0, (REG_ADBCTRL)
	bitset bConfig, 2				# set nofastboot flag
	and 0xf7ff, psw
	mov FwIntHandler, a0
	mov a0, (FwIrqVec)
	bra Qcode_Init


	/*
#**********************************************************************
 SUB_StartDriveReset:
#**********************************************************************
		#-----------------------------------------------------
		# Start of DVDReset
		#-----------------------------------------------------
		and	0xf7ff,psw
		nop	
		nop	
		sub	d2, d2						# init threads
		mov	FwThreadSetup, a2			
		mov	0x8010, a1
 xloop:
		movbu (a2), d0
		cmp	0x02, d0			
		bne	verx
		mov	a1,a0
		jsrabs FwInitThread
 verx:		
		add	0x01,d2			
		add	0x12,a1
		add	0x0e,a2
		cmp	0x02,d2			
		blt	xloop

		#-----------------------------------------------------
		# Instead of init_intsandtimers(0x80998)
		#-----------------------------------------------------
		mov 0, d0
		mov d0, (FwInitState1)
		mov d0, (FwInitState2)

		jsrabs FwSetInitState
		jsrabs FwInitSpinup				# SUB_Init_Spinup

		mov 2, d0						# 8076 = 0 -> start drive
		mov 0, d1						
		jsrabs FwStartDrive

#		mov 0x40, d0					# set copy flag (02 40)
#		movb d0,(FwHLECmdBuffer+1)
		mov 2, d0						# set lasthlecmd 2
		mov d0, (FwLastHLECmd)

		jmpabs FwContinueReset			# continue with dvdreset
#END
*/

/*	movbu (bCalibrationDone), d0
	cmp 0, d0
	bne 1f

	mov (FwCmdState1), d0
	cmp 0x00, d0
	bne 1f
	
	mov (0x80D4), d0
	cmp 0, d0
	bne 1f

	and 0xf7ff, psw
	mov 1, d0
	movb d0, (bCalibrationDone)

	movb (0x40ECF4), d0
	or 4, d0
	movb d0, (0x40ECF4)

	mov 0x81, d0
	jsrabsOrg 0x08993D

#	mov 0x81, d0
#	jsrabsOrg 0x8320A	#0x83BD0		#0x83BD0
	or 0x800,psw
1:
*/	

    #----------------------------------------------------------------------
	# On int do stuff...
	#----------------------------------------------------------------------
	movbu (bCalibrationDone), d0
	cmp 0, d0
	bne 1f

	mov (0x80D4), d0
	cmp 0, d0
	bne 1f

	mov (0x80D6), d0
	cmp 0, d0
	bne 1f

#	mov (0x040EBE4), d0
#	cmp 0, d0
#	bne 1f
#	mov (0x040EC50), d0
#	cmp 0, d0
#	bne 1f

	mov (FwCmdState1), d0
	cmp 0x00, d0
	bne 1f

	
 doCalibration:

	jsrabsOrg FwIntHandler
	and 0xf7ff, psw
#	mov 0x00, d0
#	mov d0, (0x40EC4C)

	mov 1, d0
	movb d0, (bCalibrationDone)
	jsrabsOrg 0x83BD0		#0x83BD0
#	mov 0x81, d0
#	jsrabsOrg 0x08993D
	
	mov 6, d0						# set status bytes to "id read, seed set"
	mov d0, (FwCmdState1)
	mov 3, d0
	mov d0, (FwCmdState2)	
	
	or 0x800, psw
	rts

	


#	and 0xf7ff, psw

/*
	mov 0x40EBE7, a0				# set copy flag (02 40)
	mov 0x40, d0
	movb d0,(a0)
*/
	mov 1, d0						# set status bytes to "id read, seed set"
	mov d0, (FwCmdState1)
	mov 0, d0
	mov d0, (FwCmdState2)		
		
/*	mov (0x040EC5C), d0
	or 0x10, d0
	mov d0, (0x040EC5C)

	mov 1, d0
	jsrabsOrg 0x08D170
	mov 0xff, d0
	jsrabsOrg 0x008D18A
*/	



#	movbu (0x40ec74), d0
#	mov 0x01, d1
#	or d1, d0
#	movb d0, (0x40ec74)


#	mov 0x06, d0				# motorspeed
#	mov 0x00, d1
#	jsrabsOrg 0x08B9B3

/***********************************************************************/
 SUB_StartDriveResetOLD:
/***********************************************************************/

		#-----------------------------------------------------
		# Start of DVDReset
		#-----------------------------------------------------
		and	0xf7ff,psw
		nop	
		nop	

		sub	d2, d2
		mov	0x08e9a1, a2			# version (2)
		mov	0x8010, a1
 xloop:
		movbu (a2), d0
		cmp	0x02, d0			
		bne	verx

		mov	a1,a0
		jsrabs	0x8dcad
 verx:		
		add	0x01,d2			
		add	0x12,a1
		add	0x0e,a2
		cmp	0x02,d2			
		blt	xloop

		#-----------------------------------------------------
		# Instead of init_intsandtimers(0x80998)
		#-----------------------------------------------------
#		jsrabs 0x80998

		mov 0, d0
		mov d0, (0x8071)
		mov d0, (0x8076)
		jsrabs 0x8B205
		jsrabs 0x83BA5					# SUB_INIT_SPINUP

		mov 2, d0
		mov 0, d1						# 8076 = 0 -> start drive
		jsrabs 0x878F0

#		mov 0x40EBE7, a0				# set copy flag (02 40)
#		mov 0x40, d0
#		movb d0,(a0)
	
		mov 2, d0						# set lasthlecmd 2
		mov d0, (0x80E8)

		jmpabs 0x8D956					# continue with dvdreset
#END


#	mov 0x11, d0					# dretest
#	mov d0, (0x40EC4e)
#	mov 0x01, d0
#	mov d0, (0x40EC50)
#	mov 0x40, d0					# motor speed
#	mov d0, (0x40ed0a)


	jsr SUB_SetReadOptions


#===================================================================================#
#	Adjust read options to read DVD+-RW better										#
#===================================================================================#
 SUB_SetReadOptions:

.if DREFIX == 1

/*	mov 0x286, d0
	mov d0, (0x8082)
	mov 0x01, d0
	mov d0,(0x8084)
*/
	mov 0x106, d0
	mov d0, (0x8082)
	
/*	mov	0x03, d0
	movb d0, (0x40ea54)
	mov	0x40ea52, a0
	mov	0x05, d0
	jsrabsOrg 0x8B126				# LLCMD: 00 30 03/00 00 10

	mov 0x20, d0
	movb d0, (0x40EA5E)
	mov	0x40ea5c, a0
	mov	1, d0
	jsrabsOrg 0x8B126				# LLCMD: 00 38 2c/08
*/
  .endif 

	rts
#END


 
#		jmp SUB_StartDriveReset		# Reset the drive
#		bra HandlerEnd				# call original handler

		mov 0x40d800, a0			# clear dbgout mem
		sub d0, d0
		mov 0x0200, d1
		jsrabs FwMemset


/*		mov	0x8504, a0			# COBRA copy 40c057->8504
		mov	0x40c057, d1
		mov	0x0100, d0
		jsrabs FwMemcpy
		
		mov	0x40d002, a0
		mov	0x40c000, d1
		mov	0x00006b, d0
		mov	0x00, d0			
		jsrabs FwMemcpy
*/

/* BP0 - Read ECMA Standart DVDs */
.equ BP0Addr_ReadECMA,			0x08b145		#0x08B130		
.equ BP0Addr_ReadECMA_Exit,		0x08b150

/* BP1 - auth and lbapatch   */
.equ BP1Addr_AuthPlusR,			0x089dfc		#0x089df2	#0x089D42	0x08aaf0
.equ BP1Addr_AuthDMI,			0x08aaf0

#	mov 0x2121, d0								# Tag sector		
#	mov d0, (LBA0 + 0x30, a0)

/*
#**********************************************************************
 SUB_StartDriveReset:
#**********************************************************************
		#-----------------------------------------------------
		# Start of DVDReset
		#-----------------------------------------------------
		and	0xf7ff,psw
		nop	
		nop	
		sub	d2, d2						# init threads
		mov	FwThreadSetup, a2			
		mov	0x8010, a1
 xloop:
		movbu (a2), d0
		cmp	0x02, d0			
		bne	verx
		mov	a1,a0
		jsrabs FwInitThread
 verx:		
		add	0x01,d2			
		add	0x12,a1
		add	0x0e,a2
		cmp	0x02,d2			
		blt	xloop

		#-----------------------------------------------------
		# Instead of init_intsandtimers(0x80998)
		#-----------------------------------------------------
		mov 0, d0
		mov d0, (FwInitState1)
		mov d0, (FwInitState2)
		jsrabs FwSetInitState
		jsrabs FwInitSpinup				# SUB_Init_Spinup

#		mov 2, d0						# 8076 = 0 -> start drive
#		mov 0, d1						
#		jsrabs FwStartDrive

#		mov 0x40, d0					# set copy flag (02 40)
#		movb d0,(FwHLECmdBuffer+1)
#		mov 2, d0						# set lasthlecmd 2
#		mov d0, (FwLastHLECmd)
		jmpabs FwContinueReset			# continue with dvdreset
#END
*/

	#=======================================================
	.if RELEASEBUILD == 0
		mov 0x40d800, a0		# clear dbgout mem
		sub d0, d0
		mov 0x00, d0
		mov 0x0200, d1
		jsrabs FwMemset
	.endif
	#=======================================================



	#=======================================================
	.if RELEASEBUILD == 0
		mov 0x00, d0					# disable HLELog buffer
		mov d0, (0x40ec60)
		mov 0x40d800, a0				# clear dbgout mem
		sub d0, d0
		mov 0x00, d0
		mov 0x0800, d1
		jsrabs FwMemset
	.endif
	#=======================================================





/*		and	0xf7ff,psw
		nop	
		nop	
		
		mov 0xd0, d0
		mov 0x1000, d1
		sub xxx-0x40d000, d1
		mov xxx, a0
		jsrabs FwMemset

		mov 0x0e, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x0f, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x10, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x11, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x12, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x13, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x14, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x15, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x16, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x17, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x18, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x19, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x20, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x21, d0
		mov 0x1000, d1
		jsrabs FwMemset
		mov 0x22, d0
		mov 0x1000, d1
		jsrabs FwMemset

xxx:	jmp xxx
*/
/*	#- no disk id -#
	sub	d0,d0
	movx (0x04,a3),d2		
	movx (0x04,a3),d3
	mov	(0x0c,a3),a1
	mov	(0x10,a3),a2
	add	0x14,a3
	rts	
*/	
	mov 0x45, d0								# patch country code in gameId
	movb d0, (LBA0 + 0x03)


.equ adb1_LBAinject,			0x08aeea		#0x08AEEA
.equ adb1_LBAinject_exit,		0x08aeec		#0x08AEEC

/* BP1 - gp debug breakpoint */
.equ dbgBpAddress,				0x080fbe		# or 0x01, d1
.equ dbgBpReturn,				0x080fc1


		mov 0x40d800, a0			# clear dbgout mem
		sub d0, d0
		mov 0x1800, d1
		jsrabs FwMemset

 Qcode_Init:

		mov	Qcode_Init2, a0
		mov	a0, (FwIrqVec)

 Qcode_Init2:
		
		mov (REG_IAGR), d0
		cmp 0x20, d0
		bne HandlerEnd

		and	0xf7ff,psw
		nop	
		nop	
		

	movb (0x40ead2), d0
	movb d0, (bLastLoginByte)

	#========================================================================================
	.if RELEASEBUILD == 0
		mov 0x00, d0					# disable HLELog buffer
		mov d0, (0x40ec60)
	.endif
	#========================================================================================

	
/*		mov 0x40d800, a0				# clear dbgout mem
		mov 0x00, d0
		mov 0x800, d1
		jsrabs FwMemset

		#= Test: copy drivecode to relocate it #
		mov 0x40d800, a0
		mov 0x40d000, a1
		mov 0x400, d0
		jsr SUB_Memcpy
*/


/*		.macro VOpcR Opcode P1, AddrDefine, P2
			.irp    Addr, 4, 6, 8
				\Opcode \P1, (\AddrDefine\Addr) \P2
			.endr
		.endm
		
		.macro VersionedOpcode	 Opcode P1, P2, P3
			\Opcode  \P1 \P2 \P3
		.endm
*/

		.macro VOpcL Opcode AddrDefine, P1, P2, P3
			jsr SUB_PatchVOpcode
			.irp    Addr, 4, 6, 8
				\Opcode \AddrDefine\Addr, \P1
			.endr
		.endm


#		nop
#		nop
#		VOpcL mov FwDriveState,	a2
#		VOpcL mov, (FwDriveState, )	a3
#		VOpcR mov d0, FwDriveState
#		VersionedOpcode mov 0x21, d0
#		VersionedOpcode mov, (0x2121), d0
#		VersionedOpcode mov 0x2badbc, a0		
#		VersionedOpcode mov a0, (0x40d000)
		nop
		nop


		.macro tbzx addr bit offset label
			.byte 0x90, 0xFE, (0xC0 + \bit)
			.byte 0x66, 0x66, 0x66
			.long \addr + ((\offset) * 0x1000000)
		.endm
		nop
#		tbzx 0x002233, 5, 0x10, test

		
# sendllcmd:
#	mov (0x8079), d1	# test: fix something :)
#	and 0x08, d1
#	beq 1f
#	sub	0x02, d2


#	mov 1, a1
#	mov 0x0c, d0
#	mov d0, (a3)
#	mov 0x800, d0
#	mov d0, (2, a3)
#	mov 0, d0
#	mov 0x100, d1
#	jsrabs 0x08AEE6						# llcmd6


 checkCBDCommand:

	movbu (0x80A8), d0
	movb d0, (wDummy)
	mov 0x21, d0
	movb d0, (0x80a8)

	#- call the original handler -#
	jsrabs 0x80A74

	movbu (0x80A8), d0
	cmp 0x21, d0
	bne newCDBcommand
	
	movbu (wDummy), d0
	movb d0, (0x80a8)
	rts

 newCDBcommand:

	DbgOut8 0x80a8
	DbgOut8 REG_IAGR

/*	cmp 0xa8, d0
	beq DoAudioFix1
	cmp 0xab, d0
	beq DoAudioFix1
	cmp 0xe1, d0
	beq DoAudioFix1
	cmp 0xa9, d0
	beq DoAudioFix2
	rts
*/

/*
 checkaudiofix:
	movbu (0x80A8), d0
	cmp 0xa8, d0
	beq DoAudioFix1
	cmp 0xab, d0
	beq DoAudioFix1
	cmp 0xe1, d0
	beq DoAudioFix1
	cmp 0xa9, d0
	beq DoAudioFix2
	rts

DoAudioFix1:
	mov	a2, d1
	mov	(dwAudioFixBlah),a2
	mov	(04, a0), a1
	jsr	Swap24
	add	a2, a1
	jsr	Swap24
	mov	a1, (04, a0)
	mov	d1, a2
	rts	
DoAudioFix2:
	mov	(04, a0), a1
	jsr	Swap24
	mov	a1, (dwAudioFixBlah)
	rts	

 Swap24:
	mov	a1,(0xFFBC)
	mov	(0xFFBC),a1
	rts	
*/

/*
	mov a1, (0x40d900 + 0x04)		# push all registers
	mov d0, (0x40d900 + 0x10)
	mov d1, (0x40d900 + 0x12)
	mov d2, (0x40d900 + 0x14)

	mov 0x80a8 + 4, a0
	mov 8, d0
	jsr SUB_DbgOutBuffer
	jsr Sub_FlashLED_P80
	bra Patchdone
*/
	#-----------------------------------------------------------------------
	#  TEST: send via serial out
	#-----------------------------------------------------------------------

/*	
	#= [1] =# 
	mov 2, d0							# SO_WO
	mov	d0, (0x40eb44)

	mov 0x23, d0						# SO_BUF
	movb d0, (0x40eb52)
	movb d0, (0x40eb53)	

	#= [2] =# 
	mov 0, d0
	movb d0, (0x40eb46)					# SO_RO
	
	#= [3] =# 
	mov 0x8080, d0						# S0_CTRL send, 8bit
	mov d0, (0xFD80)					
	
	#= [4] =# 
	mov 0, d0
	movb d0, (0xFC64)
	movb d0, (0xFC67)
	mov 1, d0
	movb d0, (0xFC65)

	#= [5] =# 
	movb (0x40eb52), d0					# SO_BUF -> SO_TRB
	movb d0, (0xFD82)					

	#= [6] =# 
	mov 0x12c, d0
	mov d0, (0x8074)

	bitset 0xFFC8, 1					# set p81 high
#	bitclr  0XFFC8,  1					# set p81 low

	bra Patchdone
*/


	#- tell the drive to please accept the disk -#
	mov	cactus, a0
	mov	2, d0
	bset	d0, (a0)


	mov 0x03, d0				// copy flags
	movb d0, (0x40ED01)	


/***********************************************************************/
/* Breakpoint 1 patch sector data				                  		*/
/***********************************************************************/

 adb1patch_LBAinject:

	mov (0x80f6), a0
	cmp 0, a0,						# check for LBA 0
	beq PatchLBA
	cmp 0x04, a0					
	beq PatchLBA
	bra noPatchLBA

 PatchLBA:

	mov a1, (0x40d900 + 0x04)		# push all registers
	mov d0, (0x40d900 + 0x10)
	mov d1, (0x40d900 + 0x12)

	mov a0, a1						# bak LBA in a1
	mov a0, d0						# get dma buffer address in llctrl mem into a0
	and 0x0f, d0					# 
	mov 0x810, d1					# (lower 4bits of lba * 0x810) + 0x417100
	mulu d1, d0
	mov 0x427100, a0
	add d0, a0

	cmp 0, a1
	beq 1f
	cmp 0x04, a1
	beq 2f
	bra Patchdone
1:
	jsr SUB_PatchLBA0CountryCodes
	bra Patchdone
2:
	jsr SUB_PatchAPPLoader

 Patchdone:

	mov (0x40d900 + 0x04), a1		# pop regs
	mov (0x40d900 + 0x10), d0
	mov (0x40d900 + 0x12), d1

 noPatchLBA:

	#= exit llcmd_6	   =#
	mov	a3, a0
	jmpabs adb1_LBAinject_exit
#END

	
	/* set status bytes */
/*	mov 1, d0						
	mov d0, (0x81a4)
	mov 0, d0
	mov d0, (0x8188)
*/

	/* check for external commands */
/*	movb (wExtCommand), d0
	cmp 0, d0
	beq noCommand

	mov 0, d0					# clear command
	movb d0, (wExtCommand)
	
	mov 0x80a74, a0
	mov a0, (irq_handler_vector)
	jmp (a0)

 noCommand:
*/
/*	movb (bLastLoginByte), d1
	movb (0x40ead2), d0
	cmp d0, d1
	beq noLoginByte
	movb d0, (bLastLoginByte)
	jsr SUB_DbgOutByte
noLoginByte:
*/


adb1patch_LBAinject:

#	mov a0, (0x40d900 + 0x00)
#	mov d0, (0x40d900 + 0x04)
#	mov d1, (0x40d900 + 0x06)
#	mov (0x80f6), a0
#	mov (a0), (0x40d900 + 0x08)

#	mov 0x80f6, a0
#	mov 0x40d6f4, a0

#	mov 0x40d904, a0
#	mov 4, d0
#	jsr SUB_DbgOutBuffer			
	
#	mov 0x80f6, a0
#	mov 4, d0
#	jsr SUB_DbgOutBuffer			
	
#	mov (0x40d900 + 0x00), a0
#	mov (0x40d900 + 0x04), d0
#	mov (0x40d900 + 0x06), d1
	
#	bra noPatchLBA



/*
  LLCMD EXITS:

    

/*	.byte  0x3D, 0x20, 0x80, 0x00		##  lis         r9, 0x8000
	.byte  0x61, 0x29, 0x00, 0xCC		##	ori         r9, r9, 0x00CC
	.byte  0x38, 0x00, 0x00, 0x01		##	li          r0, 1
	.byte  0x90, 0x09, 0x00, 0x00		##  stw         r0, 0 (r9)
*/
	# 0x34 bytes #
/*	.byte 0x3c, 0x80, 0xc0, 0x50
	.byte 0x3d, 0x20, 0xcc, 0x00
	.byte 0x61, 0x29, 0x20, 0x1c
	.byte 0x90, 0x89, 0x00, 0x00
	.byte 0x3c, 0xa0, 0x00, 0x30
	.byte 0x61, 0x29, 0x00, 0x00
	.byte 0x7c, 0xa9, 0x03, 0xa6
	.byte 0x94, 0x84, 0x00, 0x04
	.byte 0x42, 0x00, 0xff, 0xfc
	.byte 0x3c, 0x60, 0x81, 0x30
	.byte 0x60, 0x63, 0x00, 0x00
	.byte 0x7c, 0x68, 0x03, 0xa6
	.byte 0x4e, 0x80, 0x00, 0x21
*/	  
		
		  
/***********************************************************************/
 SUB_PatchLBA0CountryCodesReadId: /* on llcmd6 (a0) ptr to sector data  */
/***********************************************************************/
	
	#-  is it LBA 0 ?  -#
	mov (0x400304), d0					
	mov (0x400306), d1
	add d1, d0
	cmp 0, d0
	bne nopatchCID

	mov 0x50, d0				
	movb d0, (0x40f10c + 3)
	mov 0x02, d0
	movb d0, (0x40f10c + 0x440 + 0x17 + 4)

 nopatchCID:
	rts

		
.macro DbgOutReg24 RegName
	mov d0, (0x40d900 + 0x018)
	mov a0, (0x40d900 + 0x01a)
	
	mov 0x40d900 + 0x01C, a0
	mov RegName, (a0)
	mov 4, d0
	jsr 
	mov 0x40d900 + 0x01a a0, 

	jsr \CmdNum * 4 + 0x40d000 - 4 -.+\CmdNum * 4 + 0x40d000 - 4
.endm

  
			  
#- exit llcmd_6_2  -#
	mov	0x08,d0
	jmpabs adb1_LBAinject_exit
		
	# patch dma transfer size
	mov	0x800,d1					
	mov	d1,(0x02,a3)
	mov	d2,d1

	#= exit llcmd_6_2  =#
	jsrabs 0x8AEE6		
	jmpabs adb1_LBAinject_exit

	#= exit llcmd_7    =#
	jsrabs 0x8B126
	jmpabs adb1_LBAinject_exit

	#= exit llcmd_11   =#
	jsrabs 0x87A08
*/






/* check for Address Break 0 */
/*	mov	(REG_ADBCTRL), d0
	and	ADB0ACK, d0
	bne	adb0_break_handler
*/
	/* check for Address Break 1 */
/*	mov	(REG_ADBCTRL), d0
	and	ADB1ACK, d0
	bne	adb1_break_handler
*/

/*=========================================================================
 * Breakpoint 0   
 * --------------
 * hooks send_llcmd and patches seed setup (llcmd_F0) 
 * and cpr_maj offset on llcmd_6 														
 *=========================================================================*/
adb0_break_handler:

	/* ack the interrupt */
	bitclr REG_ADBCTRL 0
	bitclr UNICR 0

	/* point to the prev stack pointer*/
	mov	a3, a0
	add	4, a0	
	movbu	(irq_depth), d0
	cmp	1, d0
	bne	1f
	mov	(4, a3), a0
1:
	/* overwrite the ret addresss */
	mov	adb0_fixup, a1
	mov	a1, (0x20, a0)

	mov 0x400180 + 0x0c + 0x20, a0					
	mov 0x2121, d0
	mov d0, (a0)
	mov d0, (2, a0)
	
	mov 0x40f100 + 0x0c + 0x20, a0					
	mov 0x2121, d0
	mov d0, (a0)
	mov d0, (2, a0)

	mov 0x419140 + 0x0c + 0x20, a0					
	mov 0x2221, d0
	mov d0, (a0)
	mov d0, (2, a0)

	rts

/**************************************************************/
/* Breakpoint 1 handler											*/
/**************************************************************/

adb1_break_handler:
		
	/* ack the interrupt */
	bitclr REG_ADBCTRL 1
	bitclr UNICR 0

	/* FLASH Blue LED */
#	jsr Sub_FlashLED_P80

/*	
	mov	a3, a0
	add	4, a0

	movbu	(irq_depth), d0
	cmp	1, d0
	bne	1f
	mov	(4, a3), a0
1:

	#----------------------------------------------------------------------
	# check the breakpoint address and make the return address on 
	# the stack point to the according code replacement
	#----------------------------------------------------------------------
	mov (REG_ADB1), a1
	
	cmp adb1_authdisk, a1
	bne 1f
	mov adb1patch_authdisk, a1
	bra patchstack
	
1:	cmp adb1_sectorpatch, a1
	mov adb1patch_sectorpatch, a1
	bra patchstack

	#- Panic! we dont recognise the break address. just rts then
	rts
	
 patchstack:	
	mov	a1, (0x20, a0)
	rts
*/










.equ data2,					0x008010
.equ data2_size,			0x5e
.equ data2_copy_from,		0x08e9d8
		mov data2_copy_from, d1		# manually copy init data seg
		mov data2, a0
		mov data2_size, d0
		jsr FwMemcpy				# d1: source, a0: dest, d0: size

#	mov (0x40ebdc), a0				# get dma source buffer
#	mov 0x40ebdc, a0
#	mov 0x40BC00, a0
#	mov 0x40EB96, a0
#	add 0x40, a0
#	40EBB6
#	40EB96


/*	regs
	mov 0x40d900, a0
	mov 0x18, d0
	jsr SUB_DbgOutBuffer*/

#	mov a3, d1
#	mov 0x20, d0
#	jsr memcpy

#	mov a1, a0
#	mov 16, d0
#	jsr SUB_DbgOutBuffer	
#	mov (0x8000), a0
#	mov (0x810C), d0



	mov a0, (0x40d900 + 0x00)		# push all registers
#	mov a1, (0x40d900 + 0x04)
#	mov a2, (0x40d900 + 0x08)
#	mov a3, (0x40d900 + 0x0c)
	mov d0, (0x40d900 + 0x10)
	mov d1, (0x40d900 + 0x12)
#	mov d2, (0x40d900 + 0x14)
#	mov d3, (0x40d900 + 0x16)

/***********************************************************************/
/* Breakpoint 1 patch													*/
/***********************************************************************/

adb1_break_handler:
	/* ack the interrupt */
	movbu	(ADBCTL), d0
	and	~ADB1CK, d0
	movb	d0, (ADBCTL)
	movbu	(UNICR), d0
	and	~UNID, d0
	movb	d0, (UNICR)

	jsr Sub_FlashLED_P80

	mov	a3, a0
	add	4, a0

	movbu	(irq_depth), d0
	cmp	1, d0
	bne	1f
	mov	(4, a3), a0
1:

	mov	adb1_fixup, a1
	mov	a1, (0x20, a0)
	rts

adb1_fixup:
	/* no disk id */
	sub	d0,d0
	
	movx (0x04,a3),d2		
	movx (0x04,a3),d3
	mov	(0x0c,a3),a1
	mov	(0x10,a3),a2
	add	0x14,a3
	rts	


	movb (wBP1_Count), d0
	cmp 0x02, d0
	bne StartDriveBP

	#--------------------------------------------#
	#- handle our second breakpoint (cdb read)  -#
	#--------------------------------------------#
 ReadCBB_BP:	

	sub	d0,d0
	jmpabs 0x084A82

 	#--------------------------------------------#
	#- handle our first bp1 (startdrive auth)  -#
	#--------------------------------------------#
 StartDriveBP:

	cmp 0x01, d0 
	bne noSetBP
		
	#- set next breakpoint -#
	jsr SUB_SetCBDReadBreakpoint

 noSetBP:

	add 1, d0
	movb d0, (wBP1_Count)

	/* no disk id */
	sub	d0,d0
	
	movx (0x04,a3),d2		
	movx (0x04,a3),d3
	mov	(0x0c,a3),a1
	mov	(0x10,a3),a2
	add	0x14,a3
	rts	
	
	jmp	adb1_fixup_exit

#END


	

 patchcountrycodes:
/*
	mov (0x400304), d1					# is it sector 0 ?
	cmp 0, d1
	bne nopatch
	mov (0x400306), d1					# is it sector 0 ?
	cmp 0, d1
	bne nopatch
*/


/*	mov 0x50, d0				
	movb d0, (0x40f10c + 3)
	mov 0x02, d0
	movb d0, (0x40f10c + 0x440 + 0x17 + 4)
*/
/*	movb (0x40f10c), d0
	cmp 0x47, d0
	bne nopatch

	movb (0x40f10c + 3), d0
	cmp 0x45, d0
	bne nopatch
*/
/*	movb (0x40f10c + 0x440 + 0x17 + 4), d0
	cmp 0x01, d0
	bne nopatch
*/

	/* sector 0x12, offset 16c */

nopatch:




   sector patch


/*
	mov 0x400180, a0
	mov 0x80e0, a0
	mov 0x40EC8E, a0

	mov 0x10, d0
	jsr SUB_DbgOutBuffer
*/	

/*	mov 0x457100, a0
	mov 0x5555, d0				# f
	mov d0, (0x0c + 0x40, a0)
*/

/*	mov 0x400100 + 0x810, a0
	mov 0xCCCC, d0
	mov d0, (0x0c + 0x40, a0)
	mov 0xCCCC, d0
	mov d0, (0x0c + 0x42, a0)

	mov 0x417100 + 0x810, a0
	mov 0xBBBB, d0							# 2
	mov d0, (0x0c + 0x40, a0)
	mov 0xBBBB, d0
	mov d0, (0x0c + 0x42, a0)

	mov 0x417100 + 0x810 + 0x810, a0		# 1
	mov 0xBBBB, d0
	mov d0, (0x0c + 0x40, a0)
	mov 0xBBBB, d0
	mov d0, (0x0c + 0x42, a0)
*/

/*	mov (0x00, a0), d0			# sector 0x12 ?				
	cmp 0x300, d0
	bne nopatch
	mov (0x02, a0), d0			# sector 0x12 ?				
	cmp 0x1200, d0
	bne nopatch
*/	



/*	mov (apploader_patch + 0x00), d0
	mov d0, (0x00, a0)
	mov (apploader_patch + 0x02), d0
	mov d0, (0x00, a0)
	mov (apploader_patch + 0x04), d0
	mov d0, (0x04, a0)
	mov (apploader_patch + 0x06), d0
	mov d0, (0x06, a0)
	mov (apploader_patch + 0x08), d0
	mov d0, (0x08, a0)
	mov (apploader_patch + 0x0a), d0
	mov d0, (0x0a, a0)
	mov (apploader_patch + 0x0c), d0
	mov d0, (0x0c, a0)
	mov (apploader_patch + 0x0e), d0
	mov d0, (0x0e, a0)
*/

/* copypatch:
	movbu (a1), d0
	movb d0, (a0)
	add 1, a0
	add 1, a1
	sub 1, d1
	bne copypatch
`*/






	/*  415494				
	*/
/*	mov 0x440180, a0
	mov 0x8888, d0				# f
	mov d0, (0x0c + 0x10, a0)
	
	mov 0x4001f0, a0
	mov 0x7777, d0				# f
	mov d0, (0x0c + 0x10, a0)
*/
/*	mov 0x427100, a0
	mov 0x9999, d0				# f
	mov d0, (0x0c + 0x40, a0)
*/
/*	mov 0x417100, a0
	add 0x810, a0
	mov 0xBBBB, d0
	mov d0, (0x0c + 0x40, a0)

	add 0x810, a0
	mov 0xCCCC, d0
	mov d0, (0x0c + 0x40, a0)

	add 0x810, a0
	mov 0xDDDD, d0
	mov d0, (0x0c + 0x40, a0)

	add 0x810, a0
	mov 0xEEEE, d0
	mov d0, (0x0c + 0x40, a0)
*/




/*	mov (0x40EC4E), d0
	mov (wLastHLECmd), d1
	cmp d1, d0
	beq NoLogData

	mov d0, (wLastHLECmd)
	
	mov 0x40EC4E, a0
	mov 2, d0
	jsr SUB_DbgOutBuffer
*/

/*	mov	(0x40ec4e),d1
	#- mulql	0x0c,d1 -#
	.byte 0xF4, 0xC1, 0x4E, 0xEC, 0x40
	mov	d1, a0
	add	0x40ebe6,a0
*/

/*	mov (0x8068), d0
	and 0xff, d0					# ff ~ 1 sec
	cmp 0, d0
	bne  NoLogData
	
	mov (0x8068), d0
	add 1, d0
	mov d0, (0x8068)

	mov 0x40ebe6, a0
	mov 12, d0
	jsr SUB_DbgOutBuffer
*/


/*	mov 0x20, d0
	mov 0x40D6A6, a0	##  
	jsr SUB_DbgOutBuffer

	mov 0x40f10c, a0
	mov 4, d0
	jsr SUB_DbgOutBuffer
*/


/*	mov (0x8068), d0
	and 0xff, d0					# ff ~ 1 sec
	cmp 0, d0
	bne  NoLogData
	
	mov (0x8068), d0
	add 1, d0
	mov d0, (0x8068)
*/


/*		mov 0x240, d0
		mov d0, (0x8082)					# readopt byte1 (xor key, something)
		mov 0x01, d0
		mov d0, (0x8084)
*/


/*		mov 0x00, d0
		movb d0, (0x40EA54)					# setreadopt var1	(3, 0)
		mov 0x24, d0
		movb d0, (0x40EA5e)					# setreadopt var2	(2c, 24, ??)
*/


/*	mov 0x2121, d0			
	mov d0, (0x40006)
	mov 0x2222, d0
	mov d0, (0x4019c)						# readid country code
*/
/*	movb (0, a0), d0
	movb d0, (0x8000)
	movb (1, a0), d0
	movb d0, (0x8001)
	movb (2, a0), d0
	movb d0, (0x8002)
	movb (3, a0), d0
	movb d0, (0x8003)
	movb (4, a0), d0
	movb d0, (0x8004)
	movb (5, a0), d0
	movb d0, (0x8005)
	movb (6, a0), d0
	movb d0, (0x8006)
	movb (7, a0), d0
	movb d0, (0x8007)
	movb (8, a0), d0
	movb d0, (0x8008)
	movb (9, a0), d0
	movb d0, (0x8009)
	movb (0x0a, a0), d0
	movb d0, (0x800a)
	movb (0x0b, a0), d0
	movb d0, (0x800b)
	movb (0x0c, a0), d0
	movb d0, (0x800c)
	movb (0x0d, a0), d0
	movb d0, (0x800d)
	movb (0x0e, a0), d0
	movb d0, (0x800e)
	movb (0x0f, a0), d0
	movb d0, (0x800f)
*/