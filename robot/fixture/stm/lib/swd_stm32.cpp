#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "console.h"
#include "fixture.h"
#include "swd.h"
#include "timer.h"

//send prints to the console
#define SwdPrintf(...)        ConsolePrintf(__VA_ARGS__)

//debug config
#define SWD_STM32_DEBUG 0

#if SWD_STM32_DEBUG > 0
#define SwdDebugPrintf(...)   ConsolePrintf(__VA_ARGS__)
#else
#define SwdDebugPrintf(...)   { }
#endif

static int swd_initialized = 0;
static const int pagesize = STM32F030XX_PAGESIZE;
//STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= pagesize , swd_stm32_buf_size_check );
static uint8_t page_buffer[pagesize+4];

static void swd_stm32_setcsw(int addr, int size)
{
  //SwdDebugPrintf("  swd_setcsw @ %08x sz=%i\n", addr, size);
  unsigned long val;
  int r=0;
  r |= swd_read(1, 0x0, &val);   // Get AP reg 0
  r |= swd_read(0, 0xC, &val);   // Get result
  int csw = (val & ~0xff) | (addr << 4) | size;
  r |= swd_write(1, 0, csw);
  if (r != SWD_ACK)
    SwdDebugPrintf("  FAILED (%u) swd_setcsw(Addr=%08x, Sz=%u)\n", r, addr, size );
}

static int swd_stm32_write16(uint32_t addr, uint16_t data)
{
  SwdDebugPrintf("  swd_write16 @ %08x = %08x\n", addr, data);
  unsigned long value;
  swd_stm32_setcsw(1, 1);   // Switch to 16-bit mode    
  int r = swd_write(1, 0x4, addr);   // Set address
  r |= swd_write(1, 0xC, data);   // Set data
  r |= swd_read(0, 0xC, &value);  // Read readback reg.. for some reason?
  if (r != SWD_ACK)
    SwdDebugPrintf("  FAILED, resp=%u\n", r);
  swd_stm32_setcsw(1, 2);   // Back to 32-bit mode
  return r;
}

static bool swd_stm32_chipinit(void)
{
  SwdDebugPrintf("<swd_chipinit>\n");
  swd_phy_reset();
  unsigned long value;
  swd_read(0, 0, &value);   // Debug port (not AP), register 0 (IDCODE)
  swd_write(0, 0x4, 0x54000000);  // Power up
  swd_read(0, 0x4, &value);  // Check powered up
  if( value >> 24 != 0xf4 )
    SwdPrintf("  chipinit %x == %x FAILED\n", value >> 24, 0xf4);
  else
    SwdDebugPrintf("  chipinit %x == %x\n", value >> 24, 0xf4);
  //swd_write(0, 0x8, 0xF0);   // Set AP bank 0xF on AP 0x0 (the memory AP)
  //swd_read(1, 0xC, &value);   // Read AP (not debug port), register 0xFC (IDCODE)
  //swd_read(0, 0xC, &value);   // Read back result from RB registsr
  swd_write(0, 0x8, 0x0);   // Set AP bank 0x0 on AP 0x0 (the only useful bank)
  swd_stm32_setcsw(1,2);          // 32-bit auto-inc
  SwdDebugPrintf("</swd_chipinit, enabled=%u>\n", ((value >> 24) == 0xf4 ? 1 : 0) );
  return (value >> 24) == 0xf4;
}

static int swd_stm32_flash_page(int destaddr, u16* image)
{
  int r = 0;
  r |= swd_write32(0x40022010, 0x00000201);  // Enter program mode
  // Packed mode is not supported by this debug interface!
  swd_stm32_setcsw(1, 1);   // Switch to 16-bit mode    
  r |= swd_write(1, 0x4, destaddr);    // Start address
  u16* next = image;
  while (next < image+512)
  {
    Timer::get(); //timer bug. must poll for correct overflow handling
    r |= swd_write(1, 0xc, *next++);  
    r |= swd_write(1, 0xc, (*next++) << 16); // Due to lack of packed mode
  }
  swd_stm32_setcsw(1, 2);   // Back to 32-bit mode
  r |= swd_write32(0x40022010, 0x00000200);  // Finished!
  
  return r;
}

static int swd_stm32_init_(void)
{
  //reset,init the SWD interface
  if( !swd_stm32_chipinit() )
    return ERROR_SWD_CHIPINIT;
  
  //bring mcu out of reset
  SwdDebugPrintf("exit reset state, halt core\n");
  int r = 0;
  r |= swd_write32(0xE000EDF0, 0xA05F0003);     // Halt core, just in case
  NRST::set();                                  // Exit reset
  r |= swd_write32(0xE000EDF0, 0xA05F0003);     // Halt core, just in case
  r |= swd_write32(0xE000ED0C, 0x05FA0004);     // Reset peripherals
  if( r != SWD_ACK ) {
    SwdPrintf("exit reset FAILED (r=%u)>\n", r);
    return ERROR_SWD_INIT_FAIL;
  }
  
  return r;
}

//==============================================================================================================

//enable exceptions, or return err codes
#define THROW_RETURN(x)   throw (x)
//#define THROW_RETURN(x)   return (x)

int swd_stm32_init()
{
  if( !swd_initialized )
  {
    swd_init(); //phy
    int e = swd_stm32_init_();
    if( e != SWD_ACK ) //ERROR_SWD_CHIPINIT/INIT_FAIL
      THROW_RETURN( e );
    
    swd_initialized = 1;
  }
  return ERROR_OK;
}

int swd_stm32_deinit()
{
  if( swd_initialized )
  {
    //int r = swd_write32(0xE000EDF0, 0xA05F0003);    // Halt core, just in case
    //if( r != SWD_ACK )
    //  SwdPrintf("core halt FAILED\n");
  }
  
  swd_deinit(); //phy
  swd_initialized = 0;
  return ERROR_OK;
}

int swd_stm32_erase(void)
{
  SwdPrintf("erasing mcu\n");
  if( !swd_initialized )
    THROW_RETURN( ERROR_INVALID_STATE );
  
  uint32_t start = Timer::get();
  int r = 0;
  
  // Unlock flash chip
  //   0x40022004: Flash key register (FLASH_KEYR)
  //   unlock sequence, write 0x45670123, then 0xCDEF89AB
  r |= swd_write32(0x40022004, 0x45670123);
  r |= swd_write32(0x40022004, 0xCDEF89AB);
  
  // Start mass erase
  //   0x40022010: Flash control register (FLASH_CR)
  //    <2> MER: mass erase
  //    <6> STRT: start
  //    <9> OPTWRE: option byte write enable (set by unlock sequence, reset by writing 0)
  r |= swd_write32(0x40022010, 0x00000204);
  r |= swd_write32(0x40022010, 0x00000244);
  
  if( r != SWD_ACK )
    THROW_RETURN( ERROR_SWD_WRITE_FAULT );
  
  // Check busy flag
  //   0x4002200C: Flash status register (FLASH_SR)
  //    <0> BSY: flash operation in progress
  int timeout = 50;
  SwdDebugPrintf("  .");
  while ((swd_read32(0x4002200C) & 1) && (timeout--) > 0) {
    Timer::get(); //timer bug. must poll for correct overflow handling
    SwdDebugPrintf(".");
    Timer::wait(10000);
  }
  r |= swd_write32(0x40022010, 0x00000200);  // Finished!
  SwdDebugPrintf("\n");
  if( r != SWD_ACK )
    THROW_RETURN( ERROR_SWD_FLASH_ERASE );
  
  SwdPrintf("erase %s in %ums (r=%u)\n", (r == SWD_ACK ? "completed" : "FAILED"), (Timer::get() - start)/1000, r );
  if( timeout < 1 )
    THROW_RETURN( ERROR_SWD_FLASH_ERASE );
  
  return ERROR_OK;
}

int swd_stm32_read(uint32_t flash_addr, uint8_t *out_buf, int size)
{
  SwdPrintf("flash read %i bytes @ 0x%08x-0x%08x...", size, flash_addr, flash_addr+size-1 );
  
  if( !swd_initialized )
    THROW_RETURN( ERROR_INVALID_STATE );
  if( !out_buf || size < 1 ) {
    ConsolePrintf("BAD_ARG: swd_stm32_read() out_buf=%08x size=%i\n", (uint32_t)out_buf, size);
    THROW_RETURN( ERROR_BAD_ARG );
  }
  
  uint32_t remain = size;
  uint32_t Tstart = Timer::get();
  for(int addr=0; addr<size; addr+=4, remain-=4 )
  {
    uint32_t w = swd_read32(flash_addr+addr);
    memcpy( &out_buf[addr], &w, MIN(4,remain) );
  }
  
  SwdPrintf("done [%ums]\n", (Timer::get() - Tstart)/1000 );
  return ERROR_OK;
}

int swd_stm32_verify(uint32_t flash_addr, const uint8_t* bin_start, const uint8_t* bin_end)
{
  int size = bin_end - bin_start;
  SwdPrintf("flash verify, %u bytes (%u pages) @ 0x%08x-0x%08x\n", size, CEILDIV(size,pagesize), flash_addr, flash_addr+size-1 );
  
  if( !swd_initialized )
    THROW_RETURN( ERROR_INVALID_STATE );
  if( !bin_start || !bin_end || bin_end <= bin_start ) {
    ConsolePrintf("BAD_ARG: swd_stm32_verify() start=%06x end=%06x size=%i\n", bin_start, bin_end, (int)bin_end-(int)bin_start);
    THROW_RETURN( ERROR_BAD_ARG );
  }
  
  /*/ Unlock flash chip
  int r = 0;
  r |= swd_write32(0x40022004, 0x45670123);
  r |= swd_write32(0x40022004, 0xCDEF89AB);
  if( r != SWD_ACK )
    THROW_RETURN( ERROR_SWD_WRITE_FAULT );
  //-*/
  
  const int linesize = 40; //pretty printing
  int ecnt = 0, pgcnt = 0, printlen = 0;
  uint32_t start = Timer::get();
  for(int addr=0; addr < size; addr+=linesize)
  {
    int len = addr+linesize < size ? linesize : size-addr; //last line probably short
    
    //read next line
    for(int i=0; i < len; i+=4) {
      Timer::get(); //timer bug. must poll for correct overflow handling
      *((uint32_t*)&page_buffer[i]) = swd_read32(flash_addr+addr+i);
    }
    //if( len < linesize )
    //  memset( &page_buffer[len], 0xFF, linesize-len ); //pad incomplete line
    
    //show some progress in console so we know it didn't freeze up
    if( addr/pagesize > pgcnt ) { //each page
      pgcnt = addr/pagesize;
      printlen += SwdPrintf(".");
    }
    
    //test for errors
    int eline = 0;
    for(int i=0; i < len; i++) { //error check this line
      if( page_buffer[i] != bin_start[addr+i] )
        eline++;
    }
    
    /*/=======================DEBUG========================
    //force some errors to see how it handles
    #warning "forcing errors to test flash verification"
    const int add_wack_start = flash_addr + (4*pagesize) - 8;
    const int add_wack_end   = flash_addr + (4*pagesize) + 8;
    if( flash_addr+addr >= add_wack_start && addr < add_wack_end )
      eline = 1;
    //===================================================*/
    
    //handle errors
    if( eline )
    {
      if(printlen)
        SwdPrintf("\n");
      printlen = 0;
      
      //print binary vs read data to compare
      SwdPrintf("err @ %08x bin : ", flash_addr+addr);
      for(int i=0; i < len; i++)
        SwdPrintf("%02x", bin_start[addr+i]);
      SwdPrintf("\n               read: ");
      for(int i=0; i < len; i++)
        SwdPrintf("%02x", page_buffer[i]);
      SwdPrintf("\n");
      
      if( ++ecnt > 10 ) //show this many errors before giving up (helps debug)
        break;
    }
  }
  if(printlen)
    SwdPrintf("\n");
  
  SwdPrintf("flash verify %s in %ums\n", !ecnt ? "completed" : "FAILED", (Timer::get() - start)/1000 );
  if( ecnt > 0 )
    THROW_RETURN( ERROR_FLASH_VERIFY );
  
  return ERROR_OK;
}

int swd_stm32_flash(uint32_t flash_addr, const uint8_t* bin_start, const uint8_t* bin_end, bool verify)
{
  int e = ERROR_OK;
  int r = 0; //internal swd ack error code
  int size = bin_end - bin_start;
  
  SwdPrintf("flash write, %u bytes (%u pages) @ 0x%08x-0x%08x\n", size, CEILDIV(size,pagesize), flash_addr, flash_addr+size-1 );
  if( !swd_initialized )
    THROW_RETURN( ERROR_INVALID_STATE );
  if( !bin_start || !bin_end || bin_end <= bin_start ) {
    ConsolePrintf("BAD_ARG: swd_stm32_flash() start=%06x end=%06x size=%i\n", bin_start, bin_end, (int)bin_end-(int)bin_start);
    THROW_RETURN( ERROR_BAD_ARG );
  }
  
  // For reasons I don't understand, must flash one page at a time
  {
    uint32_t start = Timer::get();
    r = 0;
    
    // Unlock flash chip
    r |= swd_write32(0x40022004, 0x45670123);
    r |= swd_write32(0x40022004, 0xCDEF89AB);
    if( r != SWD_ACK )
      THROW_RETURN( ERROR_SWD_WRITE_FAULT );
    
    for(int page=0; size > 0; page++, size-=pagesize )
    {
      Timer::get(); //timer bug. must poll for correct overflow handling
      
      memcpy(page_buffer, &bin_start[page*pagesize], pagesize); //buffer next page
      if(size < pagesize) //final page is not full
        memset(&page_buffer[size], 0xFF, pagesize-size); //fill FFs
      
      SwdDebugPrintf(".swd_stm32_flash_page 0x%08x with %u bytes\n", flash_addr + (page*pagesize), (size > pagesize ? pagesize : size));
      int r1 = swd_stm32_flash_page(flash_addr + (page*pagesize), (u16*)page_buffer );
      //SwdDebugPrintf("...%s\n", !r1 ? "ok" : "failed");
      r |= r1;
      
      //when debug is turned off, print each page write/status AFTER op completes (includes result, not complicated by internal debug prints)
      if( !SWD_STM32_DEBUG )
        SwdPrintf(".swd_stm32_flash_page 0x%08x with %u bytes: %s\n", flash_addr + (page*pagesize), (size > pagesize ? pagesize : size), (r1 == SWD_ACK ? "ok" : " FAILED") );
    }
    
    SwdPrintf("flash write %s in %ums (r=%u)\n", (r == SWD_ACK ? "completed" : "FAILED"), (Timer::get() - start)/1000, r );
    if( r != SWD_ACK )
      THROW_RETURN( ERROR_SWD_FLASH_WRITE );
  }
  
  Timer::delayMs(10);
  
  //Readback verify
  if( verify )
    e = swd_stm32_verify(flash_addr, bin_start, bin_end);
  
  //r = swd_write32(0xE000EDF0, 0xA05F0003);    // Halt core, just in case
  //if( r != SWD_ACK )
  //  SwdPrintf("core halt FAILED\n");
  
  if( e != ERROR_OK )
    THROW_RETURN(e);
  return ERROR_OK;
}

// Lock the JTAG port - upon next reset, JTAG will not work again - EVER
// If you need a dev car, you must have Pablo swap the CPU
// Returns 0 on success, 1 on erase failure (chip will survive), 2 on program failure (dead)
int swd_stm32_lock_jtag(void)
{
  SwdDebugPrintf("stm32 lock jtag\n");
  if( !swd_initialized )
    THROW_RETURN( ERROR_INVALID_STATE );
  
  //XXX: this hasn't been tested since adding init and error checking stuffs...
  
  uint32_t start = Timer::get();
  int r = 0;
  
  r |= swd_write32(0x40022004, 0x45670123);  // Unlock flash
  r |= swd_write32(0x40022004, 0xCDEF89AB);
  r |= swd_write32(0x40022008, 0x45670123);  // Unlock option bytes
  r |= swd_write32(0x40022008, 0xCDEF89AB);
  
  r |= swd_write32(0x40022010, 0x200 + 0x20); // Option byte ERASE w/OPTWRE
  r |= swd_write32(0x40022010, 0x200 + 0x40 + 0x20); // START option byte ERASE w/OPTWRE
  
  if( r != SWD_ACK )
    THROW_RETURN( ERROR_SWD_WRITE_FAULT );
  
  int wait = 250;                       // 250ms before we give up (outer limit is 60ms)
  while (swd_read32(0x4002200C) & 1)    // Wait for erase complete
  {
    if (wait-- <= 0)
      THROW_RETURN( 1 );
    Timer::wait(1000);
  }
  
  r |= swd_write32(0x40022010, 0x200 + 0x10);// Allow option byte program w/OPTWRE
  r |= swd_stm32_write16(0x1FFFF800, 0xCC);  // RDP mode 2 - irreversible disable
  
  if( r != SWD_ACK )
    THROW_RETURN( ERROR_SWD_WRITE_FAULT );
  
  wait = 250;                        // 250ms before we give up (outer limit is 1ms)
  while (swd_read32(0x4002200C) & 1)    // Wait for program complete
  {
    if (wait-- <= 0)
      THROW_RETURN( 2 );
    Timer::wait(1000);
  }
  
  r |= swd_write32(0x40022010, 0x0);         // Disable OPTWRE
  r |= swd_write32(0x40022010, 0x80);        // Re-lock flash  
  
  if( r != SWD_ACK )
    THROW_RETURN( ERROR_SWD_WRITE_FAULT );
  return ERROR_OK;
}

