

/*---------------------------------------------------------------------\
				XenoGC Global Settings Header
\---------------------------------------------------------------------*/
.set ENABLE_CREDITS, 1




/*------------------------------------------\
	mapping to internal flags (c/asm)
\------------------------------------------*/
.if ENABLE_CREDITS == 0
	.set CREDITS,	0
	#define CREDITS	0
.else 
	.set CREDITS,	1
	#define CREDITS	1
.endif




