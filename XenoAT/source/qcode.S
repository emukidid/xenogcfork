
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
.incbin "source/qcode.bin"
qcode_end:

.align 2
#--------------------------------------
 upload:
#--------------------------------------
.incbin "source/upload.bin"
upload_end:

.align 2
#--------------------------------------
 credits:
#--------------------------------------
.if CREDITS == 1
#	.incbin "source/credits.bin"
#	.incbin "source/sdload.bin"
	.incbin "source/XenoShell.bin"
.else
	.incbin "source/NoCredits.bin"
.endif

credits_end:


