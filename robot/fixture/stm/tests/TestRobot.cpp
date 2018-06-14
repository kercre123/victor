#include <ctype.h>
#include <stdlib.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "emrf.h"
#include "flexflow.h"
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
  rcomGet(1, RCOM_SENSOR_DEBUG_INC); ConsolePutChar('\n');
  rcomRlg(0, 0, 0);
  rcomRlg(1, 0, 0);
}

static void dbg_test_emr_(bool blank_only=0, bool dont_clear=0);
static void dbg_test_emr_(bool blank_only, bool dont_clear)
{
  static uint32_t m_emr[256]; int idx;
  ConsolePrintf("EMR READ/WRITE TEST\n");
  
  //reset EMR to blank
  for(idx=1; idx < 256; idx++)
    rcomSmr(idx, 0);
  if( blank_only )
    return;
  
  //set EMR to random values, store locally for compare
  srand(Timer::get());
  for(idx=0; idx < 256; idx++)
  {
    if( !idx ) { //index 0 esn is non-writable
      m_emr[idx] = rcomGmr(idx);
      continue;
    }
    
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
  for(idx=1; idx < 256; idx++)
    rcomSmr(idx, 0);
  
  ConsolePrintf("EMR test %s: %u errors\n", mismatch > 0 ? "FAILED" : "passed", mismatch);
}

static void dbg_test_comm_loop_(int nloops, int rmin, int rmax)
{
  rmin = rmin<=0 ?   1 : rmin&0xff; //limit for random generator
  rmax = rmax<=0 ? 255 : rmax&0xff; //upper limit for random generator
  if( rmin > rmax )
    rmin = rmax;
  
  static int sensorSelect = -1;
  sensorSelect += 1;
  
  ConsolePrintf("STRESS TEST COMMS: %i loops, NN=rand{%i..%i}, sensor=%i\n", nloops, rmin, rmax, sensorSelect);
  int rmod = rmax-rmin+1; //modulo
  
  srand(Timer::get());
  for(int x = 0; x < nloops; x++)
  {
    rcomEsn(); rcomBsv();
    //if( sensorSelect == 0 ) {}
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_BATTERY);
    //else if( sensorSelect == 1 )
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_CLIFF);
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_LEFT);
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_RIGHT);
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_LIFT);
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_MOT_HEAD);
    //else if( sensorSelect == 2 )
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_PROX_TOF);
    //else if( sensorSelect == 3 )
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_BTN_TOUCH);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_RSSI);
    //rcomGet(rmin+rand()%rmod, RCOM_SENSOR_RX_PKT);
    //else if( sensorSelect == 4 )
      rcomGet(rmin+rand()%rmod, RCOM_SENSOR_DEBUG_INC);
    //else { sensorSelect = -1; break; }    
    //rcomRlg(0, 0, 0);
    //rcomRlg(1, 0, 0);
    try { for(int x=0; x<=8; x++) { rcomRlg(x, 0, 0); } } catch(int e) {}
  }
}

static void dbg_test_readlogs_(int first, int last)
{
  first = first<0 ? 0 : first;
  last = last<first ? first : last;
  ConsolePrintf("READ LOGS {%i..%i}\n", first, last);
  
  error_t err = ERROR_OK;
  for(int i=first; i<=last; i++) {
    try { rcomRlg(i, 0, 0); } catch(int e) { err = e; }
  }
  if( err != ERROR_OK )
    throw err;
}

static void dbg_test_leds_(int on_time_ms, int colorfade)
{
  on_time_ms = on_time_ms < 1 ? 500 : on_time_ms;
  ConsolePrintf("LED Test (%ims)\n", on_time_ms);
  uint8_t leds[12]; memset(leds,0,sizeof(leds));
  
  //color cycle
  for(int i=0; i<12; i++) {
    memset(leds,0,sizeof(leds));
    leds[i] = 255;
    rcomLed(leds); Timer::delayMs(on_time_ms);
  }
  
  //color fade
  if( colorfade > 0 ) {
    for(int i=0; i<3; i++) {
      memset(leds,0,sizeof(leds));
      const int delay_ms = 50;
      for(int bright=0x00; bright < 0xF0; bright+=0x10) {
        leds[0+i] = leds[3+i] = leds[6+i] = leds[9+i] = bright;
        rcomLed(leds, RCOM_PRINT_LEVEL_CMD); Timer::delayMs(delay_ms);
      }
      for(int bright=0xF0; bright >= 0x10; bright-=0x10) {
        leds[0+i] = leds[3+i] = leds[6+i] = leds[9+i] = bright;
        rcomLed(leds, RCOM_PRINT_LEVEL_CMD); Timer::delayMs(delay_ms);
      }
    }
  }
  
  memset(leds,0,sizeof(leds));
  rcomLed(leds, RCOM_PRINT_LEVEL_CMD);
}

static void run_debug(int arg[])
{
  if( arg[0] == 1 )
    dbg_test_all_();
  if( arg[0] == 2 )
    dbg_test_comm_loop_(arg[1], arg[2], arg[3]);
  if( arg[0] == 3 )
    dbg_test_readlogs_(arg[1], arg[2]);
  if( arg[0] == 4 )
    dbg_test_leds_(arg[1], arg[2]);
  
  if( arg[0] == 14 )
    dbg_test_emr_( arg[1], arg[2] );
}

const char* DBG_cmd_substitution(const char *line, int len)
{
  static char buf[25];
  
  //parse cmd line args (ints)
  int larg[6];
  int nargs = cmdNumArgs((char*)line);
  for(int x=0; x<sizeof(larg)/sizeof(int); x++)
    larg[x] = nargs > x+1 ? cmdParseInt32(cmdGetArg((char*)line,x+1) ) : 0;
  
  if( !strcmp(line, "esn") )
    return ">>esn 00 00 00 00 00 00";
  if( !strcmp(line, "bsv") )
    return ">>bsv 00 00 00 00 00 00";
  if( !strcmp(line, "mot") )
    return ">>mot ff 03";
  if( !strcmp(line, "get") )
    return ">>get 01 00"; 
  if( !strncmp(line,"eng",3) || !strncmp(line,"pwr",3) || !strncmp(line,"rlg",3) )
    return snformat(buf,sizeof(buf),">>%c%c%c %02x %02x %02x %02x %02x %02x", line[0],line[1],line[2],larg[0],larg[1],larg[2],larg[3],larg[4],larg[5]);
  if( !strncmp(line,"smr",3) || !strncmp(line,"gmr",3) ) {
    int ix  = larg[0];
    int val = larg[1];
    return snformat(buf,sizeof(buf),">>%c%c%c %02x %02x %02x %02x %02x 00", line[0],line[1],line[2], ix, val&0xFF, (val>>8)&0xff, (val>>16)&0xff, (val>>24)&0xff);
  }
  if( !strncmp(line,"debug",5) ) {
    int dbgErr = 0;
    ConsolePrintf("========== DEBUG %i %i %i %i %i %i ==========\n", larg[0], larg[1], larg[2], larg[3], larg[4], larg[5]);
    try { run_debug(larg); } catch(int e) { dbgErr = e; }
    ConsolePrintf("========== DEBUG %s e%03d ===================\n", !dbgErr ? "OK" : "FAIL", dbgErr);
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

#define IS_FIXMODE_ROBOT1()   ( g_fixmode==FIXMODE_ROBOT1  || g_fixmode==FIXMODE_ROBOT1_OL )
#define IS_FIXMODE_ROBOT3()   ( g_fixmode==FIXMODE_ROBOT3  || g_fixmode==FIXMODE_ROBOT3_OL )
#define IS_FIXMODE_PACKOUT()  ( g_fixmode==FIXMODE_PACKOUT || g_fixmode==FIXMODE_PACKOUT_OL )
#define IS_FIXMODE_OFFLINE()  ( g_fixmode==FIXMODE_ROBOT1_OL || g_fixmode==FIXMODE_ROBOT3_OL || g_fixmode==FIXMODE_PACKOUT_OL )

int detect_ma = 0, detect_mv = 0;

//test info reported to flexflow
const int numlogs = 2;
static struct {
  uint32_t      esn, hwver, model, packoutdate;
  robot_bsv_t   bsv;
  char         *log[numlogs];
  int           loglen[numlogs];
  int           bat_mv, bat_raw;
} flexnfo;

int cleanup_preserve_vext = 0;
bool TestRobotDetect(void)
{
  //on test cleanup/exit, let charger kick back in so we can properly detect removal
  int chargeDelay = (g_isDevicePresent && !Board::powerIsOn(PWR_VEXT)) ? SYSCON_CHG_PWR_DELAY_MS : 0; //condition from app::WaitForDeviceOff()
  
  Board::powerOn(PWR_VEXT, chargeDelay);
  //Board::powerOn(PWR_VBAT); //XXX DEBUG
  cleanup_preserve_vext = 0;
  
  int i_ma = Meter::getCurrentMa(PWR_VEXT,0);
  
  //running average filter, len=2^oversample
  const int oversample = 6;
  detect_ma = ( ((1<<oversample)-1)*detect_ma + i_ma ) >> oversample;
  
  // Hysteresis for shut-down robots
  return i_ma > DETECT_CURRENT_MA || (i_ma > PRESENT_CURRENT_MA && g_isDevicePresent);
}

void TestRobotDetectSpine(void)
{
  //ROBOT1 connected to stump via spine cable. Check for power input.
  const int spine_voltage_low = 3100;
  const int spine_voltage_high = 5100;
  
  rcomSetTarget(1); //rcom -> spine cable (init's DUT_UART)
  
  //wait for syscon to turn on head power
  uint32_t Tstart = Timer::get();
  while( Timer::elapsedUs(Tstart) < 2*1000*1000 )
  {
    //kick syscon out of bed
    rcomPwr(RCOM_PWR_ON, RCOM_PRINT_LEVEL_NONE); //RCOM_PRINT_LEVEL_DEFAULT
    
    detect_mv = Meter::getVoltageMv(PWR_DUTVDD,4); //6);
    if( detect_mv >= spine_voltage_low && detect_mv <= spine_voltage_high )
      break;
  }
  
  ConsolePrintf("spine voltage %imV\n", detect_mv);
  if( detect_mv < spine_voltage_low || detect_mv > spine_voltage_high )
    throw ERROR_SPINE_POWER;
  
  //check for packet response. give syscon app time to boot
  Tstart = Timer::get();
  bool working = 0;
  while( !working )
  {
    if( Timer::elapsedUs(Tstart) > 3.5*1000*1000 )
      throw ERROR_SPINE_CMD_TIMEOUT;
    
    rcomPwr(RCOM_PWR_ON, RCOM_PRINT_LEVEL_NONE); //kick syscon out of bed
    try { 
      rcomGet(1, RCOM_SENSOR_BTN_TOUCH, RCOM_PRINT_LEVEL_NONE);
      working = 1;
    } catch(...){
    }
  }
  
  ConsolePrintf("spine comms established\n");
}

void TestRobotCleanup(void)
{
  detect_ma = 0;
  detect_mv = 0;
  memset( &flexnfo, 0, sizeof(flexnfo) );
  
  if( !cleanup_preserve_vext )
    Board::powerOff(PWR_VEXT);
  //else ConsolePrintf("cleanup_preserve_vext\n"); //DEBUG
  cleanup_preserve_vext = 0;
  
  Board::powerOff(PWR_VBAT);
  //ConsolePrintf("----DBG: i=%imA\n", Meter::getCurrentMa(PWR_VEXT,4) );
  rcomSetTarget(0); //reset rcom to charge contacts (de-init's spine layers)
}

void read_robot_info_(void)
{
  if( IS_FIXMODE_ROBOT1() ) {
    //memset( &flexnfo, 0, sizeof(flexnfo) );
    flexnfo.bsv = *rcomBsv();
    rcomPrintBsv(&flexnfo.bsv); //log
    
  } else {
    uint32_t esnCmd = rcomEsn(); //not really necesary anymore since GMR[0] is esn
    flexnfo.bsv = *rcomBsv();
    
    flexnfo.esn           = rcomGmr( EMR_FIELD_OFS(ESN) );
    flexnfo.hwver         = rcomGmr( EMR_FIELD_OFS(HW_VER) );
    flexnfo.model         = rcomGmr( EMR_FIELD_OFS(MODEL) );
    uint32_t lotcode      = rcomGmr( EMR_FIELD_OFS(LOT_CODE) );
    uint32_t playpenready = rcomGmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG) );
    uint32_t playpenpass  = rcomGmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) );
    uint32_t packedout    = rcomGmr( EMR_FIELD_OFS(PACKED_OUT_FLAG) );
    flexnfo.packoutdate   = rcomGmr( EMR_FIELD_OFS(PACKED_OUT_DATE) );
    
    ConsolePrintf("EMR[%u] esn         :%08x [%08x]\n", EMR_FIELD_OFS(ESN), flexnfo.esn, esnCmd);
    ConsolePrintf("EMR[%u] hwver       :%u\n", EMR_FIELD_OFS(HW_VER), flexnfo.hwver);
    ConsolePrintf("EMR[%u] model       :%u\n", EMR_FIELD_OFS(MODEL), flexnfo.model);
    ConsolePrintf("EMR[%u] lotcode     :%u\n", EMR_FIELD_OFS(LOT_CODE), lotcode);
    ConsolePrintf("EMR[%u] playpenready:%u\n", EMR_FIELD_OFS(PLAYPEN_READY_FLAG), playpenready);
    ConsolePrintf("EMR[%u] playpenpass :%u\n", EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), playpenpass);
    ConsolePrintf("EMR[%u] packedout   :%u\n", EMR_FIELD_OFS(PACKED_OUT_FLAG), packedout);
    ConsolePrintf("EMR[%u] packout-date:%u\n", EMR_FIELD_OFS(PACKED_OUT_DATE), flexnfo.packoutdate);
  }
}

//read battery voltage
static int robot_get_batt_mv(int *out_raw)
{
  int bat_raw = rcomGet(1, RCOM_SENSOR_BATTERY)[0].bat.raw;
  int bat_mv = RCOM_BAT_RAW_TO_MV(bat_raw);
  if( out_raw ) *out_raw = bat_raw;
  
  ConsolePrintf("vbat = %imV (%i)\n", bat_mv, bat_raw);
  return bat_mv;
}

//always run this first after detect, to get into comms mode
void TestRobotInfo(void)
{
  //Board::powerOn(PWR_VBAT); //XXX Debug: work on body pcba w/o battery
  ConsolePrintf("detect current avg %i mA\n", detect_ma);
  
  if( !IS_FIXMODE_ROBOT1() ) {
    ConsolePrintf("Resetting comms interface\n");
    Board::powerOff(PWR_VEXT, 500); //turn power off to disable charging
    Contacts::setModeRx();
  }
  
  //DEBUG: console bridge, manual testing
  if( g_fixmode == FIXMODE_ROBOT0 )
    TestCommon::consoleBridge(TO_CONTACTS, 0, 0, BRIDGE_OPT_LINEBUFFER, DBG_cmd_substitution);
  //-*/
  
  read_robot_info_();
  robot_get_batt_mv(0);
}

//robot_sr_t get(uint8_t NN, uint8_t sensor
void TestRobotSensors(void)
{
  robot_sr_t bat    = rcomGet(3, RCOM_SENSOR_BATTERY   )[1];
  robot_sr_t cliff  = rcomGet(3, RCOM_SENSOR_CLIFF     )[1];
  robot_sr_t prox   = rcomGet(3, RCOM_SENSOR_PROX_TOF  )[1];
  robot_sr_t btn    = rcomGet(3, RCOM_SENSOR_BTN_TOUCH )[1];
  
  ConsolePrintf("Sensor Values:\n");
  ConsolePrintf("  battery = %i.%03iV\n", bat.bat.raw/1000, bat.bat.raw%1000);
  ConsolePrintf("  cliff = fL:%i fR:%i bR:%i bL:%i\n", cliff.cliff.fL, cliff.cliff.fR, cliff.cliff.bR, cliff.cliff.bL);
  ConsolePrintf("  prox = %imm sigRate:%i spad:%i ambientRate:%i\n", prox.prox.rangeMM, prox.prox.signalRate, prox.prox.spadCnt, prox.prox.ambientRate);
  ConsolePrintf("  btn = %i touch=%i\n", btn.btn.btn, btn.btn.touch);
  
  //XXX: what should "good" sensor values look like?
  
}

typedef struct { 
  int fwd_mid; int fwd_avg; int fwd_travel;
  int rev_mid; int rev_avg; int rev_travel;
} robot_tread_dat_t;

void print_tread_dat(robot_tread_dat_t* dat, const char* treadname) {
  const char *name = treadname ? treadname : "(NULL)";
  ConsolePrintf("tread %s FWD speed:%+i avg:%+i travel:%+i\n", name, dat->fwd_mid, dat->fwd_avg, dat->fwd_travel);
  ConsolePrintf("tread %s REV speed:%+i avg:%+i travel:%+i\n", name, dat->rev_mid, dat->rev_avg, dat->rev_travel);
}

//measure tread speed, distance etc.
static robot_tread_dat_t* robot_tread_test_(uint8_t sensor, int8_t power)
{
  power = ABS(power);
  if( sensor != RCOM_SENSOR_MOT_LEFT && sensor != RCOM_SENSOR_MOT_RIGHT ) {
    ConsolePrintf("BAD_ARG: robot_tread_test_() sensor=%i\n", sensor);
    throw ERROR_BAD_ARG;
  }
  
  const bool left = (sensor == RCOM_SENSOR_MOT_LEFT);
  const int printlvl = g_isReleaseBuild ? RCOM_PRINT_LEVEL_CMD : RCOM_PRINT_LEVEL_CMD_DAT_RSP;
    //#warning "TREAD TEST DEBUG"
    //const int printlvl = RCOM_PRINT_LEVEL_CMD;
  static robot_tread_dat_t test;
  memset(&test, 0, sizeof(test));
  robot_sr_t* psr;
  int8_t pwrL = left  ? power : 0;
  int8_t pwrR = !left ? power : 0;
  
  ConsolePrintf("%s tread test. power %i\n", left?"LEFT":"RIGHT", (left ? pwrL : pwrR));
  
  //Forward
  int start_pos = rcomGet(1, sensor, printlvl)[0].enc.pos; //get the idle start position
  psr = rcomMot(100, sensor, pwrL, pwrR, 0, 0 , printlvl);
  test.fwd_mid = psr[49].enc.speed;
  for(int x=10; x<90; x++)
    test.fwd_avg += psr[x].enc.speed;
  test.fwd_avg /= (90-10);
  
  Timer::delayMs(50); //wait for tread to stop spinning
  int end_pos = rcomGet(1, sensor, printlvl)[0].enc.pos;
  test.fwd_travel = end_pos - start_pos;
  
  //Reverse
  start_pos = end_pos;
  psr = rcomMot(100, sensor, (-1)*pwrL, (-1)*pwrR, 0, 0 , printlvl);
  test.rev_mid = psr[49].enc.speed;
  for(int x=10; x<90; x++)
    test.rev_avg += psr[x].enc.speed;
  test.rev_avg /= (90-10);
  
  Timer::delayMs(50); //wait for tread to stop spinning
  end_pos = rcomGet(1, sensor, printlvl)[0].enc.pos;
  test.rev_travel = end_pos - start_pos;
  
  return &test;
}

#define TREAD_TEST_DATA_GATHERING 0
static void TestRobotTreads_(int8_t power, int min_speed, int min_travel)
{
  robot_tread_dat_t treadL = *robot_tread_test_(RCOM_SENSOR_MOT_LEFT, power);
  robot_tread_dat_t treadR = *robot_tread_test_(RCOM_SENSOR_MOT_RIGHT, -power);
  print_tread_dat(&treadL, "LEFT ");
  print_tread_dat(&treadR, "RIGHT");
  
  #if TREAD_TEST_DATA_GATHERING > 0
  #warning "TREAD ERROR CHECKING DISABLED"
  #else
  
  if( treadL.fwd_avg < min_speed || (-1)*treadL.rev_avg < min_speed ) {
    ConsolePrintf("insufficient LEFT tread speed %i,%i < %i\n", treadL.fwd_avg, treadL.rev_avg, min_speed);
    throw ERROR_MOTOR_LEFT; //ERROR_MOTOR_LEFT_SPEED
  }
  if( treadR.fwd_avg < min_speed || (-1)*treadR.rev_avg < min_speed ) {
    ConsolePrintf("insufficient RIGHT tread speed %i,%i < %i\n", treadR.fwd_avg, treadR.rev_avg, min_speed);
    throw ERROR_MOTOR_RIGHT; //ERROR_MOTOR_RIGHT_SPEED
  }
  if( treadL.fwd_travel < min_travel || (-1)*treadL.rev_travel < min_travel ) {
    ConsolePrintf("insufficient LEFT tread travel %i,%i < %i\n", treadL.fwd_travel, treadL.rev_travel, min_travel);
    throw ERROR_MOTOR_LEFT;
  }
  if( treadR.fwd_travel < min_travel || (-1)*treadR.rev_travel < min_travel ) {
    ConsolePrintf("insufficient RIGHT tread travel %i,%i < %i\n", treadR.fwd_travel, treadR.rev_travel, min_travel);
    throw ERROR_MOTOR_RIGHT;
  }
  #endif
}

void TestRobotTreads(void)
{
  #if TREAD_TEST_DATA_GATHERING > 0
  #warning "TREAD TEST"
  for(int pwr = 127; pwr >= 60; pwr -= 5) {
    ConsolePrintf("TREAD TEST pwr = %i\n", pwr);
    TestRobotTreads_(pwr, 9999, 9999);
  }
    
  #else
  
  //full power: speed 1760-1980, travel 790-1160
  TestRobotTreads_(127, 1500, 600);
  
  //low power (72): speed 870-1070, travel 400-550
  if( !IS_FIXMODE_PACKOUT() )
    TestRobotTreads_(75, 750, 300);
  
  #endif
}

typedef struct {
  int start_active, start_passive; //starting positions: active = motor is pushing to absolute limit, passive = with motor idle
  int up_mid, up_avg, up_travel;   //median/avg speed + travel distance
  int dn_mid, dn_avg, dn_travel;   //"
} robot_range_dat_t;

void print_range_dat(robot_range_dat_t* dat, const char* sensorname) {
  const char *name = sensorname ? sensorname : "(NULL)";
  ConsolePrintf("%s POS start:%i passive:%i delta:%i\n", name, dat->start_active, dat->start_passive, dat->start_active-dat->start_passive);
  ConsolePrintf("%s UP  speed:%+i avg:%+i travel:%+i\n", name, dat->up_mid, dat->up_avg, dat->up_travel);
  ConsolePrintf("%s DN  speed:%+i avg:%+i travel:%+i\n", name, dat->dn_mid, dat->dn_avg, dat->dn_travel);
}

//measure speed and range of motion
static robot_range_dat_t* robot_range_test_(uint8_t sensor, int8_t power)
{
  power = ABS(power);
  if( sensor != RCOM_SENSOR_MOT_LIFT && sensor != RCOM_SENSOR_MOT_HEAD ) {
    ConsolePrintf("BAD_ARG: robot_range_test_() sensor=%i\n", sensor);
    throw ERROR_BAD_ARG;
  }
  
  const bool lift = (sensor == RCOM_SENSOR_MOT_LIFT);
  const bool DEBUG_PRINT = !g_isReleaseBuild; 
  const int printlvl = g_isReleaseBuild ? RCOM_PRINT_LEVEL_CMD : RCOM_PRINT_LEVEL_CMD_DAT_RSP;
    //#warning "RANGE TEST DEBUG"
    //const bool DEBUG_PRINT = 0; const int printlvl = RCOM_PRINT_LEVEL_CMD;
  static robot_range_dat_t test;
  memset(&test, 0, sizeof(test));
  robot_sr_t* psr;
  int8_t liftPwr    = lift  ? MIN(power, 80) : 0;
  int8_t liftPwrMax = lift  ?            80  : 0;
  int8_t headPwr    = !lift ? MIN(power,100) : 0;
  int8_t headPwrMax = !lift ?           100  : 0;
  
  ConsolePrintf("%s range test. power %i\n", lift?"LIFT":"HEAD", (lift ? liftPwr : headPwr));
  
  //calibrated test params for lift vs head
  uint8_t NNstart = lift ? 35 : 65;
  uint8_t NNtest  = lift ? (power >= 75 ? 55 : 85) : (power >= 90 ? 70 : 125);
  const uint8_t NNsettle = 50;
  
  //force to known starting position
  if( !IS_FIXMODE_ROBOT1() ) {
    if( DEBUG_PRINT ) ConsolePrintf("%s move to starting position [%i,%i]\n", lift ? "LIFT" : "HEAD", -liftPwrMax, -headPwrMax);
    test.start_active = rcomMot(NNstart, sensor, 0, 0, -liftPwrMax, -headPwrMax, printlvl)[NNstart-1].enc.pos;
  }
  if( DEBUG_PRINT ) ConsolePrintf("%s get passive start position\n", lift ? "LIFT" : "HEAD");
  test.start_passive = rcomGet(NNsettle, sensor, printlvl)[NNsettle-1].enc.pos; //allow time for mechanics to settle
  if( IS_FIXMODE_ROBOT1() )
    test.start_active = test.start_passive;
  
  //move up
  if( DEBUG_PRINT ) ConsolePrintf("%s move up [%i,%i]\n", lift ? "LIFT" : "HEAD", liftPwr, headPwr);
  psr = rcomMot(NNtest, sensor, 0, 0, liftPwr, headPwr, printlvl );
  int avg_cnt = 0;
  for(int x=1; x<NNtest; x++) { if( psr[x].enc.speed > 0 ) { avg_cnt++; test.up_avg += psr[x].enc.speed; } }
  test.up_avg = avg_cnt > 0 ? test.up_avg/avg_cnt : 0;
  test.up_mid = psr[avg_cnt>>1].enc.speed; //rough midnpoint of motion
  test.up_travel = psr[NNtest-1].enc.pos - psr[0].enc.pos;
  
  //force to top (sticky or power too low)
  //if( DEBUG_PRINT ) ConsolePrintf("%s force up [%i,%i]\n", lift ? "LIFT" : "HEAD", liftPwrMax, headPwrMax);
  //const uint8_t NNadjust = (power >= 100 ? 15 : 30);
  //rcomMot(NNadjust, sensor, 0, 0, liftPwrMax, headPwrMax, printlvl );
  
  //move down
  if( DEBUG_PRINT ) ConsolePrintf("%s move down [%i,%i]\n", lift ? "LIFT" : "HEAD", -liftPwr, -headPwr);
  psr = rcomMot(NNtest, sensor, 0, 0, -liftPwr, -headPwr, printlvl );
  avg_cnt = 0;
  for(int x=1; x<NNtest; x++) {
    psr[x].enc.speed *= (-1); //XXX: workaround for syscon bug, reports ABS(speed) for head/lift
    if( psr[x].enc.speed < 0 ) { avg_cnt++; test.dn_avg += psr[x].enc.speed; }
  }
  test.dn_avg = avg_cnt > 0 ? test.dn_avg/avg_cnt : 0;
  test.dn_mid = psr[avg_cnt>>1].enc.speed; //rough midnpoint of motion
  test.dn_travel = psr[NNtest-1].enc.pos - psr[0].enc.pos;
  
  //engine PID settle
  //if( DEBUG_PRINT ) ConsolePrintf("%s settling time\n", lift ? "LIFT" : "HEAD");
  //rcomGet(NNsettle, sensor, printlvl);
  
  return &test;
}

#define RANGE_TEST_DATA_GATHERING 0
typedef struct { int8_t power; int travel_min; int travel_max; int speed_min; } robot_range_t;
void TestRobotRange(robot_range_t *testlift, robot_range_t *testhead)
{
  robot_range_dat_t lift = *robot_range_test_(RCOM_SENSOR_MOT_LIFT, testlift->power);
  robot_range_dat_t head = *robot_range_test_(RCOM_SENSOR_MOT_HEAD, testhead->power);
  print_range_dat(&lift, "LIFT");
  print_range_dat(&head, "HEAD");

  #if RANGE_TEST_DATA_GATHERING > 0
  #warning "RANGE ERROR CHECKING DISABLED"
  #else
  
  const int lift_start_delta_max = 50;
  lift.dn_travel *= -1; lift.dn_avg *= -1; //positive comparisons
  if( lift.up_travel < -20 || lift.dn_travel < -20 )
    throw ERROR_MOTOR_LIFT_BACKWARD;
  else if( lift.up_travel < 20 || lift.dn_travel < 20 )
    throw ERROR_MOTOR_LIFT; //can't move?
  else if( lift.up_travel < testlift->travel_min || lift.dn_travel < testlift->travel_min )
    throw ERROR_MOTOR_LIFT_RANGE; //moves, but not enough...
  else if( lift.up_travel > testlift->travel_max || lift.dn_travel > testlift->travel_max )
    throw ERROR_MOTOR_LIFT_NOSTOP; //moves too much!
  else if( testlift->speed_min > 0 && (lift.up_avg < testlift->speed_min || lift.dn_avg < testlift->speed_min) )
    throw ERROR_MOTOR_LIFT_SPEED;
  else if( ABS(lift.start_active-lift.start_passive) > lift_start_delta_max )
    throw ERROR_MOTOR_LIFT_RANGE; //range met only when force applied
  
  const int head_start_delta_max = 50;
  head.dn_travel *= -1; head.dn_avg *= -1; //positive comparisons
  if( head.up_travel < -20 || head.dn_travel < -20 )
    throw ERROR_MOTOR_HEAD_BACKWARD;
  else if( head.up_travel < 20 || head.dn_travel < 20 )
    throw ERROR_MOTOR_HEAD; //can't move?
  else if( head.up_travel < testhead->travel_min || head.dn_travel < testhead->travel_min )
    throw ERROR_MOTOR_HEAD_RANGE; //moves, but not enough...
  else if( head.up_travel > testhead->travel_max || head.dn_travel > testhead->travel_max )
    throw ERROR_MOTOR_HEAD_NOSTOP; //moves too much!
  else if( testhead->speed_min > 0 && (head.up_avg < testhead->speed_min || head.dn_avg < testhead->speed_min) )
    throw ERROR_MOTOR_HEAD_SPEED;
  else if( ABS(head.start_active-head.start_passive) > head_start_delta_max )
    throw ERROR_MOTOR_HEAD_RANGE; //range met only when force applied
  
  #endif
}

void TestRobotRange(void)
{
  #if RANGE_TEST_DATA_GATHERING > 0
  #warning "RANGE TESTING"
  for(int pwr=100; pwr >= 45; pwr -= 5) {
    if( pwr >= 100 || (pwr < 71 && pwr > 39) ) { //DEBUG limited range of values
      ConsolePrintf("RANGE TEST pwr = %i\n", pwr);
      robot_range_t lift = { /*power*/ pwr, /*travel_min*/ 0, /*travel_max*/ 99999, /*speed_min*/ 0 };
      robot_range_t head = { /*power*/ pwr, /*travel_min*/ 0, /*travel_max*/ 99999, /*speed_min*/ 0 };
      TestRobotRange( &lift, &head );
    }
  }
  
  #else
  
  //High Power
  if( IS_FIXMODE_ROBOT1() ) {
    //NO HEAD/ARMS ATTACHED = NO STOP!
    //lift travel ~450-500 in each direction
    //head travel ~800-850 in each direction
    robot_range_t lift = { /*power*/  75, /*travel_min*/ 400, /*travel_max*/ 9999, /*speed_min*/ 1800 };
    robot_range_t head = { /*power*/ 100, /*travel_min*/ 700, /*travel_max*/ 9999, /*speed_min*/ 2300 };
    TestRobotRange( &lift, &head );
  } else if( !IS_FIXMODE_PACKOUT() ) {
    //lift: travel 195-200, speed 760-1600
    //head: travel 550-560, speed 2130-2400
    robot_range_t lift = { /*power*/  75, /*travel_min*/ 170, /*travel_max*/ 230, /*speed_min*/  650 };
    robot_range_t head = { /*power*/ 100, /*travel_min*/ 520, /*travel_max*/ 590, /*speed_min*/ 1700 };
    TestRobotRange( &lift, &head );
  }
  
  //Low Power
  if( IS_FIXMODE_ROBOT1() ) {
    //NO HEAD/ARMS ATTACHED = NO STOP!
    //lift travel ~550-600 in each direction
    //head travel ~650-??? in each direction
    robot_range_t lift = { /*power*/  50, /*travel_min*/ 400, /*travel_max*/ 9999, /*speed_min*/ 800 };
    robot_range_t head = { /*power*/  55, /*travel_min*/ 550, /*travel_max*/ 9999, /*speed_min*/ 700 };
    TestRobotRange( &lift, &head );
  } else {
    //lift: travel 190-200, speed 580-1520
    //head: travel 545-555, speed 900-1230
    robot_range_t lift = { /*power*/  65, /*travel_min*/ 170, /*travel_max*/  230, /*speed_min*/ 400 };
    robot_range_t head = { /*power*/  60, /*travel_min*/ 520, /*travel_max*/  590, /*speed_min*/ 700 };
    TestRobotRange( &lift, &head );
  }
  
  #endif
}

void EmrChecks(void)
{
  //Make sure previous tests have passed
  if( IS_FIXMODE_ROBOT3() ) {
    //no previous. first fixture with head attached
  }
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() ) {
    uint32_t ppReady  = rcomGmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG) );
    uint32_t ppPassed = rcomGmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) );
    if( ppReady != 1 || ppPassed != 1 ) {
      throw ERROR_ROBOT_TEST_SEQUENCE;
    }
  }
  
  //require retest on all downstream fixtures after rework
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() ) {
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 0 );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), 0 );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG), 0 );
  }
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() ) {
    //will throw error if Robit has been packed out
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 0 );
  }
}

void EmrUpdate(void)
{
  //from clad/src/clad/types/factoryTestTypes.clad
  uint32_t PlaypenTestMask = 0
    | 0x00000001  //BackpackElectricalError
    | 0x00000002  //UnexpectedTouchDetectedError 
    | 0x00000004  //NoisyTouchSensorError
    //| 0x00000008  //CubeRadioError
  ;
  
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() ) {
    ConsolePrintf("playpen test mask 0x%08x\n", PlaypenTestMask);
    rcomSmr( EMR_FIELD_OFS(playpenTestDisableMask), PlaypenTestMask );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG), 1 );
  }
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() ) {
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 1 );
  }
}

void RobotPowerDown(void)
{
  ConsolePrintf("robot power down\n");
  
  if( !IS_FIXMODE_ROBOT1() )
  {
    rcomPwr(RCOM_PWR_OFF);
    Contacts::powerOn(); //immdediately turn on power to prevent rebooting
    cleanup_preserve_vext = 1; //leave power on for removal detection (no cleanup pwr cycle)
    
    /*/DEBUG:
    ConsolePrintf("delay for manual powerdown check. press a key to skip\n");
    uint32_t Tstart = Timer::get();
    while( ConsoleReadChar() > -1 );
    while( Timer::elapsedUs(Tstart) < 10*1000*1000 ) { if( ConsoleReadChar() > -1 ) break; }
    //-*/
  }
  else
  {
    Contacts::powerOn(); //turn on power to prevent rebooting
    cleanup_preserve_vext = 1; //leave power on for removal detection (no cleanup pwr cycle)
    Timer::delayMs(SYSCON_CHG_PWR_DELAY_MS+100); //wait for charger to kick in (known state)
    
    rcomPwr(RCOM_PWR_OFF);
    
    //wait for syscon to turn off head power
    int spine_mv, cnt = 0;
    uint32_t Tstart = Timer::get();
    do {
      rcomPwr(RCOM_PWR_OFF, RCOM_PRINT_LEVEL_NONE);
      spine_mv = Meter::getVoltageMv(PWR_DUTVDD,5);
      cnt = spine_mv < 250 ? cnt+1 : 0;
    } while( cnt < 4 && Timer::elapsedUs(Tstart) < 2.5*1000*1000 );
    
    ConsolePrintf("spine off voltage %imV\n", spine_mv);
    if( spine_mv > 250 )
      throw ERROR_BODY_CANNOT_POWER_OFF;
    
    /*/DEBUG
    ConsolePrintf("Power Off Debug:\n");
    for(int wait=5; wait >0; wait--) {
      ConsolePrintf("%is spine_mv=%i VEXT=%s\n", wait, Meter::getVoltageMv(PWR_DUTVDD,5), Contacts::powerIsOn()?"on":"off");
      Timer::delayMs(1000);
    }//-*/
  }
}

//-----------------------------------------------------------------------------
//                  Button
//-----------------------------------------------------------------------------

static void led_manage_(int reset, int frame_period = 250*1000, int printlvl = RCOM_PRINT_LEVEL_NONE);
static void led_manage_(int reset, int frame_period, int printlvl)
{
  static const uint8_t leds[][12] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, //off
    {0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00}, //R.0,1,2
    {0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00}, //G.0,1,2
    {0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00}  //B.0,1,2
    
    /*/DEBUG: ROBOT1
    ,
    {0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00}, //R.0,1,2
    {0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0x00}, //G.0,1,2
    {0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00},  //B.0,1,2
    {0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00} //-*/
  };
  const int led_frame_count = sizeof(leds)/12;
  static uint32_t Tframe=0, idx=0, last_idx=0;
  
  if( reset & 1 ) {
    int resetCnt = IS_FIXMODE_ROBOT1() ? 10 : 1;
    for(int x=0; x < resetCnt; x++)
      rcomLed( (uint8_t*)leds[0], !x ? printlvl : RCOM_PRINT_LEVEL_NONE );
    Tframe=0, idx=0, last_idx=0;
    return;
  }
  
  if( !Tframe || Timer::elapsedUs(Tframe) > frame_period ) {
    Tframe = Timer::get();
    idx = idx+1 >= led_frame_count ? 1 : idx+1;
  }
  
  if( idx != last_idx ) //new frame
    rcomLed( (uint8_t*)leds[idx], printlvl );
  else if( IS_FIXMODE_ROBOT1() ) //spine requires packet spamming
    rcomLed( (uint8_t*)leds[idx], RCOM_PRINT_LEVEL_NONE );
  
  last_idx = idx;
}

void DEBUG_TestRobotLeds(void) {
  //const int framerate = 250*1000; const int testtime = 10*1000*1000;
  const int framerate = 1000*1000; const int testtime = 20*1000*1000;
  uint32_t Tstart = Timer::get();
  led_manage_(1, framerate, RCOM_PRINT_LEVEL_CMD);
  while( Timer::elapsedUs(Tstart) < testtime ) { led_manage_(0, framerate, RCOM_PRINT_LEVEL_CMD); }
  led_manage_(1, framerate, RCOM_PRINT_LEVEL_CMD);
}

static void btn_sample_(int *press_cnt, int *release_cnt)
{
  static uint32_t Tsample = 0;
  if( Timer::elapsedUs(Tsample) > 30*1000 )
  {
    Tsample = Timer::get();
    const int printlvl = RCOM_PRINT_LEVEL_NONE; //RCOM_PRINT_LEVEL_DAT
    bool pressed = rcomGet(1, RCOM_SENSOR_BTN_TOUCH, printlvl)[0].btn.btn > 0;
    
    //DEBUG inspect data and measure sample period (~30ms for cmd round-trip)
    //ConsolePrintf("btn %i rcomGet() cmdTime %ims\n", pressed, cmdTimeMs());
    //ConsolePrintf("btn %i\n", pressed);
    
    if(press_cnt != NULL)
      *press_cnt = !pressed ? 0 : (*press_cnt >= 65535 ? 65535 : *press_cnt+1);
    if(release_cnt != NULL)
      *release_cnt = pressed ? 0 : (*release_cnt >= 65535 ? 65535 : *release_cnt+1);
  }
}

void TestRobotButton(void)
{
  led_manage_(1, 250*1000, RCOM_PRINT_LEVEL_CMD); //reset leds
  
  ConsolePrintf("Waiting for button...\n");
  
  //wait for button press
  int btn_press_cnt = 0;
  uint32_t btn_start = Timer::get();
  while( btn_press_cnt < 3 ) {
    btn_sample_( &btn_press_cnt, 0 );
    led_manage_(0);
    if( Timer::get() - btn_start > 7*1000*1000 ) {
      ConsolePrintf("ERROR_BACKP_BTN_PRESS_TIMEOUT\n");
      led_manage_(1); //reset leds
      throw ERROR_BACKP_BTN_PRESS_TIMEOUT;
      //break;
    }
  }
  
  ConsolePrintf("btn pressed\n");
  
  //wait for button release
  int btn_release_cnt = 0;
  btn_start = Timer::get();
  while( btn_release_cnt < 5 ) {
    btn_sample_( 0, &btn_release_cnt );
    led_manage_(0);
    if( Timer::get() - btn_start > 3500*1000 ) {
      ConsolePrintf("ERROR_BACKP_BTN_RELEASE_TIMEOUT\n");
      led_manage_(1); //reset leds
      throw ERROR_BACKP_BTN_RELEASE_TIMEOUT;
      //break;
    }
  }
  
  ConsolePrintf("btn released\n");
  led_manage_(1); //reset leds
}

//-----------------------------------------------------------------------------
//                  Recharge
//-----------------------------------------------------------------------------

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
  int batt_mv = robot_get_batt_mv(0); //get initial battery level
  
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
    batt_mv = robot_get_batt_mv(0);
    
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
static void ChargeTest(void)
{
  if( IS_FIXMODE_ROBOT1() )
    RobotChargeTest( 425, 4000 ); //test charging circuit
  else
    RobotChargeTest( 425, 4100 ); //test charging circuit
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
  int batt_mv = robot_get_batt_mv(0); //get initial battery level
  
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
    if ((offContact = current_ma < PRESENT_CURRENT_MA ? offContact + 1 : 0) > 20) {
      CHARGE_TEST_DEBUG( ConsolePrintf("\n"); );
      ConsolePrintf("robot off charger\n");
      throw ERROR_BAT_CHARGER;
    }
    
    //keep an eye on output voltage from crappy power supplies
    const int undervolt = 4750, overvolt = 5300;
    if( voltage_mv < undervolt || voltage_mv > overvolt ) {
      ConsolePrintf("bad voltage: %u\n", voltage_mv );
      throw voltage_mv < undervolt ? ERROR_OUTPUT_VOLTAGE_LOW : ERROR_OUTPUT_VOLTAGE_HIGH;
    }
  }
  
  Contacts::setModeRx(); //switch to comm mode
  Timer::delayMs(500); //let battery voltage settle
  batt_mv = robot_get_batt_mv(0); //get final battery level
  
  ConsolePrintf("charge-current-ma,%d,sample-cnt,%d\r\n", avg, avgCnt);
  ConsolePrintf("charge-current-dbg,avgMax,%d,%d,iMax,%d,%d\r\n", avgMax, avgMaxTime, iMax, iMaxTime);
  if( avgCnt >= NUM_SAMPLES && avg >= i_done_ma )
    return; //OK
  
  if( batt_mv >= bat_overvolt_mv )
    throw ERROR_BAT_OVERVOLT; //error prompts operator to put robot aside for a bit
  
  throw ERROR_BAT_CHARGER;
}

//-----------------------------------------------------------------------------
//                  Flex Flow
//-----------------------------------------------------------------------------

char* const logbuf = (char*)app_global_buffer;
const int logbufsize = APP_GLOBAL_BUF_SIZE;
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= (1024 + 4096) , log_buffer_size_check );

static void RobotLogCollect(void)
{
  int ofs = 0;
  for( int i=0; i<numlogs; i++ )
  {
    int len = 0;
    flexnfo.log[i] = NULL;
    flexnfo.loglen[i] = 0;
    
    //ConsolePrintf("reading robot log%u:\n", i);
    error_t e = ERROR_OK;
    try { len = rcomRlg(i, &logbuf[ofs], logbufsize-1-ofs); } catch(int err) { e=err; len=0; }
    
    //DEBUG
    if( e != ERROR_OK && !g_isReleaseBuild ) {
      ConsolePrintf("LOG READ ERROR: %i -- press a key to approve\n", e);
      if( e == ERROR_ROBOT_MISSING_LOGFILE )
        ConsolePrintf("ERROR_ROBOT_MISSING_LOGFILE\n");
      while( ConsoleReadChar() > -1 );
      uint32_t Tstart = Timer::get();
      while( Timer::elapsedUs(Tstart) < 3*1000*1000 ) {
        if( ConsoleReadChar() > -1 ) { e=ERROR_OK; break; }
      }
    }//-*/
    
    if( e != ERROR_OK )
      throw e;
    
    if( len > 0 ) {
      logbuf[ofs+len] = '\0'; //null terminate
      flexnfo.log[i] = &logbuf[ofs];
      flexnfo.loglen[i] = len;
      ofs += (len+1);
    }
    
    /*/DEBUG:
    ConsolePrintf("-----DEBUG: log%u len=%i-----\n", i, len);
    ConsolePrintf("RECVD:\"");
    ConsoleWrite(flexnfo.log[i] ? flexnfo.log[i] : (char*)"NA");
    ConsolePrintf("\"\n");
    //-*/
  }
}

static void RobotFlexFlowPackoutReport(void)
{
  //dump collected robot logs
  for( int i=0; i<numlogs; i++ ) {
    FLEXFLOW::printf("<flex> log packout_%08x_log%u.log\n", flexnfo.esn, i);
    FLEXFLOW::write( flexnfo.log[i] == NULL ? "not found" : flexnfo.log[i]);
    FLEXFLOW::printf("\n</flex>\n");
  }
  
  //report fixture-collected info
  FLEXFLOW::printf("<flex> log packout_%08x_fix.log\n", flexnfo.esn);
  {
    FLEXFLOW::printf("esn %08x\n", flexnfo.esn );
    FLEXFLOW::printf("hwver %u\n", flexnfo.hwver );
    FLEXFLOW::printf("model %u\n", flexnfo.model );
    FLEXFLOW::printf("packout-date %08x\n", flexnfo.packoutdate );
    
    robot_bsv_t* bsv = &flexnfo.bsv;
    FLEXFLOW::printf("body-ein %08x %08x %08x %08x\n", bsv->ein[0], bsv->ein[1], bsv->ein[2], bsv->ein[3] );
    FLEXFLOW::printf("body-hwrev %u\n", bsv->hw_rev );
    FLEXFLOW::printf("body-model %u\n", bsv->hw_model );
    FLEXFLOW::printf("body-app-vers %08x %08x %08x %08x ", bsv->app_version[0], bsv->app_version[1], bsv->app_version[2], bsv->app_version[3] );
    for(int x=0; x<16; x++) {
      int c = ((uint8_t*)&bsv->app_version)[x];
      if( isprint(c) ) FLEXFLOW::putchar(c); else break;
    }
    FLEXFLOW::printf("\n");
    
    FLEXFLOW::printf("vbat %imV %i\n", flexnfo.bat_mv, flexnfo.bat_raw);
  }
  FLEXFLOW::printf("</flex>\n");
  
  //validate required stuffs
  bool valid_head_esn = !( flexnfo.esn==0 || flexnfo.esn==0xFFFFffff || (flexnfo.esn&0xFFF00000)==0 );
  if( !valid_head_esn )
    throw ERROR_ROBOT_INVALID_ESN;
  
  bool valid_body_ein = !( flexnfo.bsv.ein[0]==0 || flexnfo.bsv.ein[0]==0xFFFFffff || (flexnfo.bsv.ein[0]&0xFFF00000)==0 );
  if( !valid_body_ein ) {
    if( !g_isReleaseBuild ) //allow empty ein for debug
      ConsolePrintf("---INVALID EIN---\n");
    else
      throw ERROR_ROBOT_INVALID_BODY_EIN;
  }
  
  //if( !flexnfo.packoutdate )
  //  throw ERROR_BAD_ARG;
  
  //DEBUG check
  if( !flexnfo.bat_mv && !g_isReleaseBuild ) {
    ConsolePrintf("----------\nBAD_ARG: PackoutReport() esn=%08x ein=%08x bat_mv=%i\n----------\n", flexnfo.esn, flexnfo.bsv.ein[0], flexnfo.bat_mv);
    throw ERROR_BAD_ARG;
  }
}

//-----------------------------------------------------------------------------
//                  Other
//-----------------------------------------------------------------------------

static void BatteryCheck(void)
{
  const int VBAT_MV_MINIMUM = 3700;
  const int VBAT_MV_MAXIMUM = 4050;
  
  ConsolePrintf("battery check\n");
  Contacts::setModeRx(); //disable charge power
  Timer::delayMs(500);  //wait for battery voltage to settle
  
  flexnfo.bat_mv = robot_get_batt_mv( &flexnfo.bat_raw );
  if( flexnfo.bat_mv < VBAT_MV_MINIMUM )
    throw ERROR_BAT_UNDERVOLT;
  
  if( IS_FIXMODE_PACKOUT() ) {
    if( flexnfo.bat_mv > VBAT_MV_MAXIMUM )
      throw ERROR_BAT_OVERVOLT;
  }
}

void SadBeep(void) {
  rcomEng(RCOM_ENG_IDX_SOUND, RCOM_ENG_SOUND_DAT0_TONE_BEEP, 255 /*volume*/);
  Timer::delayMs(750); //wait for sound to finish
}

void TurkeysDone(void) {
  rcomEng(RCOM_ENG_IDX_SOUND, RCOM_ENG_SOUND_DAT0_TONE_BELL, 255 /*volume*/);
  Timer::delayMs(750); //wait for sound to finish
}

//-----------------------------------------------------------------------------
//                  Get Tests
//-----------------------------------------------------------------------------

void DBG_test_all(void) { dbg_test_all_(); }
void DBG_test_emr(void) { dbg_test_emr_(); }

TestFunction* TestRobot0GetTests(void) {
  static TestFunction m_tests[] = {
    //TestRobotButton,
    TestRobotInfo,
    //DBG_test_all,
    //DBG_test_emr,
    TestRobotSensors,
    //ChargeTest,
    //RobotLogCollect,
    //RobotPowerDown,
    //RobotFlexFlowPackoutReport,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot1GetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotDetectSpine,
    TestRobotButton,
    TestRobotInfo,
    BatteryCheck, //sensor/motors may act strange if battery is low
    TestRobotSensors,
    //DEBUG_TestRobotLeds,
    TestRobotTreads,
    TestRobotRange,
    ChargeTest,
    BatteryCheck,
    RobotPowerDown,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot2GetTests(void) {
  static TestFunction m_tests[] = {
    //TestRobotButton,
    TestRobotInfo,
    //DBG_test_emr,
    //BatteryCheck,
    //TestRobotSensors,
    //TestRobotTreads,
    TestRobotRange,
    //ChargeTest,
    BatteryCheck,
    //TurkeysDone,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobot3GetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotButton,
    TestRobotInfo,
    EmrChecks, //check previous test results and reset status flags
    BatteryCheck, //sensor/motors may act strange if battery is low
    TestRobotSensors,
    TestRobotTreads,
    TestRobotRange,
    ChargeTest,
    BatteryCheck,
    EmrUpdate, //set test complete flags
    TurkeysDone,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotPackoutGetTests(void) { 
  static TestFunction m_tests[] = {
    TestRobotInfo,
    EmrChecks, //check previous test results and reset status flags
    BatteryCheck, //sensor/motors may act strange if battery is low
    TestRobotSensors,
    TestRobotTreads,
    TestRobotRange,
    ChargeTest,
    BatteryCheck,
    RobotLogCollect,
    SadBeep, //eng sound cmd only works before packout flag set
    EmrUpdate, //set packout flag, timestamp
    RobotPowerDown,
    RobotFlexFlowPackoutReport,
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

TestFunction* TestRobotRechargeGetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    Recharge,
    NULL,
  };
  return m_tests;
}

