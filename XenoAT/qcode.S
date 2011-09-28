
/* global settings header used by various sub-projetcs	*/
.include "../XenoGlobal.h"

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
.incbin "qcode.bin"
qcode_end:

.align 2
#--------------------------------------
 upload:
#--------------------------------------
.incbin "upload.bin"
upload_end:

.align 2
#--------------------------------------
 credits:
#--------------------------------------
.if CREDITS == 1
#	.incbin "credits.bin"
#	.incbin "sdload.bin"
	.incbin "XenoShell.bin"
.else
	.incbin "NoCredits.bin"
.endif

credits_end:


