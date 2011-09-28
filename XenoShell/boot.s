

.global _start
.global main

.global szDriveVer

.global szIPL
.global szNTSC10
.global szXeno

_start:

b main
szDriveVer:		.string "  Drive Version X";


# configure the bootloader here
#MEMCARD_POS     = 0x00060600
#MEM_POS         = (0x80d00000 - 0x20)

MEMCARD_POS     = 0x0003A000
MEM_POS         = 0x80004000

.globl GetMSR
GetMSR:
	mfmsr 3
	blr

.globl SetMSR
SetMSR:
	mtmsr 3
	blr

#	mfmsr    3
#	rlwinm   r4,3,0,17,153
#	mtmsr    43


/*
##############################################################################################
 .globl dcache_flush
##############################################################################################
dcache_flush:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbst r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr

.globl flush_code
flush_code:
	lis r5, 0xFFFF
	ori r5, r5, 0xFFF1
	and r5, r5, r3
	subf r3, r5, r3
	add r4, r4, r3
1:
	dcbst r0, r5
	sync
	icbi r0, r5
	addic r5, r5, 8
	subic. r4, r4, 8
	bge 1b
	isync
	blr




##############################################################################################
 .globl dcache_inv
##############################################################################################

dcache_inv:
	cmplwi r4, 0		# zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbi r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr
*/





/*
.global SUB_BootFromMC
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
        stw 5, 0x6800(27)
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
        stmw 29, 0x6804(27)             # start dma read
        bl write_wait                   # wait until it's done

        addi 28, 28, 0x80               # next card address
        addi 29, 29, 0x80               # next ram address

        bdnz again                      # next block (Decrement CTR, branch if CTR non-zero) 
 
        addi 7, 7, 0x20					# add offset imposed by memcard
        mtlr 7                          # jump there!
        blr				
write:
        stw 3, 0x6810(27)               # data
        li  5, 0x8c0                    # memc
        stw 5, 0x6800(27)               #
        stw 4, 0x680c(27)
write_wait:
        lwz 5, 0x680c(27)
        rlwinm. 0,5,0,31,31
        bne write_wait
        blr
*/
