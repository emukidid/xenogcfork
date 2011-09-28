/*
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
  ©«                         QLiteBreakpoints.asm.c                          »©
  ©«                                                                         »©
  ©«                Copyright 2005 Anonymous Ghostwriter                     »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Description]....:  breakpoint replacement code                         »©
  ©«                                                                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [Notes......]....:                                                      »©
  ©«                                                                         »©
  ©«                                                                         »©
  ©¥~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~¥©
  ©« [History....]....:  [bc]-13.10.2005 file created                        »©
  ©«                                                                         »©
  ©Æ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Æ©
*/


 #==================================================================================#
 # [BP1] authdisk dvd+r  BP1														#
 #==================================================================================#
 BP1_AuthPlusR:

.if AUTHPLUSR==1
	tbnzAbs bConfig, 2, 1f				# set 'improved' read options
	jsr SUB_SetReadOptions				

1:	mov	FwBP1Addr_AuthDMI, a1			# set next BP1 (DMIcheck)
	mov	a1, (REG_ADB1)
	sub	d0, d0							# thumbs up ;)
	jmpabs FwBP1Addr_AuthPlusR_Exit
 #END
.endif


 #==================================================================================#
 # [BP1] auth dvd+-rw with invalid DMI												#
 #==================================================================================#
 BP1_AuthDMI:

	# status needs to be set after quick-spinup	 
	jsr SUB_SetReadIdStatus

	sub	d0, d0							#  SUPI :D
	movx (0x08, a3), d2
	mov	(0x0c, a3), a1
	add	0x10,a3
	rts
 #END

 loadoffset_1:	
	
#===========================================================#
#	Adjust read options to improve reading of DVD+-RW
#===========================================================#
.if DREFIX == 1

 .align 2
 ReadConfig:
	 .byte 0xFE, 0x10, 0x00, 0x00, 0xff, 0xff, 0x00, 0x08
#	 .ascii "That ain't working that's the way you do it..."

#--------------------------------------------
 SUB_SetReadOptions:
#--------------------------------------------
 	add -0x06, a3
	mov a0, (a3)
	mov d0, (4, a3)

	#================================================================
	 .if FASTBOOT == 1
		mov FwDriveState, a0		# patch protection opt flags
		movb (-1, a0), d0			# clr b2(4/8) & b3(6) (3)
		mov (FwFastBootMask), d1
		and d1, d0
		movb d0, (-1, a0)
	 .endif
	#================================================================
	mov ReadConfig, a0				# set read options
	jsrabs FwSetReadOptions
	
	mov 0xF0, d0					# patch retry counts
	movb d0, (0x40ec14)

	sub d0, d0						# clear mediaopt byte
	mov d0, (FwMediaOpt)

	mov (a3), a0
	mov (4, a3), d0
	add 0x06, a3
	rts
#END
.endif

.if DREFIXCFG == 1
	SUB_UndoReadOptions:
		sub d0, d0
		mov d0, (ReadConfig+6)
		mov 0x05, d0
		movb d0, (0x40ec14)
		rts
	#END
.endif


#==================================================#
#	Sets status bytes to "need readid"
#==================================================#
 SUB_SetReadIdStatus:
	mov 6, d0
	mov d0, (FwCmdState1)
	mov 3, d0
	mov d0, (FwCmdState2)
	rts
 #END

#===================================================================================#
#	[BP0] patch seed and sector offset												#
#	free regs:	a0, d0, d1															#
#	uses regs:	a0, d0, d1															#
#===================================================================================#
 BP0_ReadECMA:

	and	0xf7ff,psw

	#===================================================#
	# patch disc-auth status and quick login flags		#
	#===================================================#

	#===================================================#
	# Skip LLCmd patches if ECMA reading disabled		#
	#===================================================#
	tbnzAbs bConfig, 0, noPatchECMA

	#===================================#
	#  check for interesting llcmds     #
	#===================================#
	mov	(a0), d1
	cmp 0x0006, d1
	beq 1f
	cmp	0xF000, d1
	beq	2f

	tbnzAbs bConfig, 1, noAudioFix
	cmp	0x0608, d1
	beq 3f

 noAudioFix:
	bra noPatchECMA

	#===============================================#
	# LLCMD 0600 (Transfer sector buffer)			#
	# patch sector offset, and inject regionpatch	#
	#===============================================#
1:	mov	(0x06, a0), d1				
	add 6, d1						
	mov	d1, (0x06, a0)
	bra noPatchECMA
	#===============================================#
	# LLCMD 00F0 (Controller setup)					#
	# patch to standart dvd seed setting			#
	#===============================================#
2:	mov	0, d0						
	movb d0, (0x09, a0)		
	bra noPatchECMA
	#===============================================#
	# LLCMD 0806 (Stream audio)						#
	# adjust offset									#
	#===============================================#
3:	mov	0x02, d1
	movb d1, (0x01, a0)

 noPatchECMA:
	#===================================================================
	.if DOREGIONPATCH == 1
		mov	(a0), d1						# inject sector data 
		cmp	0x0006, d1						# on llcmd 06 00
		bne 1f
		jsr SUB_OnLBAInject	
 		1:
	.endif
 	#===================================================================
	jmpabs	FwBP0Addr_ReadECMA_Exit			# exit to sendllcmd
 #END

.if DOREGIONPATCH == 1
#	.ascii "money for nothing and chips for free"
#===================================================================================#
#	Patch sector data																#
#===================================================================================#
 SUB_OnLBAInject:

	tbnzAbs bConfig, 3, noPatchLBA

	mov (FwReadLBA), a0						# check for LBA 0
	beq 1f
	bra noPatchLBA

1:	mov a1, (0x40d900 + 0x04)
	mov d1, (0x40d900 + 0x08)
	mov d2, (0x40d900 + 0x0A)

	jsr SUB_PatchAPPLoader

	mov (0x40d900 + 0x04), a1				# pop regs
	mov (0x40d900 + 0x08), d1
	mov (0x40d900 + 0x0A), d2

 noPatchLBA:
	rts
#END


/***********************************************************************/
 SUB_PatchAPPLoader:			/* (a0) ptr to sector data  */
/***********************************************************************/

	mov (dwUsrSectorOffset), a0				# feed apploader patch with the disc 
	movbu (LBA0 + 0x440 + 0x1B, a0), d0		# region code and the original EP address
	movb d0, (Apploader_patch + 0x07)		# 32bit at offset 0x08	

	mov (LBA_APPL_EPVEC, a0), d0			 
	mov d0, (Apploader_patch + 0x0a)		 

	#---------------------------------------------------------------#
	#- append video init code to apploader                         -#
	#---------------------------------------------------------------#
	mov (LBA_APPL_SIZE, a0), d0				# get appl size
	mov d0, (REG_BSWAPL)					# byteswap size
	mov (REG_BSWAPH), d0
	
	add 0x460, d0
	and 0xF800, d0							# round size to next lba
	add 0x0800, d0							 
	mov d0, d1

	sub 0x460, d0
	
	mov d0, (REG_BSWAPL)					# byteswap back
	mov (REG_BSWAPH), d2
	mov d2, (LBA_APPL_EPVEC, a0)			# set new entrypoint to end of apploader
	
	add Applpatch_size, d0					# add patchcode size
	
	mov d0, (REG_BSWAPL)					# byteswap back
	mov (REG_BSWAPH), d0
	mov d0, (LBA_APPL_SIZE, a0)				# write new size
	
	# calculate lba of appended code
	mov d1, d0

	sub	d1, d1								# d0 /= 2048
	mov	d1, mdr
	mov 2048, d1
	divu d1, d0

	add 4, d0
	mov 0x810, d1							# lba *= 0x810
	mulu d1, d0
	
	add LBA_READAHEAD_BUFFER, a0
	add d0, a0

	mov Apploader_patch, a1					# inject apploader code
	mov Applpatch_size, d0
	jsr SUB_Memcpy

	mov	FwBP1Addr_AuthPlusR, a0				# patching is done, set next 
	mov	a0, (REG_ADB1)						# breakpoint on disc auth again

	bitset bConfig, 3						# set regionpatch-done flag
	rts
#END


.endif

