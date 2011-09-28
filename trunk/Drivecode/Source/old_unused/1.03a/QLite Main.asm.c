/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«                         QLite Main.asm.c                                »©
  ©«                                                                         »©
  ©«                Copyright 2005 Anonymous Ghostwriter                     »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Description]....:  QLite Gamecube DVD Drivecode 0.93 [RC2b]            »©
  ©«                                                                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Notes......]....:                                                      »©
  ©«                                                                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [History....]....:  [bc]-13.10.2005 file created                        »©
  ©«                                                                         »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/

#----------------------------------------------------
# qCode custom commands
#----------------------------------------------------
.equ CMD_DISABLEAUDIOFIX,	0x21
.equ CMD_DISABLEDREFIX,		0x22
.equ CMD_TOGGLE_ECMA,		0x23
.equ CMD_SETREADSTATUS,		0x24
.equ CMD_REQUESTCREDITS,	0x25
.equ CMD_DBGREAD,			0xFE

.include "source/QLite.h"
.include "source/QliteGlobal.h"

.global		_start
.section	.text

#==============================================================================================
# Main Entrypoint:
#==============================================================================================
		
_start:	jmp Qcode_Init							# sets up everything
v_irqh:	jmp main_irq_handler					# fixed address for the irqhandler

	#---------------------------------------------------------------------------
	# data section
	#---------------------------------------------------------------------------
	dwUsrSectorOffset:			.long 0x0c
	bConfig:					.byte 0x00		# 0: Disable ECMA Patch | 1: Disable AudioFix 
	bGameRegion:				.byte 0x00		# 2: disable dreFix		| 3: regionpatch done
	wCBDBackup:					.word 0x00		# used to detect new CDBs

 Qcode_Init:

#	and 0xf7ff, psw

 1:	#------------------------------------------------------------------#
	#- scan ourself and patch all fw addresses to the correct version -#
	#------------------------------------------------------------------#
	mov PatchStart, a0
	mov qMainCodeSize, d0
	jsr SUB_PatchFwAddresses

 PatchStart:
	jsr SUB_ClearDataSegs

	#---------------------------------------------#
	#- replace the current irq handler with ours -#
	#---------------------------------------------#
	mov	v_irqh, a0
	mov	a0, (FwIrqVec)

	mov 0x01, d0								# mediaopt byte
	mov d0, (FwMediaOpt)

	#--------------------------------------------------------------------------------
	# init breakpoints
	#--------------------------------------------------------------------------------
	sub d0, d0
	
	#=============================================
	 .if READ_ECMADVDS == 1
		mov	FwBP0Addr_ReadECMA, a0		
		mov	a0, (REG_ADB0)
		or ADB0_ON, d0
	 .endif
	#=============================================
	 .if AUTHPLUSR == 1	
		mov	FwBP1Addr_AuthPlusR, a0
		mov	a0, (REG_ADB1)
		or ADB1_ON, d0
	#=============================================
	 .elseif AUTHDMI == 1
		mov	FwBP1Addr_AuthDMI, a0		
		mov	a0, (REG_ADB1)
		or ADB1_ON, d0
	 .endif
	#=============================================
	movb d0, (REG_ADBCTRL)
	jsr SUB_DVDShortReset						# reset the drive silently
#END
	
	
#==============================================================================================
# Interrupt handler 
#==============================================================================================
main_irq_handler:

	#=====================================#
 	# check for  Breakpoint 0 or 1		  #
	#=====================================#
	tbnzAbs ADBCTRL, 0, BP0Handler
	tbnzAbs ADBCTRL, 1, BP1Handler

	mov	(FwTimer2), d0							# avoid spindown when drive idles for too long
	mov	d0, (FwIdleTimeout)

	mov 0x0c, d0								# patch error-byte
	mov d0, (FwErrByte)

	mov FwDriveState, a0						# clean protection flag
	mov 0x02, d0
	bset d0, (a0)

/*	mov FwDriveState, a0						# patch protection opt flags
	movb (-1, a0), d0							# clr b2(4/8) & b3(6) (3)
	mov 0xFFDB, d1
	and d1, d0
	mov 1, d0
	movb d0, (-1, a0)
*/

 	#----------------------------------------#
 	# force read error retries				 #
	#----------------------------------------#
	 .if DREFIX == 1
		.if DREFIXCFG == 1
			tbnzAbs bConfig, 2, noDREFix
		.endif
		sub d0, d0
		mov 0x40ec00, a0
		mov d0, (0x00, a0)
		movb d0, (0x09, a0)
		noDREFix:
	.endif

	mov FwCDB0, a0							# backup CDB byte and replace with fake value
	movbu (a0), d0							# to detect new commands
	movb d0, (wCBDBackup)					
	mov 0x77, d0
	movb d0, (a0)

	#-----------------------------#
 	# call the original handler   #
	#-----------------------------#
	jsrabsOrg FwIntHandler

	#==================================================================#
 	# Check for new command											   #
	#==================================================================#
	mov FwCDB0, a0
	movbu (a0), d0
	cmp 0x77, d0
	bne SUB_OnNewCDBRecieved

	movbu (wCBDBackup), d0					# restore original value
	movb d0, (a0)
 	rts

 
 /*=========================================================================
 * 'generic' Breakpoint handler
 * --------------
 * one handler used for both adb registers. determines breakpoint 
 * address handles stack issues and calls the appropriate patch code
 *=========================================================================*/

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

	#----------------------------------------------------------------------
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

 PatchStack:
	mov	a3, a0								# point a0 to the prev stack pointer
	add	4, a0

	movbu	(FwIrqDepth), d0				# handle special case when FwIrqDepth == 0
	cmp	1, d0
	bne	1f
	mov	(4, a3), a0
1:	mov	a1, (0x20, a0)						# patch ret address to our code replacement

	#=================================
	# check for original disk
	#=================================
	mov	(0x34, a0), a1						
	cmp	FwOrgDiskCheckAddr, a1
	bne noOrgDisc
	bitset bConfig, 0						# set orgdisc flag
	mov 0x06, a0							# adjust user sector offset
	mov a0, (dwUsrSectorOffset)

 noOrgDisc:
	rts


/***********************************************************************/
 SUB_OnNewCDBRecieved:
/***********************************************************************/

	#==================================================================#
 	# Check for our custom commands									   #
	#==================================================================#
	cmp CMD_DBGREAD, d0						# DBG command ?
	bne noDbgCommand					
	
	movbu (2, a0), d1						# a0 = FwCDB0				
	cmp 0x01, d1							# write mem dbg command ?
	bne 1f									# -> we better flee secretly ;)
	jsr SUB_UnloadQcode
 1:	rts

 noDbgCommand:

	#================================================================
	cmp CMD_DISABLEAUDIOFIX, d0				# set noaudiofix flag
	bne 1f
	bitset bConfig, 1
	bra patchNiceCMD			

	#================================================================
	.if DREFIXCFG == 1
 1:		cmp CMD_DISABLEDREFIX, d0			# disable drefix
		bne 1f
		bitset bConfig, 2
		jsr SUB_UndoReadOptions
		bra patchNiceCMD
	.endif
	#================================================================
	 .if CREDITS == 1
 1:		cmp CMD_REQUESTCREDITS, d0			# load credits
		bne 1f
		jsr patchNiceCMD
		jsr SUB_UnloadQcode					# fade away
		mov 0x4444, d0						# set credits request tag
		mov d0, (0x40d100)
		P81_HI								# fake reset strobe
		nop									# to engage the chip
		P81_LO
		rts
	 .endif
	#================================================================
	 
 patchNiceCMD:
	mov 0xE0, d0					# bad boi!
	movb d0, (FwCDB0)
 1:	rts



/***********************************************************************/
 SUB_UnloadQcode:
/***********************************************************************/
	#================================================================
	 .if CREDITS == 1
		and 0xf7ff, psw

		sub d0, d0							
		mov d0, (REG_ADBCTRL)				# clear breakpoints
		mov 0x80a74, a0						# restore intvector
		mov a0, (0x804c)
		mov d0, (0x40ec60)					# disable HLELog buffer
		
		mov 0x01, d0
		mov d0, (FwDbgstate)

	 .endif 
	#================================================================
	rts
	
/***********************************************************************/
 SUB_PatchFwAddresses:				# (a0: code d0: length)
/***********************************************************************/
	mov d0, a1
	add a0, a1

	sub d1, d1								# check drive version
	movbu (0x80812), d0						# bc4: 6, 12, 10	e8e4 0, 10, 28
	cmp 0x9c, d0
	beq drive6
	cmp 0x57, d0
	beq drive8
	bra AddressLoop
 drive6: 
	mov 3, d1
	bra AddressLoop
 drive8: 
	mov 6, d1
 AddressLoop:	
	movbu (a0), d0							# search for address import tag (0x3333)
	cmp 0x33, d0
	bne noOpcode
	movbu (1, a0), d0
	cmp 0x33, d0
	bne noOpcode

	movbu (2, a0), d0						# get import 'ordinal'
	sub 1, d0

	mov 9, d2								# point to IAT[ordinal*9]
	mulu d2, d0
	add d1, d0								# add version offset

	mov FwImportAddressTable, a2
	add d0, a2
	movbu (a2), d0							# get import address
	movb d0, (a0)							# patch into opcode	
	movbu (1, a2), d0						
	movb d0, (1, a0)						
	movbu (2, a2), d0						
	movb d0, (2, a0)						

 noOpcode:
	add 1, a0
	cmp a1, a0
	bls AddressLoop
	rts
 #END

/***********************************************************************/
 SUB_ClearDataSegs:
/***********************************************************************/
		mov	FwData1, a0						# copy initialized data section 
		mov	FwData1_Source, d1
		mov	FwData1_Size, d0
		jsrabs FwMemcpy

		mov	FwBSS, a0						# clear the bss section
		sub	d0, d0
		mov FwBSS_Size, d1
		jsrabs FwMemset
		rts
#END

/***********************************************************************/
 SUB_Memcpy:			# a0: dst, a1: src, d0: length					
/***********************************************************************/
		and 0xFFFE, d0						# assert even number of bytes
  MemcpyLoop:
		mov (a1), d1
		mov d1, (a0)
		add 2, a0
		add 2, a1
		sub 2, d0
		cmp 0, d0
		bne MemcpyLoop
		rts
		
#*********************************************************************
 SUB_DVDShortReset:
#*********************************************************************
		mov 0x21, d0
		movb d0, (0x400012)
		mov 0x02, d0
		movb d0, (0x400013)
		jmpabs FwReset						# 2 fwreset
#END

#===================================================================================#
#			Breakpoints 0  and 1                                                    #
#===================================================================================#
.include "source/QLiteBreakpoints.asm.c"

#===================================================================================#
#			Various debug functions													#
#===================================================================================#
.if DBGMODE == 1
 .include "source/QLiteDbg.asm.c"
.endif

.align 2
#======================================================================================
 FwImportAddressTable:				/*	[04]	[06]	[08]	*/
#======================================================================================
FwAddrImp16 FwDbgstate					0x8192, 0x8186, 0x818e
FwAddrImp16 FwBSS_Size					0x01b8,	0x01ac, 0x01b6
FwAddrImp16 FwTimer2					0x8084, 0x8078, 0x8080
FwAddrImp16 FwIdleTimeout				0x819c, 0x8190, 0x8198
FwAddrImp16 FwErrByte					0x819a, 0x818e, 0x8196
FwAddrImp16 FwCmdState1					0x81b0, 0x81a4, 0x81ac
FwAddrImp16 FwCmdState2					0x8194, 0x8188, 0x8190
FwAddrImp16 FwReadLBA					0x8102, 0x80f6, 0x80fe
FwAddrImp16 FwCDB0						0x80b4,	0x80a8, 0x80b0
FwAddrImp16 FwMediaOpt					0x8090,	0x8084, 0x808c
FwAddrImp16 FwFastBootMask				0xfffb,	0xfff7, 0xfffb
#FwAddrImp16 FwReadCfg					0x808e,	0x8082, 0x808a

FwAddrImp24 FwMemcpy					0x082f27, 0x082f2e, 0x082f27
FwAddrImp24 FwMemset					0x082f49, 0x082f50, 0x082f49
FwAddrImp24 FwOrgDiskCheckAddr			0x0885c1, 0x0885b9, 0x0885c1
FwAddrImp24 FwReset						0x08d47e, 0x08d41e, 0x08d4d9
FwAddrImp24 FwData1_Source				0x08ea2a, 0x08e9c8, 0x08ea7c
FwAddrImp24 FwDriveState				0x40ecf9, 0x40ed02, 0x40ecf5
FwAddrImp24 FwSetReadOptions			0x083977, 0x08396F, 0x083977
#FwAddrImp24 FwSendCtrlCommand			0x08ae09, 0x08b126, 0x08ae56
 

FwAddrImp24 FwBP0Addr_ReadECMA			0x08ae28, 0x08b145, 0x08ae75
FwAddrImp24 FwBP0Addr_ReadECMA_Exit		0x08ae33, 0x08b150, 0x08ae80	
FwAddrImp24 FwBP1Addr_AuthPlusR			0x089d70, 0x089df2, 0x089db6
FwAddrImp24 FwBP1Addr_AuthPlusR_Exit	0x089d70, 0x089df2, 0x089db6		
FwAddrImp24 FwBP1Addr_AuthDMI			0x08a7db, 0x08aaf0, 0x08a828

.align 2
#===============================================
 Apploader_patch:
#===============================================
	.incbin "bin/iplboot.bin"
 Apploader_patch_end:
	

.equ Applpatch_size, (Apploader_patch_end - Apploader_patch) + 0x20
.equ qMainCodeSize, Apploader_patch-PatchStart
