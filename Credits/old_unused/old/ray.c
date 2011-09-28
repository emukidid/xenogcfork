

typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;


#define XRES (640)
#define YRES (576)


#define INFINITE	2000		// 9999999


enum eTexture {
	TEX_FLOOR = 0,
	TEX_CEILING = 1,
	TEX_TRANS	= 3
};

extern unsigned char *bg;

static float inline isqrtf(float __x)
{
	float __z;
	__asm __volatile (
	"	frsqrte	%0,%1\n"
		: "=f" (__z)
		: "f" (__x));
  return __z;
}

/*
static float inline sin(float __x)
{
	float __z;
	__asm __volatile (
	"	sine	%0,%1\n"
		: "=f" (__z)
		: "f" (__x));
  return __z;
}*/

struct vec
{
	float x, y, z;
};


typedef struct tsphere_t
{
	struct vec center;
	float r;
	int texture;
	float reflect;   
}sphere_t;

sphere_t sphere[]={{{-5, 8, 9}, 10, 2, .92}};
//sphere_t sphere[]={{{-5, 8, 9}, 10, 2, .92}};
 
const u32 spheres=sizeof(sphere)/sizeof(*sphere);
     
typedef struct tplane_t {
	struct vec a;
	int d;
	int texture;
	float reflect;
//	float x0, x1, y0, y1;
}plane_t;

const plane_t plane[]={	{{  0,   1,  0.0},		10,		TEX_FLOOR,		0.1	},		// floor
						{{  0,   1,   0.00},	150,	TEX_CEILING,	0	},		// ceiling 
						{{  0,-0.4,   1},		200,	TEX_FLOOR,		0	},		// back plane
						{{.1 , 0.4,   1},		20,		TEX_TRANS,		0	}		// transparent plane
};		
 
#define TRANSPLANE_X0 -13
#define TRANSPLANE_X1  12
#define TRANSPLANE_Y0 -13
#define TRANSPLANE_Y1  -5

/*
const struct plane_t
{
	struct vec a;
	int d;
	int texture;
	float reflect;
	float x0, x1, y0, y1;
} plane[]={			{{0,1,0}, 10, 1, .1}, 
					{{0,1,0}, 150, 1, 0}, 
					{{0,-.4,1}, 200, 0, 0},  
					{{.1,.4,1}, 20, 3, 0,	-15, 10, -13, -5}};

*/

struct plane_t_dyn
{
	struct vec m1, m2;
} planedyn[sizeof(plane)/sizeof(*plane)];

const u32 planes=sizeof(plane)/sizeof(*plane);

static float MapTexture(int text, const struct vec *v);

static inline void Normalize(struct vec *v)
{
	float ilen=isqrtf(v->x*v->x+v->y*v->y+v->z*v->z);
	v->x*=ilen;
	v->y*=ilen;
	v->z*=ilen;
}

static float RayTrace(const struct vec *src, const struct vec *dst, int depth);
static struct vec CalcMirror(const struct vec *nor, const struct vec *v);

static void GenerateSpheres();

// float textmap[32*32];
static void GenerateTextures();

static float trace(float x, float y)
{
	struct vec src={0, 15, -15};
	struct vec v;
	v.x=(x)/(XRES/2);
	v.y=-(y)/(YRES/2); // *220.0/200.0;
	v.z=1;
	Normalize(&v);
	return RayTrace(&src, &v, 0);
}
 
u16 tracePixel(int x, int y, int aa)
{
	float col = 0;
	int ax, ay;
	for (ay=0; ay<aa; ay++)
		for (ax=0; ax<aa; ax++)
			col += trace(x +(float)ax/(float)aa-(XRES/2.0), y+(float)ay/(float)aa-(YRES/2.0));


//	return (((u16)(col) & 0xFFFC) << 6);	// opt aa = 2
	return (((u16)(col)) << 8);				// opt aa = 1
	
//	return ((u16)(col/ (aa * aa)) << 8);	// opt
//	return (u8)(col/ (aa * aa));			// org
}

static float traceSphere(const struct vec *src, const struct vec *dst, int s)
{
	float radrad=sphere[s].r;
	struct vec *center=&sphere[s].center;
	radrad*=radrad;
	float dx=src->x-center->x;
	float dy=src->y-center->y;
	float dz=src->z-center->z;
	float b=(dx*dst->x+dy*dst->y+dz*dst->z);
	float c=dx*dx+dy*dy+dz*dz-radrad;
	float dis=b*b-c;
	if (dis>0)
		return -(b)-1/isqrtf(dis);
	else
		return INFINITE;
}

static float tracePlane(const struct vec *src, const struct vec *dst, int yp)
{
	return 
		(+plane[yp].d 
			- src->x * plane[yp].a.x - src->y * plane[yp].a.y - src->z * plane[yp].a.z
		) / (dst->x * plane[yp].a.x + dst->y * plane[yp].a.y + dst->z * plane[yp].a.z);
}

static float RayTrace(const struct vec *src, const struct vec *dst, int depth)
{
	if (depth>20)
		return 0;

			/* the incredible hit2 hack saves not only the first, but also the second 
			   object, allowing exactly ONE transparent object. yey.
			   and it's buggy. wow. */
	enum obj { objNone, objSphere, objPlane } hit=objNone, hit2=objNone;
	int hito=0, hito2;
	float mint=INFINITE, mint2;
	u32 i;
	for (i=0; i<spheres; i++)
	{
		float t=traceSphere(src, dst, i);
		if ((t>.01) && (t<mint))
		{
			hit=objSphere;

			hito2 = hito;
			hit2 = hit;
			mint2 = mint;

			hito=i;
			mint=t;
		}
	}
	for (i=0; i<planes; i++)
	{
		float t=tracePlane(src, dst, i);
		if ((t>0.1) && (t<mint))
		{
			hit=objPlane;

			hito2 = hito;
			hit2 = hit;
			mint2 = mint;

			hito=i;
			mint=t;
		}
	}

	if (hit==objNone)
		return 128;

	float alphaCol = 0;
	struct vec tar;

	float col = 0, col2 = 0;
a:
	tar.x=src->x+dst->x*mint;
	tar.y=src->y+dst->y*mint;
	tar.z=src->z+dst->z*mint;
	

	switch (hit)
	{
		case objSphere:
		{
			struct vec nor;
			nor.x=(tar.x-sphere[hito].center.x)/sphere[hito].r;
			nor.y=(tar.y-sphere[hito].center.y)/sphere[hito].r;
			nor.z=(tar.z-sphere[hito].center.z)/sphere[hito].r;
			col=MapTexture(sphere[hito].texture, &nor);
			if (sphere[hito].reflect)
			{
				struct vec ndst=CalcMirror(&nor, dst);
				col=col*(1-sphere[hito].reflect)+RayTrace(&tar, &ndst, depth+1)*sphere[hito].reflect;
			}
			break;
		}
		case objPlane:
		{
			struct vec res;
			res.x = tar.x * planedyn[hito].m1.x + tar.y * planedyn[hito].m1.y + tar.z * planedyn[hito].m1.z;
			res.z = tar.x * planedyn[hito].m2.x + tar.y * planedyn[hito].m2.y + tar.z * planedyn[hito].m2.z;
			res.y = 1;

			col=MapTexture(plane[hito].texture, &res);

			if (plane[hito].reflect>0)
			{
				struct vec nor = plane[hito].a;
				struct vec ndst=CalcMirror(&nor, dst);
				col=col*(1-plane[hito].reflect)+RayTrace(&tar, &ndst, depth+1)*plane[hito].reflect;
			}
			
			if (plane[hito].texture == 3)
			{
				if ((res.x < TRANSPLANE_X0) || (res.x > TRANSPLANE_X1) || (res.z < TRANSPLANE_Y0) || (res.z > TRANSPLANE_Y1))
					alphaCol = 0;
				else
				{
					alphaCol = .7;
					col2 = col;
				}
				hito = hito2;
				hit = hit2;
				mint = mint2;
				hito2 = objNone;
				goto a;
			}

			break;
		}
		default:
		case objNone:
			return 128;
	}
	
	return col + (col2-col) * alphaCol;
}

#define ABS(x) ((x < 0) ? (-x):(x))

static float MapTexture(int text, const struct vec *v)
{
	float nZ;
	u8* pText1 = bg;



	switch (text)	{
		case TEX_CEILING:
//			nZ = ABS(v->z*v->x*v->y);
//			return(((int) (10/isqrtf(nZ))));
//			return ((bCheck * 40) +5.8 / isqrtf(nZ)) * 2;		//255 - (((int) (0.68 / isqrtf(nZ*nZ*100) )) & 255);
			
		case TEX_FLOOR:
			return ((((int)((v->x+100)/10))&1)^(((int)((v->z+100)/10))&1)) ? 0xc0 : 0x20;
	}

	int x = (int)((v->x+15)*10)&255;
	int y = (int)((v->z+13)*10)&255;
	
	return pText1[y * 256 + x];
}

static struct vec CalcMirror(const struct vec *nor, const struct vec *v)
{
	float sk=-v->x*nor->x -v->y*nor->y -v->z*nor->z;
	struct vec l;
	l.x=2*(nor->x*sk)+v->x;
	l.y=2*(nor->y*sk)+v->y;
	l.z=2*(nor->z*sk)+v->z;
	return l;
}

static void cross(struct vec *v1, const struct vec *v2, const struct vec *v3)
{
	v1->x = v2->y * v3->z - v2->z * v3->y;
	v1->y = v2->z * v3->x - v2->x * v3->z;
	v1->z = v2->x * v3->y - v2->y * v3->x;
}


void rayInit(void)
{
	u32 i;
	for (i=0; i<planes; ++i)
	{
		struct vec a={0,.6,.1};
		if (i == 0)
		{
			a.x = 1;
			a.y = 1;
			a.z = 1;
		}
		
		cross(&planedyn[i].m1, &a, &plane[i].a);
		cross(&planedyn[i].m2, &planedyn[i].m1, &plane[i].a);
	}
}
