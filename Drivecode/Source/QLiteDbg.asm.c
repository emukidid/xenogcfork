

.equ DELAY_BIT,			5
.equ DELAY_BYTE,		30

 #==================================================================================#
 # Local adresses																	#
 #==================================================================================#
.equ FwCurrentLoginState, 0x40ead2

 #==================================================================================#
 # Pinout toggling																	#
 #==================================================================================#
 .equ PINOUT, 1

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

 #==========================================================#
 # Macros													#
 #==========================================================#
.macro Sleep ms
	.if \ms > 0
		mov \ms, d0
		jsr SUB_Sleep
	.endif
.endm


 #==============================================#
 # Debug data									#
 #==============================================#
	bLastTime:			.byte 0x00
	bLastLoginByte:		.byte 0xff
					
 #==========================================================#
 # Debug init												#
 #==========================================================#
 DBGInit:
	# enable dbgcommands	
	mov 0x01, d0				
	movb d0, (FwDbgstate)
	jsr Sub_FlashInitPorts
	rts

 #==================================================================================#
 # Cyclic Debug update function														#
 #==================================================================================#
 DBGUpdate:
#	jsr SUB_LogDiscCheckState
#	jsr SUB_DbgUpdateTimer
	jsr SUB_UpdateSerialOut
	rts

 #==================================================================================#
 # called every 8th timer2 tick														#
 #==================================================================================#
 DbgTimerSlowCallback:
	mov 0x99, d0
	jsr FwCLI
	jsr Sub_SendByteSerial
	jsr FwSTI

/*	jsr SUB_LogDiscCheckState
	movbu (0x40ead2), d0
	cmp 0x50, d0
	bne noLS
noLS:
*/
	rts

/***********************************************************************/
 SUB_LogDiscCheckState:
/***********************************************************************/
	jsr FwCLI
	movbu (bLastLoginByte), d1
	movbu (0x40ead2), d0					# 0x8069
	movb d0, (bLastLoginByte)
	cmp d0, d1
	beq noLoginByte

	mov 0x21, d0
	jsr SUB_DbgOutByte
#	jsr Sub_SendByteSerial

 noLoginByte:
	jsr FwSTI
	rts

#END

/***********************************************************************/
 SUB_DbgUpdateTimer:
/***********************************************************************/
	mov FwTimer2,	a0
	movbu (0, a0),	d1
	and 0xFE, d1
	
	movbu (bLastTime), d0
	sub d1, d0
	movb d1, (bLastTime)
	cmp 0, d0
	beq noTick
	jsr DbgTimerSlowCallback
 noTick:
	rts
		

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
	P83_HI								# SBI0	(signal)
	P84_LO								# SBO0	(data)

	mov 0x0, d1							# disable serial port
	mov d1, (SC0CTR)
	mov 0x00, d0						# set both data and clock pins low
	movb d0, (P8OUT)	

	sub d0, d0
	mov d0, (0x40ec60)					# disable HLELog buffer
	rts

/***********************************************************************/
 Sub_SendByteSerial:		# (d0 = byte)
/***********************************************************************/
.if DBGOUTPUT == 1
		P83_LO				# set signal bit
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
.endif
#		DATA_LO
		Sleep DELAY_BYTE	# take a nap :p
		P83_HI				# clear signal bit
		rts
#END


/***********************************************************************/
 SUB_DbgOutByte:		# (d0: byte)
/***********************************************************************/
		sub 0x0a, a3				# push used regs
		mov d1, (0x00,a3)
		mov d2, (0x02,a3)
		mov d0, (0x04,a3)
		mov a0, (0x06,a3)

#		jsr FwCLI
		xor 0x49, d0

		mov DbgDumpStart, a0		# point a0 to ringbuffer write offset
		movb (DbgWritePtr), d1
		add d1, a0
		
		movb (DbgReadPtr), d2		
		add 1, d1					
		and 0xff, d1
	
		cmp d2, d1					# write ptr +1 = read ptr ?
		bne NoDbgBufferFull

		mov 0x44, d0				# write DD as buffer full sign
		movb d0, (a0)				# and dont log the byte
		bra DbgOutEnd

 NoDbgBufferFull:
		movb d0, (a0)				# write byte
		movb d1, (DbgWritePtr)		# inc write ptr

 DbgOutEnd:
#		jsr FwSTI
		mov (0x00,a3), d1			# pop used regs
		mov (0x02,a3), d2
		mov (0x04,a3), d0
		mov (0x06,a3), a0
		add 0x0a, a3
		rts


/***********************************************************************/
 SUB_DbgOutBuffer:		# (a0: address d0: bytes)
/***********************************************************************/
.if DBGOUTPUT == 1
		sub 0x06, a3					# push used regs
		mov a0, (0x00,a3)
		mov d1, (0x04,a3)
		mov d0, d1						# d1 - counter
		
		jsr FwCLI
#		mov 0xaa, d0
#		jsr SUB_DbgOutByte

 DbgOutBufferLoop:

		movbu (a0), d0
		jsr SUB_DbgOutByte

		add 1, a0
		sub 1, d1
		cmp 0, d1
		bne DbgOutBufferLoop

		mov 0xba, d0
		jsr SUB_DbgOutByte

		jsr FwSTI
		mov (0x00,a3), a0				# pop used regs
		mov (0x04,a3), d1
		add 0x06, a3
.endif
		rts
#END


/***********************************************************************/
 SUB_UpdateSerialOut:
/***********************************************************************/

	jsr FwCLI
	movb (DbgReadPtr), d1			# get read and write ptrs
	movb (DbgWritePtr), d2			
	mov DbgDumpStart, a0			# point a0 to ringbuffer read offset
	
	add d1, a0
	cmp d1, d2						# has data ?
	beq NoSendData

#	jsr FwCLI
	nop	
	nop	
	
	add 1, d1						# inc read ptr
	movb d1, (DbgReadPtr)
	movbu (a0), d0					# send data byte
	jsr Sub_SendByteSerial

/*	movb (DbgReadPtr), d1			# get read and write ptrs
	movb (DbgWritePtr), d2			
	cmp d1, d2
	bne noSleep
	Sleep 50
*/
 noSleep:
#	jsr FwSTI

 NoSendData:
	jsr FwSTI
	rts
#END 

/***********************************************************************/
 SUB_Sleep:			# (dO = DELAY in ~ msec)
/***********************************************************************/
 delayLoop1:
		mov	0x10, d1
 delayLoop2:
		sub	1, d1
		bne	delayLoop2
		sub	1, d0
		bne	delayLoop1
		rts	
#END
		


/***********************************************************************/
 SUB_DbgTest:
/***********************************************************************/

/*		jsrabsOrg FwCLI
			
		jsr Sub_FlashInitPorts
		Sleep 100
		jsr SUB_FlashEnable
		jsr SUB_FlashErase

		sub 4, a3
		mov 0x0000, a2
	flashloop:
		mov a2, d1
		mov d1, d0 

		mov 0x2BAD, d0

		mov d1, (a3)					# address	(sp)
		mov d0, (2, a3)					# data		(sp+2)

		jsr SUB_WriteFlashWord
		add 1, a2
		cmp 0x200, a2
		bne flashloop

		add 4, a3
		jsrabsOrg FwSTI
		rts
*/
/***********************************************************************/
 SUB_WaitForP80Low:
/***********************************************************************/
/*	and	0xf7ff,psw
	nop	
	nop	
	mov 1, d0
	movb d0, (P8OUT)
	
waitForPort:
	movb (P8IN), d0
	and 1, d0
	bne waitForPort
	mov 1, d0
	movb d0, (P8OUT)
	or	0x0800, psw
	rts
#END	
*/

/***********************************************************************/
 Sub_FlashLED_P80:			/* blue */									#
/***********************************************************************/
/*		sub 0x08, a3					# push regs
		mov d0, (0x00,a3)
		mov d1, (0x04,a3)
		and	0xf7ff, psw
		nop
		nop

		P80_HI
		mov 0x100, d0 
		jsr SUB_Sleep
		P80_LO

		or	0x0800, psw					# pop regs
		mov (0x00,a3), d0
		mov (0x04,a3), d1
		add 0x08, a3
		rts						
#END
*/
/***********************************************************************/
 Sub_FlashLED_P81:			/* red */									#
/***********************************************************************/
/*		and	0xf7ff, psw

		P81_HI
		mov 0x90, d0 
		jsr SUB_Sleep
		P81_LO

		or	0x0800, psw
		rts						
#END
*/

/***********************************************************************/
 Sub_FlashCover:			/* red */									#
/***********************************************************************/
#		and	0xf7ff, psw

		mov 0x20, d0
		movb d0, (P4OUT)
		Sleep 0x90

		sub d0, d0
		movb d0, (P4OUT)

#		or	0x0800, psw
		rts						
#END

