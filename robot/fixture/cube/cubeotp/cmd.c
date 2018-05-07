#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "app.h"
#include "bdaddr.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "hal_timer.h"
#include "otp.h"

//-----------------------------------------------------------
//        Output
//-----------------------------------------------------------

static void writes_(const char *s) { 
  if(s) console_write((char*)s); 
}

static int respond_(char* cmd, int status, const char* info)
{
  char b[10]; int bz = sizeof(b);
  
  //<<cmd # [info]
  writes_(RSP_PREFIX);
  writes_(cmd);
  writes_( snformat(b,bz," %i ",status) );
  writes_(info);
  writes_("\n");
  
  return status;
}

//-----------------------------------------------------------
//        OTP Region Prechecks
//-----------------------------------------------------------

enum {
  PRECHECK_INCOMPATIBLE   = -1,
  PRECHECK_OK             = 0,
  PRECHECK_WRITABLE       = 0, //OTP region may have data, but is overwritable by new stuff
  PRECHECK_EMPTY          = 1, //OTP region is empty (e.g. no write conflicts)
  PRECHECK_MATCH          = 2  //OTP region is exact data match (no write requried!)
};

static inline char* precheck_s(int retval) {
  switch(retval) {
    case PRECHECK_WRITABLE:   return "writable"; //break;
    case PRECHECK_EMPTY:      return "empty"; //break;
    case PRECHECK_MATCH:      return "match"; //break;
    
    default:
    case PRECHECK_INCOMPATIBLE: return "FAIL"; //break;
  }
}

//vanilla precheck data region for bit conflicts etc (do not use this for header)
int precheck(uint8_t* otp, const uint8_t* bin, int len)
{  
  //test for empty or matched data
  bool empty=1;
  for(int x=0; x<len; x++) {
    if( otp[x] > 0 ) {
      empty = 0;
      break;
    }
  }
  if( empty )
    return PRECHECK_EMPTY;
  
  //test for block match to bin data
  bool match=1;
  for(int x=0; x<len; x++) {
    if( otp[x] != bin[x] ) {
      match = 0;
      break;
    }
  }
  if( match )
    return PRECHECK_MATCH;
  
  //test if mismatch is over-writable
  //OTP fuses blow to 1. to be overwritable, all 1's in OTP must also be 1's in bin data
  bool writable = 1;
  for(int x=0; x<len; x++) {
    if( otp[x] != (otp[x] & bin[x]) ) {
      writable = 0;
      break;
    }
  }
  if( writable )
    return PRECHECK_WRITABLE;
  
  return PRECHECK_INCOMPATIBLE;
}

#define PRNT_START    1
#define PRNT_ERRS     2 //fields with precheck errs
#define PRNT_FIELDS   4 //all fields (even success)
#define PRNT_RESULT   8
//otp: ptr to header data fro otp, bin: ptr to header generated from binary image (to-be written)
int precheck_header(uint8_t* otp8, const uint8_t* bin8, int print_flags)
{
  static char b[60]; int bz = sizeof(b);
  uint32_t *otp = (uint32_t*)otp8, *bin = (uint32_t*)bin8;
  
  if( print_flags & PRNT_START )
    writes_("header precheck\n");
  
  int compat = PRECHECK_WRITABLE;
  for(int x=0; x < OTP_HEADER_SIZE>>2; x++)
  {
    //OTP fuses blow to 1. to be overwritable, all 1's in OTP must also be 1's in bin data
    bool conflict = otp[x] > 0 && bin[x] > 0; //header does not write bin=0 fields
    bool incompatible = conflict && (otp[x] != (otp[x] & bin[x])); //all 1's in OTP must also be 1's in bin data (bin may have additional 1s...)
    
    if( incompatible )
      compat = PRECHECK_INCOMPATIBLE;
    
    if( (incompatible && (print_flags & PRNT_ERRS)) || (print_flags & PRNT_FIELDS) )
      writes_( snformat(b,bz,"  precheck %02x otp,bin=%08x,%08x %s\n", x<<2, otp[x], bin[x], incompatible ? "INCOMPATIBLE" : "ok") );
  }
  
  if( print_flags & PRNT_RESULT )
    writes_( snformat(b,bz,"header precheck: %s\n", precheck_s(compat)) );
  
  return compat;;
}

//-----------------------------------------------------------
//        Commands
//-----------------------------------------------------------

uint32_t hex32str2ui(char *s)
{
  //compiler/target don't support ul!? strtol() max 0x7FFFffff value
  int len = strlen(s);
  uint32_t low = strtol(s+len-4,0,16); //last 4 chars
  
  char cbak = s[len-4];
  s[len-4] = '\0'; //temporarily null terminate
  uint32_t high = strtol(s,0,16);
  s[len-4] = cbak; //replace char
  
  return (high << 16) | low;
}

int cmd_process(char* s)
{
  static char b[80]; int bz = sizeof(b);
  
  if(!s)
    return STATUS_NULL;
  
  //is this a command?
  int len = strlen(s);
  if( !strncmp(s, CMD_PREFIX, sizeof(CMD_PREFIX)-1) && len > sizeof(CMD_PREFIX)-1 ) //valid cmd formatting
    s += sizeof(CMD_PREFIX)-1; //skip the prefix for parsing
  else
    return STATUS_NOT_A_CMD;
  
  //copy command arg into local buffer
  char cmd_buf[10], *cmd = console_getarg(s, 0, cmd_buf, sizeof(cmd_buf));
  
  //================
  //get version
  //>>getvers
  //<<getvers [status] [hw=version string]
  //================
  if( !strcmp(cmd, "getvers") ) {
    writes_(__DATE__ " " __TIME__ "\n");
    writes_("project = cubeotp\n");
    return respond_(cmd, STATUS_OK, snformat(b,bz,"hw=%s",HWVERS) );
  }//-*/
  
  //==========================
  //Console Echo
  //>>echo {off/on}
  /*/==========================
  if( !strcmp(cmd, "echo") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    char* arg1 = console_getargl(s,1);
    if( !strcmp(arg1, "off") )
      console_echo( g_echo_on=0 );
    else if( !strcmp(arg1, "on") )
      console_echo( g_echo_on=1 );
    else
      return respond_(cmd, STATUS_ARG_PARSE, "bad arg");
    
    return respond_(cmd, STATUS_OK, arg1);
  }//-*/
  
  //==========================
  //>>otp {write} {bd_address} {ESN:########} {hw.rev} {model} {CRC:########}
  //  example: 'otp write 01:02:03:04:05:C6 0x12345678 1 2'
  //==========================
  if( !strcmp(cmd, "otp") )
  {
    const int blocksize = 1024; //somewhat arbitrary chunking. save ram & provide nice status updates during long operations
    static uint8_t otp_buf[ blocksize ]; //MAX( blocksize, 2*sizeof(da14580_otp_header_t) ) ];
    static uint8_t otp_head_buf[ 2*sizeof(da14580_otp_header_t) ];
    int nargs = console_num_args(s);
    bdaddr_t bdaddr;
    
    writes_( snformat(b,bz,"cube bin: 0x%08x-0x%08x (%u)\n", g_CubeBoot, g_CubeBootEnd-1, g_CubeBootSize) );
    
    //read current OTP header
    //writes_("reading header from otp\n");
    da14580_otp_header_t* otphead = (da14580_otp_header_t*)&otp_head_buf[0];
    otp_read(OTP_ADDR_HEADER, sizeof(da14580_otp_header_t), (uint8_t*)otphead);
    
    //generate header from current application
    //writes_("generating application header\n");
    da14580_otp_header_t* binhead = (da14580_otp_header_t*)&otp_head_buf[sizeof(da14580_otp_header_t)];
    str2bdaddr( console_getargl(s,2) , &bdaddr); //try parse bd-address argument (fault clears to 0/invalid)
    otp_header_init( binhead, &bdaddr );
    binhead->jtag_enable = OTP_HEADER_JTAG_DISABLED; //Lock JTAG
    
//    if( nargs < 2 || (nargs > 3 /*&& !strcmp(console_getargl(s,3),"debug")*/) ) //debug
//    {
//      otp_print_header("Current OTP Header", otphead);
//      otp_print_header("Generated Header", binhead);
//      writes_( snformat(b,bz,"bdaddr: %s\n", nargs < 3 ? "(null)" : bdaddr2str(otp_header_get_bdaddr(binhead)) ) );
//      int precheck_head = precheck_header((uint8_t*)otphead, (uint8_t*)binhead, PRNT_START|PRNT_ERRS|PRNT_FIELDS|PRNT_RESULT );
//    }
//    else 
      if( !strcmp(console_getargl(s,1), "write") )
    {
      //Parse write params (ESN, hw.revision, model#, CRC) ---------------------------------
      
      if( nargs >= 7 ) {
        binhead->custom_field[0] = hex32str2ui(console_getargl(s,3)); //ESN
        binhead->custom_field[1] = strtol(console_getargl(s,4),0,0);  //hw rev
        binhead->custom_field[2] = strtol(console_getargl(s,5),0,0);  //model
        binhead->custom_field[3] = hex32str2ui(console_getargl(s,6)); //CRC
      }
      writes_( snformat(b,bz,"ESN,hwrev,model,CRC = %08x,%u,%u,%08x\n", binhead->custom_field[0], binhead->custom_field[1], binhead->custom_field[2], binhead->custom_field[3] ) );
      
      //validate parameters
      if( binhead->custom_field[0]/*ESN*/ == 0 ||
          binhead->custom_field[1]/*hwrev*/ == 0 || binhead->custom_field[1]/*hwrev*/ > 0xffff ||
          binhead->custom_field[2]/*model*/ == 0 || binhead->custom_field[2]/*model*/ > 0xffff)
      {
        return respond_(cmd, STATUS_ARG_PARSE, "bad-arg");
      }
      
      //Precheck ----------------------------------------------------------------------
      
      //Precheck header
      int precheck_head = precheck_header((uint8_t*)otphead, (uint8_t*)binhead, PRNT_RESULT);
      if( precheck_head < PRECHECK_OK ) //precheck again and log errors
        precheck_header((uint8_t*)otphead, (uint8_t*)binhead, /*PRNT_START|*/PRNT_ERRS|PRNT_RESULT);
      
      //Precheck application OTP
      writes_("precheck app:\n");
      int precheck_app = PRECHECK_WRITABLE;
      for(int addr=0; addr < g_CubeBootSize; addr += blocksize)
      {
        int oplen = MIN(blocksize, g_CubeBootSize - addr);
        writes_( snformat(b,bz,"  precheck %05x-%05x...", addr, addr+oplen-1 ) );
        
        otp_read(OTP_ADDR_BASE+addr, oplen, otp_buf); //read otp into our buffer
        int block_result = precheck(otp_buf, &g_CubeBoot[addr], oplen);
        
        writes_( snformat(b,bz,"%s\n", precheck_s(block_result) ) );
        if( block_result < PRECHECK_OK )
          precheck_app = PRECHECK_INCOMPATIBLE;
      }
      writes_( snformat(b,bz,"precheck app: %s\n", precheck_s(precheck_app)) );
      
      //gate on successful prechecks
      if( precheck_app < PRECHECK_OK || precheck_head < PRECHECK_OK )
        return respond_(cmd, STATUS_INCOMPATIBLE, "INCOMPATIBLE");
      
      //BLE Address ----------------------------------------------------------------
      
      if( nargs < 3 )
        return respond_(cmd, STATUS_ARG_NA, "missing-bdaddr");
      
      //log parsed bd address and check if it's valid per BT spec
      writes_( snformat(b,bz,"bdaddr %s\n", bdaddr2str(otp_header_get_bdaddr(binhead))) );
      if( !bdaddr_valid(&bdaddr) )
        return respond_(cmd, STATUS_ARG_PARSE, "bdaddr-invalid");
      
      //DEBUG: stop here
      //return respond_(cmd, STATUS_UNAUTHORIZED, "debug-halt");
      
      //Write ----------------------------------------------------------------------
      
      //let's burn this mother trucker!
      writes_("burning app image:\n");
      uint32_t Twrite = hal_timer_get_ticks();
      for(int addr=0; addr < g_CubeBootSize; addr += blocksize)
      {
        int oplen = MIN(blocksize, g_CubeBootSize - addr);
        writes_( snformat(b,bz,"  writing %05x-%05x...", addr, addr+oplen-1 ) );
        
        otp_read(OTP_ADDR_BASE+addr, oplen, otp_buf); //read otp into our buffer
        int block_result = precheck(otp_buf, &g_CubeBoot[addr], oplen);
        hal_timer_get_ticks(); //timer bug, must poll to handle 16-bit ovf
        
        if( block_result == PRECHECK_MATCH ) //no write requried
        {
          writes_("match\n");
        }
        else if( block_result != PRECHECK_INCOMPATIBLE ) //empty or overwritable
        {
          int wstat = otp_write((uint32_t*)(OTP_ADDR_BASE+addr), (uint32_t*)&g_CubeBoot[addr], oplen);
          if( wstat == OTP_WRITE_OK )
            writes_("done\n");
          else {
            writes_( snformat(b,bz,"failed e=%i\n", wstat) );
            return respond_(cmd, STATUS_WRITE_ERROR, 0); //bail!
          }
        }
        else //this shouldn't happen since we've already precheck'd...
        {
          writes_( snformat(b,bz,"precheck conflict %i %s\n", block_result, precheck_s(block_result) ) );
          return respond_(cmd, STATUS_WRITE_ERROR, 0);
        }
        hal_timer_get_ticks(); //timer bug, must poll to handle 16-bit ovf
      }
      writes_( snformat(b,bz,"write complete in %ums\n", hal_timer_elapsed_us(Twrite)/1000 ) );
      
      //burn the header
      writes_("burning otp header...");
      writes_( snformat(b,bz,"JTAG %s (0x%02X)\n", binhead->jtag_enable == OTP_HEADER_JTAG_DISABLED ? "disabled" : "UNLOCKED", binhead->jtag_enable) );
      uint32_t *src  = (uint32_t*)((int)binhead    + OTP_HEADER_SIZE - 4 );
      uint32_t *dest = (uint32_t*)(OTP_ADDR_HEADER + OTP_HEADER_SIZE - 4 );
      uint32_t *end  = (uint32_t*)(OTP_ADDR_HEADER);
      Twrite = hal_timer_get_ticks();
      int nwritten = 0, nmatched = 0, nskipped = 0;
      while( dest >= end ) //write words in reverse order - app flags last (app is invalid until app flags written)
      {
        hal_timer_get_ticks(); //timer bug, must poll to handle 16-bit ovf
        if( *src > 0 ) //ignore empty fields
        {
          if( *dest != *src ) //ignore data match
          {
            nwritten++;
            int res = otp_write(dest, src, sizeof(uint32_t));
            if( res != OTP_WRITE_OK ) {
              writes_( snformat(b,bz,"failed @ 0x%x otp_write().err=%i\n", dest, res) );
              return respond_(cmd, STATUS_WRITE_ERROR, 0);
            }
          }
          else
            nmatched++;
        }
        else
          nskipped++;
        
        src--, dest--;
      }
      writes_( snformat(b,bz,"ok in %ums\n", hal_timer_elapsed_us(Twrite)/1000 ) );
      writes_( snformat(b,bz,"dbg: wrote %u, matched %u, skipped %u (%u)\n", nwritten, nmatched, nskipped, nwritten + nmatched + nskipped) );
      
      //Verify ----------------------------------------------------------------------
      
      writes_("verifying otp app.");
      precheck_app = PRECHECK_INCOMPATIBLE;
      for(int addr=0; addr < g_CubeBootSize; addr += blocksize)
      {
        int oplen = MIN(blocksize, g_CubeBootSize - addr);
        otp_read(OTP_ADDR_BASE+addr, oplen, otp_buf); //read otp into our buffer
        precheck_app = precheck(otp_buf, &g_CubeBoot[addr], oplen);
        
        if( precheck_app == PRECHECK_MATCH ) {
          writes_(".");
        } else {
          writes_( snformat(b,bz,"[mismatch @ 0x%x]", addr) );
          break;
        }
      }
      writes_( precheck_app == PRECHECK_MATCH ? "ok\n" : "\n" );
      
      writes_("verifying otp header...");
      
      //genetically merge original+app headers into what should now (hopefully) exist in OTP
      for( int x=0; x<OTP_HEADER_SIZE; x++ )
        ((uint8_t*)binhead)[x] |= ((uint8_t*)otphead)[x];
      
      //read actual OTP and compare
      otp_read(OTP_ADDR_HEADER, OTP_HEADER_SIZE, (uint8_t*)otphead); //re-read current OTP into buffer
      precheck_head = precheck((uint8_t*)otphead, (uint8_t*)binhead, OTP_HEADER_SIZE);
      writes_( precheck_head == PRECHECK_MATCH ? "ok\n" : "FAIL\n");
      
      //gate on successful prechecks
      if( precheck_app != PRECHECK_MATCH || precheck_head != PRECHECK_MATCH )
        return respond_(cmd, STATUS_FAILED_VERIFY, "verify-failed");
      
      //success!!
      return respond_(cmd, STATUS_OK, 0);
    }
    return respond_(cmd, STATUS_OK, 0);
  }//-*/
  
  //==========================
  //DEBUG read OTP, compare to application bin
  //(console dump OTP mem for debug checks)
  /*/==========================
  if( !strcmp(cmd, "otpreadback") )
  {
    const int blocksize2 = 0x20;
    static uint8_t otp_buf2[ blocksize2 ];
    
    writes_("readback otp mem, compare to app bin\n");
    writes_( snformat(b,bz,"cube bin: 0x%08x-0x%08x (%u)\n", g_CubeBoot, g_CubeBootEnd-1, g_CubeBootSize) );
    
    //print header
    writes_("     OTP dat [0x040000]                                                cubebin\n");
    
    //for(int addr=0; addr < g_CubeBootSize; addr += blocksize2)
    for(int addr=0; addr < 0x8000; addr += blocksize2) //read entire OTP + header
    {
      int binlen = (g_CubeBootSize - addr) <= 0 ? 0 : MIN(blocksize2, g_CubeBootSize - addr);
      otp_read(OTP_ADDR_BASE+addr, blocksize2, otp_buf2); //read otp into our buffer
      
      //display
      writes_( snformat(b,bz,"%04X ", addr) );
      for(int x=0; x<blocksize2; x++)
        writes_( snformat(b,bz,"%02X", otp_buf2[x]) );
      
      if( binlen )
      {
        writes_("  ");
        
        for(int x=0; x<binlen; x++)
          writes_( snformat(b,bz,"%02X", g_CubeBoot[addr+x]) );
        
        int pc = precheck(otp_buf2, &g_CubeBoot[addr], binlen);
        if( pc != PRECHECK_MATCH && pc != PRECHECK_EMPTY )
          writes_(" -- MISMATCH");
      }
      
      writes_("\n");
    }
    
    return respond_(cmd, STATUS_OK, 0);
  }//-*/
  
  return respond_(cmd, STATUS_UNKNOWN_CMD, "unknown");
}

