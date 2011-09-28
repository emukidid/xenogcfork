

.global		QCODEImageBase


.section absolute
.include "source/mn10200.h"


.org 0xfcda
ADBCTRL:

.org 0xFD83
SC0STR:

.org 0xffd8
_P8IN:


/*
.org 0x40c5f0
 bMBOffsetEnable:

  .org 0x40c5f4
 dwLBAOffset:
*/

#------------------------------------------------------------------------------------------
# Read ahead dma buffer addresses
#------------------------------------------------------------------------------------------
.equ LBA_READAHEAD_BUFFER,						0x417100
.equ LBA0,										LBA_READAHEAD_BUFFER + (0x810 * 0)
.equ LBATMP,									0x40F100
.equ LBA10,										LBA_READAHEAD_BUFFER + (0x810 * 0x10)
.equ LBA4,										LBA_READAHEAD_BUFFER + (0x810 * 0x04)
.equ LBA7,										LBA_READAHEAD_BUFFER + (0x810 * 7)
.equ LBA_APPL,									LBA4		+ 0x440
.equ LBA_APPL_EPVEC,							LBA_APPL	+ 0x12
.equ LBA_APPL_SIZE,								LBA_APPL	+ 0x16
.equ LBA_APPL_REGION,							LBA0 + 0x440 + 0x1b


#------------------------------------------------------------------------------------------
# firmware fixed addresses
#------------------------------------------------------------------------------------------
.equ FwIrqVec,									0x804c		
.equ FwIrqDepth,								0x805b	
.equ FwData1,									0x008000
.equ FwData1_Size,								0x6e
.equ FwBSS,										0x00806e
.equ FwInitDrive,								0x0808aa
.equ FwIntHandler,								0x80a74
.equ FwHLECmdBuffer,							0x40ebe6

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

	
/*	.macro FwAddrImp16 Name P1, P2, P3
		X\Name: 
		IMM24Bit \P1
		.word (\P1)-(\P2)
		.word (\P1)-(\P3)
		.set \Name, 0x003333 + (FWIMPORTS << 16)
		.set FWIMPORTS, FWIMPORTS+1
	.endm

	.macro FwAddrImp24 Name P1, P2, P3
		FwAddrImp16 \Name \P1, \P2, \P3 
	.endm
*/	

	.set FWIMPORTS, 0x01;


		
/*
FwAddrImp16 FwInitState1				0x807D,	0x8071, 0x8079
FwAddrImp16 FwInitState2				0x8082, 0x8076, 0x807e
FwAddrImp16 FwLastHLECmd				0x80f4, 0x80e8, 0x80f0
FwAddrImp24 FwInitDriveLow				0x08aed8, 0x08b1f5, 0x08af25
FwAddrImp24 FwInitSpinup				0x083bad, 0x083ba5, 0x083bad
FwAddrImp24 FwStartDrive				0x0878f8, 0x0878f0, 0x0878f8
FwAddrImp24 FwSetInitState				0x08aee8, 0x08b205, 0x08af35
FwAddrImp24 FwContinueReset				0x08d9b6, 0x08d956, 0x08da11
FwAddrImp24 FwInitThread				0x08dd0d, 0x08dcad, 0x08dd68
FwAddrImp24 FwThreadSetup				0x08ea03, 0x08e9a1, 0x08ea55
*/
