
/* global settings header used by various sub-projetcs	*/
.include "../XenoGlobal.h"


#------------------------------------------------------------------------------------------
# Drivecode main configuration
#------------------------------------------------------------------------------------------
.set DREFIX,			1				/* fix some read errors	(1:on 2: switchable)	*/
.set DREFIXCFG,			0				/* dre fix is switchable						*/
.set READOPTS,			1				/* adjust read options							*/
.set FASTBOOT,			1				/* do fast login								*/
.set AUTHPLUSR,			1				/* enable login of dvd+rw						*/
.set AUTHDMI,			1				/* patch dmicheck for dvd-+rw					*/
.equ READ_ECMADVDS,		1				/* enable reading of dvd+-rw					*/		
.set DOREGIONPATCH,		1				/* enable PAL<->NTSC patching					*/
.set RELEASEBUILD,		1				/* dont do fancy nonsense :)					*/
.set BYPASS,			1				/* enable unload command						*/
.set AUTOBYPASS,		1				/* unload automatically on memwrite				*/
.set STANDBYMODE,		0				/* unload automatically on memwrite				*/


#------------------------------------------------------------------------------------------
# Debug Config
#------------------------------------------------------------------------------------------
.set DBGMODE,			0				/* enable various dbg functions		*/
.set DBGOUTPUT,			0				/* enableDEBUG OUTPUT via serial if */
.set USEDBGBP,			0				/* gp debug breakpoint				*/
