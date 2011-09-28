

	mov 0x40C602, a0
	mov a0, (0x804c)
	mov 0x0C, d0
	movb d0, (0xfcda)
	
#	jsrabsOrg ENTRY
	jmp (a0)



	mov (0x40d800), d0
	cmp 0x2121, d0
	bne 1f

	sub d0, d0
	mov d0, (0x40d800)

	mov 0xBECE, d0
	mov d0, (0x40d804)

	mov 'X', d0
	jsr send_byte
	jsr get_byte
		
	mov 0x40c600, a0
	jmp (a0)

	mov 0x80a74, a0
	jmp (a0)

#	mov a0,(0x804c)				#
#	bra Reset

1:	mov 0xBAAB, d0
	mov d0, (0x40d800)


#include <avr/io.h>
#include <compat/twi.h>		/* Note [1] */
/*
 * Note [3]
 * TWI address for 24Cxx EEPROM:
 *
 * 1 0 1 0 E2 E1 E0 R/~W	24C01/24C02
 * 1 0 1 0 E2 E1 A8 R/~W	24C04
 * 1 0 1 0 E2 A9 A8 R/~W	24C08
 * 1 0 1 0 A10 A9 A8 R/~W	24C16
 */
#define TWI_SLA_24CXX	0xa0	/* E2 E1 E0 = 0 0 0 */

/*
 * Maximal number of iterations to wait for a device to respond for a
 * selection.  Should be large enough to allow for a pending write to
 * complete, but low enough to properly abort an infinite loop in case
 * a slave is broken or not present at all.  With 100 kHz TWI clock,
 * transfering the start condition and SLA+R/W packet takes about 10
 * µs.  The longest write period is supposed to not exceed ~ 10 ms.
 * Thus, normal operation should not require more than 100 iterations
 * to get the device to respond to a selection.
 */
#define MAX_ITER	200

/*
 * Number of bytes that can be written in a row, see comments for
 * ee24xx_write_page() below.  Some vendor's devices would accept 16,
 * but 8 seems to be the lowest common denominator.
 *
 * Note that the page size must be a power of two, this simplifies the
 * page boundary calculations below.
 */
#define PAGE_SIZE 8

/*
 * Saved TWI status register, for error messages only.  We need to
 * save it in a variable, since the datasheet only guarantees the TWSR
 * register to have valid contents while the TWINT bit in TWCR is set.
 */
uint8_t twst;

/*
 * Write "len" bytes into EEPROM starting at "eeaddr" from "buf".
 *
 * This is a bit simpler than the previous function since both, the
 * address and the data bytes will be transfered in master transmitter
 * mode, thus no reselection of the device is necessary.  However, the
 * EEPROMs are only capable of writing one "page" simultaneously, so
 * care must be taken to not cross a page boundary within one write
 * cycle.  The amount of data one page consists of varies from
 * manufacturer to manufacturer: some vendors only use 8-byte pages
 * for the smaller devices, and 16-byte pages for the larger devices,
 * while other vendors generally use 16-byte pages.  We thus use the
 * smallest common denominator of 8 bytes per page, declared by the
 * macro PAGE_SIZE above.
 *
 * The function simply returns after writing one page, returning the
 * actual number of data byte written.  It is up to the caller to
 * re-invoke it in order to write further data.
 */
int
ee24xx_write_page(uint16_t eeaddr, int len, uint8_t *buf)
{
  uint8_t sla, n = 0;
  int rv = 0;
  uint16_t endaddr;

  if (eeaddr + len < (eeaddr | (PAGE_SIZE - 1)))
    endaddr = eeaddr + len;
  else
    endaddr = (eeaddr | (PAGE_SIZE - 1)) + 1;
  len = endaddr - eeaddr;

  /* patch high bits of EEPROM address into SLA */
  sla = TWI_SLA_24CXX | (((eeaddr >> 8) & 0x07) << 1);

  restart:
  if (n++ >= MAX_ITER)
    return -1;
  begin:

  /* Note [15] */
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_REP_START:		/* OK, but should not happen */
    case TW_START:
      break;

    case TW_MT_ARB_LOST:
      goto begin;

    default:
      return -1;		/* error: not in start condition */
				/* NB: do /not/ send stop condition */
    }

  /* send SLA+W */
  TWDR = sla | TW_WRITE;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_SLA_ACK:
      break;

    case TW_MT_SLA_NACK:	/* nack during select: device busy writing */
      goto restart;

    case TW_MT_ARB_LOST:	/* re-arbitrate */
      goto begin;

    default:
      goto error;		/* must send stop condition */
    }

  TWDR = eeaddr;		/* low 8 bits of addr */
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;

    case TW_MT_DATA_NACK:
      goto quit;

    case TW_MT_ARB_LOST:
      goto begin;

    default:
      goto error;		/* must send stop condition */
    }

  for (; len > 0; len--)
    {
      TWDR = *buf++;
      TWCR = _BV(TWINT) | _BV(TWEN); /* start transmission */
      while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
      switch ((twst = TW_STATUS))
	{
	case TW_MT_DATA_NACK:
	  goto error;		/* device write protected -- Note [16] */

	case TW_MT_DATA_ACK:
	  rv++;
	  break;

	default:
	  goto error;
	}
    }
  quit:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */

  return rv;

  error:
  rv = -1;
  goto quit;
}
#define F_CPU 14745600UL	/* Note [2] */
#ifndef UCSRB
# ifdef UCSR1A		/* ATmega128 */
#  define UCSRA UCSR1A
#  define UCSRB UCSR1B
#  define UBRR UBRR1L
#  define UDR UDR1
# else /* ATmega8 */
#  define UCSRA USR
#  define UCSRB UCR
# endif
#endif
#ifndef UBRR
#  define UBRR UBRRL
#endif
#if F_CPU <= 1000000UL
  /*
   * Note [4]
   * Slow system clock, double Baud rate to improve rate error.
   */
  UCSRA = _BV(U2X);
  UBRR = (F_CPU / (8 * 9600UL)) - 1; /* 9600 Bd */
#else
  UBRR = (F_CPU / (16 * 9600UL)) - 1; /* 9600 Bd */
#endif
  UCSRB = _BV(TXEN);		/* tx enable */

  /* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
#if defined(TWPS0)
  /* has prescaler (mega128 & newer) */
  TWSR = 0;
#endif

#if F_CPU < 3600000UL
  TWBR = 10;			/* smallest TWBR value, see note [5] */
#else
  TWBR = (F_CPU / 100000UL - 16) / 2;
#endif
  
  LED1_OFF; LED2_OFF;		// -> red and blue: RESET	

	FlashLED(1, 4);
	ee24xx_write_page(0x0000, 16, (uint8_t*) "cheqmate was here.");
	FlashLED(2, 4);
