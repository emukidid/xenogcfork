/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«                        QLiteApploader.asm                               »©
  ©«                                                                         »©
  ©«                Copyright 2005 Anonymous Ghostwriter                     »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Description]....:  Fake apploader from scratch                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Notes......]....:  Used to patch multiboot images on the fly           »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/

.include "source/PPC.h"
.include "source/QliteGlobal.h"

.global _start

.section code
.org 0
.text


#===========================================
# Data
#===========================================
.equ baseData,				0x80000000
.equ ApplEPRet,				0x80001800
.equ ApplMainPtr,			0x80001804
.equ ApplMainAddr,			0x80001808
.equ ApplClosePtr,			0x8000180C
.equ ApplCloseAddr,			0x80001810

/*	stwu	r9, -8(sp)					# push r9
 	lis r9, ApplEPRet@h		
	stw r0, ApplEPRet@l(r9)				# save linkreg
*/


#=====================================================================================================================
# apploader header
#=====================================================================================================================
.byte 0x32, 0x30, 0x30, 0x33, 0x2F, 0x30, 0x39, 0x2F, 0x31, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
.byte 0x81, 0x20, 0x00, 0x00, 0x00, 0x00, 0x19, 0x54, 0x00, 0x01, 0xBA, 0x70, 0x00, 0x00, 0x00, 0x00

#================================================
# Hooked apploader entrypoint called by IPL
#=================================================
 _start:
	b OnApploaderEntry

#============================================================
 OnApploaderEntry:
#============================================================
	mflr r0

	GetRelocAddr r9, OnApploaderInit
	stw r9, 0(r3)
	GetRelocAddr r9, OnApploaderMain
	stw r9, 0(r4)
	GetRelocAddr r9, OnApploaderClose
	stw r9, 0(r5)

	mtlr r0
	blr
	
#==============================================================================#
 OnApploaderInit:
#==============================================================================#
	mflr r0
	mtlr r0
	blr
		
#==============================================================================#
 OnApploaderMain:								
#==============================================================================#
	mtlr r0
	
/*	li r9, 0
	stw r9, 0(r3)
	stw r9, 0(r4)
	stw r9, 0(r5)
*/
	li32 r6, 0x09642955
	bl SUB_FillVideoMem
	li32 r3, 3000
	bl SUB_Delay
	bl SUB_HotReset
	
	lis r3, 0
	mflr r0
	blr

#==============================================================================#
 OnApploaderClose:
#==============================================================================#
	mtlr r0


	
	lis r3, 0x8130
	mflr r0
	blr

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
	
	li			r5, 0x8				# 0x20000 pixels	4B000
	ori         r5, r5, 0xb000
	mtctr		r5
 loopf:
	stwu	r6, 4(r4)
	bdnz loopf
	blr

#=======================================================================
 SUB_HotReset:
#=======================================================================
	lis r3,0
	lis r9,0xCC00
	sth r3, 0x2000(r9)
	li r4, 3
	stw r4, 0x3024(r9)
	stw r3, 0x3024(r9)
	nop
 1: b 1b

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

	
