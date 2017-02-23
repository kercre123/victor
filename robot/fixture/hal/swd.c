// Based on Drive Testfix, updated for Cozmo EP1 Testfix

// Helpful references are:
//  https://www.silabs.com/Support%20Documents/TechnicalDocs/AN0062.pdf
//  https://github.com/MarkDing/swd_programing_sram/blob/master/README.md
//  ARM Debug Interface v5
//  K02 Reference manual covers a few SWD details

// Observations:  K02 needs to have power cycled if it starts replying with FAULT

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

#define GSET(gp, pin)      gp->BSRRL = (pin)
#define GRESET(gp, pin)    gp->BSRRH = (pin)
#define GREAD(gp)          gp->IDR

int SlowPrintf(const char* format, ...);

//#define SWD_DEBUG


/************
 * SWD stuff
 */

#define SWD_ACK 1
#define SWD_WAIT 2
#define SWD_FAULT 4
#define SWD_PARITY 8

// This short delay was tested empirically - it must be fast (>125KHz) to work with the nRF51's strange glitch filter
#define bit_delay() {volatile int x = 10; while (x--);}

static char HostBus = 0; // SWD bus is controlled by the host or target

// Switch to host-controlled bus
static void swd_turnaround_host()
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
static void swd_turnaround_target()
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
static void write_bits(int nbits, const unsigned long *bits)
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
static void read_bits(int nbits, volatile unsigned long *bits)
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

// Send magic number to switch to SWD mode. This function sends many
// zeros, 1s, then the magic number, then more 1s and zeros to try to
// get the SWD state machine's attention if it is connected in some
// unusual state.
static void swd_enable(void)
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
static int swd_get_target_response(void)
{
	unsigned long data;

	read_bits(3, &data);

	return data;
}

// This function counts the number of 1s in an integer. It is used
// to calculate parity. This is the MIT HAKMEM (Hacks Memo) implementation.
static int count_ones(unsigned long n)
{
   register unsigned int tmp;

   tmp = n - ((n >> 1) & 033333333333)
           - ((n >> 2) & 011111111111);
   return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}

// This is one of the two core SWD functions. It can write to a debug port
// register or to an access port register. It implements the host->target
// write request, reading the 3-bit status, and then writing the 33 bits
// data+parity.
static int swd_write(char APnDP, int A, unsigned long data)
{
	unsigned long wpr;
	int response;

  // Bit 0 = 1, Bit 1 = AP, Bit 2 = Read, Bit 3-4 = A 2-3 (A is 0, 4, 8, C), Bit 5 = parity
	wpr = 0x81 /*0b10000001*/ | (APnDP<<1) | (A<<1); // A is a 32-bit address bits 0,1 are 0.
	if(count_ones(wpr&0x1E)%2) // odd number of 1s
		wpr |= 1<<5;	  // set parity bit
#ifdef SWD_DEBUG
	SlowPrintf("swd_write(Port=%s, Addr=%x, data=%08x, wpr=%02x) = ",
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
static int swd_read(char APnDP, int A, volatile unsigned long *data)
{
	unsigned long wpr;
	int response;

  // K02 requires more than one try - often returns WAITs when busy
  for (int retry = 0; retry < 5; retry++)
  {
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
    } else {
      break;
    }
  }
  
  // Bail out on failure
  if (response != SWD_ACK) 
    return response;
  
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

static void swd_setcsw(int addr, int size)
{
  unsigned long val;
  swd_read(1, 0x0, &val);   // Get AP reg 0
  swd_read(0, 0xC, &val);   // Get result
  int csw = (val & ~0xff) | (addr << 4) | size;
  swd_write(1, 0, csw);
}

int swd_read32(int addr)
{
  unsigned long value;
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_read(1, 0xC, &value);  // Read data from AP
  r |= swd_read(0, 0xC, &value);  // Read readback reg
#ifdef SWD_DEBUG  
  if (r != SWD_ACK)
    SlowPrintf("FAILED");
  SlowPrintf("             swd_read32 @ %08x = %08x\n", addr, value);
#endif
  if (r != SWD_ACK)
    throw ERROR_SWD_READ_FAULT;
  return value;
}

static int swd_write32(int addr, int data)
{
  unsigned long value;
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_write(1, 0xC, data);   // Set data
  r |= swd_read(0, 0xC, &value);  // Read readback reg.. for some reason?
#ifdef SWD_DEBUG  
  if (r != SWD_ACK)
    SlowPrintf("FAILED");
  SlowPrintf("             swd_write32 @ %08x = %08x\n", addr, data);
#endif
  if (r != SWD_ACK)
    throw ERROR_SWD_WRITE_FAULT;
  return r;
}

// Send 8 0 bits to flush SWD state
/*
static void swd_flush(void)
{
	unsigned long data = 0;
	write_bits(8, &data);
}
static int swd_write16(int addr, u16 data)
{
  unsigned long value;
  swd_setcsw(1, 1);   // Switch to 16-bit mode    
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_write(1, 0xC, data);   // Set data
  r |= swd_read(0, 0xC, &value);  // Read readback reg.. for some reason?
#ifdef SWD_DEBUG  
  if (r != SWD_ACK)
    SlowPrintf("FAILED");
  SlowPrintf("             swd_write16 @ %08x = %08x\n", addr, data);
#endif
  if (r != SWD_ACK)
    throw ERROR_SWD_WRITE_FAULT;
  swd_setcsw(1, 2);   // Back to 32-bit mode
  return r;
}
// Lock the JTAG port - upon next reset, JTAG will not work again - EVER
// If you need a dev robot, you must have Pablo swap the CPU
// Returns 0 on success, 1 on erase failure (chip will survive), 2 on program failure (dead)
static int swd_lock_jtag(void)
{
  // TBD
  return 0;
}
*/

// Write a CPU register in the ARM core - mostly to set up PC and SP before execution
static void swd_write_cpu_reg(int reg, int value)
{
  // Wait for debug module to free up after last write
  int dhcsr;
  do {
    dhcsr = swd_read32((int)&CoreDebug->DHCSR);
  } while ( !(dhcsr & CoreDebug_DHCSR_S_REGRDY_Msk) );
  
  swd_write32((int)&(CoreDebug->DCRDR), value);         // Write value to data reg - DCRDR
  swd_write32((int)&(CoreDebug->DCRSR), 0x10000 | reg); // Write register to selector reg - DCRSR
}

// Read a CPU register from the ARM core
static int swd_read_cpu_reg(int reg)
{
  // Wait for debug module to free up from last write
  int dhcsr;
  do {
    dhcsr = swd_read32((int)&CoreDebug->DHCSR);
  } while ( !(dhcsr & CoreDebug_DHCSR_S_REGRDY_Msk) );
  
  swd_write32((int)&(CoreDebug->DCRSR), reg);          // Write register to selector reg - DCRSR  
  
  // Wait for read to complete
  do {
    dhcsr = swd_read32((int)&CoreDebug->DHCSR);
  } while ( !(dhcsr & CoreDebug_DHCSR_S_REGRDY_Msk) );
  
  return swd_read32((int)&(CoreDebug->DCRDR));         // Read value from data reg - DCRDR
}

// Display a crash dump - for debugging the stub/firmware
static void swd_crash_dump()
{
  try {
    // Halt CPU
    swd_write32((int)&CoreDebug->DHCSR, 0xA05F0003);  // Run core - AP_DRW=RUN_CMD - DO NOT LEAVE LSB 1!

    SlowPrintf("\n\n== Crash dump from DUT\n");
  
    for (int i = 0; i < 16; i++)
      SlowPrintf("R%-2d = %08x  ", i, swd_read_cpu_reg(i));
    int stack = swd_read_cpu_reg(13), pc = swd_read_cpu_reg(15);
    
    SlowPrintf("\n== Stack at %08x:\n", stack);
    for (int i = 0; i < 16; i++)
      SlowPrintf("%08x  ", swd_read32(stack + i*4));
    
    SlowPrintf("\n== PC at %08x:\n", pc);
    for (int i = 0; i < 16; i++)
      SlowPrintf("%04x %04x ", swd_read32(pc + i*4) & 65535, (swd_read32(pc + i*4) >> 16) & 65535);
  } catch (error_t) {
    // Ignore exceptions during crash dumps so we don't lose the real root cause
    SlowPrintf("..unable to read DUT\n");
  }
  SlowPrintf("\n\n");
}

// Initialize the chip
static void swd_chipinit(void)
{
  unsigned long value, idcode;
  if (SWD_ACK != swd_read(0, 0, &idcode))   // Debug port (not AP), register 0 (IDCODE)
    throw ERROR_SWD_IDCODE;
  
  swd_write(0, 0x4, 0x54000000);  // Power up system and debug - place chip in reset   
  swd_write32((int)&CoreDebug->DHCSR, 0xA05F0003);  // Halt CPU so we can take over (w/debug)
  swd_write(0, 0x4, 0x50000000);  // Power up system and debug - disable reset
  swd_read(0, 0x4, &value);       // Check powered up and out of reset
  if (0xf0 != (value >> 24))
    throw ERROR_SWD_IDCODE;
  
  swd_write(0, 0x8, 0x0);   // Set AP bank 0x0 on AP 0x0 (the only useful bank)
  swd_setcsw(1,2);          // 32-bit auto-inc
    
  // This is for accessing the MDM-AP from the ST and K02 - not needed to distinguish K02 (M4) vs nRF51 (M0)
  //swd_write(0, 0x8, 0xF0);   // Set AP bank 0xF on AP 0x0 (the memory AP)
  //swd_read(1, 0xC, &value);   // Read AP (not debug port), register 0xFC (IDCODE)
  //swd_read(0, 0xC, &value);   // Read back result from RB registsr

  // If this is the K02, do K02 init
  if (idcode == 0x2ba01477)
  {
    // Check whether device is locked and unlocked it (see AN4835)
    
  // If this is the nRF51, do nRF51 init
  } else if (idcode == 0x0bb11477) {
    // Suggested workarounds - they don't seem to apply to us anyway
    //swd_write32(0x40000524, 0xF); // PAN-16 - workaround for RAMs not powering up (on obsolete revs)
    //swd_write32(0x40000608, 0x1); // Turn off MPU (protection) while debugging
    
  // If we don't recognize the chip, fault out
  } else {
    throw ERROR_SWD_IDCODE;
  }

  // Better engineers use a breakpoint on the flash reset vector
  // But we're just going to assume our stub is safe to run without a boot ROM first
}

// Set up the SWD GPIO
void InitSWD(void)
{
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
}

// Be nice and stop driving power into turned-off boards
void SWDDeinit(void)
{
  PIN_IN(GPIOB, PINB_SWD);
  PIN_IN(GPIOB, PINB_SWC);
  PIN_PULL_NONE(GPIOB, PINB_SWD);
  PIN_PULL_NONE(GPIOB, PINB_SWC);
}

#define STUB_TIMEOUT 1000000  // 1 second is long enough to flash a page, right?
// Values placed in stub 'command'
#define WAITING_FOR_BLOCK -1

// Reboot the MCU, then load and execute a flash stub
// Stubs are bins (see binaries.h) with initialization code at the front that manage flashing a block at a time
// Stubs consist of a load address (usually 0x2000000), temp addr (load+0x1000), and a command addr (temp+blocksize+4)
// Typical block sizes are 1KB or 2KB - see the other stubs (in fixture/flash_stubs) for examples
void SWDInitStub(u32 loadaddr, u32 cmdaddr, const u8* start, const u8* end)
{
  // Power cycle the board to clear the cobwebs
  DisableBAT();      // DisableBAT contains its own delay to empty out power
  EnableBAT();
  MicroWait(50000);  // Power should be stable by now (bit unstable at 10ms)

  // Try to contact via SWD
  InitSWD();
  swd_enable();
  swd_chipinit();

  // Write the stub to RAM
  SlowPrintf("SWD writing stub..");
  swd_write32((int)&CoreDebug->DHCSR, 0xA05F0003);            // Halt core again, just in case
  swd_write(1, 0x4, loadaddr);
  for (int i = 0; i < (end+3-start)/4; i++)
    if (SWD_ACK != swd_write(1, 0xc, ((u32*)(start))[i]))
      throw ERROR_SWD_WRITE_STUB;
 
  // Start up the stub
  swd_write32(cmdaddr, 0);          // Clear command/status - so we can watch it startup
  swd_write_cpu_reg(15, loadaddr);  // Point at start address
  // Unhalt core - leave debugging enabled, since nRF51 gives us special flashing powers in debug mode
  swd_write32((int)&CoreDebug->DHCSR, 0xA05F0001);  
    
  // Wait for stub to wake up
  SlowPrintf("waiting..\n");
  u32 starttime = getMicroCounter();
  while (swd_read32(cmdaddr) != WAITING_FOR_BLOCK)
    if (getMicroCounter() - starttime > STUB_TIMEOUT) {
      swd_crash_dump();
      throw ERROR_SWD_NOSTUB;
    }
}

// Send a file to the stub, one block at a time
void SWDSend(u32 tempaddr, int blocklen, u32 flashaddr, const u8* start, const u8* end, u32 serialaddr, u32 serial, bool quickcheck)
{ 
  // Assign serial number if requested
  if (serialaddr && !serial)
  {
    serial = swd_read32(serialaddr);    // Existing serial number
    if (0 == serial || 0xffffFFFF == serial)
      serial = GetSerial();           // Missing serial, get a new one
    ConsolePrintf("serial,%08x\r\n", serial);
  }
  
  while (start < end)
  {
    // Be sure the stub isn't busy..
    u32 starttime = getMicroCounter();
    while (swd_read32(tempaddr+blocklen) != WAITING_FOR_BLOCK)
    if (getMicroCounter() - starttime > STUB_TIMEOUT) {
      swd_crash_dump();
      throw ERROR_SWD_FLASH_TIMEOUT;
    }
  
    // Pre-test - if the data is already there, skip flashing
    SlowPrintf("SWD pre-check %08x..", flashaddr);
    bool skip = true;
    if (quickcheck)
    {
      if (swd_read32(flashaddr) !=  *(u32*)start)
        skip = false;
      if (swd_read32(flashaddr+blocklen-4) != *(u32*)(start+blocklen-4))
        skip = false;
    } else {
      for (int i = 0; i < blocklen/4; i++) {
        int addr = flashaddr+i*4;
        int val = ((u32*)(start))[i];
        int actual = swd_read32(addr);
        if (serialaddr && addr == serialaddr)
          val = serial;   // Substitute serial when we hit that address
        if (actual != val)
        {
          skip = false;
          break;          // Need to re-send the block
        }
      }
    }
    
    if (!skip)
    {
      // Send the block
      SlowPrintf("send to stub..");
      swd_write(1, 0x4, tempaddr);
      for (int i = 0; i < blocklen/4; i++) {
        int val = ((u32*)(start))[i];
        if (serialaddr && flashaddr+i*4 == serialaddr)
          val = serial;   // Substitute serial when we hit that address
        if (SWD_ACK != swd_write(1, 0xc, val))
          throw ERROR_SWD_WRITE_BLOCK;    
      }
      // nRF51 (and other M0s) only accept 1KB auto-increment, so use write32 to cross blocklen boundary
      swd_write32(tempaddr+blocklen, flashaddr);  // Send address to flash to
      
      // Wait for it to flash
      SlowPrintf("waiting..");
      starttime = getMicroCounter();
      while (swd_read32(tempaddr+blocklen) != WAITING_FOR_BLOCK)
        if (getMicroCounter() - starttime > STUB_TIMEOUT) {
          swd_crash_dump();
          throw ERROR_SWD_FLASH_TIMEOUT;
        }
        
      // Verify that it did flash
      SlowPrintf("verifying..");
      for (int i = 0; i < blocklen/4; i++) {
        int addr = flashaddr+i*4;
        int val = ((u32*)(start))[i];
        int actual = swd_read32(addr);
        if (serialaddr && addr == serialaddr)
          val = serial;   // Substitute serial when we hit that address
        if (actual != val)
        {
          SlowPrintf("Data found at %08x was %08x, not %08x\n", addr, actual, val);
          ConsolePrintf("mismatch,%08x,%08x,%08x\r\n", addr, actual, val);
          throw ERROR_SWD_MISMATCH;
        }
      }
    }
    
    SlowPrintf("ok!\r\n");
    flashaddr += blocklen;
    start += blocklen;
  }
  
  SlowPrintf("Success!\r\n");
}
