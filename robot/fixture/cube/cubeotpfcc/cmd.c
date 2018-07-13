#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "app.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "otp.h"

//-----------------------------------------------------------
//        Output
//-----------------------------------------------------------

void printInt_(int val)
{
  bool printing = 0;
  int place = 1000000000;
  
  if( val < 0 ) {
    val *= -1;
    hal_uart_putchar('-');
  }
  
  while( place ) {
    int digit = val/place;
    val -= digit*place;
    if( digit > 0 || printing || place==1 ) {
      hal_uart_putchar( 0x30 + digit );
      printing = 1; //print all following digits
    }
    place /= 10;
  }
}

static void printX08_(uint8_t val) {
  for(int x=0; x<2; x++) {
    int nibble = (val & 0xf0) >> 4;
    char ascii = nibble >= 10 ? nibble - 10 + 0x61 : nibble + 0x30; //map to ascii char
    hal_uart_putchar( ascii );
    val <<= 4;
  }
}

static void printX32_(uint32_t val) {
  for(int x=0; x<4; x++) {
    printX08_( val >> 24 );
    val <<= 8;
  }
}

static inline void writes_(const char *s) { 
  if(s) console_write((char*)s); 
}

static inline int respond_(int status) {
  //static char b[20]; int bz = sizeof(b);
  //writes_( snformat(b,bz,"<<otp %i\n", status) );
  writes_("<<otp ");
  printInt_(status);
  hal_uart_putchar('\n');
  return status;
}

static inline int compare(uint8_t* dat1, uint8_t* dat2, int len) {
  for(int x=0; x<len; x++) {
    if( dat1[x] != dat2[x] )
      return 1;
  }
  return 0;
}

//-----------------------------------------------------------
//        Commands
//-----------------------------------------------------------

int cmd_process(char* s)
{
  //static char b[80]; int bz = sizeof(b);
  
  if( !strcmp(s, ">>otp write fcc") )
  {
    const int blocksize = 1024; //somewhat arbitrary chunking. save ram & provide nice status updates during long operations
    static uint8_t otp_buf[ blocksize ]; //MAX( blocksize, 2*sizeof(da14580_otp_header_t) ) ];
    memset( &otp_buf, 0, sizeof(otp_buf) );
    
    //writes_( snformat(b,bz,"cubefcc: 0x%08x-0x%08x (%u)\n", g_CubeBoot, g_CubeBootEnd-1, g_CubeBootSize) );
    writes_("cubefcc: 0x");
    printX32_((uint32_t)g_CubeBoot);
    writes_("-0x");
    printX32_((uint32_t)g_CubeBootEnd-1);
    writes_(" (");
    printInt_(g_CubeBootSize);
    writes_(")\n");
    
    //read current OTP header
    //writes_("reading otp header\n");
    uint8_t otp_head_buf1[ sizeof(da14580_otp_header_t) ];
    da14580_otp_header_t* otphead = (da14580_otp_header_t*)&otp_head_buf1[0];
    otp_read(OTP_ADDR_HEADER, sizeof(da14580_otp_header_t), (uint8_t*)otphead);
    //-*/
    
    //generate header from current application
    //writes_("generate fcc header\n");
    uint8_t otp_head_buf2[ sizeof(da14580_otp_header_t) ];
    da14580_otp_header_t* binhead = (da14580_otp_header_t*)&otp_head_buf2[0];
    otp_header_init( binhead, NULL );
    
    //--------- No room for stowaways; skip all safetey checks -----------
    
    //let's burn this mother trucker!
    writes_("burning image:\n");
    for(int addr=0; addr < g_CubeBootSize; addr += blocksize)
    {
      int oplen = MIN(blocksize, g_CubeBootSize - addr);
      //writes_( snformat(b,bz,"  writing %05x-%05x...", addr, addr+oplen-1 ) );
      writes_("  writing "); 
      printX32_(addr); 
      hal_uart_putchar('-'); 
      printX32_(addr+oplen-1); 
      writes_("...");
      
      int wstat = otp_write((uint32_t*)(OTP_ADDR_BASE+addr), (uint32_t*)&g_CubeBoot[addr], oplen);
      if( wstat == OTP_WRITE_OK )
        writes_("done\n");
      else {
        //writes_( snformat(b,bz,"failed e=%i\n", wstat) );
        writes_( "failed e=" ); 
        printInt_(wstat); 
        hal_uart_putchar('\n');
        return respond_(STATUS_WRITE_ERROR); //bail!
      }
    }
    writes_( "write complete\n" );
    
    //burn the header
    writes_("burning otp header...");
    uint32_t *src  = (uint32_t*)((int)binhead    + OTP_HEADER_SIZE - 4 );
    uint32_t *dest = (uint32_t*)(OTP_ADDR_HEADER + OTP_HEADER_SIZE - 4 );
    uint32_t *end  = (uint32_t*)(OTP_ADDR_HEADER);
    while( dest >= end ) { //write words in reverse order - app flags last (app is invalid until app flags written)
      if( *src > 0 && *dest != *src ) { //ignore empty fields && data match
        int res = otp_write(dest, src, sizeof(uint32_t));
        if( res != OTP_WRITE_OK ) {
          //writes_( snformat(b,bz,"failed @ 0x%x otp_write().err=%i\n", dest, res) );
          writes_("failed @ 0x"); 
          printX32_((uint32_t)dest); 
          writes_(" otp_write().err="); 
          printInt_(res); 
          hal_uart_putchar('\n');
          return respond_(STATUS_WRITE_ERROR);
        }
      }
      src--, dest--;
    }
    writes_("done!\n");
    
    writes_("verifying image.");
    for(int addr=0; addr < g_CubeBootSize; addr += blocksize)
    {
      int oplen = MIN(blocksize, g_CubeBootSize - addr);
      otp_read(OTP_ADDR_BASE+addr, oplen, otp_buf); //read otp into our buffer
      
      if( compare((uint8_t*)otp_buf, (uint8_t*)&g_CubeBoot[addr], oplen) ) {
        //writes_( snformat(b,bz,"[mismatch @ 0x%x]\n", addr) );
        writes_("[mismatch @ 0x"); 
        printX32_((uint32_t)addr); 
        writes_("]\n");
        return respond_(STATUS_FAILED_VERIFY);
      }
      writes_(".");
    }
    writes_( "ok\n" );
    
    //genetically merge original+app headers into what should now (hopefully) exist in OTP
    for( int x=0; x<OTP_HEADER_SIZE; x++ )
      ((uint8_t*)binhead)[x] |= ((uint8_t*)otphead)[x];
    
    //read actual OTP and compare
    writes_("verifying otp header...");
    otp_read(OTP_ADDR_HEADER, OTP_HEADER_SIZE, (uint8_t*)otphead); //re-read current OTP into buffer
    if( compare((uint8_t*)binhead, (uint8_t*)otphead, OTP_HEADER_SIZE) ) {
      writes_("FAIL\n");
      return respond_(STATUS_FAILED_VERIFY);
    }
    writes_( "ok\n" );
    
    return respond_(STATUS_OK);
  }
  
  return STATUS_UNKNOWN_CMD; //respond_(STATUS_UNKNOWN_CMD);
}

