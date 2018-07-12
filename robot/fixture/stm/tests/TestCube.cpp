#include <string.h>
#include <ctype.h>
#include "app.h"
#include "bdaddr.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "crc32.h"
#include "dut_uart.h"
#include "fixture.h"
#include "flexflow.h"
#include "hwid.h"
#include "meter.h"
#include "portable.h"
#include "random.h"
#include "swd.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

#define TESTCUBE_DEBUG 1
static const int CURRENT_CUBE_HW_REV = CUBEID_HWREV_MP;

//generate signature for the cube bootloader binary
uint32_t cubebootSignature(bool dbg_print, int *out_cubeboot_size)
{
  static bool crc_init = 0;
  static uint32_t crc;
  static int m_cubeboot_size = -1;
  
  if( !crc_init) //cache crc. hard-coded bins won't change at runtime, I pinky-swear
  {
    //OTP stub contains nested cubeboot.bin application. location/size info is reported at hard-coded addresses...
    const uint32_t cubeboot_magic = ((uint32_t*)g_CubeStub)[40];
    const uint32_t cubeboot_start = ((uint32_t*)g_CubeStub)[41] & 0x0FFFffff; //ignore dialog's OTP/Ram section placements
    const uint32_t cubeboot_end   = ((uint32_t*)g_CubeStub)[42] & 0x0FFFffff; //ignore dialog's OTP/Ram section placements
    const int cubeboot_size = cubeboot_end - cubeboot_start;
    const int cubestub_size = g_CubeStubEnd - g_CubeStub;
    
    //validate header
    bool bad_header = cubeboot_magic != 0xc0beb007 || cubeboot_size < 4000 || cubeboot_size >= cubestub_size || cubeboot_start >= cubestub_size || cubeboot_end >= cubestub_size;
    #if TESTCUBE_DEBUG > 0
    if( bad_header || dbg_print ) {
      ConsolePrintf("CubeStub[CubeBoot] header:\n");
      ConsolePrintf("  g_CubeStub: %08x-%08x (%i)\n", 0, cubestub_size-1, cubestub_size);
      ConsolePrintf("  g_CubeBoot: %08x-%08x (%i) magic %08x\n", cubeboot_start, cubeboot_end-1, cubeboot_size, cubeboot_magic);
    }
    #endif
    if( bad_header ) {
      #if TESTCUBE_DEBUG > 0
      ConsolePrintf("bad header\n");
      #endif
      throw ERROR_CUBE_BAD_BINARY;
    }
    
    //DEBUG: byte-for-byte compare to actual binary
    #if TESTCUBE_DEBUG > 0
    const int cubeboot_raw_size = g_CubeBootEnd-g_CubeBoot;
    if( cubeboot_raw_size > 0 ) //must be included by 'binaries.s'. Development check.
    {
      int mismatch_cnt = 0;
      ConsolePrintf("CubeStub[CubeBoot] binary compare to cubeboot.bin:\n");
      ConsolePrintf(".size: %u, %u(raw) %s\n", cubeboot_size, cubeboot_raw_size, cubeboot_size != cubeboot_raw_size ? "--MISMATCH--" : "" );
      
      for( int x=0; x < MIN(cubeboot_size,cubeboot_raw_size); x++ ) {
        if( g_CubeStub[cubeboot_start+x] != g_CubeBoot[x] ) {
          ConsolePrintf(".MISMATCH @ %08x: %02x,%02x\n", x, g_CubeStub[cubeboot_start+x], g_CubeBoot[x] );
          if( ++mismatch_cnt > 20 )
            break;
        }
      }
      if( cubeboot_size != cubeboot_raw_size || mismatch_cnt > 0 )
        throw ERROR_CUBE_BAD_BINARY; //return 0;
      
      ConsolePrintf(".OK\n");
    }
    #endif
    
    m_cubeboot_size = cubeboot_size; //cache size after successful checks
    crc = crc32(0xFFFFffff, g_CubeStub+cubeboot_start, cubeboot_size );
    crc_init = 1;
  }
  
  if( out_cubeboot_size )
    *out_cubeboot_size = m_cubeboot_size;
  if( dbg_print )
    ConsolePrintf("cubeboot crc32=%08x\n", crc);
  return crc;
}

//-----------------------------------------------------------------------------
//                  Dialog Load
//-----------------------------------------------------------------------------

//Dialog DA14580, AN-B-001, Table 5: UART boot protocol
//  Byte nr.  DA1458x UTX     DA1458x URX
//  0         STX = 0x02
//  1         .               SOH = 0x01
//  2         .               LEN_LSB
//  3         .               LEN_MSB
//  4         ACK = 0x06 or
//            NACK = 0x15
//  5 to N    .               SW code bytes
//  N+1       CRC (XOR over the SW code)
//  N+2       .               ACK = 0x06

//load a pgm into ram and run it [@ 0x20000000]
static void da14580_load_program_(const uint8_t *bin, int size, const char* name)
{
  ConsolePrintf("da14580 load %s: %ikB (%u)\n", (name ? name : "program"), CEILDIV(size,1024), size );
  TestCubeCleanup();
  Board::powerOn(PWR_CUBEBAT, 0);
  DUT_RESET::write(1); //hold in reset
  DUT_RESET::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  
  //wait for cube Vcc to stabilize
  ConsolePrintf("VBAT3V=");
  uint32_t Tstart = Timer::get(), Tupdate = 0;
  int vbat3v_mv, vbat3v_last = 0;
  while(1)
  {
    vbat3v_mv = Meter::getVoltageMv(PWR_DUTVDD,4);
    if( ABS(vbat3v_mv-vbat3v_last) > 25 && Timer::elapsedUs(Tupdate) > 50*1000 ) //detect changes, rate limited
    {
      vbat3v_last = vbat3v_mv;
      Tupdate = Timer::get();
      ConsolePrintf("%u,", vbat3v_mv);
    }
    if( Timer::elapsedUs(Tupdate) > 250*1000 || Timer::elapsedUs(Tstart) > 3000*1000 ) //stable or timeout
      break;
  }
  ConsolePrintf("%u\n", vbat3v_mv); //print final value
  
  if( vbat3v_mv < 2500 )
    throw ERROR_CUBE_BAD_POWER; //ERROR_BAT_UNDERVOLT;
  if( vbat3v_mv > 2900 )
    throw ERROR_BAT_OVERVOLT; //ERROR_CUBE_BAD_POWER
  
  //Sync bootloader
  int ack = -1;
  ConsolePrintf("waiting for bootloader\n");
  DUT_UART::init( 57600 ); //ROM bootloader baud 57.6k
  DUT_RESET::write(1); //hold in reset
  DUT_RESET::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  Timer::delayMs(100);
  DUT_RESET::write(0); //release from reset
  Tstart = Timer::get();
  while(1)
  {
    int c = DUT_UART::getchar();
    
    if( c == 0x02 ) //Dialog bootloader
    {
      //Send boot header (quick! need to engage bootloader. take time to print after...)
      DUT_UART::putchar( 0x01 );
      DUT_UART::putchar( (size >> 0) & 0xFF );
      DUT_UART::putchar( (size >> 8) & 0xFF );
      ack = DUT_UART::getchar(100*1000); //Read ACK
      ConsolePrintf("send boot header: ack = 0x%02x\n", ack);
      
      if( ack != 0x06 ) //Note: 0x15 == NACK
        throw ERROR_CUBE_CANNOT_WRITE; //bootloader rejected length, or protocol error etc.
      
      //send application image
      int crc_local = 0;
      ConsolePrintf("send %u bytes ", size);
      for(int n=0; n < size; n++)
      {
        DUT_UART::putchar( bin[n] );
        crc_local ^= bin[n]; //generate expected crc
        if( (n & 0x1FF) == 0 ) //%512 == 0
          ConsolePutChar('.');
      }
      ConsolePrintf("done\n");
      
      //verify CRC
      int crc = DUT_UART::getchar(100*1000);
      ConsolePrintf("CRC: 0x%02x == 0x%02x\n", crc, crc_local);
      if( crc != crc_local )
        throw ERROR_CUBE_VERIFY_FAILED;
      
      //send ACK to run the image
      DUT_UART::putchar(0x06); //ACK
      Timer::wait(1000); //make sure ack clears the output SR before disabling UART
      //ConsolePrintf("pgm time: %ums\n", Timer::elapsedUs(Tstart)/1000); //XXX: this doesn't work. Timer isn't updated if not polling...
      
      break;
    }
    
    if( c == 0xB2 ) //sync char from programmed cube
    {
      ConsolePrintf("found cube sync char\n");
      
      char buf[75]; int len = 0;
      char *rsp = DUT_UART::getline(buf, sizeof(buf), 50*1000, &len); //discards non-ascii (repeat sync chars)
      if( len > 0 )
      {
        ConsolePrintf("rx '%s'\n", buf); //includes incomplete lines...
        int nargs = !rsp ? 0 : cmdNumArgs(rsp);
        
        if( rsp && nargs >= 6 && !strcmp(cmdGetArg(rsp,0), "cubeboot") ) //cube POR boot message
        {
          bdaddr_t bdaddr_, *bdaddr = str2bdaddr(cmdGetArg(rsp,1), &bdaddr_);
          struct { uint32_t esn; uint32_t hwrev; uint32_t model; uint32_t crc; } info;
          for(int i=0; i<4; i++)
            ((uint32_t*)&info)[i] = cmdParseHex32( cmdGetArg(rsp,2+i) );
          
          ConsolePrintf("Cube Found: esn=%08x hwrev=%u model=%u crc=%08x\n", info.esn, info.hwrev, info.model, info.crc);
          ConsolePrintf("Cube Addr : %s\n", bdaddr2str(bdaddr));
          
          uint32_t crc_current = cubebootSignature();
          uint32_t expected_model = g_fixmode == FIXMODE_CUBE2 ? CUBEID_MODEL_CUBE2 : CUBEID_MODEL_CUBE1;
          if( info.hwrev != CURRENT_CUBE_HW_REV || info.model != expected_model || info.crc != crc_current ) {
            if( info.crc != crc_current )
              ConsolePrintf("CRC mismatch != %08x\n", crc_current);
            throw ERROR_CUBE_FW_MISMATCH;
          }
          
          //running fw matches! ...but still an error because we can't test
          throw ERROR_CUBE_FW_MATCH; //already programmed
          //break;
        }
      }
    }
    
    if( Timer::elapsedUs(Tstart) > 500*1000 ) //timeout waiting for bootloader start char
      throw ERROR_CUBE_NO_COMMUNICATION;
  }
  
  DUT_UART::deinit();
  //DUT_RESET::init(MODE_INPUT, PULL_NONE);
  
  //leave the lights on to run some tests
}

//-----------------------------------------------------------------------------
//                  Debug
//-----------------------------------------------------------------------------

void DbgPowerWaitTest(void)
{
  ConsolePrintf("power on and wait (debug)\n");
  const int ima_limit_CUBEBAT = 450;
  TestCommon::powerOnProtected(PWR_CUBEBAT, 100, ima_limit_CUBEBAT );
  //leave power on
  
  //DEBUG: console bridge, manual testing
  cmdSend(CMD_IO_DUT_UART, "echo on", CMD_DEFAULT_TIMEOUT, (CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS) & ~CMD_OPTS_EXCEPTION_EN );
  TestCommon::consoleBridge(TO_DUT_UART,10*1000);
}//-*/

void DbgCubeIMeasLoop(char *title)
{
  DUT_UART::deinit();
  
  int icube = -1, cnt = 0;
  while( ConsoleReadChar() > -1 ) {}
  if(title)
    ConsolePrintf("%s:\n", title);
  
  while(1)
  {
    int i = Meter::getCurrentMa(PWR_CUBEBAT,8);
    if( icube != i ) {
      icube = i;
      ConsolePrintf("%03i,", icube);
      if(++cnt >= 10) {
        cnt = 0;
        ConsolePrintf("\n");
      }
    }
    
    if( ConsoleReadChar() > -1 ) {
      ConsolePrintf("\n");
      break;
    }
  }
  
  DUT_UART::init(57600);
}

//-----------------------------------------------------------------------------
//                  Cube Tests
//-----------------------------------------------------------------------------

static cubeid_t cubeid;

bool TestCubeDetect(void)
{
  memset( &cubeid, 0, sizeof(cubeid) );
  
  // Make sure power is not applied, as it messes up the detection code below
  TestCubeCleanup();
  
  //weakly pulled-up - it will detect as grounded when the board is attached
  DUT_SWC::init(MODE_INPUT, PULL_UP);
  Timer::wait(100);
  bool detected = !DUT_SWC::read(); //true if pin is pulled down by the board
  DUT_SWC::init(MODE_INPUT, PULL_NONE); //prevent pu from doing strange things to mcu (phantom power?)
  
  return detected;
}

void TestCubeCleanup(void)
{
  DUT_VDD::init(MODE_INPUT, PULL_NONE);
  DUT_UART::deinit();
  DUT_RESET::init(MODE_INPUT, PULL_NONE);
  Board::powerOff(PWR_CUBEBAT,100);
  Board::powerOff(PWR_DUTPROG);
}

static void ShortCircuitTest(void)
{
  ConsolePrintf("short circuit tests\n");
  const int ima_limit_CUBEBAT = 450;
  Board::powerOff(PWR_CUBEBAT);
  TestCommon::powerOnProtected(PWR_CUBEBAT, 100, ima_limit_CUBEBAT );
  Board::powerOff(PWR_CUBEBAT,100);
}

//led test array
typedef struct { char* name; uint16_t bits; int duty; int i_meas; int i_nominal; int i_variance; error_t e; } led_test_t;
led_test_t ledtest[] = {
  {(char*)"All.RED", 0x1111, 12, 0, 10,  4, ERROR_CUBE_LED    },
  {(char*)"All.GRN", 0x2222, 12, 0, 10,  4, ERROR_CUBE_LED    },
  {(char*)"All.BLU", 0x4444, 12, 0,  9,  4, ERROR_CUBE_LED    },
  {(char*)"D1.RED",  0x0001,  1, 0, 27,  5, ERROR_CUBE_LED_D1 },
  {(char*)"D1.GRN",  0x0002,  1, 0, 28,  8, ERROR_CUBE_LED_D1 },
  {(char*)"D1.BLU",  0x0004,  1, 0, 28,  8, ERROR_CUBE_LED_D1 },
  {(char*)"D2.RED",  0x0010,  1, 0, 27,  5, ERROR_CUBE_LED_D2 },
  {(char*)"D2.GRN",  0x0020,  1, 0, 28,  8, ERROR_CUBE_LED_D2 },
  {(char*)"D2.BLU",  0x0040,  1, 0, 28,  8, ERROR_CUBE_LED_D2 },
  {(char*)"D3.RED",  0x0100,  1, 0, 27,  5, ERROR_CUBE_LED_D3 },
  {(char*)"D3.GRN",  0x0200,  1, 0, 28,  8, ERROR_CUBE_LED_D3 },
  {(char*)"D3.BLU",  0x0400,  1, 0, 28,  8, ERROR_CUBE_LED_D3 },
  {(char*)"D4.RED",  0x1000,  1, 0, 27,  5, ERROR_CUBE_LED_D4 },
  {(char*)"D4.GRN",  0x2000,  1, 0, 28,  8, ERROR_CUBE_LED_D4 },
  {(char*)"D4.BLU",  0x4000,  1, 0, 28,  8, ERROR_CUBE_LED_D4 }
};

static inline bool ledtest_pass(led_test_t *ptest, int i_test) {
  return ( i_test >= ptest->i_nominal - ptest->i_variance && 
           i_test <= ptest->i_nominal + ptest->i_variance );
}

static void CubeTest(void)
{
  char b[20]; int bz = sizeof(b);
  
  da14580_load_program_( g_CubeTest, g_CubeTestEnd-g_CubeTest, "cube test" );
  Timer::delayMs(250); //wait for ROM+app reboot sequence
  DUT_UART::init(57600);
  
  //DEBUG:
  if( g_fixmode <= FIXMODE_CUBE0 ) {
    TestCommon::consoleBridge(TO_DUT_UART,3000);
  }
  
  cmdSend(CMD_IO_DUT_UART, "getvers");
  cmdSend(CMD_IO_DUT_UART, "vbat");
  
  //measure LED current draws
  cmdSend(CMD_IO_DUT_UART, "vled 1");
  Timer::delayMs(50); //wait for VLED boost reg to stabilize
  int i_base = Meter::getCurrentMa(PWR_CUBEBAT,10);
  
  //DbgCubeIMeasLoop((char*)"base current");
  
  for(int n=0; n < sizeof(ledtest)/sizeof(led_test_t); n++)
  {
    cmdSend(CMD_IO_DUT_UART, snformat(b,bz,"leds 0x%x %s", ledtest[n].bits, (ledtest[n].duty <= 1 ? "static" : "") ));
    Timer::delayMs(20);
    
    int oversample = 5;
    do {
      ledtest[n].i_meas = Meter::getCurrentMa(PWR_CUBEBAT, oversample++);
    } while( oversample <= 8 && !ledtest_pass( &ledtest[n], ledtest[n].i_meas - i_base ) );
    
    //DbgCubeIMeasLoop( snformat(b,bz,"led %i current", n) );
  }
  cmdSend(CMD_IO_DUT_UART, "leds 0");
  cmdSend(CMD_IO_DUT_UART, "vled 0");
  
  //print/verify results
  int e = ERROR_OK;
  ConsolePrintf("base current %imA\n", i_base);
  for(int n=0; n < sizeof(ledtest)/sizeof(led_test_t); n++)
  {
    int i_test = ledtest[n].i_meas - i_base;
    bool pass = ledtest_pass( &ledtest[n], i_test );
    ConsolePrintf("%s current %imA %s\n", ledtest[n].name, i_test, pass ? "ok" : "--FAIL--");
    if( !pass )
      e = ledtest[n].e;
  }
  
  //DEBUG
  if( e != ERROR_OK && g_fixmode <= FIXMODE_CUBE0 ) {
    ConsolePrintf("CUBE LED ERROR: %i -- press a key to approve\n", e);
    while( ConsoleReadChar() > -1 );
    uint32_t Tstart = Timer::get();
    while( Timer::elapsedUs(Tstart) < 2*1000*1000 ) {
      if( ConsoleReadChar() > -1 ) { e = ERROR_OK; break; }
    }
  }//-*/
  
  if( e != ERROR_OK )
      throw e;
  
  //Accelerometer (ref: status error codes in cubetest::cmd.h)
  //cmdSend(CMD_IO_DUT_UART, "leds 0x1 static"); //pull some led current for typical usecase
  //cmdSend(CMD_IO_DUT_UART, "vled 1");
  Timer::delayMs(50); //wait for VLED boost reg to stabilize
  cmdSend(CMD_IO_DUT_UART, "accel", 250, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS);
  if( cmdStatus() == 10 )
    throw ERROR_CUBE_ACCEL_PWR;
  if( cmdStatus() != 0 ) // == 11
    throw ERROR_CUBE_ACCEL;
  //cmdSend(CMD_IO_DUT_UART, "vled 0");
}

static void OTPbootloader(void)
{
  char b[70]; int bz = sizeof(b);
  
  da14580_load_program_( g_CubeStub, g_CubeStubEnd-g_CubeStub, "cube otp" );
  Timer::delayMs(250); //wait for ROM+app reboot sequence
  DUT_UART::init(57600);
  
  //DEBUG:
  if( g_fixmode <= FIXMODE_CUBE0 ) {
    TestCommon::consoleBridge(TO_DUT_UART,2000);
  }
  
  cmdSend(CMD_IO_DUT_UART, "getvers");
  
  //Generate bd address
  bdaddr_t bdaddr;
  bdaddr_generate(&bdaddr, GetRandom ); //use RNG peripheral for proper randomness
  
  //prepare hwardware ids
  cubeid.esn = CUBEID_ESN_INVALID;
  cubeid.hwrev = CURRENT_CUBE_HW_REV;
  cubeid.model = (g_fixmode == FIXMODE_CUBE1) ? CUBEID_MODEL_CUBE1 : ((g_fixmode == FIXMODE_CUBE2) ? CUBEID_MODEL_CUBE2 : CUBEID_MODEL_INVALID);
  
  //pull a new s/n for valid modes only (allow debug on all fixtures)
  if( g_fixmode == FIXMODE_CUBE1 || g_fixmode == FIXMODE_CUBE2 )
    cubeid.esn = fixtureGetSerial(); //get next 12.20 esn in the sequence
  
  uint32_t crc = cubebootSignature();
  
  //XXX: how long should this take?
  int write_result = -1;
  Board::powerOn(PWR_DUTPROG);
  {
    char *cmd = snformat(b,bz,"otp write %s %08x %u %u %08x", bdaddr2str(&bdaddr), cubeid.esn, cubeid.hwrev, cubeid.model, crc);
    
    //only burn OTP for valid cube types
    if( g_fixmode == FIXMODE_CUBE1 || g_fixmode == FIXMODE_CUBE2 ) {
      cmdSend(CMD_IO_DUT_UART, cmd, 60*1000, (CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS) & ~CMD_OPTS_EXCEPTION_EN );
      write_result = cmdStatus();
    } else {
      ConsolePrintf("skipping OTP write:\n");
      ConsolePrintf("XXX: %s\n", b);
      write_result = 0;
      //throw ERROR_UNKNOWN_MODE;
      //return;
    }
  }
  Board::powerOff(PWR_DUTPROG);
  
  if( write_result != 0 )
    throw ERROR_CUBE_VERIFY_FAILED;
}

static void burnFCC(void)
{
  da14580_load_program_( g_CubeStubFcc, g_CubeStubFccEnd-g_CubeStubFcc, "cube fcc" );
  Timer::delayMs(250); //wait for ROM+app reboot sequence
  DUT_UART::init(57600);
  
  //XXX: how long should this take?
  int write_result = -1;
  Board::powerOn(PWR_DUTPROG);
  {
    cmdSend(CMD_IO_DUT_UART, "otp write fcc", 60*1000, (CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS) & ~CMD_OPTS_EXCEPTION_EN );
    write_result = cmdStatus();
  }
  Board::powerOff(PWR_DUTPROG);
  
  if( write_result != 0 )
    throw ERROR_CUBE_VERIFY_FAILED;
}

void CubeBootDebug(void)
{
  da14580_load_program_( g_CubeBoot, g_CubeBootEnd-g_CubeBoot, "cube boot" );
  Timer::delayMs(250); //wait for ROM+app reboot sequence
  //DUT_UART::init(57600);
  TestCommon::consoleBridge(TO_DUT_UART,5000);
}

static void CubeFlexFlowReport(void)
{
  int bootSize;
  uint32_t crc = cubebootSignature(0,&bootSize);
  FLEXFLOW::printf("<flex> ESN %08x HWRev %u Model %u </flex>\n", cubeid.esn, cubeid.hwrev, cubeid.model);
  FLEXFLOW::printf("<flex> BootSize %i CRC %08x </flex>\n", bootSize, crc);
}

TestFunction* TestCubeFccGetTests(void) {
  static TestFunction m_tests[] = {
    ShortCircuitTest,
    CubeTest,
    burnFCC,
    NULL,
  };
  return m_tests;
}

TestFunction* TestCube0GetTests(void) {
  static TestFunction m_tests[] = {
    ShortCircuitTest,
    CubeTest,
    OTPbootloader,
    //CubeBootDebug,
    NULL,
  };
  return m_tests;
}

TestFunction* TestCube1GetTests(void) {
  static TestFunction m_tests[] = {
    ShortCircuitTest,
    CubeTest,
    OTPbootloader,
    CubeFlexFlowReport,
    NULL,
  };
  return m_tests;
}

TestFunction* TestCube2GetTests(void) {
  //CUBE2 needs to be same as CUBE1. cube id changes based on g_fixmode (1->2)
  return TestCube1GetTests();
}

TestFunction* TestCubeOLGetTests(void) {
  return TestCube1GetTests(); //OTP write disabled in OL mode
}

//-----------------------------------------------------------------------------
//                  Block Tests
//-----------------------------------------------------------------------------

bool TestBlockDetect(void)
{
  return false;
}

void TestBlockCleanup(void)
{
}

TestFunction* TestBlock1GetTests(void)
{
  static TestFunction m_tests[] = {
    NULL,
  };
  return m_tests;
}

TestFunction* TestBlock2GetTests(void) { 
  return TestBlock1GetTests();
}

