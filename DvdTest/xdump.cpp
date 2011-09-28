
#define CARD_FILENAME     32
#define CARD_MAXFILES    127
#define CARD_MAXICONS      8
#define CARD_SECTORSIZE 8192

#define CARD_ERROR_NONE     0
#define CARD_ERROR_NOCARD  -1
#define CARD_ERROR_CORRUPT -2
#define CARD_ERROR_FATAL   -3
#define CARD_ERROR_INIT    -4
#define CARD_ERROR_NOSPACE -5
#define CARD_ERROR_NOENTRY -6
#define CARD_ERROR_EXISTS  -7

/* Banner & Icon defines */
#define CARD_BANNER_NONE  0
#define CARD_BANNER_CI    1
#define CARD_BANNER_RGB   2
#define CARD_BANNER_W    96
#define CARD_BANNER_H    32
#define CARD_ICON_NONE    0
#define CARD_ICON_CI      1
#define CARD_ICON_RGB     2
#define CARD_ICON_LOOP    0
#define CARD_ICON_BOUNCE  4
#define CARD_ICON_W      32
#define CARD_ICON_H      32

typedef struct {
  unsigned char gamecode[5];         // Game code
  unsigned char company[3];          // Company code
  unsigned char banner_fmt;          // Banner format
  unsigned char filename[CARD_FILENAME+1]; // Filename
  unsigned long time;                // Time of last modification
  unsigned long icon_addr;           // Offset of image data (must be 0-511)
  unsigned short icon_fmt;           // Icon formats
  unsigned short icon_speed;         // Icon speeds
  unsigned char attrib;              // File attribute
  unsigned long comment_addr;        // Offset of comment
  unsigned long size;                // Size in bytes
  int filenum;                       // File number
} GC_Card_File;

typedef struct {
  unsigned long banner;                       // Offset to banner data
  unsigned long banner_tlut;                  // Offset to banner's TLUT
  unsigned long icons[CARD_MAXICONS];         // Offsets for the icons
  unsigned long icons_tlut;                   // Offset of the icons' TLUT
  unsigned long data;                         // Offset to the save data
} GC_Card_Offsets;


typedef struct {
  unsigned char gamecode[4];         // Game code
  unsigned char company[2];          // Company code
  unsigned char padding_ff;          // Always 0xff
  unsigned char banner_fmt;          // Banner format
  unsigned char filename[CARD_FILENAME];  // Filename
  unsigned long time;                // Time of last modification
  unsigned long icon_addr;           // Offset of image data (must be 0-511)
  unsigned short icon_fmt;           // Icon formats
  unsigned short icon_speed;         // Icon speeds
  unsigned char attrib;              // File attribute
  char padding_00;                   // Always 0x00
  unsigned short block;              // Block/sector file starts at
  unsigned short length;             // Length in blocks/sectors
  unsigned short padding_ffff;       // Always 0xffff;
  unsigned long comment_addr;        // Offset of comment 
} GC_Card_DirEntry;


typedef struct {
  GC_Card_DirEntry entries[CARD_MAXFILES];    // Entries for files
  unsigned char padding[58];                  // Unused bytes after last entry
  unsigned short num;                         // Increases with each update
  unsigned short checksum1;                   // 1st part of checksum
  unsigned short checksum2;                   // 2nd part of checksum
} GC_Card_DirBlock;


int curdir = 0;
GC_Card_DirBlock dirblock[2] __attribute__ ((aligned (32)));
//GC_Card_FatBlock fatblock[2] __attribute__ ((aligned (32)));


int GC_Card_GetOffsets(GC_Card_File *cfile, GC_Card_Offsets *offsets) {

  unsigned long addr, bsize, isize;
  int i, format, needs_tlut = 0;

  if (cfile->filenum < 0 || cfile->filenum >= CARD_MAXFILES)
    return CARD_ERROR_FATAL;

  addr = cfile->icon_addr;
  bsize = CARD_BANNER_W * CARD_BANNER_H;
  isize = CARD_ICON_W * CARD_ICON_H;

  // The banner is the first data at the icon address.

  if (cfile->banner_fmt & CARD_BANNER_CI) {

    // Color Index format = 8bit per pixel and 256x16bit palette.

    offsets->banner = addr;
    offsets->banner_tlut = addr + bsize;
    addr += bsize + 512;
  }
  else if (cfile->banner_fmt & CARD_BANNER_RGB) {

    // RGB5/RGB4A3 = 16bits per pixel, no palette.

    offsets->banner = addr;
    offsets->banner_tlut = 0xffff;
    addr += bsize * 2;
  }
  else {
    offsets->banner = 0xffff;
    offsets->banner_tlut = 0xffff;
  }

  for (i = 0; i < CARD_MAXICONS; i++) {

    // icon_fmt has 2 bits per icon to describe format of each one

    format = cfile->icon_fmt >> (i * 2);

    if (format & CARD_ICON_CI) {
      offsets->icons[i] = addr;
      addr += isize;
      needs_tlut = 1;
    }
    else if (format & CARD_ICON_RGB) {
      offsets->icons[i] = addr;
      addr += isize * 2;
    }
    else 
      offsets->icons[i] = 0xffff;      
  }

  if (needs_tlut) {
    offsets->icons_tlut = addr;
    addr += 512;
  }
  else
    offsets->icons_tlut = 0xffff;

  offsets->data = addr;

  return CARD_ERROR_NONE;
}

int strlen(unsigned char *pText)
{
   int iCnt=0;

   while(*pText++ != '\0') iCnt++;
   return iCnt;
}


/*
  Opens a memcard file.
*/

int GC_Card_OpenFile(GC_Card_File *cfile, GC_Card_Offsets *offsets) {

  GC_Card_DirEntry *entry;
  int i;

  cfile->filenum = -1;

  if (strlen(cfile->filename) > CARD_FILENAME ||
      strlen(cfile->gamecode) != 4 ||
      strlen(cfile->company) != 2)
    return CARD_ERROR_FATAL;

  entry = dirblock[curdir].entries;

  for (i = 0; i < CARD_MAXFILES; i++) {
          if (entry[i].gamecode[0] != 0xff) {

      if (memcmp(entry[i].gamecode, cfile->gamecode, 4) == 0 &&
          memcmp(entry[i].company, cfile->company, 2) == 0 &&
          memcmp(entry[i].filename, cfile->filename, CARD_FILENAME) == 0) {

        break;
      }
    }
  }

  // If the for loop ran it's course, then no file matches.
  if (i >= CARD_MAXFILES)
    return CARD_ERROR_NOENTRY;

  // File exists, so fill in the supplied structures.
  memcmp(cfile->filename, entry[i].filename, CARD_FILENAME);
  memcmp(cfile->gamecode, entry[i].gamecode, 4);
  memcmp(cfile->company, entry[i].company, 2);
  cfile->filename[32] = '\0';
  cfile->gamecode[4] = '\0';
  cfile->company[2] = '\0';

  cfile->banner_fmt = entry[i].banner_fmt;
  cfile->time = entry[i].time;
  cfile->icon_addr = entry[i].icon_addr;
  cfile->icon_fmt = entry[i].icon_fmt;
  cfile->icon_speed = entry[i].icon_speed;
  cfile->attrib = entry[i].attrib;
  cfile->comment_addr = entry[i].comment_addr;
  cfile->size = entry[i].length * CARD_SECTORSIZE;
  cfile->filenum = i;

  GC_Card_GetOffsets(cfile, offsets);

  return CARD_ERROR_NONE;
}


int GC_Card_ReadFile(GC_Card_File *cfile, void *buf) 
{
	GC_Card_DirEntry *entry;
	unsigned short block;
	int i;
	
	if (card_init != 1)
		return CARD_ERROR_INIT;
	
	if (cfile->filenum < 0 || cfile->filenum >= CARD_MAXFILES)
		return CARD_ERROR_FATAL;
	
	entry = &dirblock[curdir].entries[cfile->filenum];
	block = entry->block;
	
	GC_Memory_DCInvalidateRange(buf, cfile->size);
	
	for (i = 0; i < entry->length; i++) {
		if (block < CARD_SYSAREA || block >= numsectors)
			return CARD_ERROR_CORRUPT;
		
		GC_Card_ReadArray(curslot, buf+i*CARD_SECTORSIZE, block*CARD_SECTORSIZE, CARD_SECTORSIZE);
		
		// Look up the next block the files takes up in the fat
		block = fatblock[curfat].fat[block-5];
	}
	
	GC_Memory_DCFlushRange(buf, cfile->size);
	return CARD_ERROR_NONE;
}

void CMD_ReadMemcardDol()
{
    GC_Card_File file;
    GC_Card_Offsets offsets;
    
	memset((char*)&file, 0, sizeof(GC_Card_File));
    memcpy(file.gamecode,	(void*) "DOLX", 4);
    memcpy(file.company,	(void*) "00", 2);
    memcpy(file.filename,	(void*) "level3.dol", 11);

    int nError = GC_Card_OpenFile(&file, &offsets);
}




/*	.if READ_ECMADVDS == 1
		mov	BP0Addr_ReadECMA, a1			# set next breakpoint for read patch
		mov	a1, (REG_ADB0)
		mov ADB0_ON | ADB1_ON, d0
		movb d0, (REG_ADBCTRL)
	.endif
*/

/*	add	-0x08,  a3
	and	0xf7ff,psw
	nop
	nop
	
	#------------------------#
	# init command			 #
	#------------------------#
	mov	0x05, d0	
	mov	d0, (a3)
	sub	d0, d0
	mov	d0, (0x02, a3)
	mov	0x1a, d0
	jsrabs	0x8b085			

#	add 0x9900, d0
	
	mov d0, (0x80d0)
	mov d0, (0x80d2)
	mov d0, (0x80f6)					# cbd lba
	movb d0, (0x40ebb6)
	movb d0, (0x40ebb8)

	movb d0, (0x40ebb7)
	movb d0, (0x40ebb8)
	movb d0, (0x40ebb9)

	mov	0x04, d0		
	mov 0x40ebb6, a0
	jsrabs 0x829a7						# copy2di
*/	

#	mov 0x40d800, a0					# clear dbgout mem
#	mov 0xBB, d0
#	mov 0x200, d1
#	jsrabs FWMemset

/*	mov (0x8078), d0					# copy timer to dma buffer
	mov d0, (0x40d800)

	mov	0xd800, a0
	mov	0x800, d0			
	jsrabs 0x0829a7						# copy2DI

	mov 0, d0							# clear error regs
	mov d0, (0x80d0)
	mov d0, (0x80d2)
*/

#	mov 0xE0, d0						# execute cbd
#	movb d0, (0x80a8)
#	mov 0x01, d0
#	mov d0, (0x80d6)

#	mov 0x0, d0							# set lasthlecmd 2
#	mov d0, (0x80E8)
	
	#=========================#
	# TEST: adust error bit   #
	#=========================#
#	movb (0xFFC5), d0
#	xor 0xf, d0
#	movb d0, (0xFFC5)
#	bitset 0xFFC4, 7					# bit3 in dvd[0] (error)
#	bitset 0xFFC4, 6					# bit0 in dvd[1] (cover open)
#	jsrabs 0x82E10	
#	jsrabs 0x82E49						# both: err

#	mov	0x02, d0
#	mov	d0, (0x8192)
#	sub	d0, d0							# 00 | 02
#	jsrabs	0x83ae8						# execSMevent

#	jsrabs 0x84A65
#	jsrabs 0x84A68						# cmdE0 end
	
#	mov	0x02, d0
#	mov	d0, (0x8192)
#	sub	d0, d0							# 00 | 02
#	jsrabs	0x83ae8						# execSMevent
	
#	jsrabs 0x84A65						# cmde0

#	jsrabs 0x83F39						# on cdb reqerror
	
#	jsrabs 0x81970

#	mov	0x01, d0
#	mov	d0, (0x8194)
#	jsrabs	0x83ae8						# execSMevent

#	or 0x800, psw
#	add	0x08, a3




		mov 0x02, d0				# init port 80/81
		movb d0, (P8OUT)

		mov DbgDumpSize, a0			# clear dbgout mem
		mov 0x21, d0
		mov 0x800, d1
		jsrabs FWMemset

		# jsrabs FWInitDrive

		mov	data1, a0				# copy initialized data section 
		mov	data1_copy_from, d1
		mov	data1_size, d0
		jsrabs FWMemcpy

		mov	bss, a0					# clear the bss section
		sub	d0, d0
		mov	bss_size, d1
		jsrabs FWMemset


/***********************************************************************/
 SUB_PatchLBA4Test:			/* (a0) ptr to sector data  */
/***********************************************************************/
	mov 0x2121, d0
	mov d0, (LBA4)
	mov d0, (LBA4+2)
	rts


#===================================================================================#
#			Breakpoint 1 gp TEST breakpoint											#
#===================================================================================#
#.include "source/qcode bp1 dbg.asm.c"




/***********************************************************************/
 SUB_Sleep:			# (dO = DELAY in ~ msec)
/***********************************************************************/
 delayLoop1:
		mov	0x10,d1
 delayLoop2:
		sub	1, d1
		bne	delayLoop2
		sub	1, d0
		bne	delayLoop1
		rts	
#END

/***********************************************************************/
 Sub_FlashLED_P80:			/* blue */									#
/***********************************************************************/
		sub 0x08, a3					# push regs
		mov d0, (0x00,a3)
		mov d1, (0x04,a3)
		and	0xf7ff, psw

		P80_HI
		mov 0x10, d0 
		jsr SUB_Sleep
		P80_LO

		or	0x0800, psw					# pop regs
		mov (0x00,a3), d0
		mov (0x04,a3), d1
		add 0x08, a3
		rts						
#END

/***********************************************************************/
 Sub_FlashLED_P81:			/* red */									#
/***********************************************************************/
		and	0xf7ff, psw

		P81_LO
		mov 0x400, d0 
		jsr SUB_Sleep
		P81_HI
		mov 0x600, d0 
		jsr SUB_Sleep
		P81_LO

		or	0x0800, psw
		rts						
#END

/***********************************************************************/
 Sub_SendByteSerial:		# (d0 = byte)
/***********************************************************************/
.if DBGOUTPUT == 1
		xor 0xAA, d0
		mov d0, d3

		mov 0x00, d0		# set both data and clock pins low
		movb d0, (P8OUT)	
		mov 1, d0			# wait a bit
		jsr SUB_Sleep
		
		mov 0x03, d0		# set both data and clock pins high
		movb d0, (P8OUT)	# to indicate startbit
		mov 3, d0			# wait some more
		jsr SUB_Sleep

		mov 8, d2			# loop 8 times
  bitloop:
		mov d3, d0			# set next databit to P80, clear clockbit (P81)
		and 0x01, d0		
		mov d0, (P8OUT)
		mov 3, d0			# sleep a little
		jsr SUB_Sleep

		mov 0x02, d0		# set clockbit high, data low
		movb d0, (P8OUT)
		mov 3 d0			# take a nap :p
		jsr SUB_Sleep
		
		ror d3				# rotate input bits
		sub 01, d2
		bne bitloop

		mov 0x00, d0		# set both pins low again
		movb d0, (P8OUT)	
.endif
		rts
#END
		
/***********************************************************************/
 SUB_DbgOutByte:		# (d0: byte)
/***********************************************************************/
.if DBGOUTPUT == 1

		sub 0x0a, a3				# push used regs
		mov d1, (0x00,a3)
		mov d2, (0x02,a3)
		mov d0, (0x04,a3)
		mov a0, (0x06,a3)

		and	0xf7ff, psw
		nop
		nop
		
		mov DbgDumpStart, a0		# point a0 to ringbuffer write offset
		movb (DbgWritePtr), d1
		add d1, a0
		
		movb (DbgReadPtr), d2		
		add 1, d1					
		and 0xff, d1
	
		cmp d2, d1					# write ptr +1 = read ptr ?
		bne NoDbgBufferFull

		mov 0x44, d0				# write DD as buffer full sign
		movb d0, (a0)				# and dont log the byte
		bra DbgOutEnd

 NoDbgBufferFull:
		movb d0, (a0)				# write byte
		movb d1, (DbgWritePtr)		# inc write ptr

 DbgOutEnd:

		or	0x0800, psw
		mov (0x00,a3), d1			# pop used regs
		mov (0x02,a3), d2
		mov (0x04,a3), d0
		mov (0x06,a3), a0
		add 0x0a, a3
.endif
		rts
#END

/***********************************************************************/
 SUB_DbgOutBuffer:		# (a0: address d0: bytes)
/***********************************************************************/
.if DBGOUTPUT == 1
		sub 0x06, a3				# push used regs
		mov a0, (0x00,a3)
		mov d1, (0x04,a3)
		mov d0, d1					# d1 - counter

		mov 0xEF, d0
		jsr SUB_DbgOutByte

 DbgOutBufferLoop:

		movbu (a0), d0
		jsr SUB_DbgOutByte

		add 1, a0
		sub 1, d1
		cmp 0, d1
		bne DbgOutBufferLoop

		mov (0x00,a3), a0			# pop used regs
		mov (0x04,a3), d1
		add 0x06, a3
.endif
		rts
#END

/***********************************************************************/
 SUB_StartDriveReset:
/***********************************************************************/

		#-----------------------------------------------------
		# Start of DVDReset
		#-----------------------------------------------------
		and	0xf7ff,psw
		nop	
		nop	

		sub	d2, d2
		mov	0x08e9a1, a2			# version (2)
		mov	0x8010, a1
 xloop:
		movbu (a2), d0
		cmp	0x02, d0			
		bne	verx

		mov	a1,a0
		jsrabs	0x8dcad
 verx:		
		add	0x01,d2			
		add	0x12,a1
		add	0x0e,a2
		cmp	0x02,d2			
		blt	xloop

		#-----------------------------------------------------
		# Instead of init_intsandtimers(0x80998)
		#-----------------------------------------------------
		jsrabs 0x80998
#		jsr SUB_SetCBDReadBreakpoint
#		mov 0x123b, d0					# set readopt byte
#		mov d0, (0x8082)

		mov 0, d0
		mov d0, (0x8071)
		mov d0, (0x8076)
		jsrabs 0x8B205
		jsrabs 0x83BA5					# SUB_INIT_SPINUP

		mov 2, d0
		mov 0, d1						# 8076 = 0 -> start drive
		jsrabs 0x878F0

		mov 0x40EBE7, a0				# set copy flag (02 40)
		mov 0x40, d0
		movb d0,(a0)
	
		mov 2, d0						# set lasthlecmd 2
		mov d0, (0x80E8)
		jmpabs 0x8D956					# continue with dvdreset
#END

#		Magic: 7410 59c7 [8082]: 187e  -> D1: 0000 D2: 41b9, D3: 59c7	(!!!)
#		DVD_SetReadOptions(0x6bda, 0x773e, 0x165d);	
#		DVD_SetReadOptions(0x0000, 0x00ff, 0x00ff);	//							TB
#		DVD_SetReadOptions(0x0000, 0x49d0, 0x123b); //D2: 123b, D3: 123b		FIXES RE3 read error at 1bb50	

/*		mov 0x187e, d0					# set read-options
		mov d0, (0x8082)

		mov 0x40EB48, a0				# 1
		mov 0x7410, d0
		mov d0, (0x40EB4C)
		mov 0x59c7, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F		
		
		mov 0xFF00, d0					# 2
		mov d0, (0x40EB4C)
		mov 0xFF00, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F	
		
		mov 0x49D0, d0					# 3
		mov d0, (0x40EB4C)
		mov 0x123B, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					# set read-options

/*		mov 0x6BDA, d0					# set read-options
		mov d0, (0x8082)

		mov 0x40EB48, a0				# 1
		mov 0x773E, d0
		mov d0, (0x40EB4C)
		mov 0x165D, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					

		mov 0x00FF, d0					# 2
		mov d0, (0x40EB4C)
		mov 0x00FF, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					

		mov 0x49D0, d0					# 3
		mov d0, (0x40EB4C)
		mov 0x123B, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					# set read-options

*/
		
		/*
	mov (REG_IAGR), d0
	cmp 0x4c, d0
	bne noSerialInt

	#-------------------------------------
	# intercept serial input
	#-------------------------------------
	bitclr 0xFC66, 4					# ack the interrupt
	mov DbgDump, a0						# get recv buffer
	mov (wSerialInBytes), d0
	add d0, a0
	add 1, d0
	mov d0, (wSerialInBytes)
	
	movbu (0xFD82), d0					# store the input byte and
	movb d0, (a0)						# dont pass to real handler (duh)
	rts									
 noSerialInt:
*/

/*		mov DbgDumpSize, a0			# clear dbgout mem
		mov 0x21, d0
		mov 0x800, d1
		jsrabs FWMemset
		
		mov	data1, a0				# copy initialized data section 
		mov	data1_copy_from, d1
		mov	data1_size, d0
		jsrabs FWMemcpy
		
		mov	bss, a0					# clear the bss section
		sub	d0, d0
		mov	bss_size, d1
		jsrabs FWMemset
*/


#	movbu (0x8069), d0
#	cmp 5, d0
#	bge AllowInts

/*	mov (REG_IAGR), d0
	cmp 0x4c, d0
	beq AllowInts
	cmp 0x48, d0
	beq AllowInts
	cmp 0x54, d0
	beq AllowInts
	rts

AllowInts:
*/


#		DVD_SetReadOptions(0x6bda, 0x773e, 0x165d);	
#		DVD_SetReadOptions(0x0000, 0x00ff, 0x00ff);	//							TB
#		DVD_SetReadOptions(0x0000, 0x49d0, 0x123b); //D2: 123b, D3: 123b		FIXES RE3 read error at 1bb50	

/*		mov 0x6BDA, d0
		mov d0, (0x8082)

		mov 0x40EB48, a0
		mov 0x3E77, d0
		mov d0, (0x40EB4C)
		mov 0x5D16, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					# set read-options
*/
		mov 0xFF00, d0
		mov d0, (0x40EB4C)
		mov 0xFF00, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					# set read-options

		mov 0xD049, d0
		mov d0, (0x40EB4C)
		mov 0x3B12, d0
		mov d0, (0x40EB4E)
		jsrabs 0x8396F					# set read-options

#	mov 0x5000, d0						# test read retry
#	mov d0, (0x40ec09)
#	mov d0, (0x40ec19)
#	mov d0, (0x40ec20)

/*
 SUB_SetSectorpatchBP:

	sub 0x04, a3					# push regs and disable ints
	mov a0, (0x00,a3)
	and	0xf7ff, psw

	mov	adb1_LBAinject, a0
	mov	a0, (REG_ADB1)

.if USEDBGBP == 1
	mov	dbgBpAddress, a0			# set debug breakpoint
	mov	a0, (REG_ADB1)
.endif	

	or 0x800, psw
	mov (0x00,a3), a0
	add 0x04, a3
	rts
#END
*/


.if DODREFIX == 1							#- missing fwcode :s -#
		movx d2,(0x00, a3)
		movx d3,(0x04, a3)
		mov	a1, (0x08, a3)
		mov	a2, (0x0c, a3)	

		mov	d0, d2
		mov	a0, d3
		mov	d3, a2
	.endif
	
	.if DODREFIX == 1						#- missing fwcode o_O -#
		add	0x02, a2

		mov	FWDriveState, a1				# if (cactus != e0) set bit 1
		movbu (a1), d1
		cmp	0x00e0, d1			
		beq	1f
		or	0x02, d1			
		movb d1, (a1)

1:		mov	0x400024, a1
		movbu (a0), d1						# llcmd 0: 8 -> d2
		cmp	0x00, d1						#-------------------------------
		bne	2f
		mov	0x08, d2			
2:
	.endif


/***********************************************************************/
 SUB_StartDrive:
/***********************************************************************/
	rts
#END

		
/***********************************************************************/
 Sub_SendBufferSerial:		# (a0: address d0: bytes)
/***********************************************************************/
/*		sub 0x06, a3					# push used regs
		mov a0, (0x00,a3)
		mov d1, (0x04,a3)
		mov d0, d1						# d1 - counter

		mov 0x21, d0
		jsr Sub_SendByteSerial

 SendBufferLoop:

		movbu (a0), d0
		jsr Sub_SendByteSerial

		add 1, a0
		sub 1, d1
		cmp 0, d1
		bne SendBufferLoop

		mov (0x00,a3), a0			# pop used regs
		mov (0x04,a3), d1
		add 0x06, a3
		rts
#END
*/

/***********************************************************************/
 SUB_Memset:	/* (d0 = len in bytes | d1 = 16bit val | a0 = address) */
/***********************************************************************/
		sub 0x04, a3				# push used regs
		mov a0, (0x00,a3)

 memset_loop:
		mov d1, (a0)
		add 2, a0
		add -0x2, d0
		beq memset_done
		bra memset_loop
 memset_done:
		mov (0x00,a3), a0			# pop used regs
		add 0x04, a3
		rts
#END

/***********************************************************************/
 SUB_Memclear:		/* (d0 = len in bytes  a0 = address) */
/***********************************************************************/
		mov 0, d1
		jsr SUB_Memset
		rts
#END
/***********************************************************************/
 Sub_LogLLCommands:									# (a0 = llcdb)
/***********************************************************************/
/*	#=========================#
	# LOG some llCMDs         #
	#=========================#
	movbu (a0), d1						
	cmp	0x0a, d1
	beq	showllcmd
	cmp	0x06, d1
	beq	showllcmd
	cmp	0xf000, d1
	beq	showllcmd
	cmp	0x07, d1
	beq	showllcmd
	cmp	0x0a, d1
	beq	showllcmd
	cmp	0x3800, d1
	beq	showllcmd
	bra nolog_llcmd
 showllcmd:
	mov 10, d0
	jsr SUB_DbgOutBuffer
 nolog_llcmd:
*/
	rts
#END
	
.if DODREFIX == 1							#- missing fwcode :s -#
		movx d2,(0x00, a3)
		movx d3,(0x04, a3)
		mov	a1, (0x08, a3)
		mov	a2, (0x0c, a3)	

		mov	d0, d2
		mov	a0, d3
		mov	d3, a2
	.endif
	
	.if DODREFIX == 1						#- missing fwcode o_O -#
		add	0x02, a2

		mov	FWDriveState, a1				# if (cactus != e0) set bit 1
		movbu (a1), d1
		cmp	0x00e0, d1			
		beq	1f
		or	0x02, d1			
		movb d1, (a1)

1:		mov	0x400024, a1
		movbu (a0), d1						# llcmd 0: 8 -> d2
		cmp	0x00, d1						#-------------------------------
		bne	2f
		mov	0x08, d2			
2:
	.endif

/*************************************************************************//*
	BREAKPOINTS:

	llcmd6			0x08AEEA	0x08AEEC	mov a3, a0
	llcmd6_2		0x08AEFF	0x08AF01	mov	0x08,d0
	jsr llcmd6		0x0827ED	0x0827F2
	llcmd7			0x08AF3B	0x08AF3E
	hlecmd11		0x083262	0x083265
	auth DVD-R		0x08aaf0	0x08aba2
	auth DVD+-R		0x089d42	0x089dd0
	auth qoob		0x089df2	xxxxxxxx	sub	d0,d0	pop, rts
*//*************************************************************************/

/*	// write QCode lo
	dwSize = g_dwQCode_Lo_Size;			
	dwAddr = (u32*) g_pQCode_Lo;
	DVD_WriteDriveMemBlock(0x8502, dwAddr,	dwSize);

	// write QCode hi
	dwSize = g_dwQCode_Hi_Size;					
	dwAddr = (u32*) g_pQCode_Hi;
	DVD_WriteDriveMemBlock(0xff40d000, dwAddr,	dwSize);
*/


				//---------------------------------------------------
				// [DVD_TEST_STARTDRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 0) {

					DEBUG_PrintStatus("** Starting drive **");
					FlushHexDumpBuffer(0);
					DVD_CustomDbgCommand(0xFE114100, 0, 0);
					GC_Sleep(100);

					//---------------------------------------------
					// set status variables to "allow read id"
					//---------------------------------------------
					/*
					DVD_WriteDriveMemDword(0x8188, 0x02000000);
					GC_Sleep(500);
					DVD_WriteDriveMemDword(0x81a4, 0x06000300);
					GC_Sleep(500);
					*/
				}

				//---------------------------------------------------
				// [DVD_STOPDRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 1) {
					DVD_CustomDbgCommand(0xFE114000, 0, 0);
//					DVD_Stop();
				}
				//---------------------------------------------------
				// [DVD_TEST_DISABLE_SCRAMBLING]
				//---------------------------------------------------
				else if(nSpecialCmd == 2) {

					DVD_WriteDriveMemDword(0x8080, 0x6bda);
					DVD_CustomDbgCommand(0xFE100000, 0x773e165d, 0);

					// FE	0D	ki	4B	65	kk	kk	kk	79	??	??	??
					//DVD_CustomDbgCommand(0xFE0D014B, 0x6540d800, 0x79000000);

					// set lidoknoreadid
					// DVD_CustomDbgCommand(0xFE120000, 0x842b4, 0x66756e63);
					
					/*
					DVD_CustomDbgCommand(0xFE0D024B, 0x65080800, 0x79000000);
					*/
					// DVD_CustomDbgCommand(0xFE0c0100, 0, 0);
					// DVD_WriteDriveMemDword(0x8188, 0x01c00000);

					//FE	12	md	00	00	adr	adr	adr	'f'	'u'	'n'	'c'

					// u32 dwCmd = 0xFE000000 + (u32) (bCount << 16);
					// DVD_CustomDbgCommand(dwCmd, 0x11223344, 0x55667788);
					// bCount++;

					//DVD_CustomDbgCommand(0xFE120100, 0x11223344, 0x66756e63);
					//DVD_CustomDbgCommand(0xFE040100, 0x11223344, 0x66756e63);
					
					
					//DVD_CustomDbgCommand(0xFE0F0000, 0x00080000, 0x0);
					//DVD_CustomDbgCommand(0xFE030000, GC_Rand() + (GC_Rand() << 16), GC_Rand() + (GC_Rand() << 16));

					//---------------------------------------------------
					// disable edc
					//---------------------------------------------------
					//DVD_WriteDriveMemDword(0x8080, 0x6bda);
					//DVD_CustomDbgCommand(0xFE100000, 0x773e165d, 0);
					
					//---------------------------------------------------
					// sets same params as bp scramble patching
					//---------------------------------------------------
					//DVD_WriteDriveMemDword(0x8080, 0x2ab25564);
					//DVD_CustomDbgCommand(0xFE100000, 0x50aca159, 0xa02b2856);
					//DVD_WriteDriveMemDword(0x8080, 0x00);
					
					//---------------------------------------------------
					// set random params
					//---------------------------------------------------
/*					u32 dwRand1 = GC_Rand() + (GC_Rand() << 16); // 0x2484
					u32 dwRand2 = GC_Rand() + (GC_Rand() << 16); // 0x1242
					dwRand3 = GC_Rand() + (GC_Rand() << 16);
					
					DVD_WriteDriveMemDword(0x8000, dwRand1);
					DVD_WriteDriveMemDword(0x8004, dwRand2);
					DVD_WriteDriveMemDword(0x8008, dwRand3);
					
					DVD_WriteDriveMemDword(0x8080, dwRand1);
					DVD_CustomDbgCommand(0xFE100000, dwRand2, dwRand3);
*/
				}
				//---------------------------------------------------
				// [DVD_TEST_UNLOCK_DRIVE]
				//---------------------------------------------------
				else if(nSpecialCmd == 3) {

					DVD_CustomDbgCommand(0xFE114000, 0, 0);

					// scramble cdbs 40ec62 40ec76 -> 40ec75 || 40ec89 -> 0 means disable scramble
					// DVD_WriteDriveMemDword(0x40ec5c, 0x00002f07);
					// DVD_WriteDriveMemDword(0x40ec74, 0x41000000);
					// DVD_WriteDriveMemDword(0x8050, 0x00800000);
					// DVD_WriteDriveMemDword(0x8054, 0x10400000);
					// DVD_WriteDriveMemDword(0x40ec88, 0x0);
					
					// DVD_CustomDbgCommand(0xFE030000, 0, 0);
					// DVD_CustomDbgCommand(0xFE030000, GC_Rand(), GC_Rand());

					// DVD_CustomDbgCommand(0xFE114106, 0, 0);
					// DVD_CustomDbgCommand(0xFE0f0000, 0x0000000, 0x00000000);
					// DVD_WriteDriveMemDword(0x8188, 0x0201c003);
					// DVD_WriteDriveMemDword(0x8188, 0x0301c003);
					// DVD_WriteDriveMemDword(0x81a4, 0x06000300);
					// DVD_WriteDriveMemDword(0x40ea8c, 0x03000040);
					// DVD_CustomDbgCommand(0xFE030000, 0, 0);
					// DVD_WriteDriveMemDword(0x40ec74, 0x00000000);

					// DVD_WriteDriveMemDword(0x40ea5c, 0x00412c00);
					// DVD_WriteDriveMemDword(0x40ea5c, 0x00380000 + GC_Rand());
					// DVD_WriteDriveMemDword(0x40ed00, 0x00086303);
					// DVD_WriteDriveMemDword(0x40ea8c, 0x03000040);
				}

				
/*
if((g_nKey & PAD_START) && (g_nKey & PAD_R)) { \
	if(!g_bToggleWatchAddr) g_nLBA = Addr; else g_nLBA = 0;\
	g_bToggleWatchAddr = !g_bToggleWatchAddr;\
	GC_Sleep(120);\
}\
*/










					//---------------------------------------------------
					// startdrive GCLinux cactus drivecode
					//---------------------------------------------------
/*					DEBUG_PrintStatus("** Setting extensions Id **");
					FlushHexDumpBuffer(0);
					DVD_CustomDbgCommand(0x55010000, 0, 0);
					GC_Sleep(1000);
					DEBUG_PrintStatus("** Starting drive **");
					DVD_CustomDbgCommand(0xFE114100, 0, 0);
					GC_Sleep(4000);
					DEBUG_PrintStatus("** Setting status **");
					DVD_CustomDbgCommand(0xEE060300, 0, 0);
					GC_Sleep(1000);
*/




/*

asm("EnableAudioStreaming:");
asm("	mflr		r11	");

asm("	lis		r9 , 0x8000	");
asm("	lis		r5 , 0xe400	");
asm("	lbz		r3 , 8(r9)	");
asm("	cmpwi	r3 , 0		");
asm("	beq		EnableAudioStreaming_Setup");
asm("	oris		r5 , r5 , 0x0001");
asm("	lbz		r3 , 9(r9)	");
asm("	cmpwi	r3 , 0	");
asm("	bne		EnableAudioStreaming_Setup");
asm("	ori		r5 , r5 , 0x000a");

asm("EnableAudioStreaming_Setup:");
asm("	lis		r3 , 0xcc00		");
asm("	ori		r3 , r3 , 0x6000");
asm("	lwz		r4 , 0(r3)	");
asm("	ori		r4 , r4 , 0x14");
asm("	stw		r4 , 0(r3)	");
asm("	li		r4 , 0	");
asm("	stw		r4 , 4(r3)");
asm("	stw		r5 , 8(r3)");
asm("	li		r4 , 0		");
asm("	stw		r4 , 12(r3)");
asm("	stw		r4 , 16(r3)");
asm("	li		r4 , 1	");
asm("	stw		r4 , 28(r3)	");

asm("	mr		r10 , r3");
asm("	li		r3 , 100");
asm("	bl		Delay	");
asm("	mr		r3 , r10");

asm("EnableAudioStreamingSetup_Loop2:");
asm("	lwz		r4 , 0(r3)		");
asm("	andi.	r4 , r4 , 0x14	");
asm("	cmpwi	r4 , 0			");
asm("	beq		EnableAudioStreamingSetup_Loop2	");

asm("	mtlr		r11	");
asm("	blr		");

*/














/*

	DVD_WriteDriveMemDword(0x40ec74, 0x00000000);
	DVD_WriteDriveMemDword(0x40ec60, 0x13000000);


*/

/*

  //					DVD_WriteDriveMemBlock(0xff40d000, g_pBuffer, 0x800);
//					DVD_WriteDriveMemDword(0x80040, 0x21212121);
//					DVD_WriteDriveMemDword(0x80800, 0x21212121);
//					DVD_WriteDriveMemDword(0x70040, 0x21212121);
//					DVD_WriteDriveMemDword(0x70800, 0x21212121);

	u8		aBits[32];
	u8		aBitsPrev[16];
	u8		aBitHexDump[32];

	u8* aBitFreq	= &aBits[16];

	u16 dwSpeed1;
	u16 dwSpeed2;
	u16 dwSpeed3;
	
	u16 dwSpeed	= g_dwDriveSpeed >> 3;
	dwSpeed1	= g_dwDriveSpeed;
	dwSpeed2	= dwSpeed;
	DEBUG_ShowValueU16(320, 40, dwSpeed1);
	DEBUG_ShowValueU16(410, 40, dwSpeed2);
	DEBUG_ShowValueU16(500, 40, dwSpeed3);
	dwSpeed = dwSpeed3;
		memcpy(aBitsPrev, aBits, 16);

		for(nCount= 0; nCount < 16; nCount++) {
			
			u8 nBit =  (dwSpeed >> ((15 - (nCount))) & 1);
			aBits[nCount] =  nBit;

			if((aBitsPrev[nCount] != nBit) && (aBitFreq[nCount] < 0xff)) {
				aBitFreq[nCount]	+= nBit;
			}
		}

		for(nCount= 7; nCount >= 0; nCount--) {
			aBitHexDump[nCount]			= BITS2BYTE(aBits[nCount * 2],	aBits[((nCount  * 2)  + 1)]);
			aBitHexDump[nCount + 16]	= BITS2BYTE(aBitFreq[nCount * 2] / 0x2, aBitFreq[((nCount  * 2)  + 1)]  / 0x2);
		}
*/

/*
		DEBUG_PrintStatus("** DOING STD ANACONDA_SWAP in 2 secs **");
		DVD_Anaconda_Delay2();

		DEBUG_PrintStatus("** 1. RESET **");
		DVD_Reset();

		DEBUG_PrintStatus("** 2. DELAY8 **");
		DVD_Anaconda_Delay8();

		DEBUG_PrintStatus("** 3. UNLOCK **");
		DVD_SetDebugMode1();
		DVD_SetDebugMode2();

		DEBUG_PrintStatus("** 4. INJECT **");
		DVD_Anaconda_InjectFirmwareHack();

		DEBUG_PrintStatus("** 5. DELAY2 **");
		DVD_Anaconda_Delay2();

		DEBUG_PrintStatus("** 6. READ_ID **");
		DVD_ReadId((void*) g_pBuffer);

		DEBUG_PrintStatus("** 7. FINISH **");
*/


/*	u32 dwMem		= 0;		
	int dwRead		= 0;
	u32* pBuffer	= (u32 *) g_pBuffer;

	DEBUG_Print(100, 440, "** Reading drive mem ... **");

	for(dwRead = 0; dwRead < 512; dwRead++) {
		u32 dwMem = DVD_ReadDriveMemDword(0x008000 + (dwRead * 4));
		pBuffer[dwRead] = dwMem;

		if(dwMem == 0xBEEFBEEF) {
			break;
		}
	}
*/

//	u8	bHi  = HIBYTE(dwSpeed);
//	u8	bLo  = LOBYTE(dwSpeed);
//	dwSpeed  = BYTES2WORD(bLo, bHi);

/*
				DEBUG_Print(100, 440, "** Dumping drive firmware ... **");
				
				GC_Memory_memset((void*)g_pBuffer, 0xBB, 2048);
				
				u32 dwAddr		= 0x0;
				u32 dwLen		= 64 * 1024;
				u32 dwOffset	= 0;
				u32 dwChecksum	= 0;
				
				u32* pDest		= (u32 *) 0x80800000;	//g_pBuffer;

				*(pDest - 1) = *((u32*) "FWST");

				while(dwOffset < dwLen) {
					u32 dwMem = DVD_ReadDriveMemDword(dwAddr + dwOffset); //0x2BADBEEF
					pDest[dwOffset / 4] = dwMem;
					//dwChecksum	+= dwMem;
					dwOffset	+= 4;

					if(dwMem == 0xBEEFBEEF) {
						break;
					}
				}
				pDest[dwOffset / 4] = *((u32*) "FWEN");
*/

/*
	g_pdwCustomCommand = (u32*) &g_aCustomDVDCommand[0];
	
	g_pdwCustomCommand[0] = 0x2E;
	g_pdwCustomCommand[1] = 0;
	
	g_pdwCustomCommand[2] = 0xA8000000;
	g_pdwCustomCommand[3] = 0x8502;
	g_pdwCustomCommand[4] = (u32) 0xC0000;
	g_pdwCustomCommand[5] = (u32) g_pBuffer;
	g_pdwCustomCommand[6] = (u32) 0x20;
	g_pdwCustomCommand[7] = 3;
*/

//			if(g_nKey & PAD_START) {
//	//			DVD_Reset();
//	//			GC_Sleep(4000);
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(50, 440, "** SENDING DBGMODE COMMAND... **");
//				DVD_SetDebugMode();
//				DEBUG_Print(100, 440, "** DISABLING DVD DESCRAMBLE... **");
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DVD_DisableDescrambling();
//				DVD_ReadId((void*) 0x80000000);
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** OKI                         **");
//
//
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** SWAP!!!                    **");
//				GC_Sleep(4000);
//
//				
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** Reading id...                 ");
//
//				DVD_ReadId((void*) 0x80000000);
//
//				GC_Video_ClearFrameBuffer(g_pFrameBuffer, 0x86936274);
//				DEBUG_Print(100, 440, "** DONE                        **");
//	//			DVD_RequestError();
//			}

			// return to old ipl
//			asm("lis 3,		0x817c"); 

			// return to level3.dol
//			asm("lis 3,		0x8030");
//			asm("ori 3, 3,	0x0100");
//			asm("mtlr 3");
//			asm("blrl");

			// return to ipl
//			asm("lis 3,		0x80d0");
//			asm("ori 3, 3,	0x0000");
//			asm("mtlr 3");
//			asm("blrl");


//		else if(bDoRandomSeeks) {
//			u32 dwRandSector = RAND(0xa05ff);
//			DEBUG_ShowValueU32(350, 60, dwRandSector);
//			DVD_Read(g_pBuffer, 512, dwRandSector * 2048);
//			DVD_RequestError();
//		}


//		if(g_nKey & PAD_CRIGHT) {
//			dwCmd += 0x01000000;
//		}
//		else if(g_nKey & PAD_CLEFT) {
//			dwCmd -= 0x01000000;
//		}
//		else if(g_nKey & PAD_CUP) {
//			dwCmd = ++dwCmd & 0xFF0000FF;
//		}
//		else if(g_nKey & PAD_CDOWN) {
//			dwCmd = --dwCmd & 0xFF0000FF;
//		}


//		if(g_nKey & PAD_START) {
//			DEBUG_Print(50, 440, "** SENDING DBGMODE COMMAND... **");
//			DVD_SetDebugMode();
//			DEBUG_Print(100, 440, "** DISABLING DVD DESCRAMBLE... **");
//			DVD_DisableDescrambling();
//		}

//			DEBUG_Print(100, 440, "sending custom command...");
//			GC_Memory_memset((void*)g_pBuffer, 0xCC, 2048);
//			dcache_flush((void*)g_pBuffer, 2048);
//			DVD_CustomCommand(dwCmd, g_pBuffer);
//			DVD_RequestError();




//			DEBUG_Print(100, 440, "Resetting dvd drive...");
//			DVD_Reset();
//			DEBUG_Print(100, 440, "Reading id...");
//			DVD_ReadId((void*) 0x80000000);
//			DVD_RequestError();
//			DEBUG_Print(100, 440, "done.");

//			DEBUG_Print(100, 440, "dumping leadout....");
//
//			int nSec = 0xa0000;
//			u8* pDest = (u8*) (0x80040000);
//
//			for( ;nSec < 0xa0600; nSec++) {
//				DVD_Read(pDest, 2048, nSec * 2048);
//				pDest += 2048;
//			}
//			DEBUG_Print(100, 440, "done!");

//		else if(g_nKey & PAD_B) { 
//			DEBUG_Print(100, 440, "sending custom command...");
//			GC_Memory_memset((void*)g_pBuffer, 0xCC, 2048);
//			dcache_flush((void*)g_pBuffer, 2048);
//			DVD_CustomCommand(dwCmd, g_pBuffer);
//			DVD_RequestError();
//		}





//	DEBUG_Print(100, 220, "Open and close drive...");
//	while(!DVD_IsCoverOpen()) {
//	}
//	while(DVD_IsCoverOpen()) {
//	}



//		GC_PAD_Read(&sPad1, GC_PAD1);
//		if(sPad1.button & PAD_START) {
//			asm("lis 3, 0x8030");
//			asm("ori 3, 3, 0x0100");
//			asm("mtlr 3");
//			asm("blrl");
//		}



//hotreset:
//lis r3,0
//lis r9,0xCC00
//sth r3, 0x2000(r9)
//li r4, 3
//stw r4, 0x3024(r9)
//stw r3, 0x3024(r9)
//nop
//loop__: b loop__





//asm(".align 4");
//
//asm(".global g_pDriveCode");
//asm("g_pDriveCode:");

// Generated by Hex Workshop
// test.bin - Starting Offset: 0 (0x00000000) Length: 136 (0x00000088)



//#define g_pDriveCode rawData

//asm(".incbin drivecode.bin");
//asm(".global g_pDriveCodeEnd");
//asm("g_pDriveCodeEnd:");

//extern u32 g_pDriveCode;
//extern u32 g_pDriveCodeEnd;
