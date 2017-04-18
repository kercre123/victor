#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/monitor.h"
#include "hal/radio.h"
#include "hal/flash.h"

#include "app/app.h"
#include "app/fixture.h"
#include "app/binaries.h"

#include "../syscon/hal/tests.h"
#include "../syscon/hal/hardware.h"
#include "../generated/clad/robot/clad/types/motorTypes.h"
#include "../generated/clad/robot/clad/types/fwTestMessages.h"

using namespace Anki::Cozmo;
using namespace Anki::Cozmo::RobotInterface;

// XXX: Fix this if you ever fix current monitoring units
// Robot is up and running - usually around 48K, rebooting at 32K - what a mess!
#define BOOTED_CURRENT_MA   137   //OLD: 40000 (mis-calibrated 'uA' value) / 290 (scale factor to mA)
#define PRESENT_CURRENT_MA  3     //OLD: 1000  (mis-calibrated 'uA' value) / 290 (scale factor to mA)

//Bodycolors
enum {
  BODYCOLOR_EMPTY             = ~0,
  BODYCOLOR_M1_WHITE_COZ1V0   = 0,  //v1.0 Cozmo (1.0 bootloader had unused data set to 0)
  BODYCOLOR_M1_RESERVED       = 1,
  BODYCOLOR_M2_WHITE          = 2,  //standard white (v1.5 and later)
  BODYCOLOR_M3_CE_LM          = 3,  //Collectors edition, Liquid Metal
  BODYCOLOR_END,
};

//buttonTest.c
extern void ButtonTest(void);

//get averaged/LPF current measurement
static int mGetCurrentMa(void)
{
  int current_ma = 0;
  for (int i = 0; i < 64; i++) {
    MicroWait(750);
    current_ma += ChargerGetCurrentMa();
  }
  return (current_ma >> 6);
}

// Return true if device is detected on contacts
bool RobotDetect(void)
{
  // Turn power on and check for current draw
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_IN(GPIOC, PINC_CHGRX);
  MicroWait(500);
  
  // Hysteresis for shut-down robots 
  int now = ChargerGetCurrentMa();
  if (g_isDevicePresent && now > PRESENT_CURRENT_MA)
    return true;
  if (now > BOOTED_CURRENT_MA)
    return true;
  return false;
}

void SendTestChar(int c);
bool g_allowOutdated = false;
void AllowOutdated()
{
  g_allowOutdated = true;
}

void EnableChargeComms(void)
{
  //configure charge pins for communication mode and check for active pulses from robot
  //ConsolePrintf("enable charge comms\r\n");
  int e=0, n = 0;
  for (int i = 0; i < 4; i++) {
    try { SendTestChar(-1); }
    catch (int e) { n++; e=e; }
  }
  if( n > 2 ) {
    ConsolePrintf("charge comm init: %d missed pulses\r\n", n);
    throw e;
  }
  
  // Let Espressif finish booting
  MicroWait(300000);
}

static const int MAX_BODYCOLORS = 4;
static int m_bodyColorCnt = 0;
static s32 m_bodyColor = BODYCOLOR_EMPTY;
static void readBodycolor(void)
{
  struct { u32 version[2]; s32 bodycolor[MAX_BODYCOLORS]; } dat;
  
  dat.bodycolor[MAX_BODYCOLORS-1] = 0xABCD;
  SendCommand(TEST_GETVER, 0, sizeof(dat), (u8*)&dat);
  if(dat.bodycolor[MAX_BODYCOLORS-1] == 0xABCD)
    throw ERROR_BODY_OUTOFDATE;
  
  ConsolePrintf("bodycolor,%d,%d,%d,%d\r\n", dat.bodycolor[0], dat.bodycolor[1], dat.bodycolor[2], dat.bodycolor[3] );
  
  //Actual bodycolor is the highest index [valid] value
  m_bodyColorCnt = 0;
  m_bodyColor = BODYCOLOR_EMPTY;
  for( int i=MAX_BODYCOLORS-1; i>=0; i-- ) {
    if( dat.bodycolor[i] == BODYCOLOR_EMPTY ) continue; //empty slot
    if( dat.bodycolor[i] < 0 || dat.bodycolor[i] >= BODYCOLOR_END )
      throw ERROR_BODYCOLOR_INVALID;
    m_bodyColor = dat.bodycolor[i];
    m_bodyColorCnt = i+1;
    break;
  }
}

static void setBodycolor(u8 bodycolor)
{
  ConsolePrintf("set bodycolor: %d\r\n", bodycolor);
  readBodycolor(); //read current color array/count
  
  if( bodycolor != m_bodyColor ) { //only write changed values
    if( m_bodyColorCnt >= MAX_BODYCOLORS )
      throw ERROR_BODYCOLOR_FULL;
    SendCommand(TEST_SETCOLOR, bodycolor, 0, 0);
  }
  
  //verify
  readBodycolor(); //readback
  if( m_bodyColor != bodycolor ) //write failed
    throw ERROR_BODYCOLOR_INVALID;
}

//write the body color code into syscon flash
static void WriteBodyColor(void)
{
  if( g_fixtureType == FIXTURE_ROBOT3_TEST )
    setBodycolor( BODYCOLOR_M2_WHITE );
  if( g_fixtureType == FIXTURE_ROBOT3_CE_TEST )
    setBodycolor( BODYCOLOR_M3_CE_LM );
}

static void VerifyBodyColor(void)
{
  readBodycolor();
  
  if( g_fixtureType == FIXTURE_PACKOUT_TEST && m_bodyColor != BODYCOLOR_M2_WHITE )
    throw ERROR_BODYCOLOR_INVALID;
  if( g_fixtureType == FIXTURE_PACKOUT_CE_TEST && m_bodyColor != BODYCOLOR_M3_CE_LM )
    throw ERROR_BODYCOLOR_INVALID;
}

void InfoTest(void)
{
  struct { u32 version[2]; s32 bodycolor[MAX_BODYCOLORS]; } dat;
  
  EnableChargeComms();
  
  ConsolePrintf("read robot version:\r\n");
  SendCommand(1, 0, 0, 0);    // Put up info face and turn off motors
  dat.bodycolor[MAX_BODYCOLORS-1] = 0xABCD;
  SendCommand(TEST_GETVER, 0, sizeof(dat), (u8*)&dat);
  if(dat.bodycolor[MAX_BODYCOLORS-1] == 0xABCD)
    throw ERROR_BODY_OUTOFDATE;
  
  // Mimic robot format for SMERP
  int unused = dat.version[0]>>16, hwversion = dat.version[0]&0xffff, esn = dat.version[1];
  ConsolePrintf("version,%08d,%08d,%08x,00000000\r\n", unused, hwversion, esn);
  
  if (hwversion < BODY_VER_SHIP && !g_allowOutdated)
    throw ERROR_BODY_OUTOFDATE;
  
  readBodycolor();
}

const int playpenPowerOnDuration_s = 120;
void PlaypenTest(void)
{
  EnableChargeComms();
  
  // Try to put robot into playpen mode
  SendCommand(FTM_PlayPenTest, (FIXTURE_SERIAL)&63, 0, 0);
  SendCommand(TEST_POWERON, playpenPowerOnDuration_s, 0, 0);
  
  // Do this last:  Try to put fixture radio in advertising mode
  static bool setRadio = false;
  if (!setRadio)
    SetRadioMode('A',0);
  setRadio = true;
}

extern int g_stepNumber;
void PlaypenWaitTest(void)
{
  const int topOffInterval_s = 30;
  u32 powerOnTime = 0, offContact = 0, totalTime_s = 0;
  while( 1 )
  {
    //Periodically extend power-on
    if( getMicroCounter() - powerOnTime >= topOffInterval_s*1000*1000 ) {
      totalTime_s += topOffInterval_s; //track total time on fixture
      ConsolePrintf("\r\nTEST_POWERON %ds (total %ds)\r\n", playpenPowerOnDuration_s, totalTime_s);
      EnableChargeComms(); //switch to comm mode
      SendCommand(TEST_POWERON, playpenPowerOnDuration_s, 0, 0);
      powerOnTime = getMicroCounter();
    }
    
    //Turn on charging power
    PIN_SET(GPIOC, PINC_CHGTX);
    PIN_OUT(GPIOC, PINC_CHGTX);
    
    //test for robot removal
    int current_ma = mGetCurrentMa();
    ConsolePrintf("%d..", current_ma);
    if ((offContact = current_ma < PRESENT_CURRENT_MA ? offContact + 1 : 0) > 10)
      return;
  }
}

int TryMotor(s8 motor, s8 speed, bool limitToLimit = false);

// Run motors through their paces
const int SLOW_DRIVE_THRESH = 130, FAST_DRIVE_THRESH = 1100;    // mm/sec - fast drive was relaxed due to "unfair" failures
const int SLOW_LIFT_THRESH = 1000, FAST_LIFT_THRESH = 60000;    // Should be - 67.4 deg full-scale minimum, but we don't reach it at power 80
const int FAST_LIFT_MAX = FAST_LIFT_THRESH + 8000;      // Lift rarely exceeds +8 deg
const int SLOW_HEAD_THRESH = 800, FAST_HEAD_THRESH = 70000;    // Should be - 70.0 deg full-scale minimum
const int FAST_HEAD_MAX = FAST_HEAD_THRESH + 10000;     // Head is typically +6 deg (weird)
const int SLOW_POWER = 40, FAST_POWER = 124, FAST_HEADLIFT_POWER = 80;

// Run a motor forward and backward, check minimum and (optional) maximum
void CheckMotor(u8 motor, u8 power, int min, int max)
{
  int errbase = ERROR_MOTOR_LEFT + motor*10;

  // For packout, scale limits to 80%
  if (g_fixtureType == FIXTURE_PACKOUT_TEST || g_fixtureType == FIXTURE_PACKOUT_CE_TEST)
    min = (min * 13) >> 4;
  
  // Move lift/head out of the way before test
  if (motor >= 2)
  {
    if (power <= SLOW_POWER)
      TryMotor(-motor, -power);   // Give extra space at slow power
    TryMotor(-motor, -power);
  }

  // Test forward first
  int result = TryMotor(motor, power, max != 0);// If max, run limitToLimit
  if (result < 0)
    throw errbase + 2;    // Wired backward
  if (result < min)
  {
    if (power <= SLOW_POWER)
      throw errbase;      // Slow fail
    else
      throw errbase + 1;  // Fast fail
  }
  if (max && result > max)
    throw errbase + 3;    // Above max
  
  // Now test reverse
  result = TryMotor(motor, -power, max != 0);   // If max, run limitToLimit
  if (result > 0)
    throw errbase + 2;    // Wired backward
  if (result > -min)
  {
    if (power <= SLOW_POWER)
      throw errbase;      // Slow fail
    else
      throw errbase + 1;  // Fast fail
  }
  if (max && result < -max)
    throw errbase + 3;    // Above max  
}

// Check that motors can run at slow speed
void SlowMotors(void)
{
  CheckMotor(MOTOR_LEFT_WHEEL,  SLOW_POWER, SLOW_DRIVE_THRESH, 0);
  CheckMotor(MOTOR_RIGHT_WHEEL, SLOW_POWER, SLOW_DRIVE_THRESH, 0);
  CheckMotor(MOTOR_LIFT,        SLOW_POWER, SLOW_LIFT_THRESH, 0);
  CheckMotor(MOTOR_HEAD,        SLOW_POWER, SLOW_HEAD_THRESH, 0);
}

const int JAM_POWER = 124;    // Full power
const int JAM_THRESH[4] = { 2000, 2000, 150000, 70000 };  // Measured on 1 unit
void Jam(u8 motor)
{
  int pos[4];
  int lastpos = 0, diff;
  int errbase = ERROR_MOTOR_LEFT + motor*10;
  
  // Reset motor watchdog and start yanking on the motor
  SendCommand(TEST_POWERON, 5-1, 0, NULL);  // 5 seconds
  
  for (int i = 0; i < 8; i++)
  {
    // Each command takes 25-35ms (depending on timing, due to 4 bytes at 3/4 duty cycle at 5ms)
    SendCommand(TEST_RUNMOTOR, (JAM_POWER&0xFC) + motor, 0, NULL);
    SendCommand(TEST_GETMOTOR, 0, sizeof(pos), (u8*)pos);
    
    // Now driving forward, but results are from reverse
    diff = pos[motor] - lastpos;
    lastpos = pos[motor];
    if (i != 0)
    {
      ConsolePrintf("diff-rev,%d,%d,%d\r\n", motor, diff, lastpos);
      if (diff > -JAM_THRESH[motor])
        throw errbase + 6;
    }
    
    MicroWait(300000);

    // Now driving reverse, but results are from forward
    SendCommand(TEST_RUNMOTOR, ((-JAM_POWER)&0xFC) + motor, 0, NULL);
    SendCommand(TEST_GETMOTOR, 0, sizeof(pos), (u8*)pos);
    
    diff = pos[motor] - lastpos;
    lastpos = pos[motor];
    ConsolePrintf("diff-fwd,%d,%d,%d\r\n", motor, diff, lastpos);
    if (diff < JAM_THRESH[motor]*2)
      throw errbase + 7;
      
    MicroWait(150000);
  }
 
  // Stop the motor
  SendCommand(TEST_RUNMOTOR, 0 + motor, 0, NULL);  
}

// This attempts to jam the motors by running them forward and backward at high speed
void JamTest(void)
{
  Jam(MOTOR_LEFT_WHEEL);
  Jam(MOTOR_RIGHT_WHEEL);
  Jam(MOTOR_LIFT);
  Jam(MOTOR_HEAD);  
}

// This checks whether the head can reach its upper limit within a time limit at SLOW_POWER
void HeadLimits(void)
{
  // Check max limit for head only on ROBOT2 and above (head is installed)
  if (g_fixtureType < FIXTURE_ROBOT2_TEST || g_fixtureType == FIXTURE_PACKOUT_TEST || g_fixtureType == FIXTURE_PACKOUT_CE_TEST)
    return;
  
  const int MOTOR_RUNTIME = 2000 * 1000;  // Need about 2 seconds for normal variation
  int first[4], second[4];

  SendCommand(TEST_GETMOTOR, 0, sizeof(first), (u8*)first);

  // Reset motor watchdog and fire up the motor
  SendCommand(TEST_POWERON, 3-1, 0, NULL);  // 3 seconds
  SendCommand(TEST_RUNMOTOR, (SLOW_POWER&0xFC) + MOTOR_HEAD, 0, NULL);
  MicroWait(MOTOR_RUNTIME);
  
  // Get end point, then stop motor
  SendCommand(TEST_GETMOTOR, 0, sizeof(second), (u8*)second);
  SendCommand(TEST_RUNMOTOR, 0 + MOTOR_HEAD, 0, NULL);
  
  // XXX: This test is broken because it doesn't convert ticks to millidegrees!
  int ticks = second[MOTOR_HEAD] - first[MOTOR_HEAD];
  ConsolePrintf("headlimits,%d\r\n", ticks);
  
  if (ticks < FAST_HEAD_THRESH)
    throw ERROR_MOTOR_HEAD_SLOW_RANGE;
}

// This checks the full speed and range of each motor
void FastMotors(void)
{
  CheckMotor(MOTOR_LEFT_WHEEL,  FAST_POWER, FAST_DRIVE_THRESH, 0);
  CheckMotor(MOTOR_RIGHT_WHEEL, FAST_POWER, FAST_DRIVE_THRESH, 0);
  
  // Do head first to avoid triggering menu mode
  // Check max limit for head only on ROBOT2 and above (head is installed)
  CheckMotor(MOTOR_HEAD,        FAST_HEADLIFT_POWER, FAST_HEAD_THRESH, g_fixtureType < FIXTURE_ROBOT2_TEST ? 0 : FAST_HEAD_MAX);
  HeadLimits();
  
  // Check max limit for lift only on ROBOT3 and above (arms are attached)
  CheckMotor(MOTOR_LIFT,        FAST_HEADLIFT_POWER, FAST_LIFT_THRESH, g_fixtureType < FIXTURE_ROBOT3_TEST ? 0 : FAST_LIFT_MAX);

  // Per Raymond/Kenny's request, run second lift test after jamming motion
  if (g_fixtureType == FIXTURE_ROBOT3_TEST || g_fixtureType == FIXTURE_ROBOT3_CE_TEST)
    CheckMotor(MOTOR_LIFT,        SLOW_POWER, SLOW_LIFT_THRESH, 0);
}

// Check drop sensor in robot fixture
const int DROP_ON_MIN = 800, DROP_OFF_MAX = 100;
void RobotFixtureDropSensor(void)
{
  int onoff[2];
  SendCommand(TEST_DROP, 0, sizeof(onoff), (u8*)onoff);
  ConsolePrintf("drop,%d,%d\r\n", onoff[0], onoff[1]);  
  
  if (onoff[1] > DROP_OFF_MAX)
    throw ERROR_DROP_TOO_BRIGHT;

  // Reflection test only on ROBOT2 and above (drop properly fixtured)
  if (g_fixtureType < FIXTURE_ROBOT2_TEST)
    return;
  
  if (onoff[0] < DROP_ON_MIN)
    throw ERROR_DROP_TOO_DIM;
}

void SpeakerTest(void)
{
  //const int TEST_TIME_US = 1000000;
  
  // Speaker test not on robot1 (head not yet properly fixtured)
  if (g_fixtureType == FIXTURE_ROBOT1_TEST)
    return;
  SendCommand(TEST_PLAYTONE, 0, 0, 0);

  // We planned to use getMonitorCurrent() for this test, but there's too much bypass, too much noise, and/or the tones are too short
}

/*/read charger status
static bool isChargeEnabled(void)
{
  //ConsolePrintf("SendCommand(TEST_CHGENABLE)\r\n");
  u8 isEnabled = 0xFF;
  SendCommand(TEST_CHGENABLE, 0, sizeof(isEnabled), (u8*)&isEnabled ); //read charge enable status
  if( isEnabled > 1 ) //make sure body FW supports this
    throw ERROR_BODY_OUTOFDATE;
  
  ConsolePrintf("isChargeEnabled,%d\r\n", isEnabled );
  return isEnabled > 0;
}//-*/

//set charge enable/disable
static void chargeEnable(bool enable)
{
  //ConsolePrintf("SendCommand(TEST_CHGENABLE)\r\n");
  u8 dummy = 0xFF;
  SendCommand(TEST_CHGENABLE, (enable ? 1 : 2), sizeof(dummy), (u8*)&dummy ); //read charge enable status
  if( dummy > 1 ) //make sure body FW supports this
    throw ERROR_BODY_OUTOFDATE;
  
  MicroWait(20*1000); //takes 20ms to take effect
}

//read battery voltage
u16 robot_get_battVolt100x(u8 poweron_time_s, u8 adcinfo_time_s)
{
  struct { s32 vBat; s32 vExt; s32 vBatFiltered; s32 vExtFiltered; } robot = {0,0,0,0}; //ADC voltage data (read from robot)
  
  if( poweron_time_s > 0 )
    SendCommand(TEST_POWERON, poweron_time_s, 0, 0); //keep robot powered for awhile
  if( adcinfo_time_s > 0 )
    SendCommand(FTM_ADCInfo, adcinfo_time_s, 0, 0);  //display ADC info on robot face (VEXT,VBAT,status bits...)
  
  robot.vExtFiltered = 0xDEADBEEF;
  SendCommand(TEST_ADC, 0, sizeof(robot), (u8*)&robot ); //read battery/external voltages
  if( robot.vBat < 0 || robot.vExtFiltered == 0xDEADBEEF ) //old body fw truncated s32 vals to u16 (data loss)
    throw ERROR_BODY_OUTOFDATE;
  
  //convert raw value to 100x voltage - centivolts
  //float batteryVoltage = ((float)robot.vBat)/65536.0f; //<--this is how esp converts raw spine data
  u16 vBat100x     = (robot.vBat * 100)         >> 16;// / 65536;
  u16 vBatFilt100x = (robot.vBatFiltered * 100) >> 16;// / 65536;
  u16 vExt100x     = (robot.vExt * 100)         >> 16;// / 65536;
  u16 vExtFilt100x = (robot.vExtFiltered * 100) >> 16;// / 65536;
  ConsolePrintf("vBat,%03d,%03d,vExt,%03d,%03d\r\n", vBat100x, vBatFilt100x, vExt100x, vExtFilt100x );
  
  return vBatFilt100x; //return filtered vBat
}

enum {
  RECHARGE_STATUS_OK = 0,
  RECHARGE_STATUS_OFF_CONTACT,
  RECHARGE_STATUS_TIMEOUT,
};

//recharge cozmo. parameterized conditions: time, battery voltage, current threshold
int m_Recharge( u16 max_charge_time_s, u16 vbat_limit_v100x, u16 i_done_ma )
{
  const u8 BAT_CHECK_INTERVAL_S = 60;       //interrupt charging this often to test battery level
  
  EnableChargeComms(); //switch to comm mode
  MicroWait(500*1000); //let battery voltage settle
  uint16_t battVolt100x = robot_get_battVolt100x(BAT_CHECK_INTERVAL_S+10,BAT_CHECK_INTERVAL_S+10); //get initial battery level
  
  u32 chargeTime = getMicroCounter();
  while( vbat_limit_v100x == 0 || battVolt100x < vbat_limit_v100x )
  {
    if( max_charge_time_s > 0 )
      if( getMicroCounter() - chargeTime >= ((u32)max_charge_time_s*1000*1000) )
        return RECHARGE_STATUS_TIMEOUT;
    
    //Turn on charging power
    PIN_SET(GPIOC, PINC_CHGTX);
    PIN_OUT(GPIOC, PINC_CHGTX);
    
    //Don't just do something, stand there!
    int offContactCnt = 0, iDoneCnt = 0, print_len = 0;
    u32 displayTime = 0, chargeTime = getMicroCounter();
    while( getMicroCounter() - chargeTime < (BAT_CHECK_INTERVAL_S*1000*1000) )
    {
      int current_ma = mGetCurrentMa();
      
      //display real-time current usage in console
      if( getMicroCounter() - displayTime > 100*1000 )
      {
        //erase previous line
        for(int x=0; x < print_len; x++ ) {
          ConsolePutChar(0x08); //backspace
          ConsolePutChar(0x20); //space
          ConsolePutChar(0x08); //backspace
        }
        const int DISP_MA_PER_CHAR = 15;
        print_len = ConsolePrintf("%03d ", current_ma);
        for(int x=current_ma; x>0; x -= DISP_MA_PER_CHAR)
          print_len += ConsolePrintf("=");
      }
      
      //charge complete? (current threshold)
      iDoneCnt = i_done_ma > 0 && current_ma < i_done_ma ? iDoneCnt + 1 : 0;
      if( iDoneCnt > 25 ) {
        PIN_RESET(GPIOC, PINC_CHGTX); //disable power to charge contacts
        ConsolePrintf("\r\ni-done,%dmA\r\n", i_done_ma);
        return RECHARGE_STATUS_OK;
      }
      
      //test for robot removal
      if ((offContactCnt = current_ma < PRESENT_CURRENT_MA ? offContactCnt + 1 : 0) > 10) {
        PIN_RESET(GPIOC, PINC_CHGTX); //disable power to charge contacts
        ConsolePrintf("\r\nrobot off contact\r\n");
        return RECHARGE_STATUS_OFF_CONTACT;
      }
    }
    ConsolePrintf("\r\n");
    
    EnableChargeComms(); //switch to comm mode
    MicroWait(500*1000); //let battery voltage settle
    battVolt100x = robot_get_battVolt100x(BAT_CHECK_INTERVAL_S+10,BAT_CHECK_INTERVAL_S+10);
  }
  return RECHARGE_STATUS_OK;
}

void Recharge(void)
{
  const u16 BAT_MAX_CHARGE_TIME_S = 10*60;  //max amount of time to charge
  const u16 VBAT_CHARGE_LIMIT = 375;        //Voltage x100
  const u16 BAT_FULL_I_THRESH_MA = 200;     //current threshold for charging complete (experimental)
  int status;
  
  if( g_fixtureType == FIXTURE_RECHARGE2_TEST ) //charge to full battery
    status = m_Recharge( 3*BAT_MAX_CHARGE_TIME_S, 0, BAT_FULL_I_THRESH_MA );
  else //charge to specified voltage
    status = m_Recharge( BAT_MAX_CHARGE_TIME_S, VBAT_CHARGE_LIMIT, 0 );
  
  if( status == RECHARGE_STATUS_TIMEOUT )
    throw ERROR_TIMEOUT;

  if( status == RECHARGE_STATUS_OK )
  {
    EnableChargeComms(); //switch to comm mode
    try {
      SendCommand(TEST_POWERON, 0x5A, 0, 0);  //shut down immediately
    } catch(int e) {
      if( e != ERROR_NO_PULSE ) //this error is expected. robot can't pulse when it's off...
        throw e;
    }
  }
}

void _debug_robot_graph_current(int num_samples)
{
  //scale parameters for a kick-ass text-console bar-graph
  const int ma_max = 1000;
  const int ma_per_char = 15;
  const int header_ma_per_column = 100; //column granularity (header)
  
  //Turn on charging power
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  
  int offContact = 0, print_header = 0, cnt = 0;
  while(1) 
  {
    int current_ma = mGetCurrentMa();
    
    //re-print bar graph header every once in awhile
    if( --print_header <= 0 ) {
      print_header = 40;
      ConsolePrintf("     "); //value column offset
      int cur = 0;
      while( cur <= ma_max ) {
        int len = ConsolePrintf("%d",cur);
        if(cur < ma_max) { //inhibit column fill for max value column
          for(int x=len; x < header_ma_per_column/ma_per_char; x++)
            ConsolePrintf(".");
        }
        cur += header_ma_per_column;
      }
      ConsolePrintf("\r\n");
    }
    
    //Print row (current + bar graph segments)
    ConsolePrintf("%04d ", current_ma); //value column: width 5
    for(int x=current_ma; x>0; x -= ma_per_char)
      ConsolePrintf("=");
    ConsolePrintf("\r\n");
    
    //stop at user specified limit
    if( num_samples > 0 && ++cnt >= num_samples )
      break;
    
    //test for robot removal
    if ((offContact = current_ma < PRESENT_CURRENT_MA ? offContact + 1 : 0) > 5) {
      PIN_RESET(GPIOC, PINC_CHGTX); //disable power to charge contacts
      ConsolePrintf("robot off contact\r\n");
      throw ERROR_BAT_CHARGER;
    }
  }
}

//Test body charging circuit by verifying current draw
void ChargeTest(void)
{
  const int CHARGING_CURRENT_THRESHOLD_MA = 700;
  const int NUM_SAMPLES = 8;
  const int BAT_OVERVOLT_THRESHOLD = 400; //4.0V
  
  EnableChargeComms(); //switch to comm mode
  uint16_t battVolt100x = robot_get_battVolt100x(10,0); //+POWERON extra time [s]
  //TODO: throw en error if robot battery is too full?? can't properly detect charging current in CV mode
  
  //Turn on charging power
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  
  //_debug_robot_graph_current(0); //DEBUG: graph the charging current (console bar graph)
  
  int avg=0, avgCnt=0, offContact = 0;
  u32 waitTime = getMicroCounter();
  while( getMicroCounter() - waitTime < (5*1000*1000) )
  {
    int current_ma = mGetCurrentMa();
    avg = ((avg*avgCnt) + current_ma) / (avgCnt+1); //tracking average
    avgCnt = avgCnt < NUM_SAMPLES ? avgCnt + 1 : avgCnt;
    
    //finish when average rises above our threshold (after minimum sample cnt)
    if( avgCnt >= NUM_SAMPLES && avg >= CHARGING_CURRENT_THRESHOLD_MA )
      break;
    
    //error out quickly if robot removed from charge base
    if ((offContact = current_ma < PRESENT_CURRENT_MA ? offContact + 1 : 0) > 5) {
      PIN_RESET(GPIOC, PINC_CHGTX); //disable power to charge contacts
      break;
    }
  }
  
  ConsolePrintf("charge-current-ma,%d,sample-cnt,%d\r\n", avg, avgCnt);
  
  if( avgCnt >= NUM_SAMPLES && avg >= CHARGING_CURRENT_THRESHOLD_MA )
  { } //OK
  else if( battVolt100x >= BAT_OVERVOLT_THRESHOLD )
  {
    //time to hit the gym and burn off a few pounds
    const int BURN_TIME_S = 120;
    ConsolePrintf("power-on,%ds\r\n", BURN_TIME_S);
    robot_get_battVolt100x(BURN_TIME_S,BURN_TIME_S); //reset power-on timer and set face to info screen for the duration
    SendCommand(TEST_MOTORSLAM, 0, 0, 0);
    
    throw ERROR_BAT_OVERVOLT; //error prompts operator to put robot aside for a bit
  } 
  else
    throw ERROR_BAT_CHARGER;
}

//Verify battery voltage sufficient for assembly/packout
void BatteryCheck(void)
{
  const u16 VBAT_MIN_VOLTAGE = 360; //3.60V
  
  //if( g_fixtureType == FIXTURE_BODY2_TEST || g_fixtureType == FIXTURE_BODY3_TEST ) {
    //ConsolePrintf("TEST_POWERON 5s\r\n");
    SendCommand(TEST_POWERON, 5, 0, 0); //keep robot powered for a bit longer...
  //}
  
  EnableChargeComms();  //switch to comm mode
  chargeEnable(false);  //disable charger, which could interfere with valid battery measurement
  MicroWait(100*1000);  //wait for battery voltage to stabilize
  u16 vBat100x = robot_get_battVolt100x(0,0);
  chargeEnable(true);   //re-enable charger
  
  if( vBat100x < VBAT_MIN_VOLTAGE )
    throw ERROR_BAT_UNDERVOLT;
}

// Turn on power until battery dead, start slamming motors!
void LifeTest(void)
{
  SendCommand(TEST_POWERON, 0xA5, 0, 0);  
  SendCommand(TEST_MOTORSLAM, 0, 0, 0);
}

//Send a command up to the ESP to force factory revert
void FactoryRevert(void)
{
  EnableChargeComms();
  ConsolePrintf("SendCommand(21) factory revert\n");
  SendCommand(21, 0, 0, 0);
}

//intercept button test function
void mButtonTest(void)
{
  SendCommand(TEST_POWERON, 10, 0, 0); //keep robot powered through this test
  ButtonTest();
  SendCommand(TEST_POWERON, 4, 0, 0); //reset to default 4s power-off
}

// List of all functions invoked by the test, in order
TestFunction* GetInfoTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetPlaypenTestFunctions(void)
{
  static TestFunction functions[] =
  {
    PlaypenTest,
    PlaypenWaitTest,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetRobotTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    mButtonTest,
    ChargeTest,             //run after mButtonTest (sets power modes)
    SlowMotors,
    FastMotors,
    RobotFixtureDropSensor,
    BatteryCheck,
    WriteBodyColor,
    SpeakerTest,            // Must be last
    NULL
  };

  return functions;
}

TestFunction* GetRechargeTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    Recharge,
    NULL
  };

  return functions;
}

TestFunction* GetLifetestTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    LifeTest,
    NULL
  };

  return functions;
}

TestFunction* GetPackoutTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    mButtonTest,
    ChargeTest,             //run after mButtonTest (sets power modes)
    FastMotors,
    RobotFixtureDropSensor,
    BatteryCheck,
    VerifyBodyColor,
    SpeakerTest,            // Must be last
    NULL
  };

  return functions;
}

TestFunction* GetSoundTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    SpeakerTest,
    NULL
  };

  return functions;
}

TestFunction* GetFacRevertTestFunctions(void)
{
  static TestFunction functions[] =
  {
    FactoryRevert,
    NULL
  };

  return functions;
}

