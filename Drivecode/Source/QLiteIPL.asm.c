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

/*
 ³  MULTIBOOT TECHNICAL INFO                                                ³
 ³                                                                          ³
 ³  We'll release a command-line MultiBoot Disc maker next week if nobody   ³
 ³  releases a better one before we do :-) Here are the specs for Multiboot ³
 ³  discs. Note that every image should be aligned on a 32kb boundary.      ³
 ³  At offset 0x00: 43 4F 42 52-41 4D 42 31                                 ³
 ³            0x40: Position of the 1st image in bytes (Big endian)         ³
 ³            0x44: Position of the 2nd image in bytes (Big endian)         ³
 ³            ....: ...etc...                                               ³
 ³            ....: 00 00 00 00                                             ³
 ³                                                                          ³
 ³  The MultiBoot feature is stealth for the GC so it's 100% compatible     ³
 ³  with homebrew images. If you want to put several backups on one disc    ³
 ³  you'll have to remove the garbage at first.                             ³
 ³  The maximum size of a MultiBoot disc is 1459978240 bytes.               ³
 ³                                                                          ³
 */

/*	00000000 434F 4252 414D 4231 0000 0000 0000 0000 COBRAMB1........
	00000010 0000 0000 0000 0000 0000 0000 0000 0000 ................
	00000020 4D55 4C54 4920 424F 4F54 2044 6973 6B20 MULTI BOOT Disk 
	00000030 5649 5045 5220 4F6E 6C79 2100 0000 0000 VIPER Only!.....
	00000040 0000 8000 02F6 8000 0569 8000 0928 0000 .....s...i...(..
	00000050 0CE6 8000 0000 0000 0000 0000 0000 0000 ................

	00008000			ikaruga
	02F68000 (00BDA0)	zelda
	05698000			mariokart
	09280000 (024A00)	megaman
	0CE68000			cubivore
	012F3800 (004BCE)	bomberman
	16B20000 (05AC80)	bust-a-move
	206C0000 (081B00)	warioworld
*/


#======================================================
# Configuration and defines
#======================================================
.equ CREDITS_SIZE,			3840+4

#----------------------------------------------------
# qCode custom commands
#----------------------------------------------------
.equ CMD_DISABLEAUDIOFIX,	0x2100
.equ CMD_DISABLEDREFIX,		0x2200
.equ CMD_REQUESTCREDITS,	0x2700
.equ CMD_BYPASSQCODE,		0x2600
.equ CMD_ENABLESLEEP,		0x2900
.equ CMD_DENYWRITE,			0x2A00
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
	dwDiscID:			.long 0x00000000

#============================================================
 OnApploaderEntry:
#============================================================
	mflr r0	
	stwu	r9, -8(sp)					# push r9

 #*************************************************************************************************************************************
/*
	stwu	r0, -8(sp)					# push r9

#	GetRelocAddr r9, doCreditsX			# replace with our hook
	lis r9, 0x8170
	stw r9, 0(r3)
	GetRelocAddr r9, SUB_Rumble			# replace with our hook
	stw r9, 0(r4)
	stw r9, 0(r5)

 doCreditsX:
	#===============================================================#
	# retrieve Credits binary and execute instead of game			#
	#===============================================================#	
	bl SUB_Rumble
	lis r0, 0xE300
	bl SUB_SendCustomCommand

	lis r0, CMD_REQUESTCREDITS
	bl SUB_SendCustomCommand

	li32 r3, 3000
	bl SUB_Delay
	bl SUB_DVDReadMemBlock

	mfmsr    3							# disable interrupts!
	rlwinm   4,3,0,17,15
	mtmsr    4

	lis r3, 0x8170
	mtlr r3
	blr
	
	lwz r0, 0(sp)						# pop r9
	addi sp, sp, 8			
	mtlr r0
	blr									# return to IPL
*/

/*	lis r3, 0x8170
	mtlr r3
	blr
#	lis r3, 0x8170						# store target address in lr stack pos
#	stw r3, 0x78(sp)
#	b doCredits
 1:
 #*************************************************************************************************************************************
*/	
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

	#===============================================================#
	# find and dump memory											#
	# searches for 32bit imm and copies 1mb to 0x80800000			#
	#===============================================================#	
/*	lis r4, 0x8080						# Set GCOs Memdump tag
	li32 r0, 0x27657265
	stw r0, -0x10(r4)

	li32 r5, 0x716F6F62
	lis r3, 0x8000
	subi r3, r3, 4
1:	lwzu r4, 4(r3)
	sub r4, r4, r5
	cmpwi r4, 0
	bne 1b

	lis r5, 0x8080
	stw r3, -4(r5)


	li32 r3, 0x81000000
	lis r4, 0x8080						
	li32 r5, 1024*1024*1
	bl		SUB_Memcpy

#	DVDMemLog 0x8120000, 0x800
*/
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
	lwz r4, 0x6404(r3)
	srwi r4, r4, 16
#	andi. r4, r4, 0xffff
	lwz r5, 0x6408(r3)

   #=======================================================================================================
	andi. r5, r4, PAD_L					# L-Key pressed ?		-> disable audiofix
	beq 1f								# 
	lis r0, CMD_DISABLEAUDIOFIX						
	bl SUB_SendCustomCommand
   #=======================================================================================================
	.if STANDBYMODE == 1
1:		andi. r5, r4, PAD_X				# X-Key pressed ?		-> enable sleepmode / deny write
		beq 1f							# 
		lis r0, CMD_ENABLESLEEP						
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
1:		andi. r5, r4, PAD_Z				# Z-Key pressed ?	-> boot memcard
		bne XenoShellBootMC
		
		andi. r5, r4, PAD_START			# Start-Key pressed ?	-> boot shell
		bne doCredits

		GetRelocAddr r5, dwDiscID		# check for VIPER multiboot crap
		lwz r0, 0(r5)
		cmpwi r0, 0	
		bne doCredits
		b noCredits

 XenoShellBootMC:
		lis r3, 0x8000
		li32 r4, 0x2badc0de
		stw r4, 0(r3)
 
 doCredits:
		#===============================================================#
		# retrieve Credits binary and execute instead of game			#
		#===============================================================#	
		lis r0, 0xE300
		bl SUB_SendCustomCommand

		lis r0, CMD_REQUESTCREDITS
		bl SUB_SendCustomCommand

 readBuffer:
		li32 r3, 4000
		bl SUB_Delay
		bl SUB_DVDReadMemBlock

#		lis r3, 0x8170
#		lwz r3, -4(r3)
#		li32 r4, 0x2bad2bad
#		sub r3, r3, r4
#		cmpwi r3, 0
#		bne readBuffer
				
		bl SUB_DVDReset

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
.endif

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
 SUB_Rumble:
#=======================================================================
	mflr r0
	li32 r5, 0xcc006400
	li32 r4, 0x00400001
	stw r4, 0(r5)
	lis r4, 0x8000
	stw r4, 0x38(r5)
	li r3, 300
	bl SUB_Delay
	lis r4, 0x0040
	stw r4, 0(r5)
	lis r4, 0x8000
	stw r4, 0x38(r5)
	mtlr r0
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

 loopf:
	stwu	r6, 4(r4)
	bdnz loopf
	blr
*/
/*
# configure the bootloader here
#MEMCARD_POS     = 0x00060600
#MEM_POS         = (0x80d00000 - 0x20)
MEMCARD_POS     = 0x0003A000
MEM_POS         = 0x80004000


#===============================================================
 SUB_BootFromMC:
#===============================================================
 		li 6, 64*11                     # nr of blocks -> one memccard sector
        mtctr 6                         # (Count Register (equiv. to mtspr 8,Rx )) 

        lis 27, 0xCC00                  # hardware base address
        
        lis 28, MEMCARD_POS@h		    # position of file on memory card
        ori 28, 28, MEMCARD_POS@l
		lis 29, MEM_POS@h				# position of file in memory
		ori 29, 29, MEM_POS@l
        mr 7, 29                        # start address for resulting file
again:
        li  5, 0                        # disable CS
        stw 5, 0x6814(27)				# 0x6800
        lis 3, 0x5200                   # "read" command
        rlwinm 8, 28, 25, 30, 31        # page number 
        or 3, 3, 8
        rlwinm 8, 28, 31, 14, 23        # sector address
        or 3, 3, 8
        li  4, 0x35                     # write 4 bytes
        bl write
        li  3, 0x00                     # block address
        bl write
        li  4, 0x05                     # last dummy byte
        bl write
        li  30, 128                     # 128 bytes
        li  31, 3                       # exi dma read
        stmw 29, 0x6818(27)             # start dma read (0x6804)
        bl write_wait                   # wait until it's done

        addi 28, 28, 0x80               # next card address
        addi 29, 29, 0x80               # next ram address

        bdnz again                      # next block (Decrement CTR, branch if CTR non-zero) 
 
        addi 7, 7, 0x20					# add offset imposed by memcard
        mtlr 7                          # jump there!
        blr				
write:
        stw 3, 0x6824(27)               # data	0x6810
        li  5, 0x8c0                    # memc
        stw 5, 0x6814(27)               # 0x6800
        stw 4, 0x6820(27)				# 0x680c
write_wait:
        lwz 5, 0x6820(27)				# 0x680c
        rlwinm. 0,5,0,31,31
        bne write_wait
        blr
*/


#===================================================================================#
#  Debug code					                                                    #
#===================================================================================#
.if RELEASEBUILD == 0
	.include "source/QLiteIPLDbg.asm.c"
.endif
