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

	sub d0, d0
	mov d0, (0x40ec60)				# disable HLELog buffer

	mov (0x40D100), d0				# check upload request tag
	cmp 0x4444, d0
	bne uploadDrivecode

	mov 0x40D800, a0				# one up ?
	jsr SUB_DownloadCode			# download credits binary

	mov 0x40D100, a0
	sub d0, d0						# clear upload request tag
	mov d0, (a0)

#	mov 0xad2b, d0
#	mov 0x40D7FC, a0				# set upload done tag
#	mov d0, (a0)
#	mov d0, (2, a0)

	mov 0x80a74, a0					# restore org inthandler
	mov a0, (0x804c)
	jmp (a0)

 #============================
  uploadDrivecode:
 #============================
	mov ENTRY, a0
	jsr SUB_DownloadCode

 toEntry:
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

	cmp 0x21, d0
	bne Reset

 exit:
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
	rts

#=======================================
 Reset:
#=======================================
#	or 0x800, psw
#	P81_HI

	mov 0x80a74, a0					# restore org inthandler
	mov a0, (0x804c)

	mov 0x80000, a0
	jmp (a0)


