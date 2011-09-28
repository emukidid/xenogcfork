
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef volatile u8		vu8;
typedef volatile u16	vu16;
typedef volatile u32	vu32;

//#define DBGSAVEMEM
//#define DBGKEY

/* raytrace/animation settings */
#define ANTIALIAS		2

#define PRECALCFRAMES	8					
#define VIDFRAMES		67

#define CAPSTARTY		232
#define CAPSIZEY		240	// 4B000

#define CAPSTART		640 * CAPSTARTY
#define CAPSIZE			640 * CAPSIZEY

#define SPHERE_XLEFT	-25.0
#define SPHERE_XRIGHT	35.6
#define SPHERE_XMOVE	(SPHERE_XRIGHT-SPHERE_XLEFT)
#define SPHERE_XSPEED	(SPHERE_XMOVE/VIDFRAMES)

/* memory layout */
#define MEM_WORK					0x81010000
#define MEM_FONT					0x81020000

#define MEM_FB						((0xC0F00000))					// 80080000
#define MEM_FB2						((MEM_FB + 0x500))				


//#define MEM_WORK					0x80010000
//#define MEM_FONT					0x80020000
//#define MEM_BACKGROUND			0x80000000

//#define MEM_FB					((0x80070000) | 0xC0000000)			// 80080000
//#define MEM_VIDEO					((MEM_FB & 0xBFFFFFFF) + 0xC0000)	// 0x170000)				// 80130000 801F0000  size: 14D0000
//#define MEM_FB2					((MEM_FB + 0x500))



/* video stuff */
#define R_VIDEO_FRAMEBUFFER_1		*(unsigned long*)0xCC00201C
#define R_VIDEO_FRAMEBUFFER_2		*(unsigned long*)0xCC002024

#define YBORDEROFFSET				(640*2 * 32)
#define CLAMP(x,l,h)				((x > h) ? h : ((x < l) ? l : x))
#define TENT(a, b, c)				(0.25 * (a) + 0.5 * (b) + 0.25 * (c))
#define RGB2YCBR(r,g,b)				((u32)(((u8)CLAMP((0.257 * (float)r + 0.504 * (float)g + 0.098 * (float)b +  16.0 + 0.5) , 16, 235) << 24) | ((u8)CLAMP((-0.148 * (float)r - 0.291 * (float)g + 0.439 * (float)b + 128.0 + 0.5), 16, 240) << 16) | ((u8)(0.257 * (float)r + 0.504 * (float)g + 0.098 * (float)b +  16.0 + 0.5) << 8) | (u8)CLAMP((0.439 * (float)r - 0.368 * (float)g - 0.071 * (float)b + 128.0 + 0.5), 16, 240)))


/* exi */
#define EXI_SR						ebase[0]
#define EXI_DMA_MEM					ebase[1]
#define EXI_DMA_LEN					ebase[2]
#define EXI_CR						ebase[3]
#define EXI_DATA					ebase[4]
#define EXI_WAIT_EOT				while((EXI_CR)&1);    


#define PAD_LEFT        0x0001
#define PAD_RIGHT       0x0002
#define PAD_DOWN        0x0004
#define PAD_UP          0x0008
#define PAD_Z           0x0010
#define PAD_R           0x0020
#define PAD_L           0x0040
#define PAD_A           0x0100
#define PAD_B           0x0200
#define PAD_X           0x0400
#define PAD_Y           0x0800
#define PAD_START       0x1000
#define PAD_CLEFT		0x2000
#define PAD_CRIGHT      0x4000
#define PAD_CDOWN       0x8000
#define PAD_CUP         0x10000

/*+----------------------------------------------------------------------------------------------+*/
#define MEM_DVD_BASE                    (0xCC006000)   ///< Memory address of the DVD Interface
/*+----------------------------------------------------------------------------------------------+*/
#define R_DVD_STATUS_1					/*dvd[00]*/ *(vu32*)(MEM_DVD_BASE)       ///< DVD status1 register
#define R_DVD_STATUS_2                  /*dvd[01]*/ *(vu32*)(MEM_DVD_BASE+0x04)  ///< DVD status2 register
#define R_DVD_COMMAND					/*dvd[02]*/ *(vu32*)(MEM_DVD_BASE+0x08)
#define R_DVD_SUBCOMMAND				/*dvd[02]*/ *(vu32*)(MEM_DVD_BASE+0x09)
#define R_DVD_SUBCOMMAND2				/*dvd[02]*/ *(vu32*)(MEM_DVD_BASE+0x0A)	
#define R_DVD_OFFSET					/*dvd[03]*/ *(vu32*)(MEM_DVD_BASE+0x0C)	
#define R_DVD_SOURCELENGTH				/*dvd[04]*/ *(vu32*)(MEM_DVD_BASE+0x10)		
#define R_DVD_DESTINATIONBUFFER			/*dvd[05]*/ *(vu32*)(MEM_DVD_BASE+0x14)		
#define R_DVD_DESTINATIONLENGTH			/*dvd[06]*/ *(vu32*)(MEM_DVD_BASE+0x18)		
#define R_DVD_ACTION					/*dvd[07]*/ *(vu32*)(MEM_DVD_BASE+0x1C)	
#define R_DVD_IMMBUF					/*dvd[08]*/ *(vu32*)(MEM_DVD_BASE+0x20)		
#define R_DVD_CONFIG					/*dvd[09]*/ *(vu32*)(MEM_DVD_BASE+0x24)	

#define R_DVD_ERRORCODE					R_DVD_IMMBUF


typedef struct tvec {
	float x, y, z;
} vec;

typedef struct tsphere_t {
	vec center;
	float r;
	u8 texture;
	float reflect;   
} sphere_t;

typedef struct tplane_t {
	vec a;
	int d;
	u8 texture;
	float reflect;
} plane_t;

typedef struct tplane_t_dyn
{
	vec m1, m2;
} plane_t_dyn;

enum eTexture {
	TEX_FLOOR = 0,
	TEX_CEILING = 1,
	TEX_TRANS	= 3
};


extern u8 *bg;
extern sphere_t sphere[];
extern plane_t plane[];
extern const u32 planes;
extern plane_t_dyn planedyn[];

u16 tracePixel(int x, int y, int aa);
static float RayTrace(const vec *src, const vec *dst, int depth);
static vec CalcMirror(const vec *nor, const vec *v);
static float MapTexture(int text, const vec *v);
void cross(vec *v1, const vec *v2, const vec *v3);
