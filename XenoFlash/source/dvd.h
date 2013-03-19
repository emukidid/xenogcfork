#define RW	2
#define DMA	1
#define TSTART	0

#define DICMDBUF0	*0xCC006008
#define DICMDBUF1	*0xCC00600c
#define DICMDBUF2	*0xCC006010
#define DIIMMBUF	*(u32*)0xCC006020
#define DICR	*0xCC00601c

/*
 *	A lot of these Functions were found in Emu_Kidid's GCOS Source code
 *	as libOGC does not have DVD Debug commands built in.
 *
 *
 *
 *
 */
#define u32 unsigned long

/*
	According to YAGCD 5.7.2: Drive Debug Commands
	This function uses DICMBUF0 = 0xfe120000 to jsr the MN102
	to a particular internal memory address. This is useful because,
	by this time, the program will have already loaded its own code to the MN102.
 */
extern void DVD_CallFunc(u32 fnAddress);

int DVD_CustomDbgCommand(u32 dwCommand, u32 dwOffset, u32 dwLength, u32* pBuffer);

u32 DVD_ReadDriveMemDword(u32 dwAddress);

u32 DVD_RequestError();

int DVD_SetDebugMode();

int DVD_SetDebugMode1();

int DVD_SetDebugMode2();

int DVD_WaitImmediate();

int DVD_WriteDriveMemBlock(u32 dwAddress, void* pData, u32 dwSize);

int DVD_WriteDriveMemDword(u32 dwAddress, u32 dwData);

int DVD_ReadDriveMemBlock(u32 dwAddress, void* pData, u32 dwSize);

void dvd_unlock();