


#=======================================================================
 SUB_WriteDriveMem:			# R5: Address R9: DataDword
#=======================================================================	
	lis		r3 , 0xcc00					# r3 = DVD[0]						
	ori		r3 , r3 , 0x6000
	li		r4 , 0x2e					# dvd[0] = 0x2e			
	stw		r4 , 0(r3)	
	lis		r4 , 0xFE01					# dvd[2] = 0xFE010100
	ori		r4 , r4 , 0x0100	
	stw		r4 , 8(r3)
	stw		r5 , 12(r3)					# dvd[3] = Address  (R5)
	lis		r4 , 0x0004					# dvd[4] = 0x00040000	
	ori		r4 , r4 , 0x0000
	stw		r4 , 16(r3)	
	li		r4 , 3						# dvd[7] = 3
	stw		r4 , 28(r3)	
UnlockDrive_Loop1:	
	lwz		r4 , 28(r3)				
	andi.	r4 , r4 , 0x1			
	cmpwi	r4 , 0					
	bne		UnlockDrive_Loop1		
	li		r4 , 0x2e					# dvd[0] = 0x2e			
	stw		r4 , 0(r3)	
	stw		r9 , 8(r3)					# dvd[2] = DataDword (R9)
	li		r4 , 1						# dvd[7] = 1
	stw		r4 , 28(r3)		
UnlockDrive_Loop2:		
	lwz		r4 , 28(r3)				
	andi.	r4 , r4 , 0x1			
	cmpwi	r4 , 0			
	bne		UnlockDrive_Loop2	
	blr		
/*

#	li32 r5, 0x817f6500					# dvdhack
#	li32 r5, 0x817fdf60
#	li32 r5, 0x817f9ee0					# waverace

#	li32	r9, 0x800000CC				# patch tvmode variable to NTSC
#	li		r0, 1					
#	stw		r0, 0(r9)


	.equ PAD_CHANNEL0, 0xCC006400
	.equ PAD_CHANNEL1, 0xCC00640C
	.equ PAD_CHANNEL2, 0xCC006418
	.equ PAD_CHANNEL3, 0xCC006424

	li32 r3, PAD_CHANNEL0				# read pad 0
	lhz r3, 4(r3)
	andi. r3, r3, 0x0040				# l-key pressed ?
	beq 1f
	li32 r2, 0x21212121
	DVDMemLogImm r2
1:
*/

#	OrgApploaderEPVec:			.long 0x81200258
#	OrgApploaderCloseVec:		.long 0x00000000
#	DVDMemLogReg sp, 0x100
#	DVDMemLog 0x81300c70, 0x60



/*
#===============================================================
 SUB_BootFromSD:
#===============================================================
#.incbin "source/SDLoad.bin"

		mfmsr	r3
		rlwinm	r3, r3,	0,17,15
		mtmsr	r3

		lis	r3, 0xCC00
		li	r5, 0x4C9
		lis	r29, 0xFF00
		mtctr	r5

loc_1700001C:				# CODE XREF: seg000:00000020.j
		bl	sub_1700008C
		bdnz	loc_1700001C
		li	r6, 0
		li	r7, 0
		bl	sub_170000A4

loc_17000030:				# CODE XREF: seg000:00000040.j
		lis	r6, 0x100
		bl	sub_170000A4
		bl	sub_1700008C
		andis.	r31, r31, 0xFF00
		bne	loc_17000030
		stw	r3, 0x6800(r3)
		lis	r9, 0x8100
		mtctr	r5
		lis	r6, 0x1100
		bl	sub_170000A4

loc_17000058:				# CODE XREF: seg000:00000064.j
		bl	sub_1700008C
		addis	r4, r31, 0x200
		andis.	r4, r4,	0xFF00
		bne	loc_17000058

loc_17000068:				# CODE XREF: seg000:0000007C.j
		bl	sub_1700008C
		srwi	r31, r31, 24
		stbx	r31, r9, r4
		dcbf	r9, r4
		addi	r4, r4,	1
		bdnz	loc_17000068
		addi	r9, r9,	0x40
		mtlr	r9
		blrl

# ллллллллллллллл S U B	R O U T	I N E ллллллллллллллллллллллллллллллллллллллл


sub_1700008C:				# CODE XREF: seg000:0000001C.p
		# seg000:00000038.p ...
		stw	r29, 0x6810(r3)
		stw	r5, 0x680C(r3)

loc_17000094:				# CODE XREF: sub_1700008C+10.j
		lmw	r30, 0x680C(r3)
		andi.	r30, r30, 1
		bne	loc_17000094
		blr
# End of function sub_1700008C


# ллллллллллллллл S U B	R O U T	I N E ллллллллллллллллллллллллллллллллллллллл


sub_170000A4:				# CODE XREF: seg000:0000002C.p
					# seg000:00000034.p ...
		mflr	r8
		stw	r5, 0x6800(r3)
		bl	sub_1700008C
		oris	r29, r6, 0x4000
		bl	sub_1700008C
		li	r29, 4

loc_170000BC:				# CODE XREF: sub_170000A4+20.j
		bl	sub_1700008C
		subic.	r29, r29, 1
		bne	loc_170000BC
		lis	r29, 0x9500
		bl	sub_1700008C
		lis	r29, 0xFF00
		bl	sub_1700008C
		mtlr	r8
		blr
*/

/*
#=======================================================================
 SUB_LoadDol:
#=======================================================================

	stwu    r1,-16(r1)
	mflr    r0
	li      r8,0
	stw     r0,20(r1)

	l3:	rlwinm  r0,r8,2,0,29
	add     r11,r0,r3
	lwz     r9,144(r11)
	cmpwi   r9,0
	beq+    l1
	lwzx    r0,r3,r0
	addi    r9,r9,-1
	lwz     r10,72(r11)
	add     r11,r3,r0
	beq-    l1
	addi    r9,r9,1
	mtctr   r9
l2:	lbz     r0,0(r11)
	addi    r11,r11,1
	stb     r0,0(r10)
	addi    r10,r10,1
	bdnz+   l2
l1:
	addi    r8,r8,1
	cmpwi   r8,17
	ble+    l3

	lwz     r9,220(r3)
	lwz     r11,216(r3)
	cmpwi   r9,0
	addi    r9,r9,-1
	beq-    l4
	addi    r9,r9,1
	mtctr   r9
l5:	li      r0,0
	stb     r0,0(r11)
	addi    r11,r11,1
	bdnz+   l5
l4:	lwz     r3,224(r3)
	mtctr   r3
	mfmsr   r0
	ori     r0,r0,2
	rlwinm  r0,r0,0,17,15
	mtmsr   r0
	bctrl
	lwz     r0,20(r1)
	addi    r1,r1,16
	mtlr    r0
	blr
*/


/*
#=======================================================================
 SUB_FillVideoMem:
#=======================================================================
	lis         r4, 0xC0F0
	ori			r4, r4, 0x500
	lis         r9, 0xCC00
	ori         r9, r9, 0x201c
	stw         r4, 0 (r9)

	lis         r4, 0xC0F0
	lis         r9, 0xCC00
	ori         r9, r9, 0x2024
	stw         r4, 0 (r9)

	lis         r4, 0xC0F0
	
	li			r5, 0x4				# 0x20000 pixels	4B000
	ori         r5, r5, 0xb000
	mtctr		r5

	lis			r5, 0xffff			# color

 loopf:
	stwu	r5, 4(r4)
	bdnz loopf
	blr



#=======================================================================
 SUB_Delay:
#=======================================================================
	li	r3, 8000
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
*/


/*
	.long 0x0F060001, 0x476901AD, 0x02EA5140, 0x00030018
	.long 0x00020019, 0x410C410C, 0x40ED40ED, 0x80100000
	.long 0x00000000, 0x80100000, 0x00000000, 0x00000000
	.long 0x110701AE, 0x10010001, 0x00010001, 0x00010001
	.long 0x00000000, 0x00000000, 0x28500100, 0x1AE771F0
	.long 0x0DB4A574, 0x00C1188E, 0xC4C0CBE2, 0xFCECDECF
	.long 0x13130F08, 0x00080C0F, 0x00FF0000, 0x00000000
	.long 0x02800000, 0x000000FF, 0x00FF00FF, 0x00FF00FF
*/

/*
.org 0x258

	blr
	blr
	blr
	blr
end:
	b end
*/

/*	  bcl 20,31,$+4
  
	#-------------------------------------
	# show test pattern
	#-------------------------------------
videofun:
	lis         r4, 0xC050
	lis         r9, 0xCC00
	ori         r9, r9, 0x201c
	stw         r4, 0 (r9)

	li			r5, 0x0008
	ori         r5, r5, 0x0000
	mtctr		r5

 loopf:
	stwu	r4, 4(r4)
	bdnz loopf

	li	r3, 4000
Delay:
	lis	r4, 0x0003
DelayLoopInner:
	subi	r4 , r4 , 1
	cmpwi	r4 , 0
	bne		DelayLoopInner
	subi	r3 , r3 , 1
	cmpwi	r3 , 0
	bne		Delay
*/


/*		*(unsigned short*)(0xCC002000) = 0x0006;
		*(unsigned long*)(0xCC00200C) = 0x001501E6;
		*(unsigned long*)(0xCC002010) = 0x001401E7;
*/



/*
videoclear:
	lis         r4, 0xC050
	lis         r9, 0xCC00
	ori         r9, r9, 0x201c
	stw         r4, 0 (r9)

	li			r5, 0x0fff
	mtctr		r5

 loop2:
	stwu	r5, 4(r4)
	bdnz loop2


	li	r3, 4000
Delay:
	lis	r4, 0x0003
DelayLoopInner:
	subi	r4 , r4 , 1
	cmpwi	r4 , 0
	bne		DelayLoopInner
	subi	r3 , r3 , 1
	cmpwi	r3 , 0
	bne		Delay

videofun:
	lis         r4, 0xC050
	lis         r9, 0xCC00
	ori         r9, r9, 0x201c
	stw         r4, 0 (r9)

	li			r5, 0x0008
	ori         r5, r5, 0x0000
	mtctr		r5

 loop:
	stwu	r4, 4(r4)
	bdnz loop

	li	r3, 4000
Delay:
	lis	r4, 0x0003
DelayLoopInner:
	subi	r4 , r4 , 1
	cmpwi	r4 , 0
	bne		DelayLoopInner
	subi	r3 , r3 , 1
	cmpwi	r3 , 0
	bne		Delay

dvdreset:
	lis     r9, 0xCC00
	ori     r9, r9, 0x6000
	lis     r0, 0xE300
	stw     r0, 8 (r9)
	li		r0, 1
	stw		r0, 0x1c (r9)

	li	r3, 4000
Delay:
	lis	r4, 0x0003
DelayLoopInner:
	subi	r4 , r4 , 1
	cmpwi	r4 , 0
	bne		DelayLoopInner
	subi	r3 , r3 , 1
	cmpwi	r3 , 0
	bne		Delay
#	blr

hotreset:
	lis r3,0
	lis r9,0xCC00
	sth r3, 0x2000(r9)
	li r4, 3
	stw r4, 0x3024(r9)
	stw r3, 0x3024(r9)
	nop
	loop__: b loop__
*/


/*	lis r3, 4
	bl .+0x102000+0x66E84
	bl .+0x102000+0x66E80 + 0x1E8
*/	

/*
jmptoGcOs:
	lis r3, 0x8130
	ori r3, r3,	0x0000
	mtlr r3
	blrl
*/

#	b .+0x238-0x13a0
#	lis r5, 0x8136
#	ori r5, r5,	0x8fcc
#	mtlr r5
#	blr


