

#=======================================================================
# Constants
#=======================================================================	
.equ PAD_LEFT,		0x0001
.equ PAD_RIGHT, 	0x0002
.equ PAD_DOWN,		0x0004
.equ PAD_UP,		0x0008
.equ PAD_Z, 		0x0010
.equ PAD_R, 		0x0020
.equ PAD_L, 		0x0040
.equ PAD_A, 		0x0100
.equ PAD_B, 		0x0200
.equ PAD_X, 		0x0400
.equ PAD_Y, 		0x0800
.equ PAD_START, 	0x1000
.equ PAD_CLEFT, 	0x2000
.equ PAD_CRIGHT,	0x4000
.equ PAD_CDOWN, 	0x8000
.equ PAD_CUP,		0x10000


.equ DVD0,			0x6000
.equ DVD1,			0x6004
.equ DVD2,			0x6008
.equ DVD3,			0x600c
.equ DVD4,			0x6010
.equ DVD5,			0x6014
.equ DVD6,			0x6018
.equ DVD7,			0x601c
.equ DVD8,			0x6020


#=======================================================================
# General PPC macros
#=======================================================================	

.macro babs addr			
	b \addr-.+\addr
.endm

.macro li32 reg, imm32
	lis \reg, (\imm32 >> 16)
	ori \reg, \reg, (\imm32 & 0xFFFF)
.endm

.macro GetRelocAddr reg, label
	bcl 20,31,$+4						
	mflr \reg
	addi \reg, \reg, (\label)-.+4
.endm


#=======================================================================
# Log to DVD Memory macros
#=======================================================================	

#---------------------------------------------
# Logs a single 32bit value or Register
#---------------------------------------------
.macro DVDMemLogImm ImmReg, DvdMemAddress = 0x40d800
	mr r2, \ImmReg
	stwu r2, -4(sp)
	DVDMemLogRegPtr sp, 4, \DvdMemAddress
	addi sp, sp, 4
.endm

#-------------------------------------------------
# Logs a number of bytes at given address
#-------------------------------------------------
.macro DVDMemLog Address, Size, DvdMemAddress = 0x40d800
	lis		r2,(\Address >> 16)					# GCMem read address		
	ori		r2, r2, (\Address & 0xFFFF)
	DVDMemLogRegPtr r2, \Size, \DvdMemAddress
.endm

#----------------------------------------------
# Logs a number of bytes at register address
#----------------------------------------------
.macro DVDMemLogRegPtr Reg, Size, DvdMemAddress = 0x40d800
	li		r5, (\Size / 4)
	mtctr	r5
	mr r2, \Reg									# Source Address
	lis		r5,(\DvdMemAddress >> 16)			# DVDMem dest address			
	ori		r5, r5, (\DvdMemAddress & 0xFFFF)
	lwzu	r9, 0(r2)

 DumpMemLoop\DvdMemAddress:

	bl SUB_WriteDriveMem
	addi r5, r5, 4								# next address
	lwzu r9, 4(r2)								# read next dword
	bdnz DumpMemLoop\DvdMemAddress				# write to drive
.endm



