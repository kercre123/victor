#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "board.h"
#include "console.h"
#include "fixture.h"
#include "flash.h"
#include "meter.h"
#include "nvReset.h"
#include "portable.h"
#include "random.h"
#include "tests.h"
#include "timer.h"

//global fixture data
int g_fixmode = FIXMODE_NONE;
const fixmode_info_t g_fixmode_info[] = { FIXMODE_INFO_INIT_DATA };
const int g_num_fixmodes = sizeof(g_fixmode_info) / sizeof(fixmode_info_t);
bool g_allowOutdated = false;

//initialize
void fixtureInit(void) {
  //ConsolePrintf("fixtureInit()\n");
}

//safe method to get fixture name. default string if null, or invalid fixmode.
const char* fixtureName(void) {
  const char* name = g_fixmode >= 0 && g_fixmode < g_num_fixmodes ? g_fixmode_info[g_fixmode].name : NULL;
  return name ? name : "NA";
}

//safe method to run detect on current fixmode. default not-detected.
bool fixtureDetect(int min_delay_us)
{
  uint32_t Tstart = Timer::get();
  
  bool(*Detect)(void) = g_fixmode >= 0 && g_fixmode < g_num_fixmodes ? g_fixmode_info[g_fixmode].Detect : NULL;
  bool detected = Detect ? Detect() : false;
  
  while( Timer::elapsedUs(Tstart) < min_delay_us);
  return detected;
}

//safe method to run cleanup on current fixmode.
void fixtureCleanup(void) {
  void(*Cleanup)(void) = g_fixmode >= 0 && g_fixmode < g_num_fixmodes ? g_fixmode_info[g_fixmode].Cleanup : NULL;
  if( Cleanup != NULL )
    Cleanup();
}

//safe method to get current fixmode tests. Guarantee non-null return.
TestFunction* fixtureGetTests(void) {
  static TestFunction m_tests[] = { NULL };
  TestFunction*(*GetTests)(void) = g_fixmode >= 0 && g_fixmode < g_num_fixmodes ? g_fixmode_info[g_fixmode].GetTests : NULL; //check array boundaries
  TestFunction* tests = GetTests != NULL ? GetTests() : NULL;
  return tests ? tests : m_tests; //guarantee non-null return
}

//safe method to get test count. null checks etc.
int fixtureGetTestCount(void) {
  TestFunction* fn = fixtureGetTests();
  int count = 0;
  while (fn && *fn++)
    count++;
  return count;
}

//-----------------------------------------------------------------------------
//                  Debug
//-----------------------------------------------------------------------------

//#define FIXTURE_VALIDATE_PRINT  1

//development tool: validate the fixmode_info array contents - hopefully catch some stupid mistakes
bool fixtureValidateFixmodeInfo(bool print)
{
  bool valid = true;
  
  for(int n=0; n < g_num_fixmodes; n++ )
  {
    bool v = true;
    const fixmode_info_t *fixnfo = &g_fixmode_info[n];
    if( fixnfo->name != NULL && fixnfo->mode != n ) //verify array index matches 'mode' field
      v = false;
    if( fixnfo->name == NULL && (fixnfo->Detect != NULL || fixnfo->GetTests != NULL || fixnfo->mode > -1) ) //validate empty entries
      v = false;
    
    //#if FIXTURE_VALIDATE_PRINT > 0
    if( print )
      ConsolePrintf("  %02d [%02i] %-10s %8x %8x %s\n", n, fixnfo->mode, (fixnfo->name ? fixnfo->name : "-"), fixnfo->Detect, fixnfo->GetTests, (v ? "" : "<--INVALID") );
    //#endif
    
    valid = valid && v;
  }
  
  return valid;
}

//-----------------------------------------------------------------------------
//                  Non-Volatile Serial # Management
//-----------------------------------------------------------------------------

//get the next # in a power-safe, non-repeating sequence from 0-0x7ffff
static int GetSequence(bool allocate_permanently)
{
  u32 sequence;
  u8 bit;
  
  {
    sequence = 0;
    u8* serialbase = (u8*)FLASH_SERIAL_BITS;
    while (serialbase[(sequence >> 3)] == 0)
    {
      sequence += 8;
      if (sequence > 0x7ffff)
      {
        ConsolePrintf("fixtureSequence,-1\n");
        ConsolePrintf("ERROR_OUT_OF_SERIALS\n");
        if( !allocate_permanently )
          return -1;
        else
          throw ERROR_OUT_OF_SERIALS;
      }
    }
    
    u8 bitMask = serialbase[(sequence >> 3)];
    
    // Find which bit we're on
    bit = 0;
    while (!(bitMask & (1 << bit)))
    {
      bit++;
    }
    sequence += bit;
  }
  
  if( allocate_permanently )
  {
    // Reserve this test sequence
    FLASH_Unlock();
    FLASH_ProgramByte(FLASH_SERIAL_BITS + (sequence >> 3), ~(1 << bit));
    FLASH_Lock();
    
    ConsolePrintf("fixtureSequence,%i,%i\n", FIXTURE_SERIAL, sequence);
    //SlowPrintf("Allocated serial: %x\n", sequence);
  }
  
  return sequence;
}

// Get a serial number for a device in the normal 12.20 fixture.sequence format
uint32_t fixtureGetSerial(void)
{
  if ( !FIXTURE_SERIAL || FIXTURE_SERIAL > 0xfff) { //12-bit limit
    ConsolePrintf("fixture serial out of range for esn generation\n");
    throw ERROR_SERIAL_INVALID;
  }
  return (FIXTURE_SERIAL << 20) | (GetSequence(1) & 0x0Fffff);
}

int fixtureReadSequence(void) {
  return GetSequence(0); //read only, no sequence changes or flash update
}
