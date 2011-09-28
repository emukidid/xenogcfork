#include "ray.h"


#define XRES (640)
#define YRES (640)

#define INFINITE	2000		// 9999999

#define TRANSPLANE_XPOS		-8
#define TRANSPLANE_YPOS		2
#define TRANSPLANE_XSIZE	16
#define TRANSPLANE_YSIZE	05

#define TRANSPLANE_X0 TRANSPLANE_XPOS
#define TRANSPLANE_Y0 TRANSPLANE_YPOS
#define TRANSPLANE_X1 (TRANSPLANE_XPOS + TRANSPLANE_XSIZE)
#define TRANSPLANE_Y1 (TRANSPLANE_YPOS + TRANSPLANE_YSIZE)


sphere_t sphere[]={	{{SPHERE_XLEFT, 10, 14}, 6, 2,   .91},
					{{28,			12, 5}, 11,  2,  .92},
};

plane_t plane[]={	{{  0,   1,  -0.3},		0,			TEX_FLOOR,		0.1	},		// floor
					{{  0,   0.3,  0},		120,		TEX_CEILING,	0.0	},		// ceiling 
//					{{  0,   -0.3, 0},		120,		TEX_CEILING,	0.0	},		// ceiling 
//					{{  0,	 -.3,  1},		200,		TEX_CEILING,	0.0	},		// back plane
					{{	0.020,   0.4,  1.0},	20,		TEX_TRANS,		0.0	}		// transparent plane
};		

plane_t_dyn planedyn[sizeof(plane)/sizeof(*plane)];

const u32 spheres	= sizeof(sphere) / sizeof(*sphere);
const u32 planes	= sizeof(plane)  / sizeof(*plane);

static float inline isqrtf(float __x)
{
	float __z;
	__asm __volatile (
	"	frsqrte	%0,%1\n"
		: "=f" (__z)
		: "f" (__x));
  return __z;
}


static inline void Normalize(vec *v)
{
	float ilen=isqrtf(v->x*v->x+v->y*v->y+v->z*v->z);
	v->x*=ilen;
	v->y*=ilen;
	v->z*=ilen;
}


vec vCamPos	= {0, 8.01, -15};

static float trace(float x, float y)
{
	vec v;
	v.x	=	 (x) / (XRES / 2);
	v.y	=	-(y) / (YRES / 2);	// *220.0/200.0;
	v.z	=	1;

	Normalize(&v);
	return RayTrace(&vCamPos, &v, 0);
}

u16 tracePixel(int x, int y, int aa)
{
	float col = 0;
	int ax, ay;
	for (ay=0; ay<aa; ay++)
		for (ax=0; ax<aa; ax++)
			col += trace(x +(float)ax/(float)aa-(XRES/2.0), y+(float)ay/(float)aa-(YRES/2.0));

	#if ANTIALIAS == 2
		//u16 wCol = (((u16)(col) & 0xFFFC) << 6);
		//return  (wCol < (40<<8)) ? wCol | 0x80 : (wCol | ((x&1) ? 0x80:0xa0));// opt aa = 2
		return ((((u16)(col)) & 0x3FFC) << 6) | 0x80;
	#else 
		return (((u16)(col)) << 8) | 0x80;			// opt aa = 1
	#endif
	
//	return ((u16)(col/ (aa * aa)) << 8);	// opt
//	return (u8)(col/ (aa * aa));			// org
}

static float traceSphere(const vec *src, const vec *dst, int s)
{
	float radrad=sphere[s].r;
	vec *center=&sphere[s].center;
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

static float tracePlane(const vec *src, const vec *dst, int yp)
{
	return 
		(+plane[yp].d 
			- src->x * plane[yp].a.x - src->y * plane[yp].a.y - src->z * plane[yp].a.z
		) / (dst->x * plane[yp].a.x + dst->y * plane[yp].a.y + dst->z * plane[yp].a.z);
}

static float RayTrace(const vec *src, const vec *dst, int depth)
{
	if (depth>50)
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
	vec tar;

	float col = 0, col2 = 0;
a:
	tar.x=src->x+dst->x*mint;
	tar.y=src->y+dst->y*mint;
	tar.z=src->z+dst->z*mint;
	

	switch (hit)
	{
		case objSphere:
		{
			vec nor;
			nor.x=(tar.x-sphere[hito].center.x)/sphere[hito].r;
			nor.y=(tar.y-sphere[hito].center.y)/sphere[hito].r;
			nor.z=(tar.z-sphere[hito].center.z)/sphere[hito].r;
			col=MapTexture(sphere[hito].texture, &nor);
			if (sphere[hito].reflect)
			{
				vec ndst=CalcMirror(&nor, dst);
				col=col*(1-sphere[hito].reflect)+RayTrace(&tar, &ndst, depth+1)*sphere[hito].reflect;
			}
			break;
		}
		case objPlane:
		{
			vec res;
			res.x = tar.x * planedyn[hito].m1.x + tar.y * planedyn[hito].m1.y + tar.z * planedyn[hito].m1.z;
			res.z = tar.x * planedyn[hito].m2.x + tar.y * planedyn[hito].m2.y + tar.z * planedyn[hito].m2.z;
			res.y = 1;

			col=MapTexture(plane[hito].texture, &res);

			if (plane[hito].reflect>0)
			{
				vec nor = plane[hito].a;
				vec ndst=CalcMirror(&nor, dst);
				col=col*(1-plane[hito].reflect)+RayTrace(&tar, &ndst, depth+1)*plane[hito].reflect;
			}
			
			if (plane[hito].texture == 3)
			{
				if ((res.x < TRANSPLANE_X0) || (res.x > TRANSPLANE_X1) || (res.z < TRANSPLANE_Y0) || (res.z > TRANSPLANE_Y1)) {
					alphaCol = 0;
				}
				else {
					alphaCol = 0.8;//((res.x*res.x) / 210)+0.7;
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


static float MapTexture(int text, const vec *v)
{
	u8 bShade;
	u8 bCheck = ((((int)((v->x+100)/10))&1) ^ (((int)((v->z+100)/10))&1));

	switch (text)	{

		case TEX_FLOOR:
			return  bCheck ? 0xd0:0x10;

		case TEX_CEILING:
			bShade =   (((v->z * v->z + v->x * v->x)) / 2000);
			return 255 - bShade;
			//return 255 - ATMOST((0.0005f * (v->z*v->z + v->x*v->x)), 255);
			//return (220 - (( (0.04 / isqrtf(ABS(v->z*v->z*v->x))))));
			//return ((0.5 / isqrtf(v->z*v->z)));
			//return  bCheck ? 0xc0 : 0x40;
			//return ((bCheck * 40) +5.8 / isqrtf(nZ)) * 2;		//255 - (((int) (0.68 / isqrtf(nZ*nZ*100) )) & 255);
	}
	
	u8 x = (u8) (((TRANSPLANE_X1)- v->x)*16);
	u8 y = (u8) (((TRANSPLANE_Y1)- v->z)*16);
	
	return bg[(y * 256) + x];
}

static vec CalcMirror(const vec *nor, const vec *v)
{
	float sk=-v->x*nor->x -v->y*nor->y -v->z*nor->z;
	vec l;
	l.x=2*(nor->x*sk)+v->x;
	l.y=2*(nor->y*sk)+v->y;
	l.z=2*(nor->z*sk)+v->z;
	return l;
}

void cross(vec *v1, const vec *v2, const vec *v3)
{
	v1->x = v2->y * v3->z - v2->z * v3->y;
	v1->y = v2->z * v3->x - v2->x * v3->z;
	v1->z = v2->x * v3->y - v2->y * v3->x;
}

