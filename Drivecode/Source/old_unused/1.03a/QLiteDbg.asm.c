

 #==============================================#
 # Debug data									#
 #==============================================#
	bLastTime:			.byte 0x00
	bLastLoginByte:		.byte 0x99


 #==========================================================#
 # Debug init												#
 #==========================================================#
 DBGInit:

	# enable dbgcommands	
	mov 0x01, d0				
	movb d0, (FwDbgstate)		

	# enable rx and tx
	mov 0x8080, d1
	mov d1, (SC0CTR)
	rts

 #==================================================================================#
 # Cyclic Debug update function														#
 #==================================================================================#
 DBGUpdate:

	mov 0x01, d0				
	movb d0, (FwDbgstate)					# enable dbgcommands	
	jsr SUB_LogDiscCheckState
	rts
	

/***********************************************************************/
 SUB_DbgOnTimerSlow:
/***********************************************************************/
	rts

/***********************************************************************/
 SUB_LogDiscCheckState:
/***********************************************************************/

	CLI
	movbu (bLastLoginByte), d1
	movbu (0x40ead2), d0					# 0x8069
	movb d0, (bLastLoginByte)
	STI

	cmp d0, d1
	beq noLoginByte
	jsr Sub_SendByteSerial
	
/*	movbu (bLastLoginByte), d0
	add 1, d0
	movb d0, (bLastLoginByte)
*/

 noLoginByte:
	rts

#END


/***********************************************************************/
 Sub_SendByteSerial:		# (d0 = byte)
/***********************************************************************/
	CLI
	# enable rx and tx
	mov 0x8080, d1
	mov d1, (SC0CTR)
#	P80_HI								# request to send
#1:	tbnzAbs SC0STR, 7, 1b				# wait until ready for send
	movb d0, (SC0TRB)					# send
#	P80_LO		
	STI

	mov 10, d0
	jsr SUB_Sleep

	rts


/***********************************************************************/
 SUB_WaitForP80Low:
/***********************************************************************/
	and	0xf7ff,psw
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

/***********************************************************************/
 SUB_UpdateSerialOut:
/***********************************************************************/
	movb (DbgReadPtr), d1			# get read and write ptrs
	movb (DbgWritePtr), d2			
	mov DbgDumpStart, a0			# point a0 to ringbuffer read offset
	
	add d1, a0
	cmp d1, d2						# has data ?
	beq NoSendData

	and	0xf7ff,psw
	nop	
	nop	
	
	add 1, d1						# inc read ptr
	movb d1, (DbgReadPtr)
	movbu (a0), d0					# send data byte

	jsr Sub_SendByteSerial
	or 0x0800, psw

 NoSendData:
	rts
#END 

/***********************************************************************/
 SUB_Sleep:			# (dO = DELAY in ~ msec)
/***********************************************************************/
 delayLoop1:
		mov	0x10,d1
 delayLoop2:
		sub	1, d1
		bne	delayLoop2
		sub	1, d0
		bne	delayLoop1
		rts	
#END
		
/***********************************************************************/
 Sub_FlashLED_P80:			/* blue */									#
/***********************************************************************/
		sub 0x08, a3					# push regs
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

/***********************************************************************/
 Sub_FlashLED_P81:			/* red */									#
/***********************************************************************/
		and	0xf7ff, psw

		P81_HI
		mov 0x90, d0 
		jsr SUB_Sleep
		P81_LO

		or	0x0800, psw
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

		and	0xf7ff, psw
		nop
		nop
		
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

		or	0x0800, psw
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

		mov 0xEF, d0
		jsr SUB_DbgOutByte

 DbgOutBufferLoop:

		movbu (a0), d0
		jsr SUB_DbgOutByte

		add 1, a0
		sub 1, d1
		cmp 0, d1
		bne DbgOutBufferLoop

		mov (0x00,a3), a0			# pop used regs
		mov (0x04,a3), d1
		add 0x06, a3
.endif
		rts
#END

