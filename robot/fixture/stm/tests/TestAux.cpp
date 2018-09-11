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

typedef enum { 
  DRV_DISABLED = 0,
  DRV_ST = 1, 
  DRV_OPTO = 2, 
} tof_drv_t;

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
const int PROFILE_3_OPTO = 3;
const int PROFILE_COUNT = 4;
tof_calibration_dat_t tofCalProfile[PROFILE_COUNT];
const char *tofCalProfileStr[PROFILE_COUNT] = { "spad-temp", "offset", "xtalk", "opto-drv" };

static VL53L0X_Dev_t dev = {0};
static tof_drv_t tof_driver_current = DRV_DISABLED;

void print_tof_cal(tof_calibration_dat_t *cal) {
  ConsolePrintf("spad %u %u temp %u %u ofs %+ium xtalk %uMCps\n",
    cal->spad_refSpadCount, cal->spad_isApertureSpads,
    cal->temp_vhvSettings, cal->temp_phaseCal,
    cal->offset_microMeter,
    cal->xtalk_compensationRateMegaCps);
}

void tof_st_reset(void) {
  m_last_read_mm = 0;
  memset(&dev, 0, sizeof(dev));
  dev.I2cDevAddr = TOF_SENSOR_ADDRESS;
  Opto::stop(); //this may disable i2c driver
  DUT_I2C::init();
  ConsolePrintf("VL53L0X reset device....%i\n", VL53L0X_ResetDevice(&dev) );
  Timer::delayMs(50);
  tof_driver_current = DRV_DISABLED;
}

//Note: init must be sequenced
//  tof_st_init_static()
//  tof_set_calibration() or tof_perform_calibration()
//  tof_init_system()
void tof_st_init_static(void) {
  if( tof_driver_current != DRV_DISABLED ) ConsolePrintf("----- RE-INIT ALREADY INITIALIZED DRIVER -----: drv=%i\n", tof_driver_current);
  ConsolePrintf("VL53L0X data init.......%i\n", VL53L0X_DataInit(&dev) );
  ConsolePrintf("VL53L0X static init.....%i\n", VL53L0X_StaticInit(&dev) );
  tof_driver_current = DRV_ST;
}

void tof_init_system(void) {
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
  ConsolePrintf("VL53L0X set mode........%i\n",
    VL53L0X_SetDeviceMode(&dev, VL53L0X_DEVICEMODE_SINGLE_RANGING) );
  //set up gpio here for interrupt modes
}

void tof_get_calibration(tof_calibration_dat_t *cal)
{
  memset( cal, 0, sizeof(tof_calibration_dat_t) );
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
  
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
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
  
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
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
  //In case a cover glass is used on top of VL53L0X, the reference SPADs have to be recalibrated by the customer.
  //It has to be done before Ref Calibration, VL53L0X_PerformRefCalibration().
  ConsolePrintf("calibrate SPAD..........%i\n",
    VL53L0X_PerformRefSpadManagement(&dev, &cal->spad_refSpadCount, &cal->spad_isApertureSpads) );
}

void tof_calibrate_temp(tof_calibration_dat_t *cal) {
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
  ConsolePrintf("calibrate TEMP..........%i\n",
    VL53L0X_PerformRefCalibration(&dev, &cal->temp_vhvSettings, &cal->temp_phaseCal) );
}

void tof_calibrate_ofs(tof_calibration_dat_t *cal) {
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
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
  if( tof_driver_current != DRV_ST ) ConsolePrintf("----- INVALID DRIVER MODE -----: drv=%i\n", tof_driver_current);
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

void tof_init_(tof_drv_t driver = DRV_ST);
void tof_init_(tof_drv_t driver)
{
  tof_st_reset();
  
  if( driver == DRV_ST )
  {
    tof_calibration_dat_t readcal, defaultcal, *cal = &tofCalProfile[PROFILE_0_SPAD_TEMP];
    memset( cal, 0, sizeof(tof_calibration_dat_t) );
    memset( &readcal, 0, sizeof(tof_calibration_dat_t) );
    memset( &defaultcal, 0, sizeof(tof_calibration_dat_t) );
    
    tof_st_init_static(); //sets tof_driver_current = DRV_ST
    tof_get_calibration(cal);
    defaultcal = *cal;
    
    //maybe calibrate some stuff (req. in this order)
    tof_calibrate_spad(cal);
    tof_calibrate_temp(cal);
    //tof_calibrate_ofs(cal);
    //tof_calibrate_xtalk(cal);
    tof_init_system();
    
    //re-read for sanity check
    tof_get_calibration(&readcal);
    ConsolePrintf("cal,default: "); print_tof_cal( &defaultcal );
    ConsolePrintf("cal,live   : "); print_tof_cal( cal );
    ConsolePrintf("cal,verify : "); print_tof_cal( &readcal );
  }
  else if( driver == DRV_OPTO )
  {
    Opto::start();
    tof_driver_current = DRV_OPTO;
  }
}

int convert_range_status_(uint8_t DeviceRangeStatus)
{
  //ST driver converts raw status reg value using:
  //VL53L0X_GetRangingMeasurementData() ->
  //  VL53L0X_get_pal_range_status()
  
  uint8_t value, *pPalRangeStatus = &value;
	uint8_t DeviceRangeStatusInternal = ((DeviceRangeStatus & 0x78) >> 3);

  if (DeviceRangeStatusInternal == 0 ||
		DeviceRangeStatusInternal == 5 ||
		DeviceRangeStatusInternal == 7 ||
		DeviceRangeStatusInternal == 12 ||
		DeviceRangeStatusInternal == 13 ||
		DeviceRangeStatusInternal == 14 ||
		DeviceRangeStatusInternal == 15
			) {
    *pPalRangeStatus = 255;	 /* NONE */
  } else if (DeviceRangeStatusInternal == 1 ||
             DeviceRangeStatusInternal == 2 ||
             DeviceRangeStatusInternal == 3) {
    *pPalRangeStatus = 5; /* HW fail */
  } else if (DeviceRangeStatusInternal == 6 ||
             DeviceRangeStatusInternal == 9) {
    *pPalRangeStatus = 4;  /* Phase fail */
  } else if (DeviceRangeStatusInternal == 8 ||
             DeviceRangeStatusInternal == 10 ||
             0) { //SignalRefClipflag == 1) {
    *pPalRangeStatus = 3;  /* Min range */
  } else if (DeviceRangeStatusInternal == 4 ||
             0) { //RangeIgnoreThresholdflag == 1) {
    *pPalRangeStatus = 2;  /* Signal Fail */
  //} else if (SigmaLimitflag == 1) {
  //  *pPalRangeStatus = 1;  /* Sigma	 Fail */
  } else {
    *pPalRangeStatus = 0; /* Range Valid */
  }
  
  return *pPalRangeStatus;
}

tof_dat_t* tof_read(bool debug)
{
  static tof_dat_t mdat;
  VL53L0X_RangingMeasurementData_t dat;
  memset(&mdat, 0, sizeof(tof_dat_t) );
  memset(&dat, 0, sizeof(VL53L0X_RangingMeasurementData_t) );
  
  uint8_t status_raw = 0;
  uint32_t T1=0, T2=0, Tstart=Timer::get();
  if( tof_driver_current == DRV_OPTO )
  {
    mdat = *Opto::read();
    T2 = Timer::elapsedUs(Tstart);
    
    //condition status code to match ST driver
    status_raw = mdat.status; //save raw for debug
    mdat.status = convert_range_status_(mdat.status);
  }
  else if( tof_driver_current == DRV_ST )
  {
    VL53L0X_PerformSingleMeasurement(&dev);
    T1 = Timer::elapsedUs(Tstart);
    VL53L0X_GetRangingMeasurementData(&dev, &dat);
    T2 = Timer::elapsedUs(Tstart);
    
    //convert to opto format
    mdat.status = dat.RangeStatus;
    mdat.reading = __REV16(dat.RangeMilliMeter);
    mdat.signal_rate = __REV16(dat.SignalRateRtnMegaCps);
    mdat.ambient_rate = __REV16(dat.AmbientRateRtnMegaCps);
    mdat.spad_count = __REV16(dat.EffectiveSpadRtnCount);
  }
  
  if( debug )
    ConsolePrintf("tof read: %02i,%02i %03imm srate=%05i arate=%05i spad=%05i T=%i,%ius\n",
      mdat.status, status_raw,
      __REV16(mdat.reading),
      __REV16(mdat.signal_rate),
      __REV16(mdat.ambient_rate),
      __REV16(mdat.spad_count),
      T1, T2 );
  
  return &mdat;
}

//-----------------------------------------------------------------------------
//                  TOF Tests
//-----------------------------------------------------------------------------

void TOF_init(void) { 
  tof_init_(DRV_ST);
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
    tof_dat_t tof = *tof_read(0);
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
  uint32_t Tsample = 0;
  while( ConsoleReadChar() > -1 );
  
  //ignore spurious initial readings
  for(int i=0; i<5; i++)
    tof_read(0);
  
  //spew raw sensor data
  while(1)
  {
    if( Timer::elapsedUs(Tsample) > 250*1000 )
    {
      Tsample = Timer::get();
      
      tof_dat_t tof = *tof_read(1);
      uint16_t reading_raw = __REV16(tof.reading);
      uint16_t reading_adj = reading_raw > tof_window_adjust ? reading_raw - tof_window_adjust : 0;
      //ConsolePrintf("TOF: status=%03i mm=%05i mm.adj=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
      //  tof.status, reading_raw, reading_adj, __REV16(tof.signal_rate), __REV16(tof.ambient_rate), __REV16(tof.spad_count) );
      m_last_read_mm = reading_adj; //save for display
      
      //real-time measurements on helper display
      if( helperConsecutiveFailCnt()<5 ) { //give up if no helper detected
        char b[30], color = reading_adj < tof_read_min || reading_adj > tof_read_max ? 'r' : 'g';
        helperLcdShow(1,0,color, snformat(b,sizeof(b),"%imm", reading_adj) );
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
  tof_st_reset();
  tof_st_init_static();
  
  //get default cal settings
  tof_get_calibration( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  tofCalProfile[PROFILE_1_OFFSET] = tofCalProfile[PROFILE_0_SPAD_TEMP];
  tofCalProfile[PROFILE_2_XTALK] = tofCalProfile[PROFILE_0_SPAD_TEMP];
  
  //DEBUG
  ConsolePrintf("SANITY CHECK:\n");
  print_tof_cal( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  print_tof_cal( &tofCalProfile[PROFILE_1_OFFSET] );
  print_tof_cal( &tofCalProfile[PROFILE_2_XTALK] );
  print_tof_cal( &tofCalProfile[PROFILE_3_OPTO] );
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
  
  //"Opto" is not a calibration, but a separate driver entirely
  tofCalProfile[PROFILE_3_OPTO] = tofCalProfile[PROFILE_0_SPAD_TEMP];
  
  //DEBUG
  ConsolePrintf("SANITY CHECK:\n");
  print_tof_cal( &tofCalProfile[PROFILE_0_SPAD_TEMP] );
  print_tof_cal( &tofCalProfile[PROFILE_1_OFFSET] );
  print_tof_cal( &tofCalProfile[PROFILE_2_XTALK] );
  print_tof_cal( &tofCalProfile[PROFILE_3_OPTO] );
  //-*/
  
  tof_init_system();
  tofCalProfilesValid = 1;
}

void tof_sample_profile_readings(void)
{
  if( !tofCalProfilesValid ) { ConsolePrintf("invalid profiles\n"); return; }
  struct { int distance, min, max; } meas[PROFILE_COUNT];
  memset( &meas, 0, sizeof(meas) );
  
  for(int i=0; i<PROFILE_COUNT; i++)
  {
    ConsolePrintf("SAMPLE USING CAL PROFILE: %s\n", tofCalProfileStr[i] );
    tof_calibration_dat_t *cal = &tofCalProfile[i];
    
    if( i == PROFILE_3_OPTO )
      tof_init_(DRV_OPTO);
    else
    {
      //apply profile calibration
      tof_st_reset();
      tof_st_init_static();
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
        meas[i].distance = -1;
      }
    }
    
    if( meas[i].distance > -1 )
    {
      //take averaged reading
      const int nsamples=32;
      uint8_t status=0;
      
      meas[i].min = 9999;
      for(int n=-5; n<nsamples; n++)
      {
        tof_dat_t *tof = tof_read(1);
        if( n>=0 ) {
          uint16_t val = __REV16( tof->reading );
          if( val > meas[i].max ) meas[i].max = val;
          if( val < meas[i].min ) meas[i].min = val;
          meas[i].distance += val;
        }
        
        if( tof->status != 0 ) {
          ConsolePrintf("---------------MEASUREMENT ERROR: %i---------------\n", tof->status);
          status = tof->status;
        }
      }
      meas[i].distance = status==0 ? meas[i].distance/nsamples : -1;
    }
  }
  
  ConsolePrintf("distance mm:\n");
  for(int i=0; i<PROFILE_COUNT; i++) {
    int largestErr = MAX( meas[i].distance-meas[i].min, meas[i].max-meas[i].distance );
    int pctErr = (100 * largestErr) / meas[i].distance;
    ConsolePrintf("  %-10s: %3i ...var %i%% [%i,%i] %s\n", tofCalProfileStr[i], meas[i].distance, pctErr, meas[i].max, meas[i].min, pctErr>10 ? "------!!!!!!!!!!!!!---------" : "" );
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
    tof_st_reset(), tof_st_init_static(), tofCalProfilesValid = 0;
  else if( !strcmp(line,"start") )
    tof_init_system();
  else if( !strcmp(line,"initopto") )
    tof_init_(DRV_OPTO), tofCalProfilesValid = 0;
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
      tof_dat_t tof = *tof_read(1);
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
  else if( !strcmp(line,"sample") ) {
    tof_sample_profile_readings();
    return "\n"; //keeps cmd in recall buffer
  }
  
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

