#include <stdio.h>
#include "board.h"
#include "otp.h"
#include "da14580_otp_header.h"

//flash_programmer primitives
//#include "crc32.h"
#include "otpc.h"

//debug
#include "console.h"
//#define printf_(...)    console_printf(__VA_ARGS__)
//#include "hal_timer.h"
#define OTP_SIMULATE_WRITE  0

extern void printInt_(int val);

//------------------------------------------------  
//    OTP
//------------------------------------------------

void otp_read(uint32_t addr, int len, uint8_t *buf)
{
  otpc_clock_enable(); // Enable OTP clock
  
  // Before every read action, put the OTP back in MREAD mode.
  // In this way, we can handle the OTP as a normal
  // memory mapped block.
  SetBits32 (OTPC_MODE_REG, OTPC_MODE_MODE, OTPC_MODE_MREAD);

  memcpy(buf, (uint8_t*)addr, len);
  
  otpc_clock_disable();
}

int otp_write(uint32_t *dest, uint32_t *src, int size)
{
  //pre-check lots of stuff. Only get one shot at this.
  if( ((uint32_t)dest) & 3 > 0 || ((uint32_t)src) & 3 > 0 )
    return OTP_WRITE_ADDR_NOT_ALIGNED;
  if( size & 3 > 0 )
    return OTP_WRITE_SIZE_NOT_ALIGNED;
  if( (uint32_t)dest < OTP_BASE_ADDRESS || (uint32_t)dest > (OTP_BASE_ADDRESS + OTP_SIZE - 1) ) //dest address within OTP range
    return OTP_WRITE_OUTSIDE_OTP_RANGE;
  if( (uint32_t)dest+size > OTP_BASE_ADDRESS + OTP_SIZE )
    return OTP_WRITE_OUTSIDE_OTP_RANGE;
  
  //Note: caller ensure programming voltage VPP is on and stable.
  
  int res = 0;
  int nwords = size >> 2;
  for(int i=0; i<nwords; i++)
  {
    /*
     * OTP cell must be powered on, so we turn on the related clock.
     * However, the OTP LDO will not be able to sustain an infinite 
     * amount of writes. So we keep our writes limited. Turning off the
     * LDO at the end of the operation ensures that we restart the 
     * operation appropriately for the next write.
     */
    otpc_clock_enable();

    /* From the manual: The destination address is a word aligned address, that represents the OTP memory
    address. It is not an AHB bus address. The lower allowed value is 0x0000 and the maximum allowed
    value is 0x1FFF (8K words space). */
    //^^0x1FFF Wrong! Max cel_addr arg is 0x7FFF; otpc_write_fifo() internally divides to get word addressing: (cel_addr>>2)&0x1FFF
    uint32_t dest_addr = (uint32_t)&dest[i] - OTP_BASE_ADDRESS;    
    if( dest_addr < OTP_SIZE )
    {
      #if OTP_SIMULATE_WRITE > 0
        #warning "OTP using simulated writes"
        //hal_timer_wait(1000);
        volatile int x = 1000;
        while( --x )
        {
        }
        res = 0;
      #else
        res = otpc_write_fifo( (uint32_t)&src[i], dest_addr, 1 );
      #endif
    }
    else
      res = -3;
    
    otpc_clock_disable();
    if (res != 0) {
      //char b[10];
      //console_write( snformat(b,sizeof(b),"[%i,%i]\n", res, i) );
      hal_uart_putchar('[');
      printInt_(res);
      hal_uart_putchar(',');
      printInt_(i);
      hal_uart_putchar(']');
      hal_uart_putchar('\n');
      break;
    }
  }
  
  return !res ? OTP_WRITE_OK : OTP_WRITE_INTERNAL_ERR;
}

//------------------------------------------------  
//    OTP Header
//------------------------------------------------

//static da14580_otp_header_t otp_header __attribute__((at( OTP_HEADER_ADDRESS ))); //locate at actual address for hex file generation

void otp_header_init( da14580_otp_header_t* otphead, bdaddr_t *bdaddr )
{
  if( !otphead )
    return;
  
  memset(otphead, 0, sizeof(da14580_otp_header_t)); //init all zeros (default/unused value)
  
  //valid app and package info
  otphead->app_flag_1 = OTP_HEADER_APP_FLAG_1_VALID;
  otphead->app_flag_2 = OTP_HEADER_APP_FLAG_2_VALID;
  
  //32k clock selection
  otphead->src_32KHz  = OTP_HEADER_32K_SRC_RC32KHZ;
  
  //Note: dev_unique_id[] used as BLE address by Dialog stack. LSB @ 0xD4
  //if( bdaddr )
  //  memcpy( (uint8_t*)((int)otphead + OTP_HEADER_BDADDR_OFFSET), (uint8_t*)bdaddr, BDADDR_SIZE );
  
  //"keep [default=0] remap 0x00 to SRAM as the right choice?" <IDM> Correct, keep this as SRAM mapped to 0. </IDM>
  //otphead->remap_flag = OTP_HEADER_REMAP_ADDR_0_OTP; //0=OTP_HEADER_REMAP_ADDR_0_SRAM
  
  //DMA Length - Number of 32-bit words. < 0x2000 words. (>= APPLICATION LEN FOR OTP MIRRORING)
  //otphead->dma_len = (app_len/sizeof(uint32_t)) + ((app_len & 3) ? 1 : 0); //round up
  otphead->dma_len = 0x1FC0;	// NDM: Because Ian Morris told us to use this setting
  
  //XXX: lock JTAG?
  //otphead->jtag_enable = OTP_HEADER_JTAG_DISABLED;
}

//------------------------------------------------  
//    Debug
//------------------------------------------------
/*
static inline char* package_s(int package) {
  switch(package) {
    case OTP_HEADER_PACKAGE_WLCSP34: return "WLCSP34"; //break;
    case OTP_HEADER_PACKAGE_QFN40: return "QFN40"; //break;
    case OTP_HEADER_PACKAGE_QFN48: return "QFN48"; //break;
    case OTP_HEADER_PACKAGE_KGD: return "KGD"; //break;
    case OTP_HEADER_PACKAGE_QFN40_0P5MM: return "QFN40 0.5mm"; //break;
    case OTP_HEADER_PACKAGE_QFN40_FLASH: return "QFN40 +flash, DA14583-00"; //break;
    case OTP_HEADER_PACKAGE_QFN56_SC14439: return "QFN56 +SC14439, DA14582"; //break;
  } return "unknown";
}

static inline char* src32k_s(int src32k) {
  switch(src32k) {
    case OTP_HEADER_32K_SRC_XTAL32KHZ: return "XTAL32k"; //break;
    case OTP_HEADER_32K_SRC_RC32KHZ: return "RC32k"; //break;
  } return "unknown";
}

static inline char* algorithm_s(int sig_algorithm) {
  switch(sig_algorithm) {
    case OTP_HEADER_SIG_ALGORITHM_NONE: return "NONE"; //break;
    case OTP_HEADER_SIG_ALGORITHM_MD5: return "MD5"; //break;
    case OTP_HEADER_SIG_ALGORITHM_SHA1: return "SHA-1"; //break;
    case OTP_HEADER_SIG_ALGORITHM_CRC32: return "CRC32"; //break;
  } return "unknown";
}

void otp_print_header( const char* heading, da14580_otp_header_t* otphead )
{
  if(heading)
    printf_("%s:\n", heading);
  
  printf_("  app flags            %08x,%08x (%s)\n", otphead->app_flag_1, otphead->app_flag_2, (otphead->app_flag_1 == OTP_HEADER_APP_FLAG_1_VALID && otphead->app_flag_2 == OTP_HEADER_APP_FLAG_2_VALID) ? "valid" : "invalid" );
  printf_("  dev_unique_id        %08x,%08x\n", otphead->dev_unique_id[0], otphead->dev_unique_id[1] );
  printf_("  src_32KHz            %08x %s\n", otphead->src_32KHz, src32k_s(otphead->src_32KHz) );  
  printf_("  cal_flags            %08x\n", otphead->cal_flags);
  printf_("  sig_algorithm        %08x %s\n", otphead->sig_algorithm, algorithm_s(otphead->sig_algorithm));
  for(int x=0; x<15; x++)
    if(otphead->code_signature[x] > 0)
      printf_("  code_signature[%02u]   %08x\n", x, otphead->code_signature[x]);
  printf_("  remap_flag           %08x %s\n", otphead->remap_flag, otphead->remap_flag == OTP_HEADER_REMAP_ADDR_0_OTP ? "OTP" : "SRAM" );
  printf_("  dma_len              %08x (%u)\n", otphead->dma_len, otphead->dma_len*4 );
  printf_("  jtag_enable          %08x %s\n", otphead->jtag_enable, otphead->jtag_enable == OTP_HEADER_JTAG_DISABLED ? "disabled" : "enabled");
  printf_("  FAC_rf_trim          %08x\n", otphead->FAC_rf_trim);
  printf_("  FAC_crc              %08x\n", otphead->FAC_crc);
  for(int x=0; x<13; x++)
    if(otphead->FAC_reserved[x] > 0)
      printf_("  FAC_reserved[%02u]     %08x\n", x, otphead->FAC_reserved[x]);
  printf_("  FAC_timestamp        %08x\n", otphead->FAC_timestamp);
  printf_("  FAC_tester           %08x\n", otphead->FAC_tester);
  printf_("  position             %08x\n", otphead->position);
  printf_("  package              %08x %s\n", otphead->package, package_s(otphead->package));
  printf_("  FAC_trim_LNA         %08x\n", otphead->FAC_trim_LNA);
  printf_("  trim_RFIO            %08x\n", otphead->trim_RFIO);
  printf_("  FAC_trim_bandgap     %08x\n", otphead->FAC_trim_bandgap);
  printf_("  FAC_trim_RC16        %08x\n", otphead->FAC_trim_RC16);
  printf_("  trim_XTAL16          %08x\n", otphead->trim_XTAL16);
  printf_("  FAC_trim_VCO         %08x\n", otphead->FAC_trim_VCO);
  for(int x=0; x<8; x++)
    if(otphead->custom[x] > 0)
      printf_("  custom[%u]            %08x\n", x, otphead->custom[x]);
  for(int x=0; x<6; x++)
    if(otphead->custom_field[x] > 0)
      printf_("  custom_field[%02u]     %08x\n", x, otphead->custom_field[x]);
}
*/
