#include "app.h"
#include "board.h"
#include "console.h"
#include "dut_i2c.h"
#include "fixture.h"
#include "flexflow.h"
#include "meter.h"
#include "opto.h"
#include "portable.h"
#include "robotcom.h"
#include "tests.h"
#include "testcommon.h"
#include "timer.h"
#include "vl53l0x_api.h"

//force start (via console or button) skips detect
#define TOF_REQUIRE_FORCE_START 1

//-----------------------------------------------------------------------------
//                  ?????
//-----------------------------------------------------------------------------

static uint16_t m_last_read_mm = 0;
uint16_t TestAuxTofLastRead(void) {
  return m_last_read_mm;
}

int detectPrescale=0; bool detectSticky=0;
bool TestAuxTofDetect(void)
{
  if( TOF_REQUIRE_FORCE_START && !g_isDevicePresent ) {
    TestAuxTofCleanup();
    return false;
  }
  
  //throttle detection so we don't choke out the console
  if( ++detectPrescale >= 50 ) {
    detectPrescale = 0;
    DUT_I2C::init();
    uint8_t status = DUT_I2C::readReg(0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS);
    detectSticky = status != 0xff;
  }
  
  return detectSticky;
}

void TestAuxTofCleanup(void) {
  DUT_I2C::deinit();
  detectPrescale = 0;
  detectSticky = g_isDevicePresent;
}

//-----------------------------------------------------------------------------
//                  TOF Primitives
//-----------------------------------------------------------------------------

typedef struct {
  uint32_t  spad_refSpadCount;
  uint8_t   spad_isApertureSpads;
  uint8_t   temp_vhvSettings;
  uint8_t   temp_phaseCal;
  int32_t   offset_microMeter;
  uint32_t  xtalk_compensationRateMegaCps;
} tof_calibration_dat_t;

const int PROFILE_0_SPAD_TEMP = 0;
const int PROFILE_1_OFFSET = 1;
const int PROFILE_2_XTALK = 2;
const int PROFILE_COUNT = 3;
tof_calibration_dat_t tofCalProfile[PROFILE_COUNT];
const char *tofCalProfileStr[PROFILE_COUNT] = { "spad-temp", "offset", "xtalk" };

static VL53L0X_Dev_t dev = {0};

void print_tof_cal(tof_calibration_dat_t *cal) {
  ConsolePrintf("spad %u %u temp %u %u ofs %+ium xtalk %uMCps\n",
    cal->spad_refSpadCount, cal->spad_isApertureSpads,
    cal->temp_vhvSettings, cal->temp_phaseCal,
    cal->offset_microMeter,
    cal->xtalk_compensationRateMegaCps);
}

//Note: init must be sequenced
//  tof_init_static()
//  tof_set_calibration() or tof_perform_calibration()
//  tof_init_system()
void tof_init_static(void) {
  m_last_read_mm = 0;
  //ConsolePrintf("initializing tof\n");
  memset(&dev, 0, sizeof(dev));
  dev.I2cDevAddr = TOF_SENSOR_ADDRESS;
  DUT_I2C::init();
  ConsolePrintf("VL53L0X reset device....%i\n", VL53L0X_ResetDevice(&dev) );
  Timer::delayMs(50);
  ConsolePrintf("VL53L0X data init.......%i\n", VL53L0X_DataInit(&dev) );
  ConsolePrintf("VL53L0X static init.....%i\n", VL53L0X_StaticInit(&dev) );
}

void tof_init_system(void) {
  ConsolePrintf("VL53L0X set mode........%i\n",
    VL53L0X_SetDeviceMode(&dev, VL53L0X_DEVICEMODE_SINGLE_RANGING) );
  //set up gpio here for interrupt modes
}

void tof_get_calibration(tof_calibration_dat_t *cal)
{
  memset( cal, 0, sizeof(tof_calibration_dat_t) );
  
  ConsolePrintf("read SPAD calibration...%i,cnt=%i,isAp=%i\n",
    VL53L0X_GetReferenceSpads(&dev, &cal->spad_refSpadCount, &cal->spad_isApertureSpads),
    cal->spad_refSpadCount,
    cal->spad_isApertureSpads );
  ConsolePrintf("read TEMP calibration...%i,vhv=%i,phase=%i\n",
    VL53L0X_GetRefCalibration(&dev, &cal->temp_vhvSettings, &cal->temp_phaseCal),
    cal->temp_vhvSettings,
    cal->temp_phaseCal );
  ConsolePrintf("read OFS calibration....%i,%ium\n",
    VL53L0X_GetOffsetCalibrationDataMicroMeter(&dev, &cal->offset_microMeter),
    cal->offset_microMeter );
  uint8_t isXTalkEnabled = 255;
  ConsolePrintf("read XTALK calibration..%i,%i,%iMCps,en=%i\n",
    VL53L0X_GetXTalkCompensationRateMegaCps(&dev, &cal->xtalk_compensationRateMegaCps),
    VL53L0X_GetXTalkCompensationEnable(&dev, &isXTalkEnabled),
    cal->xtalk_compensationRateMegaCps,
    isXTalkEnabled );
  uint32_t tbudget=0; //not part of calibration, but useful cfg info
  ConsolePrintf("read timing budget......%i,%ius\n",
    VL53L0X_GetMeasurementTimingBudgetMicroSeconds(&dev, &tbudget),
    tbudget );
}

void tof_set_calibration(tof_calibration_dat_t *cal, bool xtalk_enable)
{
  ConsolePrintf("set SPAD calibration....%i,cnt=%i,isAp=%i\n",
    VL53L0X_SetReferenceSpads(&dev, cal->spad_refSpadCount, cal->spad_isApertureSpads),
    cal->spad_refSpadCount,
    cal->spad_isApertureSpads );
  ConsolePrintf("set TEMP calibration....%i,vhv=%i,phase=%i\n",
    VL53L0X_SetRefCalibration(&dev, cal->temp_vhvSettings, cal->temp_phaseCal),
    cal->temp_vhvSettings,
    cal->temp_phaseCal );
  ConsolePrintf("set OFS calibration.....%i,%ium\n",
    VL53L0X_SetOffsetCalibrationDataMicroMeter(&dev, cal->offset_microMeter),
    cal->offset_microMeter );
  ConsolePrintf("set XTALK calibration...%i,%i,%iMCps,en=%i\n",
    VL53L0X_SetXTalkCompensationRateMegaCps(&dev, cal->xtalk_compensationRateMegaCps),
    VL53L0X_SetXTalkCompensationEnable(&dev, xtalk_enable),
    cal->xtalk_compensationRateMegaCps,
    xtalk_enable );
}

void tof_calibrate_spad(tof_calibration_dat_t *cal) {
  //In case a cover glass is used on top of VL53L0X, the reference SPADs have to be recalibrated by the customer.
  //It has to be done before Ref Calibration, VL53L0X_PerformRefCalibration().
  ConsolePrintf("calibrate SPAD..........%i\n",
    VL53L0X_PerformRefSpadManagement(&dev, &cal->spad_refSpadCount, &cal->spad_isApertureSpads) );
}

void tof_calibrate_temp(tof_calibration_dat_t *cal) {
  ConsolePrintf("calibrate TEMP..........%i\n",
    VL53L0X_PerformRefCalibration(&dev, &cal->temp_vhvSettings, &cal->temp_phaseCal) );
}

void tof_calibrate_ofs(tof_calibration_dat_t *cal) {
  //Both Reference SPADs and Ref calibrations have to be performed before calling offset calibration.
  ConsolePrintf("---- SET WHITE TARGET 88%% REFLECTANCE ----\n");
  //TestCommon::waitForKeypress(0,1,"press a key when ready\n");
  const int TARGET_MM_ACTUAL = 100;
  const FixPoint1616_t TARGET_MM_FIX16P16 = TARGET_MM_ACTUAL << 16;
  
  ConsolePrintf("calibrating Offset @ %imm target...", TARGET_MM_ACTUAL);
  uint32_t Tstart = Timer::get();
  int vle = VL53L0X_PerformOffsetCalibration(&dev, TARGET_MM_FIX16P16, &cal->offset_microMeter);
  ConsolePrintf("%i,%ium,%ius\n", vle, cal->offset_microMeter, Timer::elapsedUs(Tstart) );
}

void tof_calibrate_xtalk(tof_calibration_dat_t *cal) {
  //see user manual UM2039 section 2.6.2 - choosing a xtalk calibration distance
  //The cross-talk correction is basically a weighted gain applied to the ranging data, based on a calibration result.
  ConsolePrintf("---- SET GREY TARGET 17%% REFLECTANCE ----\n");
  //TestCommon::waitForKeypress(0,1,"press a key when ready\n");
  const int TARGET_MM_ACTUAL = 100;
  const FixPoint1616_t TARGET_MM_FIX16P16 = TARGET_MM_ACTUAL << 16;
  
  ConsolePrintf("calibrating XTALK @ %imm target...", TARGET_MM_ACTUAL);
  uint32_t Tstart = Timer::get();
  int vle = VL53L0X_PerformXTalkCalibration(&dev, TARGET_MM_FIX16P16, &cal->xtalk_compensationRateMegaCps);
  ConsolePrintf("%i,%iMCps,%ius\n", vle, cal->xtalk_compensationRateMegaCps, Timer::elapsedUs(Tstart) );
}

tof_dat_t* tof_read(uint32_t *out_tmeas=0, bool debug=0);
tof_dat_t* tof_read(uint32_t *out_tmeas, bool debug)
{
  static tof_dat_t mdat;
  
  //mdat = *Opto::read();
  uint32_t Tstart = Timer::get();
  VL53L0X_PerformSingleMeasurement(&dev);
  uint32_t Tmeas = Timer::elapsedUs(Tstart);
  if( out_tmeas ) *out_tmeas = Tmeas;
  
  //convert to old data format
  VL53L0X_RangingMeasurementData_t dat;
  VL53L0X_GetRangingMeasurementData(&dev, &dat);
  mdat.status = dat.RangeStatus;
  mdat.reading = __REV16(dat.RangeMilliMeter);
  mdat.signal_rate = __REV16(dat.SignalRateRtnMegaCps);
  mdat.ambient_rate = __REV16(dat.AmbientRateRtnMegaCps);
  mdat.spad_count = __REV16(dat.EffectiveSpadRtnCount);
  
  if( debug )
  {
    uint16_t reading_raw = __REV16(mdat.reading);
    ConsolePrintf("tof read: %02i %03imm srate=%05i arate=%05i spad=%05i tim=%i,%i,%i\n",
      mdat.status,
      __REV16(mdat.reading),
      __REV16(mdat.signal_rate),
      __REV16(mdat.ambient_rate),
      __REV16(mdat.spad_count),
      dat.TimeStamp,
      dat.MeasurementTimeUsec,
      Tmeas
    );
  }
  
  return &mdat;
}

//-----------------------------------------------------------------------------
//                  TOF Tests
//-----------------------------------------------------------------------------

void TOF_init(void)
{
  //const bool ofs_cal_enable = 0, xtalk_cal_enable = 0;
  tof_calibration_dat_t *cal = &tofCalProfile[PROFILE_0_SPAD_TEMP];
  memset( cal, 0, sizeof(tof_calibration_dat_t) );
  
  tof_init_static();
  tof_get_calibration(cal);
  
  print_tof_cal(cal);
  
  //maybe calibrate some stuff (in this order)
  tof_calibrate_spad(cal); //always?
  tof_calibrate_temp(cal); //always?
  //if( ofs_cal_enable ) tof_calibrate_ofs(cal);
  //if( xtalk_cal_enable ) tof_calibrate_xtalk(cal);
  
  print_tof_cal(cal);
  
  //tof_set_calibration(cal, xtalk_cal_enable);
  tof_init_system();
  
  //re-read for sanity check
  tof_calibration_dat_t readcal;
  memset(&readcal, 0, sizeof(tof_calibration_dat_t) );
  tof_get_calibration(&readcal);
  print_tof_cal(&readcal);
  print_tof_cal(cal);
}

//Playpen: 80mm+/-20mm after -30 window adjustment
const int tof_window_adjust = 30;
const int tof_read_min = 80-15; //test to tighter tolerance than playpen
const int tof_read_max = 80+15;

void TOF_sensorCheck(void)
{
  ConsolePrintf("sampling TOF sensor:\n");
  int reading_avg=0; const int num_readings=25;
  for(int i=-10; i<num_readings; i++)
  {
    tof_dat_t tof = *tof_read();
    uint16_t value = __REV16(tof.reading);
    if( i>=0 ) reading_avg += value; //don't include initial readings; "warm-up" period
    if( i==0 ) ConsolePrintf("\n");
    ConsolePrintf("%i,", value );
  }
  ConsolePrintf("\n");
  
  reading_avg /= num_readings;
  uint16_t reading_adj = reading_avg > tof_window_adjust ? reading_avg - tof_window_adjust : 0;
  
  //save result for display
  m_last_read_mm = reading_adj;
  
  //FlexFlow report (print before throwing err to allow data collection on failed sensors)
  FLEXFLOW::printf("<flex> TOF reading %imm </flex>\n", reading_adj );
  
  //nominal reading expected by playpen 90-130mm raw (80mm +/-20mm after -30 window adjustment)
  if( reading_adj < tof_read_min || reading_adj > tof_read_max )
    throw ERROR_SENSOR_TOF;
}

void TOF_debugInspectRaw(void)
{
  uint32_t Tsample = 0, displayErr=0;
  while( ConsoleReadChar() > -1 );
  
  //ignore spurious initial readings
  for(int i=0; i<5; i++)
    tof_read();
  
  //spew raw sensor data
  while(1)
  {
    if( Timer::elapsedUs(Tsample) > 250*1000 )
    {
      Tsample = Timer::get();
      
      bool read_debug = true;
      tof_dat_t tof = *tof_read(0, read_debug);
      uint16_t reading_raw = __REV16(tof.reading);
      uint16_t reading_adj = reading_raw > tof_window_adjust ? reading_raw - tof_window_adjust : 0;
      if( !read_debug ) {
        ConsolePrintf("TOF: status=%03i mm=%05i mm.adj=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
          tof.status, reading_raw, reading_adj, __REV16(tof.signal_rate), __REV16(tof.ambient_rate), __REV16(tof.spad_count) );
      }
      m_last_read_mm = reading_adj; //save for display
      
      //real-time measurements on helper display
      if( displayErr < 3 ) { //give up if no helper detected
        char b[30], color = reading_adj < tof_read_min || reading_adj > tof_read_max ? 'r' : 'g';
        //int status = helperLcdShow(1,0,color, snformat(b,sizeof(b),"%imm [%i]   ", reading_adj, reading_raw) );
        int status = helperLcdShow(1,0,color, snformat(b,sizeof(b),"%imm", reading_adj) );
        displayErr = status==0 ? 0 : displayErr+1;
      }
    }
    
    //break on console input
    if( ConsoleReadChar() > -1 ) break;
    
    //break on btn4 press
    if( Board::btnEdgeDetect(Board::BTN_4, 1000, 50) > 0 )
      break;
    
    //break on device disconnect
    uint8_t status = DUT_I2C::readReg(0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS);
    if( status == 0xff ) {
      ConsolePrintf("disconnected\n");
      break;
    }
  }
  
  if( m_last_read_mm < tof_read_min || m_last_read_mm > tof_read_max )
    throw ERROR_SENSOR_TOF;
}

//-----------------------------------------------------------------------------
//                  CLI DEBUG
//-----------------------------------------------------------------------------

bool tofCalProfilesValid = 0;
void tof_build_profiles(void)
{
  //Note: make sure sensor is in calibration position
  //memset( &tofCalProfile[0], 0, PROFILE_COUNT*sizeof(tof_calibration_dat_t) );
  memset( tofCalProfile, 0, sizeof(tofCalProfile) );
  
  ConsolePrintf("BUILDING CALIBRATION PROFILES:\n");
  tof_init_static(); //reset
  
  //get default cal settings
  tof_get_calibration( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  tofCalProfile[PROFILE_1_OFFSET] = tofCalProfile[PROFILE_0_SPAD_TEMP];
  tofCalProfile[PROFILE_2_XTALK] = tofCalProfile[PROFILE_0_SPAD_TEMP];
  
  //DEBUG
  ConsolePrintf("SANITY CHECK:\n");
  print_tof_cal( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  print_tof_cal( &tofCalProfile[PROFILE_1_OFFSET] );
  print_tof_cal( &tofCalProfile[PROFILE_2_XTALK] );
  //-*/
  
  //Spad+Temp calibration
  tof_calibrate_spad( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  tof_calibrate_temp( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  
  //Offset calibration
  tofCalProfile[PROFILE_1_OFFSET] = tofCalProfile[PROFILE_0_SPAD_TEMP];
  Timer::delayMs(150);
  tof_calibrate_ofs( &tofCalProfile[PROFILE_1_OFFSET] );
  
  //Crosstalk calibration
  tofCalProfile[PROFILE_2_XTALK] = tofCalProfile[PROFILE_1_OFFSET];
  Timer::delayMs(250);
  tof_calibrate_xtalk( &tofCalProfile[PROFILE_2_XTALK] );
  
  //DEBUG
  ConsolePrintf("SANITY CHECK:\n");
  print_tof_cal( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  print_tof_cal( &tofCalProfile[PROFILE_1_OFFSET] );
  print_tof_cal( &tofCalProfile[PROFILE_2_XTALK] );
  //-*/
  
  tof_init_system();
  tofCalProfilesValid = 1;
}

void tof_sample_profile_readings(void)
{
  if( !tofCalProfilesValid ) { ConsolePrintf("invalid profiles\n"); return; }
  int distance[PROFILE_COUNT]; memset(&distance,0,sizeof(distance));
  
  for(int i=0; i<PROFILE_COUNT; i++)
  {
    ConsolePrintf("SAMPLE USING CAL PROFILE: %s\n", tofCalProfileStr[i] );
    tof_calibration_dat_t *cal = &tofCalProfile[i];
    
    //apply profile calibration
    tof_init_static(); //reset
    bool xtalk_cal_enable = i>=PROFILE_2_XTALK;
    tof_set_calibration(cal, xtalk_cal_enable);
    tof_init_system();
    
    //verify settings
    tof_calibration_dat_t readback = {0};
    tof_get_calibration(&readback);
    if( 0
        || readback.spad_refSpadCount != cal->spad_refSpadCount
        || readback.spad_isApertureSpads != cal->spad_isApertureSpads
        || readback.temp_phaseCal != cal->temp_phaseCal
        || readback.temp_vhvSettings != cal->temp_vhvSettings
        || readback.offset_microMeter != cal->offset_microMeter
        //|| readback.xtalk_compensationRateMegaCps != cal->xtalk_compensationRateMegaCps
        || ABS((int)readback.xtalk_compensationRateMegaCps - (int)cal->xtalk_compensationRateMegaCps) > 10
    )
    {
      ConsolePrintf("---------------FAILED READBACK VALIDATION---------------\n");
      print_tof_cal(cal);
      print_tof_cal(&readback);
      distance[i] = -1;
    }
    else
    {
      //take averaged reading
      int avg=0; const int nsamples=32;
      for(int n=-5; n<nsamples; n++) {
        int value = __REV16( tof_read(0,true)->reading );
        if( n>=0 ) avg += value;
      }
      distance[i] = avg/nsamples;
    }
  }
  
  ConsolePrintf("distance mm:\n");
  for(int i=0; i<PROFILE_COUNT; i++) {
    ConsolePrintf("  %-10s: %i\n", tofCalProfileStr[i], distance[i] );
  }
}

const char* TOF_CMD_HANDLER(const char *line, int len)
{
  tof_calibration_dat_t *cal = &tofCalProfile[PROFILE_0_SPAD_TEMP];
  
  //parse cmd line args (ints)
  //int larg[6];
  int nargs = cmdNumArgs((char*)line);
  //for(int x=0; x<sizeof(larg)/sizeof(int); x++)
  //  larg[x] = nargs > x+1 ? cmdParseInt32(cmdGetArg((char*)line,x+1) ) : 0;
  
  if( !strncmp(line,"reset",5) )
    tof_init_static(), tofCalProfilesValid = 0;
  else if( !strcmp(line,"start") )
    tof_init_system();
  else if( !strcmp(line,"getcal") )
    tof_get_calibration(cal), print_tof_cal(cal);
  else if( !strcmp(line,"setcal") )
    ConsolePrintf("(disabled)\n"); //tof_set_calibration(cal, xtalk_cal_enable), print_tof_cal(cal);
  else if( !strncmp(line,"cal",3) )
  {
    if( nargs < 2 ) {
      tof_calibrate_spad(cal), tof_calibrate_temp(cal);
    } else {
      for(int i=2; i<=nargs; i++) {
        char *arg = cmdGetArg((char*)line,i-1);
        if( !strcmp(arg, "spad") )  tof_calibrate_spad(cal);
        if( !strcmp(arg, "temp") )  tof_calibrate_temp(cal);
        if( !strcmp(arg, "ofs") )   tof_calibrate_ofs(cal);
        if( !strcmp(arg, "xtalk") ) tof_calibrate_xtalk(cal);
      }
    }
    print_tof_cal(cal);
  }
  else if( !strncmp(line,"read",4) )
  {
    int avg=0, readCnt=0, readLimit=0;
    if( nargs>=2 ) {
      int lim = cmdParseInt32( cmdGetArg((char*)line,1) );
      if( lim>0 ) readLimit = lim;
    }
    
    while( !readLimit || readCnt < readLimit )
    {
      tof_dat_t tof = *tof_read(0, true); //print=true
      avg += __REV16(tof.reading);
      readCnt++;
      if( !readLimit && !TestCommon::waitForKeypress(100000,0,0) )
        break;
    }
    
    if( readCnt > 0 )
      ConsolePrintf("tof read avg: %i\n", avg/readCnt);
    
    return "\n"; //sends the debug line to charge contacts (ignored by robot, but keeps cmd in recall buffer)
  }
  else if( !strcmp(line, "profile") )
    tof_build_profiles();
  else if( !strcmp(line,"sample") )
    tof_sample_profile_readings();
  
  return "";
  //return "\n"; //sends the debug line to charge contacts (ignored by robot, but keeps cmd in recall buffer)
  //return 0; //send original line unmodified
}

void TOF_CLI_DEBUG(void)
{
  const int bridgeOpts = BRIDGE_OPT_LOCAL_ECHO | BRIDGE_OPT_LINEBUFFER | BRIDGE_OPT_CHG_DISABLE;
  TestCommon::consoleBridge(TO_CONTACTS, 0, 0, bridgeOpts, TOF_CMD_HANDLER);
}

//-----------------------------------------------------------------------------
//                  Tests
//-----------------------------------------------------------------------------

TestFunction* TestAuxTofGetTests(void)
{
  static TestFunction m_tests[] = {
    TOF_init,
    TOF_sensorCheck,
    NULL,
  };
  static TestFunction m_tests_debug[] = {
    TOF_init,
    TOF_debugInspectRaw,
    NULL,
  };
  static TestFunction m_tests_calibrate[] = {
    //TOF_init,
    TOF_CLI_DEBUG,
    //TOF_sensorCheck,
    //TOF_debugInspectRaw,
    NULL,
  };
  
  if( g_fixmode==FIXMODE_TOF_DEBUG ) return m_tests_debug;
  if( g_fixmode==FIXMODE_TOF_CALIB ) return m_tests_calibrate;
  return m_tests;
}

