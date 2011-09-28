
//#include<GCLib.h>

extern void dcache_inv(void *, int);
extern volatile long *dvd;



int main()
{

				
    return 0;
}


//		else if(bDoRandomSeeks) {
//			u32 dwRandSector = RAND(0xa05ff);
//			DEBUG_ShowValueU32(350, 60, dwRandSector);
//			DVD_Read(pBuffer, 512, dwRandSector * 2048);
//			DVD_RequestError();
//		}


//		if(nKey & PAD_CRIGHT) {
//			dwCmd += 0x01000000;
//		}
//		else if(nKey & PAD_CLEFT) {
//			dwCmd -= 0x01000000;
//		}
//		else if(nKey & PAD_CUP) {
//			dwCmd = ++dwCmd & 0xFF0000FF;
//		}
//		else if(nKey & PAD_CDOWN) {
//			dwCmd = --dwCmd & 0xFF0000FF;
//		}


//		if(nKey & PAD_START) {
//			DEBUG_Print(50, 440, "** SENDING DBGMODE COMMAND... **");
//			DVD_SetDebugMode();
//			DEBUG_Print(100, 440, "** DISABLING DVD DESCRAMBLE... **");
//			DVD_DisableDescrambling();
//		}

//			DEBUG_Print(100, 440, "sending custom command...");
//			GC_Memory_memset((void*)pBuffer, 0xCC, 2048);
//			dcache_flush((void*)pBuffer, 2048);
//			DVD_CustomCommand(dwCmd, pBuffer);
//			DVD_RequestError();




//			DEBUG_Print(100, 440, "Resetting dvd drive...");
//			DVD_Reset();
//			DEBUG_Print(100, 440, "Reading id...");
//			DVD_ReadId((void*) 0x80000000);
//			DVD_RequestError();
//			DEBUG_Print(100, 440, "done.");

//			DEBUG_Print(100, 440, "dumping leadout....");
//
//			int nSec = 0xa0000;
//			u8* pDest = (u8*) (0x80040000);
//
//			for( ;nSec < 0xa0600; nSec++) {
//				DVD_Read(pDest, 2048, nSec * 2048);
//				pDest += 2048;
//			}
//			DEBUG_Print(100, 440, "done!");

//		else if(nKey & PAD_B) { 
//			DEBUG_Print(100, 440, "sending custom command...");
//			GC_Memory_memset((void*)pBuffer, 0xCC, 2048);
//			dcache_flush((void*)pBuffer, 2048);
//			DVD_CustomCommand(dwCmd, pBuffer);
//			DVD_RequestError();
//		}





//	DEBUG_Print(100, 220, "Open and close drive...");
//	while(!DVD_IsCoverOpen()) {
//	}
//	while(DVD_IsCoverOpen()) {
//	}



//		GC_PAD_Read(&sPad1, GC_PAD1);
//		if(sPad1.button & PAD_START) {
//			asm("lis 3, 0x8030");
//			asm("ori 3, 3, 0x0100");
//			asm("mtlr 3");
//			asm("blrl");
//		}



//hotreset:
//lis r3,0
//lis r9,0xCC00
//sth r3, 0x2000(r9)
//li r4, 3
//stw r4, 0x3024(r9)
//stw r3, 0x3024(r9)
//nop
//loop__: b loop__
