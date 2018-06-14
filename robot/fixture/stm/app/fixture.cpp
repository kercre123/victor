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

#include "stm32f2xx.h"
#include "stm32f2xx_rtc.h"

//global fixture data
int g_fixmode = FIXMODE_NONE;
const fixmode_info_t g_fixmode_info[] = { FIXMODE_INFO_INIT_DATA };
const int g_num_fixmodes = sizeof(g_fixmode_info) / sizeof(fixmode_info_t);
bool g_allowOutdated = false;

static void rtc_init_(void);

void fixtureInit(void)
{
  Board::init();
  
  //ConsolePrintf("fixtureInit()\n");
  
  //Try to restore saved mode
  g_fixmode = FIXMODE_NONE;
  if ( g_flashParams.fixtureTypeOverride > FIXMODE_NONE && g_flashParams.fixtureTypeOverride < g_num_fixmodes ) {
    if( g_fixmode_info[g_fixmode].name != NULL ) { //Prevent invalid modes
      g_fixmode = g_flashParams.fixtureTypeOverride;
    }
  }
  
  rtc_init_();
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
  if ( !FIXTURE_SERIAL || FIXTURE_SERIAL > 0x7ff) { //12-bit limit
    ConsolePrintf("fixture serial out of range for esn generation\n");
    throw ERROR_SERIAL_INVALID;
  }
  return (FIXTURE_SERIAL << 20) | (GetSequence(1) & 0x0Fffff);
}

uint32_t fixtureReadSerial(void) {
  uint32_t serial = FIXTURE_SERIAL > 0x7ff ? 0 : FIXTURE_SERIAL;
  return (serial << 20) | (GetSequence(0) & 0x0Fffff); //read only, no sequence changes or flash update
}

int fixtureReadSequence(void) {
  return GetSequence(0); //read only, no sequence changes or flash update
}

//-----------------------------------------------------------------------------
//                  RTC
//-----------------------------------------------------------------------------

#define FIXTURE_RTC_DEBUG 0

#if FIXTURE_RTC_DEBUG > 0
#warning fixture RTC debug enabled
#endif

//backup register metadata cfg
const uint32_t RTC_BACKUP_REG_INIT    = RTC_BKP_DR0;
const uint32_t RTC_BACKUP_REG_CENTURY = RTC_BKP_DR1;
const uint32_t RTC_INIT_VALUE = 0x32F2;

enum rtc_error_bits_e {
  RTC_ERR_NO_INIT         = 0x1000,
  RTC_ERR_APB_SYNC        = 0x0001,
  RTC_ERR_SET_TIME        = 0x0002,
  RTC_ERR_SET_DATE        = 0x0004,
  RTC_ERR_VERIFY_CENTURY  = 0x0010,
  RTC_ERR_VERIFY_TIME     = 0x0020,
  RTC_ERR_VERIFY_DATE     = 0x0040,
};

/*char* unixtimestr_(time_t time) {
  static char str[10];
  time = time % (24*3600); //discard date
  snprintf(str,sizeof(str)-1, "%0.2d:%0.2d:%0.2d", time/3600, (time%3600)/60, time%60 );
  str[sizeof(str)-1] = '\0';
  return str;
}*/

char* rtcstr_(RTC_DateTypeDef *date, RTC_TimeTypeDef *time) {
  static char str[32];
  int len = snprintf(str,sizeof(str),
    "%u %02u/%02u/%02u %02u:%02u:%02u",
    date->RTC_WeekDay,
    date->RTC_Date, date->RTC_Month, date->RTC_Year,
    time->RTC_Hours, time->RTC_Minutes, time->RTC_Seconds);
  str[len] = '\0';
  return str;
}

static int APB_register_sync(void) {
  ErrorStatus esync = RTC_WaitForSynchro(); //APB register synchronization
  #if FIXTURE_RTC_DEBUG > 0
  if( esync != /*ErrorStatus.*/SUCCESS ) 
    ConsolePrintf(">> !! APB reg sync error %i !! <<\n", esync);
  #endif
  return (esync!=SUCCESS ? RTC_ERR_APB_SYNC : 0);
}

static int RTC_SetDateTime_(RTC_DateTypeDef* RTC_DateStruct, RTC_TimeTypeDef* RTC_TimeStruct)
{
  //MUST write date,time registers in proper order (time last)
  int e = APB_register_sync();
  if( RTC_SetDate(RTC_Format_BIN, RTC_DateStruct) != /*ErrorStatus.*/SUCCESS )
    e |= RTC_ERR_SET_DATE;
  if( RTC_SetTime(RTC_Format_BIN, RTC_TimeStruct) != /*ErrorStatus.*/SUCCESS )
    e |= RTC_ERR_SET_TIME;
  
  #if FIXTURE_RTC_DEBUG > 0
  if( e != 0 ) ConsolePrintf(">> !! RTC set datetime: e=0x%04x !! <<\n", e);
  #endif
  return e;
}

static void RTC_GetDateTime_(RTC_DateTypeDef* RTC_DateStruct, RTC_TimeTypeDef* RTC_TimeStruct) {
  //MUST read time->date in proper order (time first)
  APB_register_sync();
  RTC_GetTime(RTC_Format_BIN, RTC_TimeStruct);
  RTC_GetDate(RTC_Format_BIN, RTC_DateStruct);
}

const char* fixtureTimeStr(time_t time)
{
  static char str[21];
  
  //squeeze clib localtime/asctime string to 20chars (our max display size)
  char* ltime = ctime(&time); //"Sun Sep 16 01:03:52 1973\n\0"
  memcpy( str+0,  ltime+0,  7 );  //"Sun Sep"
  memcpy( str+7,  ltime+8,  8 );  //"16 01:03"
  memcpy( str+15, ltime+19, 5 );  //" 1973"
  str[20] = '\0';
  
  #if FIXTURE_RTC_DEBUG > 0
  ConsolePrintf("fixtureTimeStr,%010u,%s,%s", time, str, ltime);
  #endif
  
  return str;
}

int fixtureSetTime(time_t time)
{
  int e = 0;
  bool rtc_inited = RTC_ReadBackupRegister(RTC_BACKUP_REG_INIT) == RTC_INIT_VALUE;
  struct { RTC_DateTypeDef date; RTC_TimeTypeDef time; } rtc;
  memset(&rtc, 0, sizeof(rtc));
  
  //unix -> stm rtc format
  struct tm bdtime = *localtime(&time); //intermediate "broken-down time" (clib)
  uint32_t century = 1900 + 100*(bdtime.tm_year/100);
  rtc.date.RTC_Year = bdtime.tm_year %100;  //years since 1900 -> {0..99}
  rtc.date.RTC_Month = bdtime.tm_mon +1;    //{0..11} -> {1..12}
  rtc.date.RTC_Date = bdtime.tm_mday;       //{1..31}
  rtc.date.RTC_WeekDay = bdtime.tm_wday +1; //{0..6} -> {1..7}
  rtc.time.RTC_H12 = RTC_H12_AM; //not used, but make sure it's valid
  rtc.time.RTC_Hours = bdtime.tm_hour;
  rtc.time.RTC_Minutes = bdtime.tm_min;
  rtc.time.RTC_Seconds = bdtime.tm_sec;
  
  #if FIXTURE_RTC_DEBUG > 0
  char *ltime = asctime(&bdtime); ltime[24] = '\0'; //rm newline
  ConsolePrintf("fixtureSetTime,%i,%010u,%s,%s,%u\n", fixtureTimeIsValid(), time, ltime, rtcstr_(&rtc.date,&rtc.time), century );
  #endif
  
  if( rtc_inited ) {
    e |= RTC_SetDateTime_(&rtc.date, &rtc.time);
    RTC_WriteBackupRegister(RTC_BACKUP_REG_CENTURY, e==0 ? century : 0); //invalidate on error
    
    //readback verify (sanity check finicky hal api)
    struct { RTC_DateTypeDef date; RTC_TimeTypeDef time; } rtcReadback;
    RTC_GetDateTime_(&rtcReadback.date, &rtcReadback.time);
    if( *((uint32_t*)&rtc.date) != *((uint32_t*)&rtcReadback.date) )
      e |= RTC_ERR_VERIFY_DATE;
    if( *((uint32_t*)&rtc.time) != *((uint32_t*)&rtcReadback.time) )
      e |= RTC_ERR_VERIFY_TIME;
    if( century != RTC_ReadBackupRegister(RTC_BACKUP_REG_CENTURY) )
      e |= RTC_ERR_VERIFY_CENTURY;
    
    #if FIXTURE_RTC_DEBUG > 0
    if( (e & (RTC_ERR_VERIFY_DATE|RTC_ERR_VERIFY_TIME|RTC_ERR_VERIFY_CENTURY)) != 0 )
      ConsolePrintf(">> !! fixtureSetTime failed verify. e=0x%04x !! <<\n", e);
    #endif
    
  } else {
    e |= RTC_ERR_NO_INIT;
    
    #if FIXTURE_RTC_DEBUG > 0
    ConsolePrintf(">> !! fixtureSetTime failed, rtc not initialized !! <<\n");
    #endif
  }
  
  return e;
}

time_t fixtureGetTime(void)
{
  time_t time = 0;
  bool valid = fixtureTimeIsValid();
  
  struct { RTC_DateTypeDef date; RTC_TimeTypeDef time; } rtc;
  RTC_GetDateTime_( &rtc.date, &rtc.time );
  
  //stm rtc format -> unix
  struct tm bdtime; memset(&bdtime,0,sizeof(bdtime));
  uint32_t century = RTC_ReadBackupRegister(RTC_BACKUP_REG_CENTURY);
  bdtime.tm_year = valid ? century-1900 + rtc.date.RTC_Year : rtc.date.RTC_Year;
  bdtime.tm_mon = rtc.date.RTC_Month -1;
  bdtime.tm_mday = rtc.date.RTC_Date;
  bdtime.tm_wday = rtc.date.RTC_WeekDay -1;
  bdtime.tm_hour = rtc.time.RTC_Hours;
  bdtime.tm_min = rtc.time.RTC_Minutes;
  bdtime.tm_sec = rtc.time.RTC_Seconds;
  
  if( valid )
    time = mktime(&bdtime); //"broken-down time" (clib) -> unix
  
  #if FIXTURE_RTC_DEBUG > 0
  char *ltime = asctime(&bdtime); ltime[24] = '\0'; //rm newline
  ConsolePrintf("fixtureGetTime,%i,%010u,%s,%s,%u\n", valid, time, ltime, rtcstr_(&rtc.date,&rtc.time), century );
  #endif
  
  return time;
}

bool fixtureTimeIsValid(void) {
  return RTC_ReadBackupRegister(RTC_BACKUP_REG_INIT) == RTC_INIT_VALUE && 
         RTC_ReadBackupRegister(RTC_BACKUP_REG_CENTURY) >= 1900;
}

static void rtc_init_(void)
{
  #if FIXTURE_RTC_DEBUG > 0
  ConsolePrintf("********** RTC Init **********\n");
  #endif
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  
  if (RTC_ReadBackupRegister(RTC_BACKUP_REG_INIT) != RTC_INIT_VALUE)
  {
    //set up 32k LSE
    RCC_LSEConfig(RCC_LSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
      ;
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    APB_register_sync();
    
    //RTC init
    RTC_InitTypeDef RTC_InitStructure;
    RTC_InitStructure.RTC_AsynchPrediv = 0x7F; //clock prescaler for 32768 -> 1Hz
    RTC_InitStructure.RTC_SynchPrediv = 0xFF;  //"
    RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
    if (RTC_Init(&RTC_InitStructure) == ERROR) {
      #if FIXTURE_RTC_DEBUG > 0
      ConsolePrintf(">> !! RTC Prescaler Config failed !! <<\n");
      #endif
    }
    
    struct { RTC_DateTypeDef date; RTC_TimeTypeDef time; } rtc;
    memset( &rtc, 0, sizeof(rtc) );
    rtc.date.RTC_WeekDay = RTC_Weekday_Thursday;
    rtc.date.RTC_Month = 1; //Jan
    rtc.date.RTC_Date = 1;  //1
    rtc.date.RTC_Year = 70; //1970
    rtc.time.RTC_H12 = RTC_H12_AM;
    
    if( !RTC_SetDateTime_(&rtc.date, &rtc.time) ) {
      RTC_WriteBackupRegister(RTC_BACKUP_REG_INIT, RTC_INIT_VALUE);
      RTC_WriteBackupRegister(RTC_BACKUP_REG_CENTURY, 0);
      
      #if FIXTURE_RTC_DEBUG > 0
      ConsolePrintf("RTC initialized\n");
      #endif
    }
    
  }
  else
  {
    #if FIXTURE_RTC_DEBUG > 0
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET) //Power On Reset
      ConsolePrintf("Power On Reset occurred....\n");
    else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) //Pin Reset
      ConsolePrintf("External Reset occurred....\n");
    else
      ConsolePrintf("Unknown Reset occurred....\n");
    #endif
    
    //APB_register_sync();
  }
  
  #if FIXTURE_RTC_DEBUG > 0
  fixtureGetTime();
  #endif
}


//-----------------------------------------------------------------------------
//                  RTC Testbench
//-----------------------------------------------------------------------------

#define RTC_TESTBENCH_EN  0

void testprint_(const char* prefix, time_t time, const char* post) {
  char *ltime = ctime(&time); ltime[24] = '\0'; //"Sun Sep 16 01:03:52 1973\n\0"
  ConsolePrintf("%s,%i,%010u,%s,%s,\n", 
    prefix ? prefix : "",
    fixtureTimeIsValid(),
    time,
    ltime,
    post ? post : ""
  );
}

extern void fixtureRtcTestbench(void);
void fixtureRtcTestbench(void)
{
  #if RTC_TESTBENCH_EN > 0
  #warning "RTC Testbench Enabled"
  
  typedef struct { time_t time; const char* str; } rtc_tb_t;
  const rtc_tb_t test[] = {
    {3234567890, "Fri Jul 01 03:04:50 2072"},
    {         1, "Thu Jan  1 00:00:01 1970"},
    {4123456789, "Wed Sep 01 04:39:49 2100"},
    {         0, "Thu Jan  1 00:00:00 1970"},
    {1528756287, "Mon Jun 11 22:31:27 2018"},
    {1234567890, "Fri Feb 13 23:31:30 2009"},
  };
  const int numTests = sizeof(test)/sizeof(rtc_tb_t);
  int numerrors = 0;
  
  ConsolePrintf("********** RTC Testbench **********\n");
  ConsolePrintf("inited:%i valid:%i\n", RTC_ReadBackupRegister(RTC_BACKUP_REG_INIT) == RTC_INIT_VALUE, fixtureTimeIsValid());
  
  for(int n=0; n<numTests; n++)
  {
    ConsolePrintf("TEST %02i:%010u %s\n", n, test[n].time, test[n].str);
    testprint_("  set", test[n].time, 0);
    fixtureSetTime( test[n].time );
    
    for(int x=0; x<5; x++) {
      time_t tget = fixtureGetTime();
      numerrors = tget != test[n].time+x ? numerrors+1 : numerrors;
      testprint_("  get", tget, tget != test[n].time+x ? "----MISMATCH----" : 0 );
      Timer::delayMs(1010);
    }
  }
  ConsolePrintf("num errors: %i\n", numerrors);
  ConsolePrintf("********** RTC Test Done **********\n");
  
  #endif
}

