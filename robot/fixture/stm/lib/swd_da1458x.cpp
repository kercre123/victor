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
#define SWD_DA1458X_DEBUG 1

#if SWD_DA1458X_DEBUG > 0
#define SwdDebugPrintf(...)   ConsolePrintf(__VA_ARGS__)
#else
#define SwdDebugPrintf(...)   { }
#endif

static const int pagesize = 1024;
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= pagesize , swd_stm32_buf_size_check );

static void swd_da1458x_setcsw(uint32_t addr, uint32_t size)
{
  SwdDebugPrintf("  setcsw @ %08x sz=%i\n", addr, size);
  unsigned long val;
  int r=0;
  r |= swd_read(ACCESS_PORT, 0x0, &val);   // Get AP reg 0
  r |= swd_read(DEBUG_PORT, SWDP_REG_R_RDBUF, &val); // Get result
  int csw = (val & ~0xff) | (addr << 4) | size;
  r |= swd_write(ACCESS_PORT, 0, csw);
  if (r != SWD_ACK)
    SwdDebugPrintf("  FAILED (%u) setcsw(Addr=%08x, Sz=%u)\n", r, addr, size );
}

static bool swd_da1458x_chipinit(void)
{
  SwdDebugPrintf("<da1458x_chipinit>\n");
  unsigned long value;
  
  //read IDCODE
  swd_read(DEBUG_PORT, SWDP_REG_R_IDCODE, &value);
  SwdDebugPrintf("  IDCODE=0x%08x version=%u PARTNO=%u Designer=%u\n", (uint32_t)value, IDCODE_GET_VERSION(value), IDCODE_GET_PARTNO(value), IDCODE_GET_DESIGNER(value) );
  
  //power up and reset
  swd_write(DEBUG_PORT, SWDP_REG_W_SELECT, 0x0); // Set AP bank 0x0 on AP 0x0. CTRLSEL=0
  swd_write(DEBUG_PORT, SWDP_REG_RW_CTRLSTAT, SWDP_CTRLSTAT_CSYSPWRUPREQ | SWDP_CTRLSTAT_CDBGPWRUPREQ | SWDP_CTRLSTAT_CDBGRSTREQ ); //Power up + DP reset
  swd_read( DEBUG_PORT, SWDP_REG_RW_CTRLSTAT, &value);      //Check powered up
  const uint32_t CTRLSTAT_verify = SWDP_CTRLSTAT_CSYSPWRUPACK | SWDP_CTRLSTAT_CSYSPWRUPREQ | SWDP_CTRLSTAT_CDBGPWRUPACK | SWDP_CTRLSTAT_CDBGPWRUPREQ | SWDP_CTRLSTAT_CDBGRSTREQ; //0xf4 << 24
  bool pwrup_rst_ok = (value & 0xFF000000) == CTRLSTAT_verify;
  SwdDebugPrintf("  CTRLSTAT=0x%08x %s\n", (uint32_t)value, (pwrup_rst_ok ? "ok" : "FAIL"));
  
  swd_da1458x_setcsw(1,2);  // 32-bit auto-inc
  SwdDebugPrintf("</da1458x_chipinit, enabled=%u>\n", pwrup_rst_ok );
  return pwrup_rst_ok;
}

int swd_da1458x_load_bin(const uint8_t* bin_start, const uint8_t* bin_end)
{
  //const uint32_t ram_load_addr = 0x08000000; //XXX: what's the ram start address?
  int e = ERROR_OK;
  int r = 0; //internal swd ack error code
  //uint8_t * page_buffer = app_global_buffer;
  
  SwdDebugPrintf("swd_da1458x_load_bin 0x%08x-0x%08x (%u)\n", bin_start, bin_end, bin_end-bin_start);
  if( !bin_start || !bin_end || bin_end <= bin_start ) {
    ConsolePrintf("BAD_ARG: swd_da1458x_load_bin() start=%06x end=%06x size=%i\n", bin_start, bin_end, (int)bin_end-(int)bin_start);
    return ERROR_BAD_ARG;
  }
  
  // Try to contact via SWD
  swd_phy_reset();
  if( !swd_da1458x_chipinit() )
    return ERROR_ACK1;
  
  //uh...
  volatile int ignore = e; ignore = r;
  return ERROR_INVALID_STATE; //not finished
  
  //return e;
}

int swd_da1458x_run()
{
  SwdPrintf("swd_da1458x_run\n");
  return -1;
}

