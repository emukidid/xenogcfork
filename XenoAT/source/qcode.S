
/* global settings header used by various sub-projetcs	*/
.include "XenoGlobal.h"

.text

.global qcode
.global qcode_end
.global upload
.global upload_end
.global credits
.global credits_end


.align 2
#--------------------------------------
 qcode:
#--------------------------------------
.incbin "XenoAT/source/qcode.bin"
qcode_end:

.align 2
#--------------------------------------
 upload:
#--------------------------------------
.incbin "XenoAT/source/upload.bin"
upload_end:

.align 2
#--------------------------------------
 credits:
#--------------------------------------
.if CREDITS == 1
#	.incbin "XenoAT/source/credits.bin"
#	.incbin "XenoAT/source/sdload.bin"
	.incbin "XenoAT/source/XenoShell.bin"
.else
	.incbin "XenoAT/source/NoCredits.bin"
.endif

credits_end:


