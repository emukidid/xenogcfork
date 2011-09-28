
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;


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
#define MEM_WORK					0x80010000
#define MEM_FONT					0x80020000
#define MEM_BACKGROUND				0x80000000

#define MEM_FB						((0x80070000) | 0xC0000000)			// 80080000
#define MEM_VIDEO					((MEM_FB & 0xBFFFFFFF) + 0xC0000)	// 0x170000)				// 80130000 801F0000  size: 14D0000
#define MEM_FB2						((MEM_FB + 0x500))				


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
