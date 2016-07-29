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

#include "app/fixture.h"
#include "app/binaries.h"

#include "../syscon/hal/tests.h"
#include "../syscon/hal/hardware.h"
#include "../syscon/hal/motors.h"
#include "../generated/clad/robot/clad/types/fwTestMessages.h"

using namespace Anki::Cozmo::RobotInterface;

// XXX: Fix this if you ever fix current monitoring units
// Robot is up and running - usually around 48K, rebooting at 32K - what a mess!
#define BOOTED_CURRENT  40000
#define PRESENT_CURRENT 1000

extern BOOL g_isDevicePresent;

// Return true if device is detected on contacts
bool RobotDetect(void)
{
  // Turn power on and check for current draw
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_IN(GPIOC, PINC_CHGRX);
  MicroWait(500);
  
  // Hysteresis for shut-down robots 
  int now = MonitorGetCurrent();
  if (g_isDevicePresent && now > PRESENT_CURRENT)
    return true;
  if (now > BOOTED_CURRENT)
    return true;
  return false;
}

void SendTestChar(int c);

void InfoTest(void)
{
  unsigned int version[2];
  
  // Pump the comm-link 4 times before trying to send
  for (int i = 0; i < 4; i++)
    try {
      SendTestChar(-1);
    } catch (int e) { }
  // Let Espressif finish booting
  MicroWait(300000); 
  
  SendCommand(1, 0, 0, 0);    // Put up info face and turn off motors
  SendCommand(TEST_GETVER, 0, sizeof(version), (u8*)version);

  // Mimic robot format for SMERP
  int unused = version[0]>>16, hwversion = version[0]&0xffff, esn = version[1];
  ConsolePrintf("version,%08d,%08d,%08x,00000000\r\n", unused, hwversion, esn);
  
  if (hwversion < BODY_VER_SHIP)
    throw ERROR_BODY_OUTOFDATE;
}

void PlaypenTest(void)
{
  // Pump the comm-link 4 times before trying to send
  for (int i = 0; i < 4; i++)
    try {
      SendTestChar(-1);
    } catch (int e) { }
  // Let Espressif finish booting
  MicroWait(300000); 
    
  // Try to put robot into playpen mode
  SendCommand(FTM_PlayPenTest, (FIXTURE_SERIAL)&63, 0, 0);    
  
  // Do this last:  Try to put fixture radio in advertising mode
  static bool setRadio = false;
  if (!setRadio)
    SetRadioMode('A');
  setRadio = true;
}

extern int g_stepNumber;
void PlaypenWaitTest(void)
{
  int offContact = 0;
  
  // Act like a charger - the robot expects to start on one!
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);

  while (1)
  {
    int current = 0;
    for (int i = 0; i < 64; i++)
    {
      MicroWait(750);
      current += MonitorGetCurrent();
    }
    current >>= 6;
    ConsolePrintf("%d..", current);
    if (current < 2000)
      offContact++;
    else {
      offContact = 0;
    }
    if (offContact > 10)
      return;
  }
}

int TryMotor(s8 motor, s8 speed, bool limitToLimit = false);

// Run motors through their paces
const int SLOW_DRIVE_THRESH = 130, FAST_DRIVE_THRESH = 1170;    // mm/sec
const int SLOW_LIFT_THRESH = 1000, FAST_LIFT_THRESH = 67400;   // 67.4 deg full-scale minimum
const int FAST_LIFT_MAX = FAST_LIFT_THRESH + 5000;      // Lift rarely exceeds +5 deg
const int SLOW_HEAD_THRESH = 800, FAST_HEAD_THRESH = 70000;    // 70.0 deg full-scale minimum
const int FAST_HEAD_MAX = FAST_HEAD_THRESH + 10000;     // Head is typically +6 deg (weird)
const int SLOW_POWER = 40, FAST_POWER = 124;

// Run a motor forward and backward, check minimum and (optional) maximum
void CheckMotor(u8 motor, u8 power, int min, int max)
{
  int errbase = ERROR_MOTOR_LEFT + motor*10;

  // Move lift/head out of the way before test
  if (motor >= 2)
  {
    if (power < FAST_POWER)
      TryMotor(-motor, -power);   // Give extra space at slow power
    TryMotor(-motor, -power);
  }

  // Test forward first
  int result = TryMotor(motor, power, max != 0);// If max, run limitToLimit
  if (result < 0)
    throw errbase + 2;    // Wired backward
  if (result < min)
  {
    if (power < FAST_POWER)
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
    if (power < FAST_POWER)
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

// This checks whether the head can reach its upper limit within a time limit at SLOW_POWER
void HeadLimits(void)
{
  // Check max limit for head only on ROBOT2 and above (head is installed)
  if (g_fixtureType < FIXTURE_ROBOT2_TEST)
    return;
  
  const int MOTOR_RUNTIME = 2000 * 1000;  // Need about 2 seconds for normal variation
  int first[4], second[4];

  SendCommand(TEST_GETMOTOR, 0, sizeof(first), (u8*)first);

  // Reset motor watchdog and fire up the motor
  SendCommand(TEST_POWERON, 2, 0, NULL);  // 3 seconds
  SendCommand(TEST_RUNMOTOR, (SLOW_POWER&0xFC) + MOTOR_HEAD, 0, NULL);
  MicroWait(MOTOR_RUNTIME);
  
  // Get end point, then stop motor
  SendCommand(TEST_GETMOTOR, 0, sizeof(second), (u8*)second);
  SendCommand(TEST_RUNMOTOR, 0 + MOTOR_HEAD, 0, NULL);
  
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
  CheckMotor(MOTOR_HEAD,        FAST_POWER, FAST_HEAD_THRESH, g_fixtureType < FIXTURE_ROBOT2_TEST ? 0 : FAST_HEAD_MAX);
  HeadLimits();
  
  // Check max limit for lift only on ROBOT3 and above (arms are attached)
  CheckMotor(MOTOR_LIFT,        FAST_POWER, FAST_LIFT_THRESH, g_fixtureType < FIXTURE_ROBOT3_TEST ? 0 : FAST_LIFT_MAX);
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
  // Speaker test not on robot1 (head not yet properly fixtured)
  if (g_fixtureType != FIXTURE_ROBOT1_TEST)
    return;
  SendCommand(TEST_PLAYTONE, 0, 0, 0);
}

// List of all functions invoked by the test, in order
TestFunction* GetRobotTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    SlowMotors,
    FastMotors,
    RobotFixtureDropSensor,
    SpeakerTest,            // Must be last
    NULL
  };

  return functions;
}

// Charge for 2 minutes, then shut off reboot and restart
void Recharge(void)
{
  SendCommand(TEST_POWERON, 120, 0, 0);
  SendCommand(FTM_ADCInfo, 120, 0, 0);  
}

// Turn on power until battery dead, start slamming motors!
void LifeTest(void)
{
  SendCommand(TEST_POWERON, 0xA5, 0, 0);  
  SendCommand(TEST_MOTORSLAM, 0, 0, 0);
}

TestFunction* GetRechargeTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    Recharge,
    PlaypenWaitTest,
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
    SpeakerTest,            // Must be last
    NULL
  };

  return functions;
}
