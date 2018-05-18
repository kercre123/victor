#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "console.h"
#include "fixture.h"
#include "swd.h"
#include "timer.h"

//send prints to the console
#define SwdPrintf(...)        ConsolePrintf(__VA_ARGS__)

//debug config
#define SWD_DEBUG 0
#define SWD_DEBUG_PRINT_RW  0 /*low level r/w ops usually too verbose, clog up the debug console*/

#if SWD_DEBUG > 0
#define SwdDebugPrintf(...)   ConsolePrintf(__VA_ARGS__)
#else
#define SwdDebugPrintf(...)   { }
#endif

//swd response descriptions
#if SWD_DEBUG > 0
static inline char* resp_str(swd_resp_t swd_response) {
  switch( swd_response ) {
    case SWD_ACK: return (char*)"OK"; //break;
    case SWD_WAIT: return (char*)"WAIT"; //break;
    case SWD_FAULT: return (char*)"FAULT"; //break;
    case SWD_PARITY: return (char*)"PARITY"; //break;
  }
  return (char*)"SWDERROR";
}
#endif

//-----------------------------------
//        SWD PHY
//-----------------------------------

// 80 would occasionally fail a board, 150 seems enough margin - tested empirically
#define delay_data_setup() {volatile int x = 15; while (x--);}
#define delay_clock_high() {volatile int x = 15; while (x--);}

static char HostBus = 0; // SWD bus is controlled by the host or target

// Send magic number to switch to SWD mode. This function sends many
// zeros, 1s, then the magic number, then more 1s and zeros to try to
// get the SWD state machine's attention if it is connected in some
// unusual state.
void swd_phy_reset(void)
{
	unsigned long data;
  //SwdDebugPrintf("<swd_phy_reset()>\n");

  //1. Line Reset
  //2. SWD switchover from JTAG
  //3. Line Reset
  //4. Two idle clocks to get into idle state (SWD low)
  
	// write 0s
	data = 0;
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	
	// write FFs
	data = 0xFFFFFFFF;
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);

	data = 0xE79E;
	swd_phy_write_bits(16, &data);
	
	// write FFs
	data = 0xFFFFFFFF;
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);

  // write 0s
	data = 0;
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
	swd_phy_write_bits(32, &data);
  
  //SwdDebugPrintf("</swd_phy_reset()>\n");
}

// Switch to host-controlled bus
void swd_phy_turnaround_host()
{
  if(!HostBus)
	{
    delay_data_setup();
    
    SWCLK::set();
    delay_clock_high();
    delay_clock_high();
    SWCLK::reset(); //clock idle low
    
    // Take over SWDIO pin
    SWDIO::set();
    SWDIO::mode(MODE_OUTPUT);

		HostBus = 1;
	}
}

// Switch to target-controlled bus
void swd_phy_turnaround_target()
{
  if(HostBus)
  {
    //release SWD
    SWDIO::set(); //drive data idle before float (has pull-up)
    delay_data_setup();
    SWDIO::mode(MODE_INPUT);
    
    delay_data_setup();
    delay_data_setup(); //TODO: maybe remove this to optimize. test first
    
    //clock TRN
    SWCLK::set();
    delay_clock_high();
    delay_clock_high();
    SWCLK::reset();
    
    HostBus = 0;
  }
}

// Sends bits out the wire. This is a low-level function that does not
// implement any SWD protocol. The bits are clocked out on the clock falling
// edge for the SWD state machine to latch on the rising edge. After it
// is done this function leaves the clock low.
void swd_phy_write_bits(int nbits, const unsigned long *pdat)
{
	unsigned long wbuf = *pdat;
	swd_phy_turnaround_host();
  
	while(nbits--)
	{
    delay_data_setup();
    if ( wbuf & 1 )
      SWDIO::set();
    else
      SWDIO::reset();
    delay_data_setup();
    
    SWCLK::set();
    delay_clock_high();
    SWCLK::reset();
    
    wbuf >>= 1;
	}
}

// Reads bits from the wire. This is a low-level function that does not
// implement any SWD protocol. The bits are expected to be clocked out on
// the clock rising edge and then they are latched on the clock falling edge.
// After it is done this function leaves the clock low.
void swd_phy_read_bits(int nbits, volatile unsigned long *bits)
{
  int i=0;
	swd_phy_turnaround_target();

	*bits = 0;
	while(nbits--)
	{
    delay_data_setup();
    (*bits) |= (SWDIO::read() ? 1 : 0) << i++;  // read bit from SWDIO line
    
    SWCLK::set();
    delay_clock_high();
    SWCLK::reset();
	}
}

//-----------------------------------
//        SWD Common
//-----------------------------------

// The SWD protocol consists of read and write requests sent from
// the host in the form of 8-bit packets. These requests are followed
// by a 3-bit status response from the target and then a data phase
// which is 32 bits of data + 1 bit of parity. This function reads
// the three bit status responses.
static inline swd_resp_t swd_get_target_response_(void)
{
	unsigned long data;
	swd_phy_read_bits(3, &data);
	return data;
}

// This function counts the number of 1s in an integer. It is used
// to calculate parity. This is the MIT HAKMEM (Hacks Memo) implementation.
static int count_ones_(unsigned long n)
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
static swd_resp_t swd_write_(char APnDP, int A, unsigned long data)
{
	unsigned long wpr;
	swd_resp_t response;

  // Bit 0 = 1, Bit 1 = AP, Bit 2 = Read, Bit 3-4 = A 2-3 (A is 0, 4, 8, C), Bit 5 = parity
	wpr = 0x81 /*0b10000001*/ | (APnDP<<1) | (A<<1); // A is a 32-bit address bits 0,1 are 0.
	if(count_ones_(wpr&0x1E)%2) // odd number of 1s
		wpr |= 1<<5;	  // set parity bit
  
  if( SWD_DEBUG_PRINT_RW )
    SwdDebugPrintf("    swd_write(Port=%s, Addr=%d, data=%08x, wpr=%02x) = ", (APnDP ? "Access" : "Debug"), A, data, wpr);

	swd_phy_write_bits( 8, &wpr);
	// now read acknowledgement
	response = swd_get_target_response_();
	if(response != SWD_ACK)
	{
    if( SWD_DEBUG_PRINT_RW )
      SwdDebugPrintf("%s (%u)\n", (response == SWD_FAULT ? "FAULT" : "WAIT"), response );
    //else //only print faults
    //  SwdDebugPrintf("    %s (%u) swd_write(Port=%s, Addr=%d, data=%08x, wpr=%02x)\n", (response == SWD_WAIT ? "WAIT" : "FAULT"), response, (APnDP ? "Access" : "Debug"), A, data, wpr );
    
		swd_phy_turnaround_host();
		return response;
	}

	swd_phy_write_bits( 32, &data); // send write data
	wpr = 0;
	if(count_ones_(data)%2) // odd number of 1s
		wpr = 1;	  // set parity bit
	swd_phy_write_bits( 1, &wpr); // send parity

  if( SWD_DEBUG_PRINT_RW )
    SwdDebugPrintf("OK\n");
  
	return SWD_ACK;
}

swd_resp_t swd_write(char APnDP, int A, unsigned long data)
{
  swd_resp_t r;
  for(int retry=0; retry < 10; retry++)
  {
    if( (r = swd_write_(APnDP, A, data)) == SWD_WAIT )
      Timer::wait(10*1000);
    else
      break;
  }
  
  //if 'print-everything' debug is off, at least print the errors
  if( SWD_DEBUG && !SWD_DEBUG_PRINT_RW ) {
    if( r != SWD_ACK )
      SwdDebugPrintf("    %s (%u) swd_write(Port=%s, Addr=%d, data=%08x)\n", resp_str(r), r, (APnDP ? "Access" : "Debug"), A, data );
  }
  
  return r;
}

// This is one of the two core SWD functions. It can read from a debug port
// register or an access port register. It implements the host->target
// read request, reading the 3-bit status, and then reading the 33 bits
// data+parity.
static swd_resp_t swd_read_(char APnDP, int A, volatile unsigned long *data)
{
	unsigned long wpr;
	swd_resp_t response;

	wpr = 0x85 /*0b10000101*/ | (APnDP<<1) | (A<<1); // A is a 32-bit address bits 0,1 are 0.
	if(count_ones_(wpr&0x1E)%2) // odd number of 1s
		wpr |= 1<<5;	  // set parity bit
  
  if( SWD_DEBUG_PRINT_RW )
    SwdDebugPrintf("    swd_read(Port=%s, Addr=%d, wpr=%02x) = ", (APnDP ? "Access" : "Debug"), A, wpr);

	swd_phy_write_bits( 8, &wpr);
  
	// now read acknowledgement
	response = swd_get_target_response_();
	if(response != SWD_ACK)
	{
    if( SWD_DEBUG_PRINT_RW )
      SwdDebugPrintf("%s (%u)\n", (response == SWD_WAIT ? "WAIT" : "FAULT"), response );
    //else //only print faults
    //  SwdDebugPrintf("    %s (%u) swd_read(Port=%s, Addr=%d, wpr=%02x)\n", (response == SWD_WAIT ? "WAIT" : "FAULT"), response, (APnDP ? "Access" : "Debug"), A, wpr);
    
		swd_phy_turnaround_host();
		return response;
	}
  
  Timer::wait(100);
	swd_phy_read_bits( 32, data); // read data
	swd_phy_read_bits( 1, &wpr); // read parity
  
	if(count_ones_(wpr)%2) { // odd number of 1s
		if(wpr != 1)
      response = SWD_PARITY; // bad parity
	} else {
		if(wpr != 0)
      response = SWD_PARITY; // bad parity
	}

  if( SWD_DEBUG_PRINT_RW )
    SwdDebugPrintf("%08x%s\n", *data, (response == SWD_PARITY ? " PARITY ERROR" : "") );
  //else if(response != SWD_ACK) //only print faults
  //  SwdDebugPrintf("    %s (%u) swd_read(Port=%s, Addr=%d, wpr=%02x) = %08x\n", (response == SWD_PARITY ? " PARITY ERROR" : "ERROR"), response, (APnDP ? "Access" : "Debug"), A, wpr, *data );

  swd_phy_turnaround_host();
	return response;
}

swd_resp_t swd_read(char APnDP, int A, volatile unsigned long *data)
{
  swd_resp_t r;
  for(int retry=0; retry < 10; retry++)
  {
    if( (r = swd_read_(APnDP, A, data)) == SWD_WAIT )
      Timer::wait(10*1000);
    else
      break;
  }
  
  //if 'print-everything' debug is off, at least print the errors
  if( SWD_DEBUG && !SWD_DEBUG_PRINT_RW ) {
    if( r != SWD_ACK ) {
      if( r != SWD_PARITY )
        *data = 0; //didn't read anything. don't print junk
      SwdDebugPrintf("    %s (%u) swd_read(Port=%s, Addr=%d) = %08x\n", resp_str(r), r, (APnDP ? "Access" : "Debug"), A, *data );
    }
  }
  
  return r;
}

swd_resp_t swd_write32(uint32_t addr, uint32_t data)
{
  //SwdDebugPrintf("  swd_write32 @ %08x = %08x\n", addr, data);
  unsigned long value;
  swd_resp_t r;
  r  = swd_write(1, 0x4, addr);   // Set address
  r |= swd_write(1, 0xC, data);   // Set data
  r |= swd_read(0, 0xC, &value);  // Read readback reg.. for some reason?
  if (r != SWD_ACK)
    SwdDebugPrintf("  FAILED (%u) swd_write32(Addr=%08x, Dat=%08x)\n", r, addr, data);
    //SwdDebugPrintf("  FAILED, resp=%u\n", r);
  return r;
}

uint32_t swd_read32(uint32_t addr)
{
  //SwdDebugPrintf("  swd_read32 @ %08x\n", addr);
  unsigned long value;
  swd_resp_t r;
  r  = swd_write(1, 0x4, addr);   // Set address
  r |= swd_read(1, 0xC, &value);  // Read data from AP
  r |= swd_read(0, 0xC, &value);  // Read readback reg
  if (r != SWD_ACK) {
    SwdDebugPrintf("  FAILED (%u) swd_read32(Addr=%08x)\n", r, addr );
    //SwdDebugPrintf("  FAILED, resp=%u\n", r);
    throw ERROR_SWD_READ_FAULT;
  }
  return value;
}

void swd_init(void)
{
  HostBus = 0;
  //USAGE: make sure target power is off here
  
  SWCLK::reset();
  SWDIO::init(MODE_INPUT,  PULL_UP,   TYPE_PUSHPULL);
  SWCLK::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  NRST::init( MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  NRST::reset(); // Default in reset
  
  // Wait for lines to settle
  Timer::wait(200);
  
  //USAGE: power on target after init returns
}

void swd_deinit(void)
{
  NRST::reset(); //drive into reset
  Timer::wait(20*1000);
  
  SWDIO::init(MODE_INPUT, PULL_NONE );
  SWCLK::init(MODE_INPUT, PULL_NONE );
  NRST::init( MODE_INPUT, PULL_NONE );
  Timer::wait(200);
}

