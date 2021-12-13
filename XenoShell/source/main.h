
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef volatile u8		vu8;
typedef volatile u16	vu16;
typedef volatile u32	vu32;

#define BS_ADDRESS 0x80800000

/*** memory layout ***/
#define MEM_TEMP		0x81000000
#define MEM_WORK		0x81010000
#define MEM_FONT		0x81020000
#define MEM_FB			((0xC0F00000))	// 80080000
#define MEM_FB2			((MEM_FB + 0x500))

#define GC_INIT_BASE		 (0x80000020)
#define GC_INIT_BASE_PTR	(u32*)GC_INIT_BASE

/*** video stuff ***/
#define VI_BASE			(u32*) 0xCC002000
#define VI_BASE2		(u32*) 0xCC002040
#define MBLIST_Y		(3 * 24)

#define R_VIDEO_FRAMEBUFFER_1	*(unsigned long*)0xCC00201C
#define R_VIDEO_FRAMEBUFFER_2	*(unsigned long*)0xCC002024

#define YBORDEROFFSET		(640*2 * 32)
#define CLAMP(x,l,h)		((x > h) ? h : ((x < l) ? l : x))
#define TENT(a, b, c)		(0.25 * (a) + 0.5 * (b) + 0.25 * (c))
#define RGB2YCBR(r,g,b)		((u32)(((u8)CLAMP((0.257 * (float)r + 0.504 * (float)g + 0.098 * (float)b +  16.0 + 0.5) , 16, 235) << 24) | ((u8)CLAMP((-0.148 * (float)r - 0.291 * (float)g + 0.439 * (float)b + 128.0 + 0.5), 16, 240) << 16) | ((u8)(0.257 * (float)r + 0.504 * (float)g + 0.098 * (float)b +  16.0 + 0.5) << 8) | (u8)CLAMP((0.439 * (float)r - 0.368 * (float)g - 0.071 * (float)b + 128.0 + 0.5), 16, 240)))

/*** exi definitions ***/
#define EXI_SR		ebase[0]
#define EXI_DMA_MEM	ebase[1]
#define EXI_DMA_LEN	ebase[2]
#define EXI_CR		ebase[3]
#define EXI_DATA	ebase[4]
#define EXI_WAIT_EOT	while((EXI_CR)&1);    

/* time */
#define TB_CLOCK	40500000
#define mftb(rval) ({unsigned long u; do { \
	 asm volatile ("mftbu %0" : "=r" (u)); \
	 asm volatile ("mftb %0" : "=r" ((rval)->l)); \
	 asm volatile ("mftbu %0" : "=r" ((rval)->u)); \
	 } while(u != ((rval)->u)); })

typedef struct {
	unsigned long l, u;
} tb_t;

/*** memcard stuff ***/
#define MEMCARD_EXI_READ_SIZE		0x200
#define MEMCARD_BLOCK_SIZE			0x2000

#define MEMCARD_A_EXI_REG_BASE 0xCC006800
#define MEMCARD_B_EXI_REG_BASE 0xCC006814

struct MemCardHeader_t
{
	u8 unknown[12];
	u8 ostimeValue[8];
	u8 unknown2[12];
	u16 zeropadding;
	u16 memcardSizeInBits;
	u16 encoding;
	u8 unused[468];
	u16 updateCounter;
	u16 checksum1;
	u16 checksum2;
	u8 unused2[7680]
} __attribute__((__packed__));

typedef struct MemCardHeader_t MemCardHeader;

struct DirectoryEntry_t
{
	u32 gamecode;
	u16 makercode;
	u8 unused;
	u8 animKey;
	char filename[32]; // Ptr to a 32 char string
	u32 timestamp;
	u32 imageDataOffset;
	u16 iconsgfxformat;
	u16 iconsanimspeed;
	u8 filePermission;
	u8 copyCounter;
	u16 firstBlockIndex;
	u16 fileLength;
	u16 unused2;
	char* commentsStrings; // Apparently points to 2 32 char strings that must fit in a single block 
} __attribute__((__packed__));

typedef struct DirectoryEntry_t DirectoryEntry;

#define DIRECTORY_SIZE 127

struct Directory_t
{
	DirectoryEntry entries[DIRECTORY_SIZE];
	u8 padding[0x3A];
	u16 updateCounter;
	u16 checksum1;
	u16 checksum2;
} __attribute__((__packed__));

typedef struct Directory_t Directory;

struct Fat_t
{
	u16 checksum1;
	u16 checksum2;
	u16 updateCounter;
	u16 numFreeBlocks;
	u16 lastAllocatedBlockindex;
	u16 blockAllocTable[0xffb];
} __attribute__((__packed__));

typedef struct Fat_t Fat;

#define INVALID_BLOCK 0x0000
#define LAST_BLOCK 0xFFFF

typedef struct
{
	MemCardHeader header;
	Directory directory1;
	Directory directory2;
	Fat fat1;
	Fat fat2;
} MemCard;

#define XENO_DOL_NAME "xeno.dol"