#include <stdlib.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "emrf.h"
#include "meter.h"
#include "portable.h"
#include "robotcom.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

//-----------------------------------------------------------------------------
//                  Debug
//-----------------------------------------------------------------------------

extern void read_robot_info_(void);
static void dbg_test_all_(void)
{
  //test all supported commands
  read_robot_info_(); //esn,bsv,gmr...
  ConsolePutChar('\n');
  
  rcomMot(100, RCOM_SENSOR_MOT_LEFT, 127, 0, 0, 0); ConsolePutChar('\n');
  rcomMot(100, RCOM_SENSOR_MOT_RIGHT, 0, -127, 0, 0); ConsolePutChar('\n');
  rcomMot(50,  RCOM_SENSOR_MOT_LIFT, 0, 0, 100, 0); ConsolePutChar('\n');
  rcomMot(75,  RCOM_SENSOR_MOT_LIFT, 0, 0, -100, 0); ConsolePutChar('\n');
  rcomMot(50,  RCOM_SENSOR_MOT_HEAD, 0, 0, 0, 100); ConsolePutChar('\n');
  rcomMot(75,  RCOM_SENSOR_MOT_HEAD, 0, 0, 0, -100); ConsolePutChar('\n');
  
  rcomGet(1, RCOM_SENSOR_BATTERY); ConsolePutChar('\n');
  rcomGet(3, RCOM_SENSOR_BATTERY); ConsolePutChar('\n');
  rcomGet(5, RCOM_SENSOR_BATTERY); ConsolePutChar('\n');
  rcomGet(1, RCOM_SENSOR_CLIFF); ConsolePutChar('\n');
  //rcomGet(1, RCOM_SENSOR_MOT_LEFT); ConsolePutChar('\n');
  //rcomGet(1, RCOM_SENSOR_MOT_RIGHT); ConsolePutChar('\n');
  //rcomGet(1, RCOM_SENSOR_MOT_LIFT); ConsolePutChar('\n');
  //rcomGet(1, RCOM_SENSOR_MOT_HEAD); ConsolePutChar('\n');
  rcomGet(1, RCOM_SENSOR_PROX_TOF); ConsolePutChar('\n');
  rcomGet(1, RCOM_SENSOR_BTN_TOUCH); ConsolePutChar('\n');
  rcomGet(1, RCOM_SENSOR_RSSI); ConsolePutChar('\n');
  rcomGet(1, RCOM_SENSOR_RX_PKT); ConsolePutChar('\n');
}

static void dbg_test_emr_(bool blank_only=0, bool dont_clear=0);
static void dbg_test_emr_(bool blank_only, bool dont_clear)
{
  static uint32_t m_emr[256]; int idx;
  ConsolePrintf("EMR READ/WRITE TEST\n");
  
  //reset EMR to blank
  for(idx=0; idx < 256; idx++)
    rcomSmr(idx, 0);
  if( blank_only )
    return;
  
  //set EMR to random values, store locally for compare
  srand(Timer::get());
  for(idx=0; idx < 256; idx++)
  {
    uint32_t val = ((rand()&0xffff) << 16) | (rand()&0xffff);
    
    //keep some fields 0 to prevent strange robot behavior
    if( idx == EMR_FIELD_OFS(PLAYPEN_READY_FLAG) || 
        idx == EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) ||
        idx == EMR_FIELD_OFS(PACKED_OUT_FLAG) ||
        (idx >= EMR_FIELD_OFS(playpen[0]) && idx <= EMR_FIELD_OFS(playpen[7])) )
    {
      val = 0;
    }
    
    rcomSmr(idx, val);
    m_emr[idx] = val;
  }
  
  //readback verify
  int mismatch = 0;
  for(idx=0; idx < 256; idx++) {
    uint32_t val = rcomGmr(idx);
    if( val != m_emr[idx] ) {
      mismatch++;
      ConsolePrintf("-------> EMR MISMATCH @[%u]: %08x != %08x\n", idx, val, m_emr[idx]);
    }
  }
  
  //results!
  ConsolePrintf("EMR test %s: %u errors\n", mismatch > 0 ? "FAILED" : "passed", mismatch);
  Timer::delayMs(2000); //so we can see it before blanking emr (which might fail...)
  if( dont_clear )
    return;
  
  //reset EMR to blank
  for(idx=0; idx < 256; idx++)
    rcomSmr(idx, 0);
  
  ConsolePrintf("EMR test %s: %u errors\n", mismatch > 0 ? "FAILED" : "passed", mismatch);
}

static void dbg_test_comm_loop_(int nloops, int rmax, int rmin)
{
  rmin = rmin<=0 ?   1 : rmin&0xff; //limit for random generator
  rmax = rmax<=0 ? 255 : rmax&0xff; //upper limit for random generator
  if( rmin > rmax )
    rmin = rmax;
  
  ConsolePrintf("STRESS TEST COMMS: %i loops, NN=rand{%i..%i}\n", nloops, rmin, rmax);
  int rmod = rmax-rmin+1; //modulo
  
  static int sensorSelect = -1;
  sensorSelect += 1;
  
  srand(Timer::get());
  for(int x = 0; x < nloops; x++)
  {
    rcomEsn(); rcomBsv();
    if( sensorSelect == 0 ) {}
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_BATTERY);
    else if( sensorSelect == 1 )
    rcomGet(rmin+rand()%rmod, RCOM_SENSOR_CLIFF);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_LEFT);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_RIGHT);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_LIFT);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_HEAD);
    else if( sensorSelect == 2 )
    rcomGet(rmin+rand()%rmod, RCOM_SENSOR_PROX_TOF);
    else if( sensorSelect == 3 )
    rcomGet(rmin+rand()%rmod, RCOM_SENSOR_BTN_TOUCH);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_RSSI);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_RX_PKT);
    else if( sensorSelect == 4 )
    rcomGet(rmin+rand()%rmod, RCOM_SENSOR_DEBUG_INC);
    else { sensorSelect = -1; break; }
  }
}

static void run_debug(int arg[])
{
  if( arg[0] == 1 )
    dbg_test_all_();
  if( arg[0] == 2 )
    dbg_test_comm_loop_(arg[1], arg[2], arg[3]);
  if( arg[0] == 3 )
    dbg_test_emr_( arg[1], arg[2] );
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
  if( !strncmp(line,"debug",5) )
  {
    int arg[4];
    int nargs = cmdNumArgs((char*)line);
    for(int x=0; x<sizeof(arg)/4; x++)
      arg[x] = nargs > x+1 ? cmdParseInt32(cmdGetArg((char*)line,x+1) ) : 0;
    
    int dbgErr = 0;
    ConsolePrintf("========== DEBUG %i %i %i %i ==========\n", arg[0], arg[1], arg[2], arg[3]);
    try { run_debug(arg); } catch(int e) { dbgErr = e; }
    ConsolePrintf("========== DEBUG %s e%03d ==========\n", !dbgErr ? "OK" : "FAIL", dbgErr);
    
    return "\n"; //sends the debug line to charge contacts (ignored by robot, but keeps cmd in recall buffer)
  }
  return 0;
}

//-----------------------------------------------------------------------------
//                  Robot
//-----------------------------------------------------------------------------

#define PRESENT_CURRENT_MA  10
#define DETECT_CURRENT_MA   100
#define SYSCON_CHG_PWR_DELAY_MS 250 /*delay from robot's on-charger detect until charging starts*/

int detect_ma = 0; int detect_mv = 0;
bool TestRobotDetect(void)
{
  if( g_fixmode == FIXMODE_ROBOT1 )
  {
    //ROBOT1 connected to stump via spine cable. Check for power input.
    int vdut_mv = Meter::getVoltageMv(PWR_DUTVDD,0);
    
    //running average filter, len=2^oversample
    const int oversample = 6;
    detect_mv = ( ((1<<oversample)-1)*detect_mv + vdut_mv ) >> oversample;
    
    return vdut_mv > 3000;    
  }
  
  //on test cleanup/exit, let charger kick back in so we can properly detect removal
  int chargeDelay = (g_isDevicePresent && !Board::powerIsOn(PWR_VEXT)) ? SYSCON_CHG_PWR_DELAY_MS : 0; //condition from app::WaitForDeviceOff()
  
  Board::powerOn(PWR_VEXT, chargeDelay);
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
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  //ConsolePrintf("----DBG: i=%imA\n", Meter::getCurrentMa(PWR_VEXT,4) );
}

void read_robot_info_(void)
{
  uint32_t esn0 = rcomEsn();
  uint32_t esn1     = rcomGmr( EMR_FIELD_OFS(ESN) );
  uint32_t hwver    = rcomGmr( EMR_FIELD_OFS(HW_VER) );
  uint32_t model    = rcomGmr( EMR_FIELD_OFS(MODEL) );
  uint32_t lot_code = rcomGmr( EMR_FIELD_OFS(LOT_CODE) );
  uint32_t playpenready = rcomGmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG) );
  uint32_t playpenpass  = rcomGmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) );
  uint32_t packedout    = rcomGmr( EMR_FIELD_OFS(PACKED_OUT_FLAG) );
  uint32_t packoutdate  = rcomGmr( EMR_FIELD_OFS(PACKED_OUT_DATE) );
  rcomBsv();
  
  ConsolePrintf("EMR[%u] esn         :%08x [%08x]\n", EMR_FIELD_OFS(ESN), esn1, esn0);
  ConsolePrintf("EMR[%u] hwver       :%u\n", EMR_FIELD_OFS(HW_VER), hwver);
  ConsolePrintf("EMR[%u] model       :%u\n", EMR_FIELD_OFS(MODEL), model);
  ConsolePrintf("EMR[%u] lotcode     :%u\n", EMR_FIELD_OFS(LOT_CODE), lot_code);
  ConsolePrintf("EMR[%u] playpenready:%u\n", EMR_FIELD_OFS(PLAYPEN_READY_FLAG), playpenready);
  ConsolePrintf("EMR[%u] playpenpass :%u\n", EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), playpenpass);
  ConsolePrintf("EMR[%u] packedout   :%u\n", EMR_FIELD_OFS(PACKED_OUT_FLAG), packedout);
  ConsolePrintf("EMR[%u] packout-date:%u\n", EMR_FIELD_OFS(PACKED_OUT_DATE), packoutdate);
}

//always run this first after detect, to get into comms mode
void TestRobotInfo(void)
{
  Board::powerOn(PWR_VBAT); //XXX Debug: work on body pcba w/o battery
  
  if( g_fixmode == FIXMODE_ROBOT1 )
    ConsolePrintf("detect voltage avg %imV\n", detect_mv);
  else
    ConsolePrintf("detect current avg %i mA\n", detect_ma);
  
  ConsolePrintf("Resetting comms interface\n");
  Board::powerOff(PWR_VEXT, 500); //turn power off to disable charging
  Contacts::setModeRx();
  
  read_robot_info_();
  
  //DEBUG: console bridge, manual testing
  if( g_fixmode == FIXMODE_ROBOT0 )
    TestCommon::consoleBridge(TO_CONTACTS, 0, 0, BRIDGE_OPT_LINEBUFFER, DBG_cmd_substitution);
  if( g_fixmode == FIXMODE_ROBOT1 )
    TestCommon::consoleBridge(TO_CONTACTS, 1000, 0, BRIDGE_OPT_LINEBUFFER, DBG_cmd_substitution);
  //-*/
}

//robot_sr_t get(uint8_t NN, uint8_t sensor
void TestRobotSensors(void)
{
  //XXX struct copies don't seem to work correctly...
  robot_sr_t bat    = rcomGet(3, RCOM_SENSOR_BATTERY   )[1];
  robot_sr_t cliff  = rcomGet(3, RCOM_SENSOR_CLIFF     )[1];
  robot_sr_t prox   = rcomGet(3, RCOM_SENSOR_PROX_TOF  )[1];
  robot_sr_t btn    = rcomGet(3, RCOM_SENSOR_BTN_TOUCH )[1];
  //robot_sr_t rssi   = rcomGet(3, RCOM_SENSOR_RSSI      )[1];
  //robot_sr_t pktcnt = rcomGet(3, RCOM_SENSOR_RX_PKT    )[1];
  
  ConsolePrintf("Sensor Values:\n");
  ConsolePrintf("  battery = %i.%03iV\n", bat.bat.raw/1000, bat.bat.raw%1000);
  ConsolePrintf("  cliff = fL:%i fR:%i bR:%i bL:%i\n", cliff.cliff.fL, cliff.cliff.fR, cliff.cliff.bR, cliff.cliff.bL);
  ConsolePrintf("  prox = %imm sigRate:%i spad:%i ambientRate:%i\n", prox.prox.rangeMM, prox.prox.signalRate, prox.prox.spadCnt, prox.prox.ambientRate);
  ConsolePrintf("  btn = %i touch=%i\n", btn.btn.btn, btn.btn.touch);
  //ConsolePrintf("  rf = %idBm %i packets\n", rssi.fccRssi.rssi, rssi.fccRx.pktCnt);
  
  //XXX: what should "good" sensor values look like?
  
}

//measure tread speed, distance etc.
typedef struct { int fwd_mid; int fwd_avg; int fwd_travel; int rev_mid; int rev_avg; int rev_travel; } motor_speed_t;
static motor_speed_t* tread_test_(uint8_t sensor, int8_t power)
{
  const int cmd_opts = (CMD_OPTS_DEFAULT);// & ~(CMD_OPTS_LOG_RSP | CMD_OPTS_LOG_ASYNC));
  static motor_speed_t test;
  memset(&test, 0, sizeof(test));
  robot_sr_t* psr;
  
  int8_t pwrL = sensor == RCOM_SENSOR_MOT_LEFT ? power : 0;
  int8_t pwrR = sensor == RCOM_SENSOR_MOT_RIGHT ? power : 0;
  if( sensor != RCOM_SENSOR_MOT_LEFT && sensor != RCOM_SENSOR_MOT_RIGHT )
    throw ERROR_BAD_ARG;
  
  //Forward
  int start_pos = rcomGet(1, sensor, cmd_opts)[0].enc.pos; //get the idle start position
  psr = rcomMot(100, sensor, pwrL, pwrR, 0, 0 , cmd_opts);
  test.fwd_mid = psr[49].enc.speed;
  for(int x=10; x<90; x++)
    test.fwd_avg += psr[x].enc.speed;
  test.fwd_avg /= (90-10);
  
  Timer::delayMs(50); //wait for tread to stop spinning
  int end_pos = rcomGet(1, sensor, cmd_opts)[0].enc.pos;
  test.fwd_travel = end_pos - start_pos;
  
  //Reverse
  start_pos = end_pos;
  psr = rcomMot(100, sensor, (-1)*pwrL, (-1)*pwrR, 0, 0 , cmd_opts);
  test.rev_mid = psr[49].enc.speed;
  for(int x=10; x<90; x++)
    test.rev_avg += psr[x].enc.speed;
  test.rev_avg /= (90-10);
  
  Timer::delayMs(50); //wait for tread to stop spinning
  end_pos = rcomGet(1, sensor, cmd_opts)[0].enc.pos;
  test.rev_travel = end_pos - start_pos;
  
  return &test;
}

typedef struct {
  int start_active;   //starting position, while motor is pushing to absolute limit
  int start_passive;  //starting position, with motor idle
} motor_limits_t;

void TestRobotMotors(void)
{
  motor_speed_t treadL = *tread_test_(RCOM_SENSOR_MOT_LEFT, 127);
  motor_speed_t treadR = *tread_test_(RCOM_SENSOR_MOT_RIGHT, -127);
  
  //check range of motion
  int lift_start = rcomMot(50, RCOM_SENSOR_MOT_LIFT, 0, 0, -100,    0 )[49].enc.pos; //start at bottom
  int lift_top   = rcomMot(35, RCOM_SENSOR_MOT_LIFT, 0, 0,  100,    0 )[34].enc.pos; //up
  int lift_bot   = rcomMot(35, RCOM_SENSOR_MOT_LIFT, 0, 0, -100,    0 )[34].enc.pos; //down
  int lift_travel_up = lift_top - lift_start;
  int lift_travel_down = lift_top - lift_bot;
  int head_start = rcomMot(65, RCOM_SENSOR_MOT_HEAD, 0, 0,    0, -127 )[64].enc.pos; //start at bottom
  int head_top   = rcomMot(65, RCOM_SENSOR_MOT_HEAD, 0, 0,    0,  100 )[64].enc.pos; //up
  int head_bot   = rcomMot(65, RCOM_SENSOR_MOT_HEAD, 0, 0,    0, -100 )[64].enc.pos; //down
  int head_travel_up    = head_top - head_start;
  int head_travel_down  = head_top - head_bot;
  
  ConsolePutChar('\n');
  ConsolePrintf("tread LEFT  fwd speed:%i avg:%i travel:%i\n", treadL.fwd_mid, treadL.fwd_avg, treadL.fwd_travel);
  ConsolePrintf("tread LEFT  rev speed:%i avg:%i travel:%i\n", treadL.rev_mid, treadL.rev_avg, treadL.rev_travel);
  ConsolePrintf("tread RIGHT fwd speed:%i avg:%i travel:%i\n", treadR.fwd_mid, treadR.fwd_avg, treadR.fwd_travel);
  ConsolePrintf("tread RIGHT rev speed:%i avg:%i travel:%i\n", treadR.rev_mid, treadR.rev_avg, treadR.rev_travel);
  ConsolePrintf("lift pos: start,up,down %i,%i,%i travel: up,down %i,%i\n", lift_start, lift_top, lift_bot, lift_travel_up, lift_travel_down );
  ConsolePrintf("head pos: start,up,down %i,%i,%i travel: up,down %i,%i\n", head_start, head_top, head_bot, head_travel_up, head_travel_down );
  ConsolePutChar('\n');
  
  const int min_speed = 1500; //we normally see 1700-2000
  if( treadL.fwd_avg < min_speed || (-1)*treadL.rev_avg < min_speed ) {
    ConsolePrintf("insufficient LEFT tread speed %i %i\n", treadL.fwd_avg, treadL.rev_avg);
    throw ERROR_MOTOR_LEFT; //ERROR_MOTOR_LEFT_SPEED
  }
  if( (-1)*treadR.fwd_avg < min_speed || treadR.rev_avg < min_speed ) {
    ConsolePrintf("insufficient RIGHT tread speed %i %i\n", treadR.fwd_avg, treadR.rev_avg);
    throw ERROR_MOTOR_RIGHT; //ERROR_MOTOR_RIGHT_SPEED
  }
  
  const int min_travel = 600;
  if( treadL.fwd_travel < min_travel || (-1)*treadL.rev_travel < min_travel ) {
    ConsolePrintf("insufficient LEFT tread travel %i %i\n", treadL.fwd_travel, treadL.rev_travel);
    throw ERROR_MOTOR_LEFT;
  }
  if( (-1)*treadR.fwd_travel < min_travel || treadR.rev_travel < min_travel ) {
    ConsolePrintf("insufficient RIGHT tread travel %i %i\n", treadR.fwd_travel, treadR.rev_travel);
    throw ERROR_MOTOR_RIGHT;
  }
  
  /*/XXX: calibration sample of 1, travel should be about -230
  lift_travel *= -1; //positive travel comparisons
  if( (-1)*lift_travel > 20 )
    throw ERROR_MOTOR_LIFT_BACKWARD;
  else if( lift_travel < 20 )
    throw ERROR_MOTOR_LIFT; //can't move?
  else if( lift_travel < 100 )
    throw ERROR_MOTOR_LIFT_RANGE; //moves, but not enough...
  else if( lift_travel > 300 )
    throw ERROR_MOTOR_LIFT_NOSTOP; //moves too much!
  //-*/
  
  /*/XXX: calibration sample of 1, travel should be about -570
  head_travel *= -1; //positive travel comparisons
  if( (-1)*head_travel > 20 )
    throw ERROR_MOTOR_HEAD_BACKWARD;
  else if( head_travel < 20 )
    throw ERROR_MOTOR_HEAD; //can't move?
  else if( head_travel < 400 )
    throw ERROR_MOTOR_HEAD_RANGE; //moves, but not enough...
  else if( head_travel > 700 )
    throw ERROR_MOTOR_HEAD_NOSTOP; //moves too much!
  //-*/
}

void TestRobotMotorsSlow(void)
{
}

void EmrChecks(void)
{
  //Make sure previous tests have passed
  if( g_fixmode == FIXMODE_ROBOT3 ) {
    //XXX: ROBOT1,2,MIC_TEST etc results in EMR.fixture[?]
  }
  if( g_fixmode == FIXMODE_PACKOUT ) {
    uint32_t ppReady  = rcomGmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG) );
    uint32_t ppPassed = rcomGmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) );
    if( ppReady != 1 || ppPassed != 1 ) {
      throw ERROR_ROBOT_TEST_SEQUENCE;
    }
  }
  
  //requrie retest on all downstream fixtures after rework
  if( g_fixmode == FIXMODE_ROBOT3 ) {
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 0 );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), 0 );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG), 0 );
  }
  if( g_fixmode == FIXMODE_PACKOUT ) {
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 0 );
  }
}

void EmrUpdate(void)
{
  if( g_fixmode == FIXMODE_ROBOT3 ) {
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG), 1 );
  }
  if( g_fixmode == FIXMODE_PACKOUT ) {
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 1 );
  }
}

//-----------------------------------------------------------------------------
//                  Recharge
//-----------------------------------------------------------------------------

//read battery voltage
int robot_get_batt_mv(void)
{
  int raw = rcomGet(1, RCOM_SENSOR_BATTERY)[0].bat.raw;
  int vBatMv = RCOM_BAT_RAW_TO_MV(raw);
  
  ConsolePrintf("vbat = %u.%03uV (%i)\n", vBatMv/1000, vBatMv%1000, raw);
  return vBatMv;
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
  Timer::delayMs(SYSCON_CHG_PWR_DELAY_MS); //delay for syscon to enable charger
  
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
int m_Recharge( uint16_t max_charge_time_s, uint16_t bat_limit_mv, uint16_t i_done_ma, bool dbgPrint )
{
  const u8 BAT_CHECK_INTERVAL_S = 90; //interrupt charging this often to test battery level
  ConsolePrintf("recharge,%dcV,%dmA,%ds\r\n", bat_limit_mv, i_done_ma, max_charge_time_s );
  
  Contacts::setModeRx();
  Timer::delayMs(500); //let battery voltage settle
  int batt_mv = robot_get_batt_mv(); //get initial battery level
  
  uint32_t Tstart = Timer::get();
  while( bat_limit_mv == 0 || batt_mv < bat_limit_mv )
  {
    if( max_charge_time_s > 0 && Timer::elapsedUs(Tstart) >= ((uint32_t)max_charge_time_s*1000*1000) )
      return RECHARGE_STATUS_TIMEOUT;
    
    //charge for awhile, then re-check battery voltage
    int chgStat = charge1_(BAT_CHECK_INTERVAL_S, i_done_ma, dbgPrint );
    ConsolePrintf("total charge time: %ds\n", Timer::elapsedUs(Tstart)/1000000 );
    Contacts::setModeRx();
    Timer::delayMs(500); //let battery voltage settle
    batt_mv = robot_get_batt_mv();
    
    //charge loop detected robot removal or charge completion?
    if( chgStat != RECHARGE_STATUS_TIMEOUT )
      return chgStat;
  }
  
  return RECHARGE_STATUS_OK;
}

void Recharge(void)
{
  const u16 BAT_MAX_CHARGE_TIME_S = 25*60;  //max amount of time to charge
  const u16 VBAT_CHARGE_LIMIT_MV = 3900;
  const u16 BAT_FULL_I_THRESH_MA = 200;     //current threshold for charging complete (experimental)
  int status;
  
  bool dbgPrint = g_fixmode == FIXMODE_RECHARGE0 ? 1 : 0;
  
  //Notes from test measurements (Cozmo) ->
  //conditions: 90s charge intervals, interrupted to measure vBat)
  //full charge (3.44V-4.15V) 1880s (31.3min)
  //typical charge (3.65V-3.92V) 990s (16.5min)
  
  if( g_fixmode == FIXMODE_RECHARGE0 )
    status = m_Recharge( 2*BAT_MAX_CHARGE_TIME_S, 0, BAT_FULL_I_THRESH_MA, dbgPrint ); //charge to full battery
  else
    status = m_Recharge( BAT_MAX_CHARGE_TIME_S, VBAT_CHARGE_LIMIT_MV, 0, dbgPrint ); //charge to specified voltage
  
  if( status == RECHARGE_STATUS_TIMEOUT )
    throw ERROR_TIMEOUT;
  
  if( status == RECHARGE_STATUS_OK )
    cmdSend(CMD_IO_CONTACTS, "powerdown", 50, CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_ERRORS | CMD_OPTS_EXCEPTION_EN));
}

extern void RobotChargeTest( u16 i_done_ma, u16 bat_overvolt_mv );
static void ChargeTest(void) {
  RobotChargeTest( 425, 4200 );
}

//Test charging circuit by verifying current draw
//@param i_done_ma - average current (min) to pass this test
//@param bat_overvolt_mv - battery too full voltage level. special failure handling above this threshold.
void RobotChargeTest( u16 i_done_ma, u16 bat_overvolt_mv )
{
  #define CHARGE_TEST_DEBUG(x)    x
  const int NUM_SAMPLES = 16;
  
  Contacts::setModeRx(); //switch to comm mode
  Timer::delayMs(500); //let battery voltage settle
  int batt_mv = robot_get_batt_mv(); //get initial battery level
  
  //Turn on charging power
  Board::powerOn(PWR_VEXT,0);
  Timer::delayMs(SYSCON_CHG_PWR_DELAY_MS); //delay for syscon to enable charger
  
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
  
  Contacts::setModeRx(); //switch to comm mode
  Timer::delayMs(500); //let battery voltage settle
  batt_mv = robot_get_batt_mv(); //get final battery level
  
  ConsolePrintf("charge-current-ma,%d,sample-cnt,%d\r\n", avg, avgCnt);
  ConsolePrintf("charge-current-dbg,avgMax,%d,%d,iMax,%d,%d\r\n", avgMax, avgMaxTime, iMax, iMaxTime);
  if( avgCnt >= NUM_SAMPLES && avg >= i_done_ma )
    return; //OK
  
  if( batt_mv >= bat_overvolt_mv ) {
    //const int BURN_TIME_S = 120;  //time to hit the gym and burn off a few pounds
    //ConsolePrintf("power-on,%ds\r\n", BURN_TIME_S);
    //robot_get_batt_mv(BURN_TIME_S,BURN_TIME_S); //reset power-on timer and set face to info screen for the duration
    throw ERROR_BAT_OVERVOLT; //error prompts operator to put robot aside for a bit
  }
  
  throw ERROR_BAT_CHARGER;
}

//-----------------------------------------------------------------------------
//                  Get Tests
//-----------------------------------------------------------------------------

void DBG_test_all(void) { dbg_test_all_(); }
void DBG_test_emr(void) { dbg_test_emr_(); }

TestFunction* TestRobot0GetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    //DBG_test_all,
    //DBG_test_emr,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot1GetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot2GetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    //DBG_test_emr,
    TestRobotSensors,
    //TestRobotMotors,
    ChargeTest,
    TestRobotSensors,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot3GetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    EmrChecks, //check previous test results and reset status flags
    TestRobotSensors,
    TestRobotMotors,
    ChargeTest,
    EmrUpdate, //set test complete flags
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotInfoGetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotPackoutGetTests(void) { 
  return TestRobot3GetTests();
}

TestFunction* TestRobotRechargeGetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    Recharge,
    NULL,
  };
  return m_tests;
}

