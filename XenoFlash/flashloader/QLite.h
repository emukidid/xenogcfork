

.global		QCODEImageBase


.section absolute
.include "mn10200.h"


.org 0xfcda
ADBCTRL:

.org 0xFD83
SC0STR:

#------------------------------------------------------------------------------------------
# Read ahead dma buffer addresses
#------------------------------------------------------------------------------------------
.equ LBA_READAHEAD_BUFFER,				0x417100
.equ LBA0,						LBA_READAHEAD_BUFFER + (0x810 * 0)
.equ LBA4,						LBA_READAHEAD_BUFFER + (0x810 * 4)
.equ LBA7,						LBA_READAHEAD_BUFFER + (0x810 * 7)
.equ LBA_APPL,						LBA4		+ 0x440
.equ LBA_APPL_EPVEC,					LBA_APPL	+ 0x12
.equ LBA_APPL_SIZE,					LBA_APPL	+ 0x16
.equ LBA_APPL_CODE,					LBA_APPL	+ 0x20


#------------------------------------------------------------------------------------------
# firmware fixed addresses
#------------------------------------------------------------------------------------------
.equ FwIrqVec,						0x804c
.equ FwIrqDepth,					0x805b
.equ FwData1,						0x008000
.equ FwData1_Size,					0x6e
.equ FwBSS,						0x00806e
.equ FwInitDrive,					0x0808aa
.equ FwIntHandler,					0x80a74
.equ FwHLECmdBuffer,					0x40ebe6

#------------------------------------------------------------------------------------------
# firmware version addresses
#------------------------------------------------------------------------------------------

	.macro IMM24Bit Addr
		.byte (\Addr & 0xFF)
		.byte (\Addr >> 8 & 0xFF)
		.byte (\Addr >> 16 & 0xFF)
	.endm

	.macro FwAddrImp16 Name P1, P2, P3
		X\Name: 
		IMM24Bit \P1
		IMM24Bit \P2
		IMM24Bit \P3
		.set \Name, 0x003333 + (FWIMPORTS << 16)
		.set FWIMPORTS, FWIMPORTS+1
	.endm

	.macro FwAddrImp24 Name P1, P2, P3
		X\Name: 
		IMM24Bit \P1
		IMM24Bit \P2
		IMM24Bit \P3
		.set \Name, 0x003333 + (FWIMPORTS << 16)
		.set FWIMPORTS, FWIMPORTS+1
	.endm

	.set FWIMPORTS, 0x01;
