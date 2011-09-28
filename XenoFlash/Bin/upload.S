

.text


.equ SC0CTR, 0xfd80
.equ SC0TRB, 0xfd82
.equ SC0STR, 0xfd83
.equ P8OUT,  0xffc8
.equ ENTRY, 0x40c600


/* helper macro for correct absolute 24bit jsrs */
.macro jsrabsOrg addr			
	jsr \addr-.+\addr
.endm

.macro P81_HI
	.byte 0xF5, 0xD1, 0xC8 ,0xFF
.endm
.macro P81_LO
	.byte 0xF5, 0xD9, 0xC8 ,0xFF
.endm



#=======================================
 do_upload:
#=======================================
	and	0xf7ff, psw
	P81_LO							# avoid reset detection

	mov (0x40D100), d0				# check credit request tag
	cmp 0x4444, d0
	bne 1f

	mov 0x40D800, a0
	jsr SUB_DownloadCode

#	P81_LO							# avoid reset detection
	
	mov 0x40D100, a0
	sub d0, d0						# clear tag
	mov d0, (a0)

#	mov 0x80a74, a0					# restore org inthandler
	mov 0x40C602, a0
	mov a0, (0x804c)

	mov 0x0c, d0					# restore breakpoints
	mov d0, (0xfcda)

	jmp (a0)

1:	mov ENTRY, a0
	jsr SUB_DownloadCode
	jsrabsOrg ENTRY					# jmp to drivecode 

#=======================================
 SUB_DownloadCode:
#=======================================
	mov 0xc080, d0
	mov d0, (SC0CTR)
	
	mov 'Q', d0
	jsr SUB_SendByte
	jsr SUB_GetByte

	mov 'C', d0
	jsr SUB_SendByte
	jsr SUB_GetByte
	
	mov 256, d1
	mul d0, d1

/*	mov d0, d1
	add d1, d1
	add d1, d1
	add d1, d1
	add d1, d1
	add d1, d1
	add d1, d1
	add d1, d1
	add d1, d1
*/
	mov 'O', d0
	jsr SUB_SendByte
	jsr SUB_GetByte
	add d0, d1
	mov 'D', d0
	jsr SUB_SendByte
	jsr SUB_GetByte

	mov d1, d0
	jsr SUB_SendByte
	sub d2, d2

 download_loop:
	jsr SUB_GetByte
	jsr SUB_SendByte
	movb d0, (a0)
	add d0, d2
	jsr SUB_GetByte
	jsr SUB_SendByte
	movb d0, (1, a0)
	add d0, d2
	add 2, a0
	sub 1, d1
	bne download_loop

	mov d2, d0
	jsr SUB_GetByte
	jsr SUB_SendByte
#	jsr SUB_GetByte

	cmp 0x21, d0
	bne Reset

 exit:
#	mov 0xC080, d0
#	mov d0, (SC0CTR)
#	movb d0, (0xFD83)
	P81_LO
	rts

#=======================================
 SUB_SendByte:
#=======================================
	sub a2, a2
 waitsend:
	movb (0xFD83), d3
	and 0x80, d3
	beq 1f
	add 1, a2
	cmp 2000, a2
	beq Reset
	bra waitsend

1:	movb d0, (SC0TRB)
#	P81_LO
	rts

#=======================================
 SUB_GetByte:
#=======================================:
	sub a2, a2
 waitrecv:
	movb (0xFD83), d3
	and 0x10, d3
	bne 1f
	add 1, a2
	cmp 2000, a2
	beq Reset
	bra waitrecv

1:	movbu (SC0TRB), d0
#	P81_LO
	rts
#=======================================
 Reset:
#=======================================
#	rts
#	mov 0x80a74, a0					# restore org inthandler
#	mov a0, (0x804c)
	
#	or 0x800, psw
	mov 0x80000, a0
	jmp (a0)







/*  mov 0x40d800, a0
	mov 0x2121, d0
	mov d0,(a0)
	mov d0,(2,a0)

	sub d0, d0
	jsrabsOrg  0x8b0eb

	movb (0x81F1), d0
	and 0xfffe, d0
	movb d0, (0x81F1)

LogStep:
	add -4, a3
	mov d0,(a3)
	mov (0x40d800), d0
	add 0x01, d0
	mov d0, (0x40d800)
	movb d3, (0x40d803)
	mov (a3), d0
	add 4, a3
	rts

#	.byte 0xf3, 0xfe, 0xcf, 0x83, 0xfd, 0x00, 0xf9
#	.byte 0xf3, 0xfe, 0xc4, 0x83, 0xfd, 0x00, 0xf9

#	jsr LogStep
#	mov 0x8080, d3
#	mov d3, (SC0CTR)

*/

	
