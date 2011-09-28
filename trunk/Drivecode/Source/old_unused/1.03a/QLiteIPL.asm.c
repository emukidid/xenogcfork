/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«                        QLiteIPL.asm                                     »©
  ©«                                                                         »©
  ©«                Copyright 2005 Anonymous Ghostwriter                     »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Description]....:  This code gets injected into the apploader to       »©
  ©«                     do things like patching the region                  »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Notes......]....:  Runs via hooked Apploader EP. Then hooks ApplClose  »©
  ©«                     to do its thing only when actually running a game.  »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [History....]....:  [bc]-03.12.2004 file created                        »©
  ©«                                                                         »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/

.include "source/PPC.h"
.include "source/QliteGlobal.h"

.global _start

.section code
.org 0
.text

#======================================================
# Configuration and defines
#======================================================
.equ CREDITS_SIZE,			4480

#----------------------------------------------------
# qCode custom commands
#----------------------------------------------------
.equ CMD_DISABLEAUDIOFIX,	0x2100
.equ CMD_DISABLEDREFIX,		0x2200
.equ CMD_REQUESTCREDITS,	0x2500
.equ CMD_BYPASSQCODE,		0x2600
.equ CMD_STOPDRIVE,			0xE300

#===========================================
# Data
#===========================================
.equ baseData,				0x80000000
.equ ApplEPRet,				0x80001800
.equ ApplMainPtr,			0x80001804
.equ ApplMainAddr,			0x80001808
.equ ApplClosePtr,			0x8000180C
.equ ApplCloseAddr,			0x80001810


#================================================
# Hooked apploader entrypoint called by IPL
#=================================================
 _start:
	b OnApploaderEntry

	#==============================================
	 DataSection:
	#==============================================
	dwRegionCode:		.long 0x00000000
	dwOrgEP:			.long 0x81200258

#============================================================
 OnApploaderEntry:
#============================================================

	mflr r0	
	stwu	r9, -8(sp)					# push r9
	lis r9, ApplEPRet@h		
	
	stw r0, ApplEPRet@l(r9)				# save linkreg
	stw r4, ApplMainPtr@l(r9)			# save param0 (R3:ApplMainPtr)
	stw r5, ApplClosePtr@l(r9)			# save param2 (R5:ApplClosePtr)

	GetRelocAddr r9, dwOrgEP			# get org EP address to lr
	lwz r9, 0(r9)
	mtlr	r9

	lwz r9, 0(sp)						# pop r9
	addi sp, sp, 8			
	blrl								# call original EntryPoint to recieve Appl vectors
	
	#===============================================================#
	# hook Aploader Main and Close vectors							#
	#===============================================================#	
	subi sp, sp, 0x78					# push all gp registers
	stmw r2, 0(sp)
	
	lis r9, baseData@h					# get backed-up EP params
	lwz r4, ApplMainPtr@l(r9)			
	lwz r5, ApplClosePtr@l(r9)

	lwz r3, 0(r4)						# get ApplMainPtr
	stw r3, ApplMainAddr@l(r9)			# make a backup
	GetRelocAddr r3, OnApploaderMain	# replace with our hook
	stw r3, 0(r4)

	lwz r3, 0(r5)						# get ApplClosePtr
	stw r3, ApplCloseAddr@l(r9)			# make a backup
	GetRelocAddr r3, OnApploaderClose	# replace with our hook
	stw r3, 0(r5)

	lwz r9, ApplEPRet@l(r9)				# restore lr
	mtlr	r9

	lmw r2, 0(sp)						# pop all gp registers
	addi sp, sp, 0x78
	blr									# return to IPL

#==============================================================================#
 OnApploaderMain:								
#==============================================================================#	

	mflr r0								# push lr
	stwu r0, -0x08(sp)
	subi sp, sp, 0x78					# push all gp registers
	stmw r2, 0(sp)
	#===============================================================================

	lis		r9, 0x8000					# determine real IPL region
	lwz		r9, 0xcc(r9)				# check tvmode
	cmplwi  r9, 0x01
	beq REGION_PAL

	lis r9, 0xCC00						# if NTSC also check VIDSEL bit
	lhz r9, 0x206E(r9)
	rlwinm. r9, r9, 0,30
	beq REGION_USA

REGION_JAP:
	li r0, 0x00
	b PatchGameRegion
REGION_USA:
	li r0, 0x01
	b PatchGameRegion
REGION_PAL:
	li r0, 0x02

PatchGameRegion:
	#===============================================================================
	lis r9, 0x8000						# patch discblock once the 
	lwz r3, 0xf4(r9)					# lowmem ptr is valid
	cmplwi  r3, 0x00
	beq 1f
	stw	r0, 0x18(r3)				
1:	#===============================================================================

	lis r5, ApplMainAddr@h				# get saved original ApplMain address
	lwz r3, ApplMainAddr@l(r5)
	mtlr r3		
	
	lmw r2, 0(sp)						# pop all gp registers & lr					
	addi sp, sp, 0x78
	blrl								# call real ApplMain

	lwz r0, 0(sp)						# pop lr
	addi sp, sp, 0x08
	mtlr r0
	blr									# return to IPL

#==============================================================================#
 OnApploaderClose:								
#==============================================================================#	

	mflr r0								# push lr
	stwu r0, -0x04(sp)
	subi sp, sp, 0x78					# push all gp registers
	stmw r2, 0(sp)

	lis r3, 0xCC00						# read PAD0
	lhz r4, 0x6404(r3)
	lwz r5, 0x6408(r3)

   #=======================================================================================================
	andi. r5, r4, PAD_L					# L-Key pressed ?		-> disable audiofix
	beq 1f								# 
	lis r0, CMD_DISABLEAUDIOFIX						
	bl SUB_SendCustomCommand
   #=======================================================================================================
	.if DREFIXCFG == 1					#
1:		andi. r5, r4, PAD_R				# R-Key pressed ?		-> disable dreFix
		beq 1f							# 
		lis r0, CMD_DISABLEDREFIX						
		bl SUB_SendCustomCommand
	.endif
   #=======================================================================================================
1:	andi. r5, r4, PAD_Y					# Y-Key pressed ?		-> force inv. region
	beq 1f								 
	GetRelocAddr r5, dwRegionCode		 
	lwz r0, 0(r5)						# invert value
	xori r0, r0, 3
	andi. r0, r0, 2
	stw r0, 0(r5)
   #=======================================================================================================
	.if CREDITS == 1
1:		andi. r5, r4, PAD_START			# L-Key pressed ?	-> disable audiofix
		beq noCredits					# 
		
		lis r0, 0xE300
		bl SUB_SendCustomCommand

		lis r0, CMD_REQUESTCREDITS
		bl SUB_SendCustomCommand
		
		li32 r3, 3000
		bl SUB_Delay
	
 		bl SUB_DVDReadMemBlock

		li r0, 0x02						# set pal region
		GetRelocAddr r5, dwRegionCode	
		stw r0, 0(r5)

		bl SUB_PatchVideoMode

		li32 r0, 0x28500100				# patch videomode for credits
		stw  r0, 0x18(r4)
		li r0, 0x11F5
		sth  r0, -0x30(r4)
		li32 r0, 0x00010023
		stw  r0, -0x24(r4)
		li   r0, 0x24
		stw  r0, -0x20(r4)
		
		lis r3, 0x8170
		lwzu r0, 0x78(sp)				# pop lr
		b exit

	 noCredits:
	.endif

1:	bl SUB_PatchVideoMode				# switch videoMode

	lis r5, ApplCloseAddr@h				# get saved original ApplClose address
	lwz r3, ApplCloseAddr@L(r5)
	mtlr r3	

	lmw r2, 0(sp)						# pop all gp registers & lr					
	addi sp, sp, 0x78
	blrl								# call AppClose

	lwz r0, 0(sp)						# pop lr
	addi sp, sp, 0x04
exit:
	mtlr r0
	blr									# return to IPL

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

.if CREDITS == 1
#=======================================================================
 SUB_DVDReadMemBlock:					# R0: DVD-Address
#=======================================================================	
	mflr    r31
	lis		r9, 0xCC00								
	li32	r3, 0x40D800				# R3: DVD Address
	li32	r4, (0x81700000-4)			# R4: GC Dest mem address
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
.endif


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

#===============================================================
 SUB_Memcpy:			# R3: src R4: dst R5: dwords
#===============================================================
	mtctr	r5
	subi r3, r3, 4
	subi r4, r4, 4
 copyLoop:
	lwzu	r5, 4(r3)					# copy first part
	stwu	r5, 4(r4)
	bdnz copyLoop
	blr		

#===============================================================
 SUB_PatchVideoMode:
#===============================================================
	mflr r9

	GetRelocAddr r3, vidModeNTSC		# get NTSC table address
	GetRelocAddr r4, dwRegionCode		# check game region
	
	lwz r0, 0(r4)						
	cmplwi r0, 0x02
	beq setPAL
	
	li  r0, 0							# NTSC tvmode
  	b setTVMode

 setPAL:	
	li  r0, 1							# PAL tvmode
	addi r3, r3, 0x34					# add PAL table offset
 
 setTVMode:	
	lis r4, 0x8000						# set tvmode
	stw r0, 0x00CC(r4)
	
	#----------------------------------------------------------
	# Set videomode timing registers
	#----------------------------------------------------------
	lis		r4, 0xCC00					# Dest
	ori		r4, r4, 0x2000
	li		r5, 13						# counter
	bl		SUB_Memcpy

	mtlr r9
	blr		

vidModeNTSC:
	.long 0x00060001, 0x476901AD, 0x02EA5140, 0x001501E6	# NTSC Black
	.long 0x001401E7, 0x410C410C, 0x40ED40ED, 0x80100000
	.long 0x00000000, 0x80100000, 0x00000000, 0x00000000
	.long 0x110701AE

vidModePAL:
	.long(0x00060101), 0x4B6A01B0, 0x02F85640,(0x001501E6)	# PAL Black
	.long(0x001401E7), 0x4D2B4D6D, 0x4D8A4D4C, 0x00435A4E
	.long 0x00000000, 0x00435A4E, 0x00000000, 0x013C0144
	.long 0x113901B1
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

 loopf:
	stwu	r6, 4(r4)
	bdnz loopf
	blr
*/
#===================================================================================#
#  Debug code					                                                    #
#===================================================================================#
.if RELEASEBUILD == 0
	.include "source/QLiteIPLDbg.asm.c"
.endif
