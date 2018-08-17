#include <assert.h>
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

static void dbg_test_vext_stress_(int nloops, int loop_wait_ms)
{
  if( loop_wait_ms < 0 ) loop_wait_ms = 5000;
  for(int i=0; i<nloops; i++)
  {
    Contacts::setModeTx();
    Timer::delayMs(35);
    Contacts::setModeRx();
    Timer::delayMs(150);
    cmdSend(CMD_IO_CONTACTS, "getvers", 50, CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_ERRORS | CMD_OPTS_EXCEPTION_EN));
    cmdSend(CMD_IO_CONTACTS, "getvers", 50, CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_ERRORS | CMD_OPTS_EXCEPTION_EN));
    Timer::delayMs(50);
    cmdSend(CMD_IO_CONTACTS, "testcommand  extra  long  string  to  measure  gate  rise  abcd  1234567890", 50, CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_ERRORS | CMD_OPTS_EXCEPTION_EN));
    Timer::delayMs(50);
    cmdSend(CMD_IO_CONTACTS, "getvers", 50, CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_ERRORS | CMD_OPTS_EXCEPTION_EN));
    Timer::delayMs(100);
    
    Timer::delayMs( loop_wait_ms );
  }
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
  if( arg[0] == 5 )
    dbg_test_vext_stress_(arg[1], arg[2]);
  
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

#define IS_FIXMODE_ROBOTNFO() ( g_fixmode==FIXMODE_INFO )
#define IS_FIXMODE_ROBOT1()   ( g_fixmode==FIXMODE_ROBOT1  || g_fixmode==FIXMODE_ROBOT1_OL )
#define IS_FIXMODE_ROBOT3()   ( g_fixmode==FIXMODE_ROBOT3  || g_fixmode==FIXMODE_ROBOT3_OL )
#define IS_FIXMODE_PACKOUT()  ( g_fixmode==FIXMODE_PACKOUT || g_fixmode==FIXMODE_PACKOUT_OL )
#define IS_FIXMODE_OFFLINE()  ( g_fixmode==FIXMODE_ROBOT1_OL || g_fixmode==FIXMODE_ROBOT3_OL || g_fixmode==FIXMODE_PACKOUT_OL )
#define IS_FIXMODE_UNPACKOUT() ( g_fixmode==FIXMODE_UN_PACKOUT )

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
    
    //read playpen calibration
    uint32_t playpenTouchSensorMinValid     = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorMinValid) );
    uint32_t playpenTouchSensorMaxValid     = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorMaxValid) );
    uint32_t playpenTouchSensorRangeThresh  = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorRangeThresh) );
    uint32_t playpenTouchSensorStdDevThresh = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorStdDevThresh) );
    uint32_t playpenTestDisableMask         = rcomGmr( EMR_FIELD_OFS(playpenTestDisableMask) );
    
    //read factory test dataz
    uint32_t packoutCnt     = rcomGmr( EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_CNT );
    uint32_t packoutVbatMv  = rcomGmr( EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_VBAT_MV );
    uint32_t robot3VbatMv   = rcomGmr( EMR_FIELD_OFS(fixture)+EMRF_ROBOT3_VBAT_MV );
    
    #define U32FLOAT(u)     ( *((float*)&(u)) )
    
    ConsolePrintf("EMR[%u] esn         :%08x [%08x]\n", EMR_FIELD_OFS(ESN), flexnfo.esn, esnCmd);
    ConsolePrintf("EMR[%u] hwver       :%u\n", EMR_FIELD_OFS(HW_VER), flexnfo.hwver);
    ConsolePrintf("EMR[%u] model       :%u\n", EMR_FIELD_OFS(MODEL), flexnfo.model);
    ConsolePrintf("EMR[%u] lotcode     :%u\n", EMR_FIELD_OFS(LOT_CODE), lotcode);
    ConsolePrintf("EMR[%u] playpenready:%u\n", EMR_FIELD_OFS(PLAYPEN_READY_FLAG), playpenready);
    ConsolePrintf("EMR[%u] playpenpass :%u\n", EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), playpenpass);
    ConsolePrintf("EMR[%u] packedout   :%u\n", EMR_FIELD_OFS(PACKED_OUT_FLAG), packedout);
    ConsolePrintf("EMR[%u] packout-date:%u\n", EMR_FIELD_OFS(PACKED_OUT_DATE), flexnfo.packoutdate);
    ConsolePrintf("EMR[%u] playpenTouchSensorMinValid:%u\n", EMR_FIELD_OFS(playpenTouchSensorMinValid), playpenTouchSensorMinValid);
    ConsolePrintf("EMR[%u] playpenTouchSensorMaxValid:%u\n", EMR_FIELD_OFS(playpenTouchSensorMaxValid), playpenTouchSensorMaxValid);
    ConsolePrintf("EMR[%u] playpenTouchSensorRangeThresh :%f\n", EMR_FIELD_OFS(playpenTouchSensorRangeThresh), U32FLOAT(playpenTouchSensorRangeThresh) );
    ConsolePrintf("EMR[%u] playpenTouchSensorStdDevThresh:%f\n", EMR_FIELD_OFS(playpenTouchSensorStdDevThresh), U32FLOAT(playpenTouchSensorStdDevThresh) );
    ConsolePrintf("EMR[%u] playpenTestDisableMask:%08x\n", EMR_FIELD_OFS(playpenTestDisableMask), playpenTestDisableMask);
    ConsolePrintf("EMR[%u] packoutCnt:%i\n", EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_CNT, packoutCnt);
    ConsolePrintf("EMR[%u] packoutVbatMv:%i\n", EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_VBAT_MV, packoutVbatMv);
    ConsolePrintf("EMR[%u] robot3VbatMv:%i\n", EMR_FIELD_OFS(fixture)+EMRF_ROBOT3_VBAT_MV, robot3VbatMv);
    
    //inspect robot3 motor data
    if( (!g_isReleaseBuild && IS_FIXMODE_ROBOT3() ) || IS_FIXMODE_ROBOTNFO() )
    {
      struct {
        struct {
          struct { emr_tread_dat_t L; emr_tread_dat_t R; } lo;
          struct { emr_tread_dat_t L; emr_tread_dat_t R; } hi; 
        }tread;
        struct {
          struct { emr_range_dat_t lift; emr_range_dat_t head; } lo;
          struct { emr_range_dat_t lift; emr_range_dat_t head; } hi;
        }range;
      }dat;
      
      //read from emr
      for(int i=0; i < sizeof(dat)/4; i++)
        ((uint32_t*)&dat)[i] = rcomGmr( EMR_FIELD_OFS(fixture) + EMRF_ROBOT3_TREAD_L_LO + i );
      
      ConsolePrintf("tread LEFT  LO pwr:%i FWD speed:%+i travel:%+i REV speed:%+i travel:%+i\n", dat.tread.lo.L.power, dat.tread.lo.L.fwd.speed, dat.tread.lo.L.fwd.travel, dat.tread.lo.L.rev.speed, dat.tread.lo.L.rev.travel);
      ConsolePrintf("tread RIGHT LO pwr:%i FWD speed:%+i travel:%+i REV speed:%+i travel:%+i\n", dat.tread.lo.R.power, dat.tread.lo.R.fwd.speed, dat.tread.lo.R.fwd.travel, dat.tread.lo.R.rev.speed, dat.tread.lo.R.rev.travel);
      ConsolePrintf("tread LEFT  HI pwr:%i FWD speed:%+i travel:%+i REV speed:%+i travel:%+i\n", dat.tread.hi.L.power, dat.tread.hi.L.fwd.speed, dat.tread.hi.L.fwd.travel, dat.tread.hi.L.rev.speed, dat.tread.hi.L.rev.travel);
      ConsolePrintf("tread RIGHT HI pwr:%i FWD speed:%+i travel:%+i REV speed:%+i travel:%+i\n", dat.tread.hi.R.power, dat.tread.hi.R.fwd.speed, dat.tread.hi.R.fwd.travel, dat.tread.hi.R.rev.speed, dat.tread.hi.R.rev.travel);
      ConsolePrintf("range LIFT LO pwr:%i NN:%i UP speed:%+i travel:%+i DN speed:%+i travel:%+i\n", dat.range.lo.lift.power, dat.range.lo.lift.NN, dat.range.lo.lift.up.speed, dat.range.lo.lift.up.travel, dat.range.lo.lift.dn.speed, dat.range.lo.lift.dn.travel);
      ConsolePrintf("range HEAD LO pwr:%i NN:%i UP speed:%+i travel:%+i DN speed:%+i travel:%+i\n", dat.range.lo.head.power, dat.range.lo.head.NN, dat.range.lo.head.up.speed, dat.range.lo.head.up.travel, dat.range.lo.head.dn.speed, dat.range.lo.head.dn.travel);
      ConsolePrintf("range LIFT HI pwr:%i NN:%i UP speed:%+i travel:%+i DN speed:%+i travel:%+i\n", dat.range.hi.lift.power, dat.range.hi.lift.NN, dat.range.hi.lift.up.speed, dat.range.hi.lift.up.travel, dat.range.hi.lift.dn.speed, dat.range.hi.lift.dn.travel);
      ConsolePrintf("range HEAD HI pwr:%i NN:%i UP speed:%+i travel:%+i DN speed:%+i travel:%+i\n", dat.range.hi.head.power, dat.range.hi.head.NN, dat.range.hi.head.up.speed, dat.range.hi.head.up.travel, dat.range.hi.head.dn.speed, dat.range.hi.head.dn.travel);
    }
  }
}

const int bat_raw_min = RCOM_BAT_MV_TO_RAW(2500);
const int bat_raw_max = RCOM_BAT_MV_TO_RAW(6000);

static int robot_get_batt_mv(int *out_raw=0, bool sanity_check=true, int printlvl = RCOM_PRINT_LEVEL_DEFAULT);
static int robot_get_batt_mv(int *out_raw, bool sanity_check, int printlvl)
{
  int bat_raw = rcomGet(1, RCOM_SENSOR_BATTERY, printlvl)[0].bat.raw;
  int bat_mv = RCOM_BAT_RAW_TO_MV(bat_raw);
  if( out_raw ) *out_raw = bat_raw;
  ConsolePrintf("vbat = %imV (%i)\n", bat_mv, bat_raw);
  
  //detect body electrical + fw errors
  if( sanity_check && (bat_raw < bat_raw_min || bat_raw > bat_raw_max) )
    throw ERROR_SENSOR_VBAT;
  
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
  robot_get_batt_mv(0,false); //no error check
}

#include "../../syscon/schema/messages.h" //#include "messages.h"
void TestRobotSensors(void)
{
  robot_sr_t *psr;
  //robot_sr_t bat    = rcomGet(3, RCOM_SENSOR_BATTERY   )[1];
  //robot_sr_t cliff  = rcomGet(3, RCOM_SENSOR_CLIFF     )[1];
  //robot_sr_t prox   = rcomGet(3, RCOM_SENSOR_PROX_TOF  )[1];
  //robot_sr_t btn    = rcomGet(3, RCOM_SENSOR_BTN_TOUCH )[1];
  
  ConsolePrintf("Sensor Testing...\n");
  int failCode = BOOT_FAIL_NONE;
  
  //BATTERY (ADC input)
  {
    const uint8_t NN_bat=3;
    psr = rcomGet(NN_bat, RCOM_SENSOR_BATTERY);
    for(int x=0; x<NN_bat; x++) {
      if( psr[x].bat.raw < bat_raw_min || psr[x].bat.raw > bat_raw_max )
        throw ERROR_SENSOR_VBAT;
    }
    robot_sr_t bat = psr[NN_bat>>1];
    ConsolePrintf("bat,raw,%i,mv,%i\n", bat.bat.raw, RCOM_BAT_RAW_TO_MV(bat.bat.raw));
  }
  
  //CLIFF
  {
    //per software team, intermittent 0 values are common. Sensor is 12?-bit max
    const uint8_t NN_cliff=15, NN_ok=10; //allow a few noise values
    struct { int fL; int fR; int bL; int bR; } cnt = {0,0,0,0};
    psr = rcomGet(NN_cliff, RCOM_SENSOR_CLIFF);
    for(int x=0; x<NN_cliff; x++) 
    {
      const int cliff_min=1, cliff_max=1<<12;
      if( psr[x].cliff.fL >= cliff_min && psr[x].cliff.fL <= cliff_max ) cnt.fL++;
      if( psr[x].cliff.fR >= cliff_min && psr[x].cliff.fR <= cliff_max ) cnt.fR++;
      if( psr[x].cliff.bL >= cliff_min && psr[x].cliff.bL <= cliff_max ) cnt.bL++;
      if( psr[x].cliff.bR >= cliff_min && psr[x].cliff.bR <= cliff_max ) cnt.bR++;
    }
    robot_sr_t cliff = psr[NN_cliff>>1];
    ConsolePrintf("cliff,fL,%i,fR,%i,bL,%i,bR,%i\n", cliff.cliff.fL, cliff.cliff.fR, cliff.cliff.bL, cliff.cliff.bR);
    ConsolePrintf("..cnt,fL,%i,fR,%i,bL,%i,bR,%i\n", cnt.fL, cnt.fR, cnt.bL, cnt.bR);
    
    //check syscon-reported failures
    failCode = cliff.meta.failureCode;
    ConsolePrintf("cliff,failureCode,%04x\n", failCode);
    if( failCode == BOOT_FAIL_CLIFF1 ) throw ERROR_SENSOR_CLIFF_BOOT_FAIL_FL;
    if( failCode == BOOT_FAIL_CLIFF2 ) throw ERROR_SENSOR_CLIFF_BOOT_FAIL_FR;
    if( failCode == BOOT_FAIL_CLIFF3 ) throw ERROR_SENSOR_CLIFF_BOOT_FAIL_BL;
    if( failCode == BOOT_FAIL_CLIFF4 ) throw ERROR_SENSOR_CLIFF_BOOT_FAIL_BR;
    
    if( cnt.fL<2 && cnt.fR<2 && cnt.bL<2 && cnt.bR<2 ) throw ERROR_SENSOR_CLIFF_ALL; //all-zero data. hmm...
    if( cnt.fL < NN_ok )  throw ERROR_SENSOR_CLIFF_FL;
    if( cnt.fR < NN_ok )  throw ERROR_SENSOR_CLIFF_FR;
    if( cnt.bL < NN_ok )  throw ERROR_SENSOR_CLIFF_BL;
    if( cnt.bR < NN_ok )  throw ERROR_SENSOR_CLIFF_BR;
  }
  
  //TOUCH btn
  {
    const uint8_t NN_touch=15, NN_ok=12; //filter some noise
    int validCnt = 0;
    psr = rcomGet(NN_touch, RCOM_SENSOR_BTN_TOUCH);
    for(int x=0; x<NN_touch; x++) {
      const int touch_min=100, touch_max=1200; //playpen uses range 500-700 (off charger). Ours will be noisier.
      if( psr[x].btn.touch >= touch_min && psr[x].btn.touch <= touch_max )
        validCnt++;
    }
    robot_sr_t btn = psr[NN_touch>>1];
    ConsolePrintf("btn,touch,%i,btn,%i,cnt,%i\n", btn.btn.touch, btn.btn.btn, validCnt);
    
    if( validCnt < NN_ok )
      throw ERROR_SENSOR_TOUCH;
  }
  
  //TOF
  {
    const uint8_t NN_tof=5, emax=1;
    int ecount=0;
    psr = rcomGet(NN_tof, RCOM_SENSOR_PROX_TOF);
    for(int x=0; x<NN_tof; x++) {
      //XXX: no idea what valid TOF values look like...
      //just make sure they are non-zero
      if( psr[x].prox.rangeMM<1 || psr[x].prox.signalRate<1 || psr[x].prox.spadCnt<1 || psr[x].prox.ambientRate<1 )
        ecount++;
    }
    robot_sr_t prox = psr[NN_tof>>1];
    ConsolePrintf("prox,mm,%i,sigRate,%i,spad,%i,ambientRate,%i\n", prox.prox.rangeMM, prox.prox.signalRate, prox.prox.spadCnt, prox.prox.ambientRate);
    
    //check syscon-reported failures
    failCode = prox.meta.failureCode;
    ConsolePrintf("tof,failureCode,%04x\n", failCode);
    if( failCode == BOOT_FAIL_TOF )
      throw ERROR_SENSOR_TOF_BOOT_FAIL;
    
    if( ecount > emax )
      throw ERROR_SENSOR_TOF;
  }
  
  //Unhandled FailureCode
  ConsolePrintf("robotsensor,failureCode,%04x\n", failCode);
  if( failCode != BOOT_FAIL_NONE )
    throw ERROR_SENSOR_UNHANDLED_FAILURE;
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

//convert tread data to emr record
static void get_emr_tread_dat_(int8_t power, robot_tread_dat_t *ptread, emr_tread_dat_t *out_emr_tread) {
  assert( ptread != NULL && out_emr_tread != NULL );
  memset(out_emr_tread, 0, sizeof(emr_tread_dat_t));
  out_emr_tread->power = power;
  out_emr_tread->fwd.speed  = ptread->fwd_avg;
  out_emr_tread->fwd.travel = ptread->fwd_travel;
  out_emr_tread->rev.speed  = ptread->rev_avg;
  out_emr_tread->rev.travel = ptread->rev_travel;
}

#define TREAD_TEST_DATA_GATHERING 0
static void TestRobotTreads_(bool pwr_hi_nlow, int8_t power, int min_speed, int min_travel)
{
  robot_tread_dat_t treadL = *robot_tread_test_(RCOM_SENSOR_MOT_LEFT, power);
  robot_tread_dat_t treadR = *robot_tread_test_(RCOM_SENSOR_MOT_RIGHT, -power);
  print_tread_dat(&treadL, "LEFT ");
  print_tread_dat(&treadR, "RIGHT");
  
  #if TREAD_TEST_DATA_GATHERING > 0
  #warning "TREAD ERROR CHECKING DISABLED"
  #else
  
  //record test data
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() )
  {
    emr_tread_dat_t emrL, emrR;
    get_emr_tread_dat_( power, &treadL, &emrL );
    get_emr_tread_dat_( power, &treadR, &emrR );
    
    int ofsHi = pwr_hi_nlow ? 2*sizeof(emr_tread_dat_t)/4 : 0; //offset for high speed data block
    
    for(int x=0; x < sizeof(emr_tread_dat_t)/4; x++)
      rcomSmr( EMR_FIELD_OFS(fixture) + EMRF_ROBOT3_TREAD_L_LO + ofsHi + x, ((uint32_t*)&emrL)[x] );
    for(int x=0; x < sizeof(emr_tread_dat_t)/4; x++)
      rcomSmr( EMR_FIELD_OFS(fixture) + EMRF_ROBOT3_TREAD_R_LO + ofsHi + x, ((uint32_t*)&emrR)[x] );
  }
  
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
    TestRobotTreads_(0, pwr, 9999, 9999);
  }
    
  #else
  
  //full power: speed 1760-1980, travel 790-1160
  TestRobotTreads_(1, 127, 1500, 600);
  
  //low power (72): speed 870-1070, travel 400-550
  if( !IS_FIXMODE_PACKOUT() )
    TestRobotTreads_(0, 75, 750, 300);
  
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
static robot_range_dat_t* robot_range_test_(uint8_t sensor, uint8_t NNtest, int8_t power)
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
  
  const uint8_t NNstart = lift ? 35 : 65;
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

//convert range data to emr record
static void get_emr_range_dat_(uint8_t NN, int8_t power, robot_range_dat_t *prange, emr_range_dat_t *out_emr_range) {
  assert( prange != NULL && out_emr_range != NULL );
  memset(out_emr_range, 0, sizeof(emr_range_dat_t));
  out_emr_range->NN = NN;
  out_emr_range->power = power;
  out_emr_range->up.speed  = prange->up_avg;
  out_emr_range->up.travel = prange->up_travel;
  out_emr_range->dn.speed  = prange->dn_avg;
  out_emr_range->dn.travel = prange->dn_travel;
}

#define RANGE_TEST_DATA_GATHERING 0
typedef struct { uint8_t NN; int8_t power; int travel_min; int travel_max; int speed_min; } robot_range_t;
void TestRobotRange(bool pwr_hi_nlow, robot_range_t *testlift, robot_range_t *testhead)
{
  robot_range_dat_t lift = *robot_range_test_(RCOM_SENSOR_MOT_LIFT, testlift->NN, testlift->power);
  robot_range_dat_t head = *robot_range_test_(RCOM_SENSOR_MOT_HEAD, testhead->NN, testhead->power);
  print_range_dat(&lift, "LIFT");
  print_range_dat(&head, "HEAD");

  #if RANGE_TEST_DATA_GATHERING > 0
  #warning "RANGE ERROR CHECKING DISABLED"
  #else
  
  //record test data
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() )
  {
    emr_range_dat_t emrLift, emrHead;
    get_emr_range_dat_(testlift->NN, testlift->power, &lift, &emrLift);
    get_emr_range_dat_(testhead->NN, testhead->power, &head, &emrHead);
    
    int ofsHi = pwr_hi_nlow ? 2*sizeof(emr_range_dat_t)/4 : 0; //offset for high pwr data block
    
    for(int x=0; x < sizeof(emr_range_dat_t)/4; x++)
      rcomSmr( EMR_FIELD_OFS(fixture) + EMRF_ROBOT3_RANGE_LIFT_LO + ofsHi + x, ((uint32_t*)&emrLift)[x] );
    for(int x=0; x < sizeof(emr_range_dat_t)/4; x++)
      rcomSmr( EMR_FIELD_OFS(fixture) + EMRF_ROBOT3_RANGE_HEAD_LO + ofsHi + x, ((uint32_t*)&emrHead)[x] );
  }
  
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
      uint8_t NNlift = pwr<75 ? 85 : 55;
      uint8_t NNhead = pwr<90 ? 125 : 70;
      robot_range_t lift = { NNlift, /*power*/ pwr, /*travel_min*/ 0, /*travel_max*/ 99999, /*speed_min*/ 0 };
      robot_range_t head = { NNhead, /*power*/ pwr, /*travel_min*/ 0, /*travel_max*/ 99999, /*speed_min*/ 0 };
      TestRobotRange(0, &lift, &head );
    }
  }
  #else
  
  //High Power
  if( IS_FIXMODE_ROBOT1() ) {
    //NO HEAD/ARMS ATTACHED = NO STOP!
    //lift travel ~450-500 in each direction
    //head travel ~800-850 in each direction
    robot_range_t lift = { /*NN*/  55, /*power*/  75, /*travel_min*/ 400, /*travel_max*/ 9999, /*speed_min*/ 1800 };
    robot_range_t head = { /*NN*/  70, /*power*/ 100, /*travel_min*/ 700, /*travel_max*/ 9999, /*speed_min*/ 2100 };
    TestRobotRange(1, &lift, &head );
  } else if( !IS_FIXMODE_PACKOUT() ) {
    //lift: travel 195-200, speed 760-1600
    //head: travel 550-560, speed 2130-2400
    robot_range_t lift = { /*NN*/  55, /*power*/  75, /*travel_min*/ 170, /*travel_max*/ 230, /*speed_min*/  650 };
    robot_range_t head = { /*NN*/  70, /*power*/ 100, /*travel_min*/ 520, /*travel_max*/ 590, /*speed_min*/ 1700 };
    TestRobotRange(1, &lift, &head );
  }
  
  //Low Power
  if( IS_FIXMODE_ROBOT1() ) {
    //NO HEAD/ARMS ATTACHED = NO STOP!
    //lift travel ~550-600 in each direction
    //head travel ~650-??? in each direction
    robot_range_t lift = { /*NN*/  85, /*power*/  50, /*travel_min*/ 400, /*travel_max*/ 9999, /*speed_min*/ 800 };
    robot_range_t head = { /*NN*/ 125, /*power*/  55, /*travel_min*/ 550, /*travel_max*/ 9999, /*speed_min*/ 700 };
    TestRobotRange(0, &lift, &head );
  } else {
    //lift: travel 190-200, speed 580-1520
    //head: travel 545-555, speed 900-1230
    robot_range_t lift = { /*NN*/  85, /*power*/  65, /*travel_min*/ 170, /*travel_max*/  230, /*speed_min*/ 400 };
    robot_range_t head = { /*NN*/ 125, /*power*/  60, /*travel_min*/ 520, /*travel_max*/  590, /*speed_min*/ 700 };
    TestRobotRange(0, &lift, &head );
  }
  
  #endif
}

void EmrChecks(void)
{
  //Make sure previous tests have passed
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() ) {
    uint32_t ppReady  = rcomGmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG) );
    uint32_t ppPassed = rcomGmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) );
    if( ppReady != 1 || ppPassed != 1 ) {
      throw ERROR_ROBOT_TEST_SEQUENCE;
    }
  }
  
  //require retest on all downstream fixtures after rework
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() )
  {
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 0 );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), 0 );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG), 0 );
    
    ConsolePrintf("clear previous test data...");
    for(int i=EMRF_ROBOT3_VBAT_MV; i < EMRF_ROBOT3_RANGE_HEAD_HI+((sizeof(emr_range_dat_t)/4)); i++ ) {
      if( !g_isReleaseBuild )
        ConsolePrintf("%i,", EMR_FIELD_OFS(fixture)+i );
      rcomSmr( EMR_FIELD_OFS(fixture)+i, 0);
    }
    ConsolePutChar('\n');
  }
  
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() ) {
    //will throw error if Ribbit has been packed out
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 0 );
  }
}

void EmrUpdate(void)
{
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() )
  {
    #define FLOAT2U32(f)    ( *((uint32_t*)(&(f))) )
    #define U32FLOAT(u)     ( *((float*)&(u)) )
    
    //manual adjustments for playpen touch sensor error thresholds
    rcomSmr( EMR_FIELD_OFS(playpenTouchSensorMinValid), 500 ); //default 500
    rcomSmr( EMR_FIELD_OFS(playpenTouchSensorMaxValid), 700 ); //default 700
    float rangeThres = 12.1f; //default 11.0
    rcomSmr( EMR_FIELD_OFS(playpenTouchSensorRangeThresh), FLOAT2U32(rangeThres) );
    float stdDevThres = 3.1f; //default 1.8
    rcomSmr( EMR_FIELD_OFS(playpenTouchSensorStdDevThresh), FLOAT2U32(stdDevThres) );
    
    //disable some playpen errors
    const uint32_t PlaypenTestMask = 0 //from clad/src/clad/types/factoryTestTypes.clad
      //| 0x00000001  //BackpackElectricalError
      //| 0x00000002  //UnexpectedTouchDetectedError 
      //| 0x00000004  //NoisyTouchSensorError
      //| 0x00000008  //CubeRadioError
      | 0x00000010  //WifiScanError
    ;
    rcomSmr( EMR_FIELD_OFS(playpenTestDisableMask), PlaypenTestMask );
    rcomSmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG), 1 );
    
    //readback for log
    uint32_t playpenTouchSensorMinValid     = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorMinValid) );
    uint32_t playpenTouchSensorMaxValid     = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorMaxValid) );
    uint32_t playpenTouchSensorRangeThresh  = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorRangeThresh) );
    uint32_t playpenTouchSensorStdDevThresh = rcomGmr( EMR_FIELD_OFS(playpenTouchSensorStdDevThresh) );
    uint32_t playpenTestDisableMask         = rcomGmr( EMR_FIELD_OFS(playpenTestDisableMask) );
    ConsolePrintf("EMR[%u] playpenTouchSensorMinValid:%u\n", EMR_FIELD_OFS(playpenTouchSensorMinValid), playpenTouchSensorMinValid);
    ConsolePrintf("EMR[%u] playpenTouchSensorMaxValid:%u\n", EMR_FIELD_OFS(playpenTouchSensorMaxValid), playpenTouchSensorMaxValid);
    ConsolePrintf("EMR[%u] playpenTouchSensorRangeThresh :%f\n", EMR_FIELD_OFS(playpenTouchSensorRangeThresh), U32FLOAT(playpenTouchSensorRangeThresh));
    ConsolePrintf("EMR[%u] playpenTouchSensorStdDevThresh:%f\n", EMR_FIELD_OFS(playpenTouchSensorStdDevThresh), U32FLOAT(playpenTouchSensorStdDevThresh));
    ConsolePrintf("EMR[%u] playpenTestDisableMask:%08x\n", EMR_FIELD_OFS(playpenTestDisableMask), playpenTestDisableMask);
  }
  
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() )
  {
    uint32_t packoutCnt = 1 + rcomGmr( EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_CNT );
    rcomSmr( EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_CNT, packoutCnt );
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_DATE), flexnfo.packoutdate );
    rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), 1 );
  }
}

void EmrUnPackout(void)
{
  const uint32_t WIPE_PACKOUT_MAGIC = 0x57495045;
  ConsolePrintf("Unpacking the ribbit...\n");
  
  //writing magic value clears several emr registers:
  //  PACKED_OUT_FLAG, PACKED_OUT_DATE, PLAYPEN_PASSED_FLAG, PLAYPEN_READY_FLAG
  rcomSmr( EMR_FIELD_OFS(PACKED_OUT_FLAG), WIPE_PACKOUT_MAGIC, RCOM_PRINT_LEVEL_CMD ); //RCOM_PRINT_LEVEL_ALL );
  
  //change packout flag causes delay while some internal processes react
  ConsolePrintf("wait for system reconfig...\n");
  uint32_t Tstart = Timer::get(); int reconfig=0;
  while(reconfig < 5) {
    if( Timer::elapsedUs(Tstart) > 5*1000*1000 ) 
      throw ERROR_TIMEOUT; //ERROR_ROBOT_PACKED_OUT;
    try {
      rcomGmr( EMR_FIELD_OFS(ESN), RCOM_PRINT_LEVEL_NONE ); //read something and see if it succeeds
      reconfig++;
    } catch(...) { 
      reconfig=0;
    }
  }
  ConsolePrintf("done in %ims\n", Timer::elapsedUs(Tstart)/1000 );
  
  //readback verify
  uint32_t playpenready = rcomGmr( EMR_FIELD_OFS(PLAYPEN_READY_FLAG) );
  uint32_t playpenpass  = rcomGmr( EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG) );
  uint32_t packedout    = rcomGmr( EMR_FIELD_OFS(PACKED_OUT_FLAG) );
  uint32_t packoutdate   = rcomGmr( EMR_FIELD_OFS(PACKED_OUT_DATE) );
  //uint32_t packoutCnt     = rcomGmr( EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_CNT );
  ConsolePrintf("EMR[%u] playpenready:%u\n", EMR_FIELD_OFS(PLAYPEN_READY_FLAG), playpenready);
  ConsolePrintf("EMR[%u] playpenpass :%u\n", EMR_FIELD_OFS(PLAYPEN_PASSED_FLAG), playpenpass);
  ConsolePrintf("EMR[%u] packedout   :%u\n", EMR_FIELD_OFS(PACKED_OUT_FLAG), packedout);
  ConsolePrintf("EMR[%u] packout-date:%u\n", EMR_FIELD_OFS(PACKED_OUT_DATE), packoutdate);
  //ConsolePrintf("EMR[%u] packoutCnt:%i\n", EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_CNT, packoutCnt);
  
  if( playpenready > 0 || playpenpass > 0 || packedout > 0 || packoutdate > 0 ) {
    ConsolePrintf("---UNPACK FAILED---\n");
    throw ERROR_ROBOT_PACKED_OUT;
  }
}

void RobotPowerDown(void)
{
  ConsolePrintf("robot power down\n");
  
  if( !IS_FIXMODE_ROBOT1() )
  {
    if( IS_FIXMODE_UNPACKOUT() ) {
      //un-packout hack is a little unstable. Add some reliability to power-down.
      int retries=5;
      while(1) { 
        try{ rcomPwr(RCOM_PWR_OFF); break; }
        catch(...) { if( --retries <= 0 ) throw; } 
      }
    } else {
      rcomPwr(RCOM_PWR_OFF);
    }
    Contacts::powerOn(); //immdediately turn on power to prevent rebooting
    cleanup_preserve_vext = 1; //leave power on for removal detection (no cleanup pwr cycle)
    Timer::delayMs(1000); //power off command has a delayed reaction time
    
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
  batt_mv = robot_get_batt_mv(); //get final battery level
  
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

static time_t getRtc_(void)
{
  time_t now = fixtureGetTime();
  bool valid = fixtureTimeIsValid();
  ConsolePrintf("rtc,%i,%010u,%s\n", valid, now, fixtureTimeStr(now) );
  
  if( !valid ) {
    ConsolePrintf("---- ERROR_INVALID_RTC ----\n");
    if( g_isReleaseBuild && /*IS_FIXMODE_PACKOUT() &&*/ !IS_FIXMODE_OFFLINE() )
      throw ERROR_INVALID_RTC;
  }
  
  return now;
}

void TestRobotRtcValid(void) {
  getRtc_(); //print and error check
}

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
  flexnfo.packoutdate = getRtc_(); //get current clock (+error check)
  
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
    FLEXFLOW::printf("packout-date %010u %s", flexnfo.packoutdate, ctime(&flexnfo.packoutdate) );
    
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
    ConsolePrintf("--- ERROR_ROBOT_INVALID_BODY_EIN ---\n");
    if( g_isReleaseBuild )
      throw ERROR_ROBOT_INVALID_BODY_EIN;
  }
  
  //sanity check known logs
  for( int i=0; i<numlogs; i++ ) {
    int len_min = i==0 ? 750 : 250; //0=playpen (~1450 bytes), 1=cloud (~445 bytes)
    if( !flexnfo.log[i] || flexnfo.loglen[i] < len_min ) {
      ConsolePrintf("--- ERROR_ROBOT_BAD_LOGFILE[%i] len %i ---\n", i, flexnfo.loglen[i] );
      if( g_isReleaseBuild )
        throw ERROR_ROBOT_BAD_LOGFILE;
    }
  }
  
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
  
  //record test data
  if( IS_FIXMODE_ROBOT3() && !IS_FIXMODE_OFFLINE() ) {
    rcomSmr( EMR_FIELD_OFS(fixture)+EMRF_ROBOT3_VBAT_MV, (uint32_t)flexnfo.bat_mv );
  }
  if( IS_FIXMODE_PACKOUT() && !IS_FIXMODE_OFFLINE() ) {
    rcomSmr( EMR_FIELD_OFS(fixture)+EMRF_PACKOUT_VBAT_MV, (uint32_t)flexnfo.bat_mv );
  }
}

void EngPlaySound(uint8_t select, uint8_t volume=255, int delayms=750);
void EngPlaySound(uint8_t select, uint8_t volume, int delayms) {
  rcomEng(RCOM_ENG_IDX_SOUND, select, volume);
  Timer::delayMs(delayms); //wait for sound to finish
}

void SadBeep(void) { EngPlaySound(RCOM_ENG_SOUND_DAT0_TONE_BEEP); }
void TurkeysDone(void) { EngPlaySound(RCOM_ENG_SOUND_DAT0_TONE_BELL); }

void SoundLoop(void)
{
  int offContact = 0;
  while( ConsoleReadChar() > -1 );
  
  while(1)
  {
    Contacts::setModeRx();
    Timer::delayMs(10);
    
    //play sound
    if( g_fixmode == FIXMODE_SOUND1 )
      EngPlaySound(RCOM_ENG_SOUND_DAT0_TONE_BEEP, 255); //SadBeep();
    else if( g_fixmode == FIXMODE_SOUND2 )
      EngPlaySound(RCOM_ENG_SOUND_DAT0_TONE_BELL, 255); //TurkeysDone();
    
    //robot detect requires power draw
    Contacts::powerOn();
    uint32_t Twait = Timer::get();
    while( Timer::elapsedUs(Twait) < (750+SYSCON_CHG_PWR_DELAY_MS)*1000 )
    {
      int current_ma = Meter::getCurrentMa(PWR_VEXT,6);
      int voltage_mv = Meter::getVoltageMv(PWR_VEXT,4);
      
      //error out quickly if robot removed from charge base
      if ((offContact = current_ma < PRESENT_CURRENT_MA ? offContact + 1 : 0) > 5) {
        ConsolePrintf("robot off charger\n");
        break;
      }
    }
    
    if( ConsoleReadChar() > -1 ) break;
  }
}

void LogDownload(void)
{
  int nerrs=0;
  
  //download until robot runs out of logs...
  for( int i=0; true ; i++ )
  {
    FLEXFLOW::printf("************************ log%i ************************\n", i);
    
    error_t e = ERROR_OK; int len=0;
    int printlvl = RCOM_PRINT_LEVEL_ALL; //RCOM_PRINT_LEVEL_CMD | RCOM_PRINT_LEVEL_RSP;
    try { len = rcomRlg(i, &logbuf[0], logbufsize-1, printlvl); } catch(int err) { e=err; len=0; }
    
    //DEBUG
    if( e != ERROR_OK && e != ERROR_ROBOT_MISSING_LOGFILE ) {
      ConsolePrintf("LOG READ ERROR: %i -- press a key to approve\n", e);
      while( ConsoleReadChar() > -1 );
      uint32_t Tstart = Timer::get();
      while( Timer::elapsedUs(Tstart) < 3*1000*1000 ) {
        if( ConsoleReadChar() > -1 ) { e=ERROR_OK; break; }
      }
      if( e != ERROR_OK )
        throw e;
    }//-*/
    
    if( len > 0 ) {
      logbuf[len] = '\0'; //null terminate
      FLEXFLOW::printf("<flex> log logdl_%08x_log%u.log\n", flexnfo.esn, i);
      FLEXFLOW::write( e==ERROR_OK && len>0 ? logbuf : "not found");
      FLEXFLOW::printf("\n</flex>\n");
    }
    
    if( !(e==ERROR_OK && len>0) ) {
      if( ++nerrs >= 1 && i>1 ) //always attempt log0+log1
        break;
    }
  }
}

void SweatinToTheOldies(void)
{
  const int batt_mv_cutoff=3600;
  int batt_mv=9999;
  
  while(1)
  {
    //feel the burn
    try {
      int8_t treadPwrL=100; int8_t treadPwrR=100; int8_t liftPwr=70; int8_t headPwr=70;
      int printlvl = RCOM_PRINT_LEVEL_CMD; // | RCOM_PRINT_LEVEL_NFO; // RCOM_PRINT_LEVEL_DAT | RCOM_PRINT_LEVEL_RSP
      rcomMot(120, RCOM_SENSOR_MOT_LEFT,   treadPwrL, -treadPwrR,  liftPwr, -headPwr, printlvl);
      rcomMot(120, RCOM_SENSOR_MOT_RIGHT, -treadPwrL,  treadPwrR, -liftPwr,  headPwr, printlvl);
    } catch(...) {
    }
    
    //Check battery voltage
    Contacts::setModeRx();
    Timer::delayMs(50); //let battery voltage settle
    batt_mv = robot_get_batt_mv(0, true, RCOM_PRINT_LEVEL_NONE);
    if( batt_mv <= batt_mv_cutoff )
      break;
    //-*/
    
    //test for robot removal
    Contacts::powerOn();
    //Timer::delayMs(SYSCON_CHG_PWR_DELAY_MS); //wait for charger to kick in
    int current_ma = Meter::getCurrentMa(PWR_VEXT,4);//6);
    ConsolePrintf("current = %imA\n", current_ma);
    //int voltage_mv = Meter::getVoltageMv(PWR_VEXT,4);
    if( current_ma < PRESENT_CURRENT_MA )
      break;
    //-*/
  }

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
    EmrUpdate, //set test complete flags
    TurkeysDone,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotPackoutGetTests(void) { 
  static TestFunction m_tests[] = {
    TestRobotInfo,
    TestRobotRtcValid,
    EmrChecks, //check previous test results and reset status flags
    BatteryCheck, //sensor/motors may act strange if battery is low
    TestRobotSensors,
    TestRobotTreads,
    TestRobotRange,
    ChargeTest,
    BatteryCheck,
    RobotLogCollect,
    SadBeep, //eng sound cmd only works before packout flag set
    RobotFlexFlowPackoutReport, //final report and error checks
    EmrUpdate, //set packout flag, timestamp
    RobotPowerDown,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotUnPackoutGetTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    EmrUnPackout,
    RobotPowerDown,
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

TestFunction* TestRobotSoundGetTests(void) {
  static TestFunction m_tests[] = {
    //TestRobotInfo,
    SoundLoop,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotLogDownloadTests(void) {
  static TestFunction m_tests[] = {
    TestRobotInfo,
    LogDownload,
    NULL,
  };
  return m_tests;
}

TestFunction* TestRobotGymGetTests(void)
{
  static TestFunction m_tests[] = {
    TestRobotInfo,
    SweatinToTheOldies,
    NULL,
  };
  return m_tests;
}

