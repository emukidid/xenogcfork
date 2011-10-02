/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«                         flashloader.asm.c                               »©
  ©«                                                                         »©
  ©«                Copyright 2006 Anonymous Ghostwriter                     »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Description]....:  QLite DVD flashing 0.1                              »©
  ©«                                                                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Notes......]....:                                                      »©
  ©«                                                                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [History....]....:                                                      »©
  ©«                                                                         »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/

#----------------------------------------------------
# qCode custom commands
#----------------------------------------------------
.equ CMD_INITFLASH,		0x0026
.equ CMD_ERASEFLASH,	0x0027
.equ CMD_WRITEFLASH,	0x0028

.equ DELAY_BIT,			1
.equ DELAY_BYTE,		0
.equ DELAY_WRITEFLASH,	5
.equ DELAY_INIT,		100
.equ DELAY_ERASE,		1000


.equ CHUNKSIZE,			0x200	
.equ WRITEBUFFER,		0x0040bbc0			# 0x0040bbc0  # 0x0040d800

.set READCHUNK,			0x200
.set READBUFFER,		0x40d800


.include "QLite.h"
.include "QliteGlobal.h"

.global		_start
.section	.text


 #==========================================================#
 # Macros													#
 #==========================================================#
.macro Sleep ms
	.if \ms > 0
		mov \ms, d0
		jsr SUB_Sleep
	.endif
.endm

 #==================================================================================#
 # Local adresses																	#
 #==================================================================================#
.equ FwCurrentLoginState, 0x40ead2


 #==================================================================================#
 # Pinout toggling																	#
 #==================================================================================#
 .equ PINOUT, 2

.if PINOUT == 1
	.macro CLOCK_LO
		P84_LO
	.endm
	.macro CLOCK_HI
		P84_HI
	.endm
	.macro DATA_LO
		P82_LO
	.endm
	.macro DATA_HI
		P82_HI
	.endm
.else
	.macro CLOCK_LO
		P82_LO
	.endm
	.macro CLOCK_HI
		P82_HI
	.endm
	.macro DATA_LO
		P84_LO
	.endm
	.macro DATA_HI
		P84_HI
	.endm
.endif

#==============================================================================================
# Main Entrypoint:
#==============================================================================================
		
_start:	jmp Qcode_Init							# sets up everything
v_irqh:	jmp main_irq_handler					# fixed address for the irqhandler

	jmp SUB_FlashErase
	jmp SUB_FlashEnable
	jmp SUB_WriteFlashBlock						
	jmp SUB_UnloadQcode
	jmp SUB_ReadFlashBlock
#	jmp SUB_ReadFuseBits						
#	jmp SUB_WriteFuseBits						

	#---------------------------------------------------------------------------
	# data section
	#---------------------------------------------------------------------------
	wCBDBackup:					.word 0x00		# used to detect new CDBs

 Qcode_Init:

	and 0xf7ff, psw

	sub d0, d0									# clear breakpoints and
	movb d0, (0xfcda)							# restore original inthandler
	mov FwIntHandler, a0						
	mov	a0, (FwIrqVec)

 1:	#------------------------------------------------------------------#
	#- scan ourself and patch all fw addresses to the correct version -#
	#------------------------------------------------------------------#
	mov PatchStart, a0
	mov qMainCodeSize, d0
	jsr SUB_PatchFwAddresses

 PatchStart:
	jsr SUB_ClearDataSegs

	#---------------------------------------------#
	#- Setup ports for in circuit flashing       -#
	#---------------------------------------------#
	jsr Sub_FlashInitPorts
	bra SUB_StartDriveReset				# reset the drive silently
#END

	
#==============================================================================================
# Interrupt handler 
#==============================================================================================
main_irq_handler:


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
		mov 1, d0
		mov d0, (FwInitState1)
		mov d0, (FwInitState2)

		jsrabs FwSetInitState
		jsrabs FwInitSpinup				# SUB_Init_Spinup
		jmpabs FwContinueReset			# continue with dvdreset


 #==========================================================#
  Sub_FlashInitPorts:										#
 #==========================================================#
	mov 0x1F, d0						# set P80-84 as outputs
	movb d0,(P8DIR)						

	mov 0x00, d0						# set P80-82 as ports
	movb d0, (P8LMD)		

	mov 0x20, d0						# P83: Port (org SBI)
	movb d0, (P8MMD)					# P84: Port (org SBO)		0x2d
										
	P82_LO								# SBT0	(clock)	(was both hi)
	P83_LO								# SBI0	(signal)
	P84_LO								# SBO0	(data)

	mov 0x0, d1							# disable serial port
	mov d1, (SC0CTR)
	mov 0x00, d0						# set both data and clock pins low
	movb d0, (P8OUT)	

	sub d0, d0
	mov d0, (0x40ec60)					# disable HLELog buffer
	rts


/***********************************************************************/
 Sub_SendByteSerialX:		# (d0 = byte)
/***********************************************************************/
		mov d0, d3
		mov 8, d2			# loop 8 times
  bitloop:
		mov d3, d0			# get next databit
		and 0x80, d0		# set it to P84
		beq 1f				
		DATA_HI
		bra 2f
	1:	DATA_LO
	2:	CLOCK_HI			# set clock
		Sleep DELAY_BIT		# take a nap :p
		CLOCK_LO			# clear clock

		rol d3				# rotate input bits
		sub 01, d2
		bne bitloop

		DATA_LO
		Sleep DELAY_BYTE	# take a nap :p
		rts
#END

#		tbzAbs P8IN, 3, inZero		# get next databit

/***********************************************************************/
 Sub_GetByteSerialX:		# (d0 = byte)
/***********************************************************************/
		sub d3, d3
		mov 8, d2			# loop 8 times

	bitloopIn:
		rol d3				# rotate input bits

		CLOCK_HI			# set clock
		Sleep DELAY_BIT		# take a nap :p

		movbu (P8IN), d0
		and 0x08, d0
		beq inZero
		or 0x01, d3
	inZero:
		CLOCK_LO			# clear clock

		sub 01, d2
		bne bitloopIn

		Sleep DELAY_BYTE	# take a nap :p
		mov d3, d0
		rts
#END


/***********************************************************************/
 SUB_FlashEnable:
/***********************************************************************/
		jsr FwCLI
		jsr Sub_FlashInitPorts
		mov 0xAC, d0					
		jsr Sub_SendByteSerialX
		mov 0x53, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX		
		Sleep DELAY_INIT
		jsr FwSTI
		rts

/***********************************************************************/
 SUB_FlashErase:
/***********************************************************************/
		jsr FwCLI
		mov 0xAC, d0
		jsr Sub_SendByteSerialX
		mov 0x80, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX
		Sleep DELAY_ERASE			
		jsr FwSTI
		rts
		
/***********************************************************************/
 SUB_WriteFlashWord:				# d0: word	d1: address
/***********************************************************************/
		mov 0x40, d0					# WRITE_PROG_LO
		jsr Sub_SendByteSerialX
		movbu (5, a3), d0				# addr_hi
		jsr Sub_SendByteSerialX
		movbu (4, a3), d0				# addr_lo
		jsr Sub_SendByteSerialX
		movb (6, a3), d0				# data_lo
		jsr Sub_SendByteSerialX
				
		mov 0x48, d0					# WRITE_PROG_HI
		jsr Sub_SendByteSerialX
		movbu (5, a3), d0				# addr_hi
		jsr Sub_SendByteSerialX
		movbu (4, a3), d0				# addr_lo
		jsr Sub_SendByteSerialX
		movbu (7, a3), d0				# data_hi
		jsr Sub_SendByteSerialX

		#--------------------------------------------------------#
		#- one page (32*16bit) uploaded ? -> write flash page	-#
		#--------------------------------------------------------#
		mov (4, a3), d0					# check flash address
		and 0x1f, d0
		cmp 0x1f, d0
		beq writePage
		rts

	#------------------------------
	 writePage:
	#------------------------------
		mov 0x4C, d0					# WRITE_PAGE
		jsr Sub_SendByteSerialX
		movbu (5, a3), d0				# addr_hi
		jsr Sub_SendByteSerialX
		movbu (4, a3), d0				# addr_lo & 0xE0 = page address
		and 0xE0, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX
		Sleep DELAY_WRITEFLASH			# take a nap :p
		rts

/******************************************************************************/
 SUB_WriteFlashBlock:					# d0 -> (flash dest address / 2)
/******************************************************************************/
		jsr FwCLI
		sub 8, a3
		mov a2, (4, a3)						# push a2
		mov WRITEBUFFER, a2					# a2-> ptr to src data

		mov (WRITEBUFFER-2), d0				# get target flash address
		mov d0, (a3)						# set flash address	(sp)

	WriteFlash:
		mov (a2), d0						# get next data word
		mov d0, (2, a3)						# and store it at (sp+2)

		jsr SUB_WriteFlashWord				# write word

		add 2, a2							# increment source address
		mov (a3), d0						# increment flash address (sp)
		add 1, d0
		mov d0, (a3)

		cmp WRITEBUFFER+CHUNKSIZE, a2	# block done ?
		bne WriteFlash

		mov (4, a3), a2						# pop a2
		add 8, a3
		jsr FwSTI
		rts


/***********************************************************************/
 SUB_ReadFlashWord:				# d0: word	d1: address
/***********************************************************************/
		mov 0x20, d0					# WRITE_PROG_LO
		jsr Sub_SendByteSerialX
		movbu (5, a3), d0				# addr_hi
		jsr Sub_SendByteSerialX
		movbu (4, a3), d0				# addr_lo
		jsr Sub_SendByteSerialX
		
		jsr Sub_GetByteSerialX			# get # data_lo
		movb d0, (6, a3)
				
		mov 0x28, d0					# WRITE_PROG_HI
		jsr Sub_SendByteSerialX
		movbu (5, a3), d0				# addr_hi
		jsr Sub_SendByteSerialX
		movbu (4, a3), d0				# addr_lo
		jsr Sub_SendByteSerialX

		jsr Sub_GetByteSerialX			# get # data_hi
		movb d0, (7, a3)
		rts


/******************************************************************************/
 SUB_ReadFlashBlock:						# d0 -> (flash dest address / 2)
/******************************************************************************/

		mov	READBUFFER, a0					# clear the readbuffer
		mov 0x21, d0
		mov READCHUNK, d1
		jsrabs FwMemset

		jsr FwCLI
		sub 8, a3
		mov a2, (4, a3)						# push a2
		mov READBUFFER, a2					# a2-> ptr to dest buffer

		mov (WRITEBUFFER-2), d0				# get target flash address
		mov d0, (a3)						# set flash address	(sp)

	ReadFlash:
		jsr SUB_ReadFlashWord				# read word
		mov (2, a3), d0						# get word
		mov d0, (a2)						# write to buffer

		add 2, a2							# increment source address
		mov (a3), d0						# increment flash address (sp)
		add 1, d0
		mov d0, (a3)

		cmp READBUFFER+READCHUNK, a2		# block done ?
		bne ReadFlash

		mov (4, a3), a2						# pop a2
		add 8, a3
		jsr FwSTI
		rts

.ifdef NIXDA

/***********************************************************************/
 SUB_ReadFuseBits:
/***********************************************************************/
		sub a0, a0
		mov a0, (0x40d800)

		mov 0x50, d0					# READ_FUSE_LOW
		jsr Sub_SendByteSerialX
		mov 0x00, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX

		jsr Sub_GetByteSerialX			# get data_lo
		movb d0, (0x40D800)

		mov 0x58, d0					# READ_FUSE_HI
		jsr Sub_SendByteSerialX
		mov 0x08, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX

		jsr Sub_GetByteSerialX			# get data_hi
		movb d0, (0x40D801)
		rts

/***********************************************************************/
 SUB_WriteFuseBits:
/***********************************************************************/
		mov 0xAC, d0					# WRITE_FUSE_LOW
		jsr Sub_SendByteSerialX
		mov 0xA0, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX
		mov 0xE4, d0
		jsr Sub_SendByteSerialX

		mov 0xAC, d0					# WRITE_FUSE_HI
		jsr Sub_SendByteSerialX
		mov 0xA8, d0
		jsr Sub_SendByteSerialX
		mov 0xFF, d0
		jsr Sub_SendByteSerialX
		mov 0xD9, d0
		jsr Sub_SendByteSerialX
		rts
.endif

/***********************************************************************/
 SUB_UnloadQcode:
/***********************************************************************/
	#================================================================
#		and 0xf7ff, psw
		sub d0, d0							
		movb d0, (REG_ADBCTRL)				# clear breakpoints
		mov 0x80a74, a0						# restore intvector
		mov a0, (0x804c)
		mov d0, (0x40ec60)					# disable HLELog buffer
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


/***********************************************************************/
 SUB_Sleep:			# (dO = DELAY in ~ msec)
/***********************************************************************/
 delayLoop1:
		mov	0x0d, d1
 delayLoop2:
		sub	1, d1
		bne	delayLoop2
		sub	1, d0
		bne	delayLoop1
		rts	
#END

/***********************************************************************/
# Safe CLI/STI routines
/***********************************************************************/
FwCLI:
	and 0xf7ff, psw
	rts

FwSTI:
	mov	0xa3,d0			
	mov	d0, (0x806c)
	movbu (0x805b), d0
	cmp	0,d0			
	beq	1f
	mov	0x01,d0			
	rts
 1: sub d0, d0
	rts

		

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

FwAddrImp24 FwMemcpy					0x082f27, 0x082f2e, 0x082f27
FwAddrImp24 FwMemset					0x082f49, 0x082f50, 0x082f49
FwAddrImp24 FwOrgDiskCheckAddr			0x0885c1, 0x0885b9, 0x0885c1
FwAddrImp24 FwReset						0x08d47e, 0x08d41e, 0x08d4d9
FwAddrImp24 FwData1_Source				0x08ea2a, 0x08e9c8, 0x08ea7c
FwAddrImp24 FwDriveState				0x40ecf9, 0x40ed02, 0x40ecf5
FwAddrImp24 FwSetReadOptions			0x083977, 0x08396F, 0x083977
#FwAddrImp24 FwSendCtrlCommand			0x08ae09, 0x08b126, 0x08ae56

FwAddrImp16 FwInitState1				0x807D,	0x8071, 0x8079
FwAddrImp16 FwInitState2				0x8082, 0x8076, 0x807e
FwAddrImp16 FwLastHLECmd				0x80f4, 0x80e8, 0x80f0
FwAddrImp24 FwInitDriveLow				0x08aed8, 0x08b1f5, 0x08af25
FwAddrImp24 FwInitSpinup				0x083bad, 0x083ba5, 0x083bad
FwAddrImp24 FwStartDrive				0x0878f8, 0x0878f0, 0x0878f8
FwAddrImp24 FwSetInitState				0x08aee8, 0x08b205, 0x08af35
FwAddrImp24 FwContinueReset				0x08d9b6, 0x08d956, 0x08da11
FwAddrImp24 FwInitThread				0x08dd0d, 0x08dcad, 0x08dd68
FwAddrImp24 FwThreadSetup				0x08ea03, 0x08e9a1, 0x08ea55

#FwAddrImp24 FwCLI						0x08d1c4, 0x08d164, 0x08d21f
#FwAddrImp24 FwSTI						0x08d19c, 0x08d169, 0x08d224


.align 2

 Apploader_patch_end:

.equ qMainCodeSize, Apploader_patch_end-PatchStart
