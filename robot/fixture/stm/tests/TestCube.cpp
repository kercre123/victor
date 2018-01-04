#include <string.h>
#include "app.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "dut_uart.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "swd.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

#define CUBE_CMD_OPTS   (CMD_OPTS_DEFAULT)
//#define CUBE_CMD_OPTS   (CMD_OPTS_LOG_ERRORS | CMD_OPTS_REQUIRE_STATUS_CODE) /*disable exceptions*/

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
  
  //power up and hold in reset
  Board::enableVBAT();
  DUT_RESET::write(1);
  DUT_RESET::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  
  //wait for cube Vcc to stabilize
  ConsolePrintf("VBAT3V=");
  uint32_t Tstart = Timer::get(), Tupdate = 0;
  int vbat3v_mv, vbat3v_last = 0;
  while(1)
  {
    vbat3v_mv = Board::getAdcMv(ADC_DUT_VDD_IN) << 1; //this input has a resistor divider
    if( ABS(vbat3v_mv-vbat3v_last) > 50 && Timer::elapsedUs(Tupdate) > 50*1000 ) //detect changes, rate limited
    {
      vbat3v_last = vbat3v_mv;
      Tupdate = Timer::get();
      ConsolePrintf("%u,", vbat3v_mv);
    }
    if( Timer::elapsedUs(Tupdate) > 100*1000 || Timer::elapsedUs(Tstart) > 1000*1000 ) //stable or timeout
      break;
  }
  ConsolePrintf("%u\n", vbat3v_mv); //print final value
  
  if( vbat3v_mv < 2500 )
    throw ERROR_BAT_UNDERVOLT;
  if( vbat3v_mv > 2900 )
    throw ERROR_BAT_OVERVOLT;
  
  //Sync bootloader
  int ack = -1;
  ConsolePrintf("waiting for bootloader\n");
  DUT_UART::init( 57600 ); //ROM bootloader baud 57.6k
  DUT_RESET::write(0); //release from reset
  Tstart = Timer::get();
  while(1)
  {
    if( DUT_UART::getchar() == 0x02 )
    {
      //Send boot header (quick! need to engage bootloader. take time to print after...)
      DUT_UART::putchar( 0x01 );
      DUT_UART::putchar( (size >> 0) & 0xFF );
      DUT_UART::putchar( (size >> 8) & 0xFF );
      ack = DUT_UART::getchar(100*1000); //Read ACK
      ConsolePrintf("send boot header: ack = 0x%02x\n", ack);
      
      if( ack != 0x06 ) //Note: 0x15 == NACK
        throw ERROR_CUBE_CANNOT_WRITE; //bootloader rejected length, or protocol error etc.
      break;
    }
    
    if( Timer::elapsedUs(Tstart) > 500*1000 ) //timeout waiting for bootloader start char
      throw ERROR_CUBE_NO_COMMUNICATION;
  }
  
  //send application image
  int crc_local = 0;
  ConsolePrintf("send %u bytes ", size);
  for(int n=0; n < size; n++)
  {
    DUT_UART::putchar( bin[n] );
    crc_local ^= bin[n]; //generate expected crc
    if( (n & 0x1FF) == 0 ) //%512 == 0
      ConsolePrintf(".");
  }
  ConsolePrintf("done\n");
  
  //verify CRC
  int crc = DUT_UART::getchar(100*1000);
  ConsolePrintf("CRC: 0x%02x == 0x%02x\n", crc, crc_local);
  if( crc != crc_local )
    throw ERROR_CUBE_VERIFY_FAILED;
  
  //send ACK to run the image
  DUT_UART::putchar(0x06); //ACK
  //ConsolePrintf("pgm time: %ums\n", Timer::elapsedUs(Tstart)/1000); //XXX: this doesn't work. Timer isn't updated if not polling...
  
  Timer::wait(1000); //make sure ack clears the output SR before disabling UART
  DUT_UART::deinit();
  DUT_RESET::init(MODE_INPUT, PULL_NONE);
  
  //leave the lights on to run some tests
}

//-----------------------------------------------------------------------------
//                  Cube Tests
//-----------------------------------------------------------------------------

bool TestCubeDetect(void)
{
  // Make sure power is not applied, as it messes up the detection code below
  TestCubeCleanup();
  
  //weakly pulled-up - it will detect as grounded when the board is attached
  DUT_RX::init(MODE_INPUT, PULL_UP);
  Timer::wait(100);
  bool detected = !DUT_RX::read(); //true if pin is pulled down by the board
  DUT_RX::init(MODE_INPUT, PULL_NONE); //prevent pu from doing strange things to mcu (phantom power?)
  
  return detected;
}

void TestCubeCleanup(void)
{
  Board::disableVBAT();
  Board::disableDUTPROG();
  DUT_VDD::init(MODE_INPUT, PULL_NONE);
  DUT_UART::deinit();
  DUT_RESET::init(MODE_INPUT, PULL_NONE);
}

static void ShortCircuitTest(void)
{
  ConsolePrintf("short circuit tests\n");
  const int ima_limit_VBAT = 450;
  TestCommon::powerShortVBAT( 100, ima_limit_VBAT );
  Board::disableVBAT();
}

static void CubeTest(void)
{
  char b[20]; int bz = sizeof(b);
  
  da14580_load_program_( g_CubeTest, g_CubeTestEnd-g_CubeTest, "cube test" );
  Timer::delayMs(250); //wait for ROM+app reboot sequence
  DUT_UART::init(57600);
  
  cmdSend(CMD_IO_DUT_UART, "getvers", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  cmdSend(CMD_IO_DUT_UART, "delay 500", 600, CUBE_CMD_OPTS);
  cmdSend(CMD_IO_DUT_UART, "vbat", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  cmdSend(CMD_IO_DUT_UART, "vled 1", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS); //turns on VLED reg
  
  uint32_t start = Timer::get();
  uint16_t leds[] = {0x249/*RED*/, 0x492/*GRN*/, 0x924/*BLU*/};
  while( Timer::elapsedUs(start) < 1500*1000 ) {
    for(int n=0; n<sizeof(leds)/sizeof(uint16_t); n++) {
      cmdSend(CMD_IO_DUT_UART, snformat(b,bz,"leds 0x%x", leds[n]), CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
      Timer::delayMs(500);
    }
  }
  cmdSend(CMD_IO_DUT_UART, "leds 0", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  cmdSend(CMD_IO_DUT_UART, "vled 0", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  
  //DEBUG: "test" accel
  cmdSend(CMD_IO_DUT_UART, "accel", 250, CUBE_CMD_OPTS);
  
  //DEBUG: console bridge, manual testing
  cmdSend(CMD_IO_DUT_UART, "echo on", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  TestCommon::consoleBridge(TO_DUT_UART,2000);
  //-*/
}

static void OTPbootloader(void)
{
  //XXX: is this getting rolled into cubetest?
  da14580_load_program_( g_CubeStub, g_CubeStubEnd-g_CubeStub, "cube otp" );
  Timer::delayMs(250); //wait for ROM+app reboot sequence
  DUT_UART::init(57600);
  
  cmdSend(CMD_IO_DUT_UART, "getvers", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  
  //DEBUG: console bridge, manual testing
  cmdSend(CMD_IO_DUT_UART, "echo on", CMD_DEFAULT_TIMEOUT, CUBE_CMD_OPTS);
  Board::enableDUTPROG();
  TestCommon::consoleBridge(TO_DUT_UART,2000);
  Board::disableDUTPROG();
  //-*/
}

TestFunction* TestCube1GetTests(void)
{
  static TestFunction m_tests[] = {
    ShortCircuitTest,
    CubeTest,
    OTPbootloader,
    NULL,
  };
  return m_tests;
}

TestFunction* TestCube2GetTests(void)
{
  static TestFunction m_tests[] = {
    NULL
  };
  return m_tests;
}

bool TestCubeFinishDetect(void)
{
  return false;
}

TestFunction* TestCubeFinish1GetTests(void)
{
  static TestFunction m_tests[] = {
    NULL
  };
  return m_tests;
}

TestFunction* TestCubeFinish2GetTests(void)
{
  static TestFunction m_tests[] = {
    NULL
  };
  return m_tests;
}

TestFunction* TestCubeFinish3GetTests(void)
{
  static TestFunction m_tests[] = {
    NULL
  };
  return m_tests;
}

TestFunction* TestCubeFinishXGetTests(void)
{
  static TestFunction m_tests[] = {
    NULL
  };
  return m_tests;
}

