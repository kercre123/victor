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

//backup register metadata cfg
const uint32_t RTC_BACKUP_REG_INIT    = RTC_BKP_DR0;
const uint32_t RTC_BACKUP_REG_CENTURY = RTC_BKP_DR1;
const uint32_t RTC_INIT_VALUE = 0x32F2;

/*
char* rtctime_str(RTC_TimeTypeDef *time) {
  static char str[10];
  snprintf(str,sizeof(str)-1, "%0.2d:%0.2d:%0.2d", time->RTC_Hours, time->RTC_Minutes, time->RTC_Seconds);
  str[sizeof(str)-1] = '\0';
  return str;
}
*/

char* unixtime_str(time_t time) {
  static char str[10];
  time = time % (24*3600); //discard date
  snprintf(str,sizeof(str)-1, "%0.2d:%0.2d:%0.2d", time/3600, (time%3600)/60, time%60 );
  str[sizeof(str)-1] = '\0';
  return str;
}

void fixtureSetTime(time_t time)
{
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
  ConsolePrintf("fixtureSetTime,%i,%08x,%s,%s", rtc_inited, time, unixtime_str(time), asctime(&bdtime)); //asctime appends '\n'
  #endif
  
  if( rtc_inited ) {
    RTC_SetTime(RTC_Format_BIN, &rtc.time);
    RTC_SetDate(RTC_Format_BIN, &rtc.date);
    RTC_WriteBackupRegister(RTC_BACKUP_REG_CENTURY, century);
  }
}

time_t fixtureGetTime(void)
{
  time_t time = 0;
  bool valid = fixtureTimeIsValid();
  
  struct { RTC_DateTypeDef date; RTC_TimeTypeDef time; } rtc;
  RTC_GetDate(RTC_Format_BIN, &rtc.date);
  RTC_GetTime(RTC_Format_BIN, &rtc.time);
  
  //stm rtc format -> unix
  struct tm bdtime; memset(&bdtime,0,sizeof(bdtime));
  bdtime.tm_year = valid ? RTC_ReadBackupRegister(RTC_BACKUP_REG_CENTURY) - 1900 + rtc.date.RTC_Year : rtc.date.RTC_Year;
  bdtime.tm_mon = rtc.date.RTC_Month -1;
  bdtime.tm_mday = rtc.date.RTC_Date;
  bdtime.tm_wday = rtc.date.RTC_WeekDay -1;
  bdtime.tm_hour = rtc.time.RTC_Hours;
  bdtime.tm_min = rtc.time.RTC_Minutes;
  bdtime.tm_sec = rtc.time.RTC_Seconds;
  
  if( valid )
    time = mktime(&bdtime); //"broken-down time" (clib) -> unix
  
  #if FIXTURE_RTC_DEBUG > 0
  ConsolePrintf("fixtureGetTime,%i,%08x,%s,%s", valid, time, unixtime_str(time), asctime(&bdtime)); //asctime appends '\n'
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
  
  if (RTC_ReadBackupRegister(RTC_BACKUP_REG_INIT) != RTC_INIT_VALUE)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    
    //set up 32k LSE
    RCC_LSEConfig(RCC_LSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
      ;
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro(); //APB register synchronization
    
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
    
    //RTC_TimeRegulate(); /* Configure the time register */
    RTC_TimeTypeDef rtcTime;
    rtcTime.RTC_H12 = RTC_H12_AM;
    rtcTime.RTC_Hours = 0;
    rtcTime.RTC_Minutes = 0;
    rtcTime.RTC_Seconds = 0;
    
    if( RTC_SetTime(RTC_Format_BIN, &rtcTime) == ERROR ) {
      #if FIXTURE_RTC_DEBUG > 0
      ConsolePrintf(">> !! RTC Set Time failed. !! <<\n");
      #endif
    } else {
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
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    RTC_WaitForSynchro();
    
    //RTC_ClearFlag(RTC_FLAG_ALRAF); //Clear the RTC Alarm Flag
    //EXTI_ClearITPendingBit(EXTI_Line17); //Clear the EXTI Line 17 Pending bit (Connected internally to RTC Alarm)
  }
  
  #if FIXTURE_RTC_DEBUG > 0
  fixtureGetTime();
  #endif
}

