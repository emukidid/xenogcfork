

.global _start
.global main

.global szDriveVer

.global szIPL
.global szNTSC10
.global szXeno

_start:
lis 1, 0x817F
b main

.global __eabi
__eabi:
blr

.globl GetMSR
GetMSR:
	mfmsr 3
	blr

.globl SetMSR
SetMSR:
	mtmsr 3
	blr
	
.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 4, 4, 0x20 
1:
	addi 4, 4, 0x1f
	srwi 4, 4, 5
	mtctr 4
2:
	dcbf 0, 3
	icbi 0, 3
	addi 3, 3, 0x20
	bdnz 2b
	sc
	sync
	isync
	blr

szDriveVer:		.string "  Drive Version X";
