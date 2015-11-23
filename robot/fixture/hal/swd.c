// Based on Drive Testfix, updated for Cozmo EP1 Testfix

//#define SWD_DEBUG

#include "hal/console.h"
#include "hal/flash.h"
#include "hal/timers.h"
#include "hal/testport.h"
#include "hal/portable.h"
#include "hal/board.h"
#include "hal/uart.h"
#include "../app/fixture.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define GPIOB_SWD       GPIO_Pin_0
#define PINB_SWD        GPIO_PinSource0
#define GPIOB_SWC       GPIO_Pin_2
#define PINB_SWC        GPIO_PinSource2

#define GPIOA_TXRX    GPIO_Pin_2

#define GSET(gp, pin)      gp->BSRRL = (pin)
#define GRESET(gp, pin)    gp->BSRRH = (pin)
#define GREAD(gp)          gp->IDR

void SlowPrintf(const char* format, ...);

//#define SWD_DEBUG


/************
 * SWD stuff
 */

#define SWD_ACK 1
#define SWD_WAIT 2
#define SWD_FAULT 4
#define SWD_PARITY 8

// This short delay was tested empirically - it must be fast (~10) to work with the nRF51's strange glitch filter
#define bit_delay() {volatile int x = 10; while (x--);}

char HostBus = 0; // SWD bus is controlled by the host or target

// Switch to host-controlled bus
void swd_turnaround_host()
{
	if(!HostBus)
	{ // clock is idle (low)
		bit_delay();
    
		// Drive clock high and set SWDIO
    GSET(GPIOB, GPIOB_SWC | GPIOB_SWD);

		// Take over SWDIO pin
		PIN_OUT(GPIOB, PINB_SWD);

		HostBus = 1;
		// clock is idle (high)
	}
}

// Switch to target-controlled bus
void swd_turnaround_target()
{
	if(HostBus)
	{ // clock is idle (high)
		bit_delay();
		// Drive clock low and set SWDIO
		GRESET(GPIOB, GPIOB_SWC);
    GSET(GPIOB, GPIOB_SWD);

		bit_delay();
		// Drive clock high
		GSET(GPIOB, GPIOB_SWC);
		bit_delay();
		// Drive clock low
		GRESET(GPIOB, GPIOB_SWC);

		// Release SWDIO pin
		PIN_IN(GPIOB, PINB_SWD);

		HostBus = 0;
	}
}

// Sends bits out the wire. This is a low-level function that does not
// implement any SWD protocol. The bits are clocked out on the clock falling
// edge for the SWD state machine to latch on the rising edge. After it
// is done this function leaves the clock low.
void write_bits(int nbits, const unsigned long *bits)
{
	int i;
	unsigned long wbuf = *bits;

	swd_turnaround_host();
	// clock is idle (high)

	// Set up 11U port bitmask so we can write data + clock
	// at the same time. Done in swd_turnaround_host.

	i = 0;
	while(nbits)
	{
		bit_delay();
		
		// drive SWCLK low and output next bit to SWDIO line
    GRESET(GPIOB, GPIOB_SWC);
		if ((wbuf)&1)
      GSET(GPIOB, GPIOB_SWD);
    else
      GRESET(GPIOB, GPIOB_SWD);
		bit_delay();

		// prepare for next bit
		nbits--;
		i++;
		if(i>=32)
			bits++,wbuf=*bits,i=0;
		else
			(wbuf) >>= 1;

		// drive SWCLK high
    GSET(GPIOB, GPIOB_SWC);
	}
}

// Reads bits from the wire. This is a low-level function that does not
// implement any SWD protocol. The bits are expected to be clocked out on
// the clock rising edge and then they are latched on the clock falling edge.
// After it is done this function leaves the clock low.
void read_bits(int nbits, volatile unsigned long *bits)
{
	int i;

	swd_turnaround_target();
	// clock is idle (low)

	*bits = 0;
	i = 0;
	while(nbits)
	{
		bit_delay();
		// read bit from SWDIO line
		(*bits) |= ((GREAD(GPIOB)&GPIOB_SWD)?1:0) << i;

		// drive SWCLK high
		GSET(GPIOB, GPIOB_SWC);

		bit_delay();

		// drive SWCLK low
		GRESET(GPIOB, GPIOB_SWC);

		// prepare for next bit
		i++;
		if(i>=32)
		{
			bits++;
			if(nbits>1)
				*bits=0; // prevent hard faults
			i=0;
		}
		nbits--;
	}
}

// Send 8 0 bits to flush SWD state
void swd_flush(void)
{
	unsigned long data = 0;

	write_bits(8, &data);
}

// Send magic number to switch to SWD mode. This function sends many
// zeros, 1s, then the magic number, then more 1s and zeros to try to
// get the SWD state machine's attention if it is connected in some
// unusual state.
void swd_enable(void)
{
	unsigned long data;

	// Just for nRF51 - write 150 FFs at >= 125KHz
	data = 0xFFFFFFFF;
  for (int i = 0; i < 25; i++)
    write_bits(32, &data);
  
	// write 0s
	data = 0;
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);
	
	// write FFs
	data = 0xFFFFFFFF;
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);

	data = 0xE79E;
	write_bits(16, &data);
	
	// write FFs
	data = 0xFFFFFFFF;
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);

	// write 0s
	data = 0;
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);
	write_bits(32, &data);
}

// The SWD protocol consists of read and write requests sent from
// the host in the form of 8-bit packets. These requests are followed
// by a 3-bit status response from the target and then a data phase
// which is 32 bits of data + 1 bit of parity. This function reads
// the three bit status responses.
int swd_get_target_response(void)
{
	unsigned long data;

	read_bits(3, &data);

	return data;
}

// This function counts the number of 1s in an integer. It is used
// to calculate parity. This is the MIT HAKMEM (Hacks Memo) implementation.
int count_ones(unsigned long n)
{
   register unsigned int tmp;

   tmp = n - ((n >> 1) & 033333333333)
           - ((n >> 2) & 011111111111);
   return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}
/*
int count_ones(unsigned long bits)
{
	char i;
	char ones = 0;

	for(i=0;i<32;i++)
	{
		if(bits&1)
			ones++;
		bits >>= 1;
	}

	return ones;
}*/

// This is one of the two core SWD functions. It can write to a debug port
// register or to an access port register. It implements the host->target
// write request, reading the 3-bit status, and then writing the 33 bits
// data+parity.
int swd_write(char APnDP, int A, unsigned long data)
{
	unsigned long wpr;
	int response;

  // Bit 0 = 1, Bit 1 = AP, Bit 2 = Read, Bit 3-4 = A 2-3 (A is 0, 4, 8, C), Bit 5 = parity
	wpr = 0x81 /*0b10000001*/ | (APnDP<<1) | (A<<1); // A is a 32-bit address bits 0,1 are 0.
	if(count_ones(wpr&0x1E)%2) // odd number of 1s
		wpr |= 1<<5;	  // set parity bit
#ifdef SWD_DEBUG
	SlowPrintf("swd_write(Port=%s, Addr=%d, data=%08x, wpr=%02x) = ",
			APnDP ? "Access" : "Debug",
			A, data, wpr);
#endif

	write_bits( 8, &wpr);
	// now read acknowledgement
	response = swd_get_target_response();
	if(response != SWD_ACK)
	{
#ifdef SWD_DEBUG
		SlowPrintf("%s starting write\n", response == SWD_FAULT ? "FAULT" : "WAIT");
#endif
		swd_turnaround_host();
		return response;
	}

	write_bits( 32, &data); // send write data
	wpr = 0;
	if(count_ones(data)%2) // odd number of 1s
		wpr = 1;	  // set parity bit
	write_bits( 1, &wpr); // send parity

#ifdef SWD_DEBUG
	SlowPrintf("OK\n");
#endif
	return SWD_ACK;
}
// This is one of the two core SWD functions. It can read from a debug port
// register or an access port register. It implements the host->target
// read request, reading the 3-bit status, and then reading the 33 bits
// data+parity.
int swd_read(char APnDP, int A, volatile unsigned long *data)
{
	unsigned long wpr;
	int response;

	wpr = 0x85 /*0b10000101*/ | (APnDP<<1) | (A<<1); // A is a 32-bit address bits 0,1 are 0.
	if(count_ones(wpr&0x1E)%2) // odd number of 1s
		wpr |= 1<<5;	  // set parity bit
#ifdef SWD_DEBUG
	SlowPrintf("swd_read(Port=%s, Addr=%d, wpr=%02x) = ",
			APnDP ? "Access" : "Debug",
			A, wpr);
#endif

	write_bits( 8, &wpr);
	// now read acknowledgement
	response = swd_get_target_response();
	if(response != SWD_ACK)
	{
#ifdef SWD_DEBUG
		SlowPrintf("%s starting read\n", response == SWD_FAULT ? "FAULT" : "WAIT");
#endif
		swd_turnaround_host();
		return response;
	}

	read_bits( 32, data); // read data
	read_bits( 1, &wpr); // read parity
	if(count_ones(wpr)%2) // odd number of 1s
	{
		if(wpr != 1)
		{
			swd_turnaround_host();
			return SWD_PARITY; // bad parity
		}
	} else
	{
		if(wpr != 0)
		{
			swd_turnaround_host();
			return SWD_PARITY; // bad parity
		}
	}

#ifdef SWD_DEBUG
	SlowPrintf("%08x\n", *data);
#endif
	swd_turnaround_host();
	return SWD_ACK;
}

void swd_setcsw(int addr, int size)
{
  unsigned long val;
  swd_read(1, 0x0, &val);   // Get AP reg 0
  swd_read(0, 0xC, &val);   // Get result
  int csw = (val & ~0xff) | (addr << 4) | size;
  swd_write(1, 0, csw);
}

void swd_chipinit(void)
{
  unsigned long value;
  swd_read(0, 0, &value);   // Debug port (not AP), register 0 (IDCODE)
  swd_write(0, 0x4, 0x54000000);  // Power up
  swd_read(0, 0x4, &value);  // Check powered up
  SlowPrintf("%x == %x\n", value >> 24, 0xf4);
  //swd_write(0, 0x8, 0xF0);   // Set AP bank 0xF on AP 0x0 (the memory AP)
  //swd_read(1, 0xC, &value);   // Read AP (not debug port), register 0xFC (IDCODE)
  //swd_read(0, 0xC, &value);   // Read back result from RB registsr
  swd_write(0, 0x8, 0x0);   // Set AP bank 0x0 on AP 0x0 (the only useful bank)
  swd_setcsw(1,2);          // 32-bit auto-inc
}

int swd_read32(int addr)
{
  unsigned long value;
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_read(1, 0xC, &value);  // Read data from AP
  r |= swd_read(0, 0xC, &value);  // Read readback reg
#ifdef SWD_DEBUG  
  if (r != SWD_ACK)
    SlowPrintf("FAILED swd_read32 @ %08x\n", addr);
#endif  
  return value;
}

int swd_write32(int addr, int data)
{
  unsigned long value;
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_write(1, 0xC, data);   // Set data
  r |= swd_read(0, 0xC, &value);  // Read readback reg.. for some reason?
#ifdef SWD_DEBUG  
  if (r != SWD_ACK)
    SlowPrintf("FAILED swd_write32 @ %08x = %08x\n", addr, data);
#endif
  return r;
}

int swd_write16(int addr, u16 data)
{
  unsigned long value;
  swd_setcsw(1, 1);   // Switch to 16-bit mode    
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_write(1, 0xC, data);   // Set data
  r |= swd_read(0, 0xC, &value);  // Read readback reg.. for some reason?
#ifdef SWD_DEBUG  
  if (r != SWD_ACK)
    SlowPrintf("FAILED swd_write16 @ %08x = %08x\n", addr, data);
#endif
  swd_setcsw(1, 2);   // Back to 32-bit mode
  return r;
}

extern u16 g_Body[], g_BodyEnd[];
//extern u16 g_appbin[], g_append[];

int swd_flash_page(int destaddr, u16* image)
{
  int r = 0;
  r |= swd_write32(0x40022010, 0x00000201);  // Enter program mode
  // Packed mode is not supported by this debug interface!
  swd_setcsw(1, 1);   // Switch to 16-bit mode    
  r |= swd_write(1, 0x4, destaddr);    // Start address
  u16* next = image;
  while (next < image+512)
  {
    r |= swd_write(1, 0xc, *next++);  
    r |= swd_write(1, 0xc, (*next++) << 16); // Due to lack of packed mode
  }
  swd_setcsw(1, 2);   // Back to 32-bit mode
  r |= swd_write32(0x40022010, 0x00000200);  // Finished!
  
  //STM_EVAL_LEDToggle(LED2);   // Blue LED
  
  return r;
}

// Lock the JTAG port - upon next reset, JTAG will not work again - EVER
// If you need a dev robot, you must have Pablo swap the CPU
// Returns 0 on success, 1 on erase failure (chip will survive), 2 on program failure (dead)
int swd_lock_jtag(void)
{
  // TBD
  return 0;
}

void InitSWD(void)
{
  // Put board power in safe state
  GPIO_SetBits(GPIOC, GPIO_Pin_12);   // Charge power off

  GPIO_InitTypeDef  GPIO_InitStructure; 
  GPIO_InitStructure.GPIO_Pin = GPIOB_SWD;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIOB_SWC;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = GPIOA_TXRX;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // Wait for lines to settle
  MicroWait(200);
  
  // Power it on
  GPIO_ResetBits(GPIOC, GPIO_Pin_12);

  TestEnableTx();
}

static u8 GetSerialNumber(u32* serialNumberOut, u8* bitOut)
{
  u32 serialNumber;
  u8 bit;
  
  {
    // Compute serial number
    SlowPutString("Getting serial...\r\n");
    serialNumber = 2048;
    u8* serialbase = (u8*)FLASH_SERIAL_BITS;
    while (serialbase[(serialNumber >> 3)] == 0)
    {
      serialNumber += 8;
      if (serialNumber > 0x7ffff)
      {
        return 1;
      }
    }
    
    u8 bitMask = serialbase[(serialNumber >> 3)];
    
    // Find which bit we're on
    bit = 0;
    while (!(bitMask & (1 << bit)))
    {
      bit++;
    }
    serialNumber += bit;
    
    SlowPrintf("serial: %i, %08X, %i\r\n", serialNumber, serialNumber, bit);
  }
  
  serialNumber |= (FIXTURE_SERIAL & 0xFFF) << 20;
  
  *serialNumberOut = serialNumber;
  *bitOut = bit;
  
  return 0;
}

void FlashSWD(const uint8_t* shim, const uint8_t* bin, int address, int serialAddress)
{
  u32 serialNumber;
  u8 bit;

  int good = 0;

  // Try to contact via SWD
  swd_enable();
  swd_chipinit();

  swd_write32(0xE000EDF0, 0xA05F0003);            // Halt core, just in case
  
  int r = swd_write32(0xE000EDF0, 0xA05F0003);    // Halt core, just in case
  swd_write32(0xE000ED0C, 0x05FA0004);            // Reset peripherals 
  
  int uid0 = swd_read32(0x1FFFF7AC);
  
   
  // Grab (and program) the serial number first in the factory block so we
  // can put it in the bootloader too.
  //if (serialNumber == 0xFFFFffff)
  if (GetSerialNumber(&serialNumber, &bit))
  {
    SlowPrintf("OUT OF SERIALS!\r\n");
    throw ERROR_OUT_OF_SERIALS;
  }
  
  good = 1;
  u8* bootBuffer = GetGlobalBuffer();
  memcpy(bootBuffer, g_Body, (g_BodyEnd - g_Body) << 1);

  // Personalize bootloader for this car

  // For reasons I don't understand, must flash one page at a time
  int pagecount = ((g_BodyEnd - g_Body) + 511) >> 9;      
  for (int pg = 0; pg < pagecount; pg++)
  {
    swd_flash_page(0x08000000 + (pg * 1024), (u16*)&bootBuffer[pg * 1024]);
  }
  
  // Verify bootloader part
  u16* next = (u16*)bootBuffer;
  u32 size = (u8*)g_BodyEnd - (u8*)g_Body;
  int addr = 0x08000000;
  while (next < (u16*)&bootBuffer[size])
  {
    int orig = next[0] | (next[1] << 16);
    int board = swd_read32(addr);
    next += 2;
    addr += 4;
    if (orig != board)
    {
      SlowPrintf("Bad Match: %08X: %04X, %04X\r\n", addr, orig, board);
      good = 0;
      break;
    }
  }
  
  swd_write32(0xE000EDF0, 0xA05F0003);    // Halt core, just in case

  
  // On success, reserve this serial number - wait, shouldn't we do this earlier?
  if (good)
  {
    u32 location = FLASH_SERIAL_BITS + ((serialNumber & 0x7ffff) >> 3);
    
    FLASH_Unlock();
    FLASH_ProgramByte(location, ~(1 << bit));
    FLASH_Lock();
    
    ConsolePrintf("esn,%08X\r\n", serialNumber);
    
    SlowPutString("OK\r\n");
  } else {
    throw ERROR_HEAD_BOOTLOADER;
  }    
}
