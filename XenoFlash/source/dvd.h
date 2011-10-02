/*
#define RW	2
#define DMA	1
#define TSTART	0

#define DICMDBUF0	*0xCC006008
#define DICMDBUF1	*0xCC00600c
#define DICMDBUF2	*0xCC006010
#define DIIMMBUF	*0xCC006020
#define DICR	*0xCC00601c
*/

/*
 *	A lot of these Functions were found in Emu_Kidid's GCOS Source code
 *	as libOGC does not have DVD Debug commands built in.
 *
 *
 *
 *
 */

volatile long *dvd = (volatile long *) 0xCC006000;

void DVD_CallFunc(unsigned long fnAddress);

void DVD_CustomDbgCommand(u32 dwCommand, u32 dwOffset, u32 dwLength, u32* pBuffer);

u32 DVD_ReadDriveMemDword(u32 dwAddress);

u32 DVD_RequestError();

void DVD_SetDebugMode();

int DVD_SetDebugMode1();

int DVD_SetDebugMode2();

void DVD_WaitImmediate();

void DVD_WriteDriveMemBlock(u32 dwAddress, void* pData, u32 dwSize);

void DVD_WriteDriveMemDword(u32 dwAddress, u32 dwData);

