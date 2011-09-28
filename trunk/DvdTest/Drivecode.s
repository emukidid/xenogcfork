

.globl SUB_BootXenoShell


.equ DVD0,					0x6000
.equ DVD1,					0x6004
.equ DVD2,					0x6008
.equ DVD3,					0x600c
.equ DVD4,					0x6010
.equ DVD5,					0x6014
.equ DVD6,					0x6018
.equ DVD7,					0x601c
.equ DVD8,					0x6020


.macro li32 reg, imm32
	lis \reg, (\imm32 >> 16)
	ori \reg, \reg, (\imm32 & 0xFFFF)
.endm

.macro GetRelocAddr reg, label
	bcl 20,31,$+4						
	mflr \reg
	addi \reg, \reg, (\label)-.+4
.endm

.equ CREDITS_SIZE,			4056+4
.equ CMD_REQUESTCREDITS,	0x2700

#=======================================================================
 SUB_BootXenoShell:
#=======================================================================
 
		mflr r30
		subi sp, sp, 0x78					# push all gp registers
		stmw r2, 0(sp)


 XenoShellBootMC:
#		lis r3, 0x8000
#		li32 r4, 0x2badc0de
#		stw r4, 0(r3)
 
 doCredits:
		#===============================================================#
		# retrieve Credits binary and execute instead of game			#
		#===============================================================#	
		lis r0, 0xE300
		bl SUB_SendCustomCommand

		lis r0, CMD_REQUESTCREDITS
		bl SUB_SendCustomCommand

 readBuffer:
		li32 r3, 3000
		bl SUB_Delay
		bl SUB_DVDReadMemBlock

		lis r3, 0x8170
		lwz r3, -4(r3)
		li32 r4, 0x2bad2bad
		sub r3, r3, r4
		cmpwi r3, 0
		bne readBuffer 

		bl SUB_DVDReset

		lmw r2, 0(sp)						# pop all gp registers & lr					
		addi sp, sp, 0x78
		mtlr r30							# exit
		blr


#=======================================================================
 SUB_SendCustomCommand:					# R0: Command Byte
#=======================================================================	
	lis		r9 , 0xCC00					# r3 = DVD[0]						
	stw		r0 , 0x6008(r9)				# dvd[2] = Command
	li		r0 , 0x2e					# dvd[0] = 0x2e			
	stw		r0 , 0x6000(r9)	
	li		r0 , 1						
	stw		r0 , 0x601C(r9)				# dvd[7] = 1

SendCmd_Loop:		
	lwz		r0 , 0x601C(r9)				
	andi.	r0 , r0 , 0x01			
	cmpwi	r0 , 0			
	bne		SendCmd_Loop	
	blr	


#=======================================================================
 SUB_DVDReadMemBlock:					# R0: DVD-Address
#=======================================================================	
	mflr    r31
	lis		r9, 0xCC00								
	li32	r3, (0x40D800-4)			# R3: DVD Address
	li32	r4, (0x81700000-8)			# R4: GC Dest mem address
	li		r5, CREDITS_SIZE			# R5: bytes to read

 ReadBlockLoop:
	stw		r3,	DVD3(r9)				# dvd[3] = DVD-Address
	lis		r0, 0x0001					# dvd[4] = 0x00010000
	stw		r0,	DVD4(r9)

	lis	r0, 0xFE01						# dvd[2] = 0x0xFE010000
	bl		SUB_SendCustomCommand

	lwz		r0, DVD8(r9)				# get read dword
	stwu	r0, 4(r4)					# store in mem

	addi	r3, r3, 4
	subi	r5, r5, 4
	cmpwi	r5, 0
	bne ReadBlockLoop
	mtlr    r31
	blr

#=======================================================================
 SUB_Delay:								# R3: Delay
#=======================================================================
Delay:
	lis	r4, 0x0003
DelayLoopInner:
	subi	r4 , r4 , 1
	cmpwi	r4 , 0
	bne		DelayLoopInner
	subi	r3 , r3 , 1
	cmpwi	r3 , 0
	bne		Delay
	blr

#=======================================================================
 SUB_DVDReset:
#=======================================================================
		lis		r4 , 0xcc00					
		ori		r4 , r4 , 0x3024
		lwz		r5 , 0(r4)
		rlwinm	r5 , r5 , 0 , 30 , 28
		ori		r5 , r5 , 1
		stw		r5 , 0(r4)
		lwz		r5 , 0(r4)
		ori		r5 , r5 , 4
		ori		r5 , r5 , 1
		stw		r5 , 0(r4)
		blr

/*
.global _g_pQCode
.global _g_pQCode_End

.global _g_pQCode_Lo
.global _g_pQCode_Lo_End

.global _g_pQCode_Hi
.global _g_pQCode_Hi_End



.global _g_pDolImage2
.global _g_pDolImage_End2


.align 5
_g_pQCode:		
#-------------------------------------
#.incbin "qcode.bin"
#-------------------------------------
_g_pQCode_End:


.global _g_pDolImage
.global _g_pDolImage_End

.align 5
_g_pDolImage:
#-------------------------------------
#.incbin "IPL_PAL.dol"
#.incbin "gcos.dol"
#.incbin "credits.bin"
#.incbin "sdload.dol"
#-------------------------------------
_g_pDolImage_End:

.align 5
_g_pDolImage2:
#-------------------------------------
#.incbin "credits.bin"
#.incbin "sdload.dol"
#-------------------------------------
_g_pDolImage_End2:

.align 5
_g_pQCode_Lo:
#-------------------------------------
#.incbin "qcode_lo.bin"
#-------------------------------------
_g_pQCode_Lo_End:

.align 5
_g_pQCode_Hi:
#-------------------------------------
#.incbin "qcode_hi.bin"
#-------------------------------------
_g_pQCode_Hi_End:
*/



	