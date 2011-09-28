


/*##############################################################################*/
/*	Variables address defines													*/
/*##############################################################################*/
.set DbgWritePtr,		0x8004
.set DbgReadPtr,		0x8006
.set DbgDump,			0x40d800
.set DbgDumpStart,		0x40d800
.set DbgDumpSize,		0x40d800

/*##############################################################################*/
/*	Register address defines													*/
/*##############################################################################*/
.set	P0OUT,		0xffc0		## PortXOutputRegister
.set	P1OUT,		0xffc1
.set	P2OUT,		0xffc2
.set	P3OUT,		0xffc3
.set	P4OUT,		0xffc4
.set	P5OUT,		0xffc5
.set	P6OUT,		0xffc6
.set	P7OUT,		0xffc7
.set	P8OUT,		0xffC8		
.set	P9OUT,		0xffC9
.set	P0IN,		0xffd0		## PortXInputRegister
.set	P1IN,		0xffd1
.set	P2IN,		0xffd2
.set	P3IN,		0xffd3
.set	P4IN,		0xffd4
.set	P5IN,		0xffd5
.set	P6IN,		0xffd6
.set	P7IN,		0xffd7
.set	P8IN,		0xffd8		
.set	P9IN,		0xffd9		
.set	P1DIR,		0xffe1		## PortXInput/Output Dir Register
.set	P2DIR,		0xffe2
.set	P3DIR,		0xffe3
.set	P4DIR,		0xffe4
.set	P5DIR,		0xffe5
.set	P6DIR,		0xffe6
.set	P7DIR,		0xffe7
.set	P8DIR,		0xffe8		
.set	P9DIR,		0xffe9

.set	P8PLU,		0xffb8		## Port8Pull-upControlRegister
.set	P8LMD,		0xfffc		## Port8ModeRegister
.set	P8MMD,		0xfffd		## Port8ModeRegister
.set	P8HMD,		0xfffe		## Port8ModeRegister

	
.set	REG_IAGR,		0xfc0e
.set	REG_UNICR,		0xfc44
.set 	UNID,			(1<<0)

.set	REG_ADB0,		0xfcd2
.set	REG_ADB1,		0xfcd6
.set	REG_ADBCTRL,	0xfcda

.set 	ADB0ACK,		(0x01)
.set 	ADB1ACK,		(0x02)
.set 	ADB0_ON,		(0x04)
.set 	ADB1_ON,		(0x08)
.set 	ADB01_ON,		(ADB0_ON | ADB1_ON) 


.set	NMICR,			0xfc40

.set	REG_BSWAPL,		0xFFCC
.set	REG_BSWAPH,		0xFFCE

.set	REG_SWAP16READ,	0xFFCC
.set	REG_SWAP16WRITE,0xFFCE

.set	SC0CTR,			0xfd80
.set	SC0TRB,			0xfd82


.set	SC3CTR,			0xfd98
.set	SC3TRB,			0xfd9a


/*##############################################################################*/
/*	Macros																		*/
/*##############################################################################*/

.macro CLI
	and 0xf7ff, psw
	nop
	nop
.endm

.macro STI
	or 0x800, psw
.endm


/* helper macro for correct absolute 24bit jumps */
.macro jmpabsOrg addr			
	jmp \addr-.+\addr
.endm

.macro jmpabs addr		
	mov a0, (-8, a3)
	mov \addr, a0
	mov a0, (-4, a3)
	mov (-8, a3), a0
	add -4, a3
	rts
.endm

.macro jsrabs addr			# damn 0x14 bytes... help :s
	add -8, a3				#
	mov a0, (-4, a3)		# save a0
	mov .+0x11, a0			# push return address (here)
	mov a0, (4, a3)
	mov \addr, a0			# push return address (jsr dest)
	mov a0, (a3)
	mov (-4, a3), a0		# restore a0
	rts
.endm


/* helper macro for correct absolute 24bit jsrs */
.macro jsrabsOrg addr			
	jsr \addr-.+\addr
.endm

/* jumps to Addr by using the stack, without modifying registers */
.macro JmpSaveReg Addr
	sub 8, a3
	mov a1, (a3)
	mov \Addr, a1
	mov a1, (4, a3)
	mov (a3), a1
	add 4, a3
	rts
.endm

.macro IMM24Bit Addr
	.byte ((\Addr) & 0xFF)
	.byte (((\Addr) >> 8) & 0xFF)
	.byte ((\Addr) >> 16) & 0xFF)
.endm

.macro bitset Addr bit
	.byte 0xF3, 0xFE, 0xD0 + \bit
	.long (\Addr) + 0xF6000000		# ohne worte :p
.endm

.macro bitset16 address bit
	.byte 0xF5, 0xD0 + \bit
	.word \address
.endm

.macro bitclr address bit
	.byte 0xF5, 0xD8 + \bit
	.word \address
.endm

.macro tbzAbs addr, bit, label
	.byte 0xF3, 0xFE, (0xC0 + \bit)
	.set O\addr\bit\label, (\label-.)-4
	.long \addr + (O\addr\bit\label << 24)
.endm

.macro tbnzAbs addr, bit, label
	.byte 0xF3, 0xFE, (0xC8 + \bit)
	.set O\addr\bit\label, (\label-.)-4
	.long \addr + (O\addr\bit\label << 24)
.endm


.macro CallDCCommand CmdNum
	jsr \CmdNum * 4 + 0x40d000 - 4 -.+\CmdNum * 4 + 0x40d000 - 4
.endm


.macro DBGOut8 Value
	mov \Value, a0
	mov 1, d0
	jsr SUB_DbgOutBuffer
.endm

.macro DBGOutReg8 Reg
	mov \Reg, d0
	jsr SUB_DbgOutByte
.endm

.macro DBGOutReg16 Reg
	movb \Reg, d0
	jsr SUB_DbgOutByte
	ror d0
	ror d0
	ror d0
	ror d0
	ror d0
	ror d0
	ror d0
	ror d0
	jsr SUB_DbgOutByte
.endm



/*##############################################################################*/
/*	P80, P81 serial transfer macros												*/
/*##############################################################################*/


.macro P4IN_SET5
	.byte 0xF5, 0xD5, 0xD4, 0xFF
.endm

.macro P4IN_CLR5
	.byte 0xF5, 0xDD, 0xD4, 0xFF
.endm

.macro P80_HI
	.byte 0xF5, 0xD0, 0xC8 ,0xFF	# P80_HI
.endm

.macro P80_LO
	.byte 0xF5, 0xD8, 0xC8 ,0xFF	# P80_LO
.endm

.macro P81_HI
	.byte 0xF5, 0xD1, 0xC8 ,0xFF	# P81_HI
.endm

.macro P81_LO
	.byte 0xF5, 0xD9, 0xC8 ,0xFF	# P81_LO
.endm

.macro P82_HI
	.byte 0xF5, 0xD2, 0xC8 ,0xFF	
.endm

.macro P82_LO
	.byte 0xF5, 0xDa, 0xC8 ,0xFF	
.endm

.macro P83_HI
	.byte 0xF5, 0xD3, 0xC8 ,0xFF	
.endm

.macro P83_LO
	.byte 0xF5, 0xDB, 0xC8 ,0xFF	
.endm

.macro P84_HI
	.byte 0xF5, 0xD4, 0xC8 ,0xFF	
.endm

.macro P84_LO
	.byte 0xF5, 0xDC, 0xC8 ,0xFF	
.endm






/*	TBZ(ADDR_24)BIT, DEST:	
	f3 fe c0 + bit addr_l addr_m addr_h reloffset


.macro m_tbz addr bit dest			
	.byte 0xF3, 0xFE
	.byte 0xC0 + \bit
	.word \addr
	.byte 0x00
	.byte 0xfd
	.byte \dest-.
.endm

m_tbz 0x1234 5 drivecode_init
m_tbz 0x1234 5 drivecode_init

*/