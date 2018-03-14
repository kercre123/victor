#include <stdlib.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "meter.h"
#include "portable.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

#define PRESENT_CURRENT_MA  10
#define DETECT_CURRENT_MA   100

int detect_ma = 0;
bool TestRobotDetect(void)
{
  Board::powerOn(PWR_VEXT,0);
  //Board::powerOn(PWR_VBAT); //XXX DEBUG
  
  int i_ma = Meter::getCurrentMa(PWR_VEXT,0);
  
  //running average filter, len=2^oversample
  const int oversample = 6;
  detect_ma = ( ((1<<oversample)-1)*detect_ma + i_ma ) >> oversample;
  
  // Hysteresis for shut-down robots
  return i_ma > DETECT_CURRENT_MA || (i_ma > PRESENT_CURRENT_MA && g_isDevicePresent);
}

void TestRobotCleanup(void)
{
  detect_ma = 0;
  Board::powerOff(PWR_VEXT); //Contacts::powerOff();
  Board::powerOff(PWR_VBAT);
}

static int m_debug[4];
static void run_debug(void)
{
  srand(Timer::get());
  ConsolePrintf("DEBUG %i %i %i %i\n", m_debug[0], m_debug[1], m_debug[2], m_debug[3]);
  
  if( m_debug[0] == 1 )
  {
    int nloops = m_debug[1];
    int rlim = m_debug[2]<=0 ? 255 : m_debug[2]&0xff; //limit for random generator
    ConsolePrintf("===== STRESS TEST COMMS: %i loops, rlim=%i =====\n", nloops, rlim);
    //Timer::delayMs(1000);
    try {
      for(int x = 0; x < nloops; x++) {
        cmdRobotEsn(); cmdRobotBsv();
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_BATTERY);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_BATTERY);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_BATTERY);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_CLIFF);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_MOT_LEFT);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_MOT_RIGHT);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_MOT_LIFT);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_MOT_HEAD);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_PROX_TOF);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_BTN_TOUCH);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_RSSI);
        cmdRobotGet(1+rand()%rlim, 1, CCC_SENSOR_RX_PKT);
      }
    } catch(int e){
      ConsolePrintf("===== STRESS TEST FAIL e%03d =====\n", e );
    }
    ConsolePrintf("===== DONE =====\n");
  }
}

const char* DBG_cmd_substitution(const char *line, int len)
{
  static char buf[25];
  
  if( !strcmp(line, "esn") )
    return ">>esn 00 00 00 00 00 00";
  if( !strcmp(line, "bsv") )
    return ">>bsv 00 00 00 00 00 00";
  if( !strcmp(line, "mot") )
    return ">>mot ff 03";
  if( !strcmp(line, "get") )
    return ">>get 01 00";
  if( !strncmp(line,"smr",3) || !strncmp(line,"gmr",3) || !strncmp(line,"rlg",3) || !strncmp(line,"eng",3) || !strncmp(line,"lfe",3) || !strncmp(line,"fcc",3) ) {
    int nargs = cmdNumArgs((char*)line);
    int ix  = nargs >= 2 ? cmdParseInt32(cmdGetArg((char*)line,1)) : 0;
    int val = nargs >= 3 ? cmdParseInt32(cmdGetArg((char*)line,2)) : 0;
    return snformat(buf,sizeof(buf),">>%c%c%c %02x %02x %02x %02x %02x 00", line[0],line[1],line[2], ix, val&0xFF, (val>>8)&0xff, (val>>16)&0xff, (val>>24)&0xff);
  }
  if( !strncmp(line,"debug",5) ) {
    int nargs = cmdNumArgs((char*)line);
    for(int x=0; x<sizeof(m_debug)/4; x++)
      m_debug[x] = nargs > x+1 ? cmdParseInt32(cmdGetArg((char*)line,x+1) ) : 0;
    run_debug();
    return "\n";
  }
  return 0;
}

//always run this first after detect, to get into comms mode
void TestRobotInfo(void)
{
  Board::powerOn(PWR_VBAT); //XXX Debug: work on body pcba w/o battery
  
  ConsolePrintf("detect current avg %i mA\n", detect_ma);
  
  ConsolePrintf("Resetting comms interface\n");
  Board::powerOff(PWR_VEXT, 500); //turn power off to disable charging
  Contacts::setModeRx();
  
  try { //DEBUG, cmd doesn't always succeed in dev builds
  cmdRobotEsn();
  cmdRobotBsv();
  } catch(int e) {}
  
  //DEBUG: console bridge, manual testing
  if( g_fixmode == FIXMODE_ROBOT0 )
    TestCommon::consoleBridge(TO_CONTACTS, 0, 0, BRIDGE_OPT_LINEBUFFER, DBG_cmd_substitution);
  if( g_fixmode == FIXMODE_ROBOT1 )
    TestCommon::consoleBridge(TO_CONTACTS, 1000, 0, BRIDGE_OPT_LINEBUFFER, DBG_cmd_substitution);
  //-*/
}

void DBG_RobotCmdTest(void)
{
  //test all supported commands - make sure they return a valid response
  if( g_fixmode > FIXMODE_ROBOT0 && g_fixmode <= FIXMODE_ROBOT2 )
  {
    ConsolePrintf("\n=====================================================================\n");
    
    cmdRobotEsn(); //ConsolePutChar('\n');
    cmdRobotBsv(); //ConsolePutChar('\n');
    cmdRobotEsn(); //ConsolePutChar('\n');
    cmdRobotBsv(); //ConsolePutChar('\n');
    cmdRobotEsn(); //ConsolePutChar('\n');
    
    cmdRobotGet(1, 1, CCC_SENSOR_BATTERY); ConsolePutChar('\n');
    cmdRobotGet(3, 3, CCC_SENSOR_BATTERY); ConsolePutChar('\n');
    cmdRobotGet(5, 3, CCC_SENSOR_BATTERY); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_CLIFF); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_MOT_LEFT); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_MOT_RIGHT); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_MOT_LIFT); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_MOT_HEAD); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_PROX_TOF); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_BTN_TOUCH); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_RSSI); ConsolePutChar('\n');
    cmdRobotGet(1, 1, CCC_SENSOR_RX_PKT); ConsolePutChar('\n');
    
    cmdRobotMot(100, 20, CCC_SENSOR_MOT_LEFT, 127, 0, 0, 0); ConsolePutChar('\n');
    cmdRobotMot(100, 20, CCC_SENSOR_MOT_RIGHT, 0, -127, 0, 0); ConsolePutChar('\n');
    cmdRobotMot(50,  15, CCC_SENSOR_MOT_LIFT, 0, 0, 100, 0); ConsolePutChar('\n');
    cmdRobotMot(75,  20, CCC_SENSOR_MOT_LIFT, 0, 0, -100, 0); ConsolePutChar('\n');
    cmdRobotMot(50,  20, CCC_SENSOR_MOT_HEAD, 0, 0, 0, 100); ConsolePutChar('\n');
    cmdRobotMot(75,  20, CCC_SENSOR_MOT_HEAD, 0, 0, 0, -100); ConsolePutChar('\n');
    
    uint32_t esn = cmdRobotGmr(0); //ConsolePutChar('\n');
    uint32_t hw_ver = cmdRobotGmr(1); //ConsolePutChar('\n');
    uint32_t model = cmdRobotGmr(2); //ConsolePutChar('\n');
    uint32_t lot_code = cmdRobotGmr(3); //ConsolePutChar('\n');
    
    /*/test set/get some EMR fields
    srand(Timer::get());
    for(uint8_t idx = 0; idx < 8; idx++)
    {
      uint32_t val = cmdRobotGmr(idx); //ConsolePutChar('\n');
      val = 0x80000000 | rand(); //((rand()&0xffff) << 16) | (rand()&0xffff);
      cmdRobotSmr(idx, val);
      uint32_t val2 = cmdRobotGmr(idx);
      cmdRobotSmr(idx, 0);
      
      if( val != val2 )
        ConsolePrintf("========MISMATCH========\n");
      ConsolePutChar('\n');
    }//-*/
    
    ConsolePrintf("\n=====================================================================\n");
  }
}

void DBG_RobotEMRTest(void)
{
  static uint32_t m_emr[256]; int idx;
  
  if( g_fixmode > FIXMODE_ROBOT0 && g_fixmode <= FIXMODE_ROBOT2 )
  {
    ConsolePrintf("\n======== Full EMR Test =============\n");
    
    //reset EMR to blank
    for(idx=0; idx < 256; idx++)
      cmdRobotSmr(idx, 0);
    
    ConsolePrintf("======== EMR Blank =============\n");
    Timer::delayMs(1000);
    
    //set EMR to random values, store locally for compare
    srand(Timer::get());
    for(idx=0; idx < 256; idx++) {
      uint32_t val = ((rand()&0xffff) << 16) | (rand()&0xffff);
      if( idx == 4 || idx == 5 || idx == 6 ) //PLAYPEN_READY_FLAG, PLAYPEN_PASSED_FLAG, PACKED_OUT_FLAG
        val = 0; //keep 0 to prevent strange robot behavior
      cmdRobotSmr(idx, val);
      m_emr[idx] = val;
    }
    
    ConsolePrintf("======== EMR Set Ok =============\n");
    Timer::delayMs(1000);
    
    //readback verify
    int mismatch = 0;
    for(idx=0; idx < 256; idx++) {
      uint32_t val = cmdRobotGmr(idx);
      if( val != m_emr[idx] ) {
        mismatch++;
        ConsolePrintf("-------> EMR MISMATCH @[%u]: %08x != %08x\n", idx, val, m_emr[idx]);
      }
    }
    
    //results!
    ConsolePrintf("======== EMR test %s: %u errors =============\n", mismatch > 0 ? "FAILED" : "passed", mismatch);
    Timer::delayMs(1000);
    
    //reset EMR to blank
    for(idx=0; idx < 256; idx++)
      cmdRobotSmr(idx, 0);
    
    //results (again)
    ConsolePrintf("======== EMR Blank =============\n");
    ConsolePrintf("======== EMR test %s: %u errors =============\n", mismatch > 0 ? "FAILED" : "passed", mismatch);
    Timer::delayMs(1000);
  }
}

//read battery voltage
int robot_get_battVolt100x(void)
{
  //XXX: hook this up to a real battery command
  #define VBAT_SCALE(v) ((v) = (v)<0 ? -1 : (v)>>16)
  
  ConsolePrintf(">vbat (simulated)\n");
  char* rsp = (char*)"vbat 0 65536 655360 6553600 3276800"; //cmdSend(CMD_IO_CONTACTS, "vbat");
  //int status = cmdStatus();
  
  //convert raw values to 100x voltage [centivolts]
  int vBat100x     = cmdParseInt32( cmdGetArg(rsp,2) ); VBAT_SCALE(vBat100x);
  int vBatFilt100x = cmdParseInt32( cmdGetArg(rsp,3) ); VBAT_SCALE(vBatFilt100x);
  int vExt100x     = cmdParseInt32( cmdGetArg(rsp,4) ); VBAT_SCALE(vExt100x);
  int vExtFilt100x = cmdParseInt32( cmdGetArg(rsp,5) ); VBAT_SCALE(vExtFilt100x);
  ConsolePrintf("vBat,%03d,%03d,vExt,%03d,%03d\r\n", vBat100x, vBatFilt100x, vExt100x, vExtFilt100x );
  
  //return vBatFilt100x; //return filtered vBat
  return -1;
}

enum {
  RECHARGE_STATUS_OK = 0,
  RECHARGE_STATUS_OFF_CONTACT,
  RECHARGE_STATUS_TIMEOUT,
};

//single charge cycle. end on timeout or undercurrent.
static inline int charge1_(uint16_t timeout_s, uint16_t i_done_ma, bool dbgPrint)
{
  int offContactCnt = 0, iDoneCnt = 0, print_len = 0;
  uint32_t Tdisplay = 0;
  uint32_t Tstart = Timer::get();
  uint32_t Tdlatch = Timer::get() - (8*1000*1000); //first latch after ~2s. Waits for charger to kick in.
  
  //Turn on charging power
  Board::powerOn(PWR_VEXT,0);
  
  int status = RECHARGE_STATUS_TIMEOUT;
  while( Timer::elapsedUs(Tstart) < (timeout_s*1000*1000) )
  {
    int current_ma = Meter::getCurrentMa(PWR_VEXT,6);
    int voltage_mv = Meter::getVoltageMv(PWR_VEXT,4);
    
    //Debug: print real-time current usage in console (erasing behavior can cause problems with some terminals, logging etc)
    if( dbgPrint && Timer::elapsedUs(Tdisplay) > 50*1000 )
    {
      Tdisplay = Timer::get();
      
      //erase previous line
      for(int x=0; x < print_len; x++ ) {
        ConsolePutChar(0x08); //backspace
        ConsolePutChar(0x20); //space
        ConsolePutChar(0x08); //backspace
      }
      
      const int DISP_MA_PER_CHAR = 15;
      print_len = ConsolePrintf("%04dmV %03d ", voltage_mv, current_ma);
      for(int x=current_ma; x>0; x -= DISP_MA_PER_CHAR)
        print_len += ConsoleWrite((char*)"=");
      
      //preserve a current history in the console window
      if( Timer::elapsedUs(Tdlatch) > 10*1000*1000 ) {
        Tdlatch = Timer::get();
        ConsolePutChar('\n');
        print_len = 0;
      }
    }
    
    //test for robot removal
    offContactCnt = current_ma < PRESENT_CURRENT_MA ? offContactCnt + 1 : 0;
    if ( offContactCnt > 10 && Timer::elapsedUs(Tstart) > 3*1000*1000 ) {
      status = RECHARGE_STATUS_OFF_CONTACT;
      break;
    }
    
    //charge complete? (current threshold)
    iDoneCnt = i_done_ma > 0 && current_ma < i_done_ma ? iDoneCnt + 1 : 0;
    if( iDoneCnt > 10 && Timer::elapsedUs(Tstart) > 3*1000*1000 ) {
      status = RECHARGE_STATUS_OK;
      break;
    }
  }
  if( print_len )
    ConsolePutChar('\n');
  
  Board::powerOff(PWR_VEXT);
  return status;
}

//recharge cozmo. parameterized conditions: time, battery voltage, current threshold
int m_Recharge( uint16_t max_charge_time_s, uint16_t vbat_limit_v100x, uint16_t i_done_ma, bool dbgPrint )
{
  const u8 BAT_CHECK_INTERVAL_S = 90; //interrupt charging this often to test battery level
  ConsolePrintf("recharge,%dcV,%dmA,%ds\r\n", vbat_limit_v100x, i_done_ma, max_charge_time_s );
  
  Contacts::setModeRx();
  Timer::delayMs(500); //let battery voltage settle
  int battVolt100x = robot_get_battVolt100x(); //get initial battery level
  
  uint32_t Tstart = Timer::get();
  while( vbat_limit_v100x == 0 || battVolt100x < vbat_limit_v100x )
  {
    if( max_charge_time_s > 0 && Timer::elapsedUs(Tstart) >= ((uint32_t)max_charge_time_s*1000*1000) )
      return RECHARGE_STATUS_TIMEOUT;
    
    //charge for awhile, then re-check battery voltage
    int chgStat = charge1_(BAT_CHECK_INTERVAL_S, i_done_ma, dbgPrint );
    ConsolePrintf("total charge time: %ds\n", Timer::elapsedUs(Tstart)/1000000 );
    Contacts::setModeRx();
    Timer::delayMs(500); //let battery voltage settle
    battVolt100x = robot_get_battVolt100x();
    
    //charge loop detected robot removal or charge completion?
    if( chgStat != RECHARGE_STATUS_TIMEOUT )
      return chgStat;
  }
  
  return RECHARGE_STATUS_OK;
}

void Recharge(void)
{
  const u16 BAT_MAX_CHARGE_TIME_S = 25*60;  //max amount of time to charge
  const u16 VBAT_CHARGE_LIMIT = 390;        //Voltage x100
  const u16 BAT_FULL_I_THRESH_MA = 200;     //current threshold for charging complete (experimental)
  int status;
  
  bool dbgPrint = g_fixmode == FIXMODE_RECHARGE0 ? 1 : 0;
  
  //Notes from test measurements ->
  //conditions: 90s charge intervals, interrupted to measure vBat)
  //full charge (3.44V-4.15V) 1880s (31.3min)
  //typical charge (3.65V-3.92V) 990s (16.5min)
  
  if( g_fixmode == FIXMODE_RECHARGE0 )
    status = m_Recharge( 2*BAT_MAX_CHARGE_TIME_S, 0, BAT_FULL_I_THRESH_MA, dbgPrint ); //charge to full battery
  else
    status = m_Recharge( BAT_MAX_CHARGE_TIME_S, VBAT_CHARGE_LIMIT, 0, dbgPrint ); //charge to specified voltage
  
  if( status == RECHARGE_STATUS_TIMEOUT )
    throw ERROR_TIMEOUT;
  
  if( status == RECHARGE_STATUS_OK )
    cmdSend(CMD_IO_CONTACTS, "powerdown", 50, CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_ERRORS | CMD_OPTS_EXCEPTION_EN));
}

extern void RobotChargeTest( u16 i_done_ma, u16 vbat_overvolt_v100x );
static void ChargeTest(void) {
  RobotChargeTest( 425/*mA*/, 420/*cV*/ );
}

//Test charging circuit by verifying current draw
//@param i_done_ma - average current (min) to pass this test
//@param vbat_overvolt_v100x - battery too full voltage level. special failure handling above this threshold.
void RobotChargeTest( u16 i_done_ma, u16 vbat_overvolt_v100x )
{
  #define CHARGE_TEST_DEBUG(x)    x
  const int NUM_SAMPLES = 16;
  
  Contacts::setModeRx(); //switch to comm mode
  Timer::delayMs(500); //let battery voltage settle
  int battVolt100x = robot_get_battVolt100x(); //get initial battery level
  
  battVolt100x = 360; //XXX battery command doesn't work yet
  
  //Turn on charging power
  Board::powerOn(PWR_VEXT,0);
  
  CHARGE_TEST_DEBUG( int ibase_ma = 0; uint32_t Tprint = 0;  );
  int avg=0, avgCnt=0, avgMax = 0, iMax = 0, offContact = 0;
  uint32_t avgMaxTime = 0, iMaxTime = 0, Twait = Timer::get();
  while( Timer::elapsedUs(Twait) < 5*1000*1000 )
  {
    int current_ma = Meter::getCurrentMa(PWR_VEXT,6);
    int voltage_mv = Meter::getVoltageMv(PWR_VEXT,4);
    avg = ((avg*avgCnt) + current_ma) / (avgCnt+1); //tracking average
    avgCnt = avgCnt < NUM_SAMPLES ? avgCnt + 1 : avgCnt;
    
    //DEBUG: log charge current as bar graph
    CHARGE_TEST_DEBUG( {
      const int DISP_MA_PER_CHAR = 15;
      const int IDIFF_MA = 25;
      if( ABS(current_ma - ibase_ma) >= IDIFF_MA || ABS(avg - ibase_ma) >= IDIFF_MA || 
          Timer::elapsedUs(Tprint) > 500*1000 || (avgCnt >= NUM_SAMPLES && avg >= i_done_ma) )
      {
        ibase_ma = current_ma;
        Tprint = Timer::get();
        ConsolePrintf("%04umV %03d/%03d ", voltage_mv, avg, current_ma );
        for(int x=1; x <= (avg > current_ma ? avg : current_ma); x += DISP_MA_PER_CHAR )
          ConsolePrintf( x <= avg && x <= current_ma ? "=" : x > avg ? "+" : "-" );
        ConsolePrintf("\n");
      }
    } );
    
    //save some metrics for debug
    if( current_ma > iMax ) {
      iMax = current_ma;
      iMaxTime = Timer::elapsedUs(Twait);
    }
    if( avg > avgMax ) {
      avgMax = avg;
      avgMaxTime = Timer::elapsedUs(Twait);
    }
    
    //finish when average rises above our threshold (after minimum sample cnt)
    if( avgCnt >= NUM_SAMPLES && avg >= i_done_ma )
      break;
    
    //error out quickly if robot removed from charge base
    if ((offContact = current_ma < PRESENT_CURRENT_MA ? offContact + 1 : 0) > 5) {
      CHARGE_TEST_DEBUG( ConsolePrintf("\n"); );
      ConsolePrintf("robot off charger\n");
      throw ERROR_BAT_CHARGER;
    }
    
    //keep an eye on output voltage from crappy power supplies
    const int undervolt = 4700, overvolt = 5300;
    if( voltage_mv < undervolt || voltage_mv > overvolt ) {
      ConsolePrintf("bad voltage: %u\n", voltage_mv );
      throw voltage_mv < undervolt ? ERROR_OUTPUT_VOLTAGE_LOW : ERROR_OUTPUT_VOLTAGE_HIGH;
    }
  }
  
  ConsolePrintf("charge-current-ma,%d,sample-cnt,%d\r\n", avg, avgCnt);
  ConsolePrintf("charge-current-dbg,avgMax,%d,%d,iMax,%d,%d\r\n", avgMax, avgMaxTime, iMax, iMaxTime);
  if( avgCnt >= NUM_SAMPLES && avg >= i_done_ma )
    return; //OK
  
  if( battVolt100x >= vbat_overvolt_v100x ) {
    //const int BURN_TIME_S = 120;  //time to hit the gym and burn off a few pounds
    //ConsolePrintf("power-on,%ds\r\n", BURN_TIME_S);
    //robot_get_battVolt100x(BURN_TIME_S,BURN_TIME_S); //reset power-on timer and set face to info screen for the duration
    throw ERROR_BAT_OVERVOLT; //error prompts operator to put robot aside for a bit
  }
  
  throw ERROR_BAT_CHARGER;
}

TestFunction* TestRobotInfoGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot0GetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    //DBG_RobotCmdTest,
    //ChargeTest,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot1GetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    DBG_RobotCmdTest,
    //ChargeTest,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot2GetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    DBG_RobotEMRTest,
    //ChargeTest,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot3GetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    ChargeTest,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotPlaypenGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotPackoutGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    ChargeTest,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotLifetestGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotRecharge0GetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    Recharge,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotRecharge1GetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    Recharge,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotSoundGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotFacRevertGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotEMGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

