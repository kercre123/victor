#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/cube.h"
#include "hal/monitor.h"
#include "hal/motorled.h"
#include "hal/portable.h"
#include "app/app.h"
#include "app/fixture.h"
#include "binaries.h"

//buttonTest.c
extern void ButtonTest(void);

const int ENCODER_DETECT_VOLTAGE = 1400;  // Above 1400mV, an encoder is probably pulling us up

// Return true if device is detected on contacts
bool MotorDetect(void)
{  
  // Spend about 2ms sampling the encoder LED
  PIN_PULL_DOWN(GPIOA, PINA_ENCLED);
  PIN_IN(GPIOA, PINA_ENCLED);
  if (GrabADC(PINA_ENCLED) > ENCODER_DETECT_VOLTAGE)
  {
    for (int i = 0; i < LEDCnt(); i++) {
      if (g_fixtureType == FIXTURE_MOTOR2H_TEST)
        LEDOn(i);
      MicroWait(150);  // Extra debounce time
    }
    
    if (g_fixtureType == FIXTURE_MOTOR2H_TEST)
      LEDOn(255); //turn off

    return true;
  }
  return false;
}

typedef enum { SHORT, RED, GREEN, BLUE, OPEN } VoltRange;
const char * VoltRangeStr[] = { "SHORT", "RED", "GREEN", "BLUE", "OPEN" };

// Check which 'voltage range' (0-4) an LED is in
// 0:SHORT, 1:RED, 2:GREEN, 3:BLUE, 4:OPEN
VoltRange ledmv2voltrange(int x)
{
  if (x < 1200)
    return SHORT;
  if (x < 1800)
    return RED;
  if (x < 2200)
    return GREEN;
  if (x < 2600)
    return BLUE;
  return OPEN;
}

//separate for extern usage
void TestLED(int i)
{
  //get voltages
  int mv_meas = LEDGetHighSideMv(i);
  int mv_exp  = LEDGetExpectedMv(i);
  
  //convert to voltage ranges
  VoltRange vrmeas = ledmv2voltrange( mv_meas );
  VoltRange vrexp  = ledmv2voltrange( mv_exp );
  
  ConsolePrintf("led,%d,%i,%i,%s,%s\r\n", i, mv_meas, mv_exp, VoltRangeStr[vrmeas], VoltRangeStr[vrexp]);
  
  if( vrmeas != vrexp )
    throw ERROR_BACKPACK_LED;
}

// Test LEDs if present - if not wired up, maybe pass anyway?
static void TestLEDs(void)
{
  // Check the LEDs forward and backward for faults
  for (int i = 0; i < LEDCnt(); i++)
    TestLED(i);
  for (int i = LEDCnt()-1; i >= 0; i--)
    TestLED(i);
}

// Test encoder (not motor)
const int MIN_ENC_ON = 2310, MAX_ENC_OFF = 800;   // In millivolts (with padding) - must be 0.3x to 0.7x VDD
const int ENC_SLOW_US = 1000;
const int ENC_L_US = 200, ENC_H_US = 200;   // Rise/fall time for 2.5KHz (since real thing must get to 2KHz)
void TestEncoders(void)
{
  // Read encoder when turned on and off
  int ona, onb, offa, offb, skip;

  // For slow reading, don't really care about timing, or if it was on or off
  ReadEncoder(true, ENC_SLOW_US, ona, onb);
  ReadEncoder(false, ENC_SLOW_US, offa, offb);
  ConsolePrintf("encoders,%d,%d,%d,%d,%d\r\n", ENC_SLOW_US, ona, onb, offa, offb);
  
  if (offa > MAX_ENC_OFF || offb > MAX_ENC_OFF)
    throw ERROR_ENCODER_FAULT;  
  if (ona < MIN_ENC_ON || onb < MIN_ENC_ON)
    if (ona > MAX_ENC_OFF && onb > MAX_ENC_OFF)
      throw ERROR_ENCODER_UNDERVOLT;
    else
      throw ERROR_ENCODER_FAULT;

  // "Warm up" encoder by toggling the LED, then grab "A" side on last toggle
  const int ENC_US = (g_fixtureType == FIXTURE_MOTOR1L_TEST) ? ENC_L_US : ENC_H_US;
  for (int i = 0; i < 50; i++)
  {
    ReadEncoder(true, ENC_US, ona, onb);
    ReadEncoder(false, ENC_US, offa, offb);
  }
  // Now grab "B" side
  ReadEncoder(true, ENC_US, skip, onb, true);
  ReadEncoder(false, ENC_US, skip, offb, true);
  ConsolePrintf("encoders,%d,%d,%d,%d,%d\r\n", ENC_US, ona, onb, offa, offb);

  if (ona < MIN_ENC_ON || onb < MIN_ENC_ON)
    throw ERROR_MOTOR_FAST;
  if (offa > MAX_ENC_OFF || offb > MAX_ENC_OFF)
    throw ERROR_MOTOR_FAST;
}

// Count encoder ticks, check direction, and collect min/max a/b values
const int OVERSAMPLE = 18;    // 2^N samples
const int ENC_LOW = 1000, ENC_HIGH = 2300;   // Low/high threshold
int MeasureMotor(int speed, bool fast, bool reverse = false )
{
  int mina = 2800, minb = 2800, maxa = 0, maxb = 0;
  bool ahi = 0, bhi = 0;
  int aticks = 0, bticks = 0;
  
  MotorMV(speed, reverse);
  MicroWait(250000);  // Spin up time

  // Collect samples at full speed
  u32 start = getMicroCounter();
  for (int i = 0; i < (1 << OVERSAMPLE); i++)
  {
    int a, b;
    ReadEncoder(true, 0, a, b);
    // Count ticks
    if (a > ENC_HIGH && !ahi)
    {
      ahi = 1;
      aticks += bhi ? 1 : -1;
    }
    if (a < ENC_LOW)
      ahi = 0;
    if (b > ENC_HIGH && !bhi)
    {
      bhi = 1;
      bticks += ahi ? -1 : 1;
    }
    if (b < ENC_LOW)
      bhi = 0;
    // Capture min/max
    if (a > maxa)
      maxa = a;
    if (a < mina)
      mina = a;
    if (b > maxb)
      maxb = b;
    if (b < minb)
      minb = b;
    getMicroCounter();  // Required to measure long intervals
  }
  u32 test_time = getMicroCounter()-start;
  MotorMV(0);
  
  //Keep the old-style debug printing so we can do comparisons
  int hz = (ABS(aticks)*15625)/((int)(test_time>>6));  // Rising edges per second
  int normalized = aticks >> (OVERSAMPLE-13);   // Original calibration was at OVERSAMPLE=13
  ConsolePrintf("motortest,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", speed, normalized, hz, aticks, mina, maxa, bticks, minb, maxb);
  
  //Add new verbose printing
  int a_hz = ABS( (aticks*15625)/((int)(test_time>>6)) );
  int b_hz = ABS( (bticks*15625)/((int)(test_time>>6)) );
  int a_norm = aticks>>(OVERSAMPLE-13);
  int b_norm = bticks>>(OVERSAMPLE-13);
  ConsolePrintf("motortest,%s,speed=%d,time=%dus\r\n", reverse?"rev":"fwd", speed, test_time);
  ConsolePrintf("  a:ticks,norm,hz,min,max = %i,%i,%i,%i,%i\r\n", aticks, a_norm, a_hz, mina, maxa );
  ConsolePrintf("  b:ticks,norm,hz,min,max = %i,%i,%i,%i,%i\r\n", bticks, b_norm, b_hz, minb, maxb );
  
  // Throw some errors - note that 'fast' tests are really checking the encoder (not wiring)
  int diff = aticks-bticks;
  if (diff > 2 || diff < -2)
    throw fast ? ERROR_ENCODER_SPEED_FAULT : ERROR_ENCODER_FAULT;
  if ( (!reverse && aticks < 0) || (reverse && aticks > 0) )
    throw fast ? ERROR_ENCODER_RISE_TIME : ERROR_MOTOR_BACKWARD;
  
  return a_norm;
}

// MotorL: Lift motor with encoders
const int MOTOR_LOW_MV = 1200, MOTOR_FULL_MV = 5000;   // In millivolts
void TestMotorL(void)
{
  const int TICKS_SLOW = 10 / 2;  //adjust for reduced flag cnt on new motors
  const int TICKS_FAST = 80 / 2;  //"
  int mm_fwd_low, mm_fwd_full, mm_rev_low, mm_rev_full;
  
  mm_fwd_low  = MeasureMotor(MOTOR_LOW_MV, false);
  mm_fwd_full = MeasureMotor(MOTOR_FULL_MV, true);
  
  if( g_fixtureRev >= BOARD_REV_1_5_0 )
  {
    ConsolePrintf("motor brake...");
    MotorMV(-1); //motor brake before reverse direction
    MicroWait(100*1000);
    MotorMV(0);
    MicroWait(250*1000);
    ConsolePrintf("release\r\n");
    
    mm_rev_low = MeasureMotor(MOTOR_LOW_MV, false, true);
    mm_rev_full = MeasureMotor(MOTOR_FULL_MV, true, true);
  } 
  else 
  {
    #warning "SKIP REVERSE DIRECTION MOTOR TESTS DURING EP2 TRANSITION-------------"
    ConsolePrintf("WARNING: Skipping motor reverse direction testing\r\n");
    mm_rev_low = -TICKS_SLOW;
    mm_rev_full = -TICKS_FAST;
  }
  
  if (mm_fwd_low < TICKS_SLOW)    //Forward slow
    throw ERROR_MOTOR_SLOW;
  if (mm_fwd_full < TICKS_FAST)   //Forward fast
    throw ERROR_MOTOR_FAST;
  if (-mm_rev_low < TICKS_SLOW)   //Reverse slow
    throw ERROR_MOTOR_SLOW;
  if (-mm_rev_full < TICKS_FAST)  //Reverse fast
    throw ERROR_MOTOR_FAST;
}

// MotorH (head motor) makes about same number of ticks
void TestMotorH(void)
{
  TestMotorL();
}

//enforce hardware compatibility for motor tests
static void CheckFixtureCompatibility(void)
{
  ConsolePrintf("fixture rev %s\r\n", GetBoardRevStr());
  
  //new tests require v1.5 hardware w/ H-Bridge driver
  #warning "WHITELISTING FIXTURE COMPATIBILITY DURING EP2 TRANSITION-------------"
  if( g_fixtureRev < BOARD_REV_1_5_0 )
    ConsolePrintf("WARNING: Fixture Rev 1.0 is incompatible with this test\r\n"); //throw ERROR_INCOMPATIBLE_FIX_REV;
}

// List of all functions invoked by the test, in order
TestFunction* GetMotor1LTestFunctions(void)
{
  static TestFunction functions[] =
  {
    // LEDs are not yet wired in at this station
    TestEncoders,   // 1L (lift) and 1H (head) use same test
    NULL
  };

  return functions;
}

TestFunction* GetMotor1HTestFunctions(void)
{
  return GetMotor1LTestFunctions();
}

// List of all functions invoked by the test, in order
TestFunction* GetMotor2LTestFunctions(void)
{
  static TestFunction functions[] =
  {
    CheckFixtureCompatibility,
    TestMotorL,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetMotor2HTestFunctions(void)
{
  static TestFunction functions[] =
  {
    CheckFixtureCompatibility,
    TestLEDs,
    ButtonTest,
    TestMotorH,
    NULL
  };

  return functions;
}
