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
#include "app/fixture.h"
#include "binaries.h"

#define ENCODER_DETECT_VOLTAGE  1400    // Above 1400mV, an encoder is probably pulling us up

void AllOn()
{
  // Light up all the LEDs in fixture 2B
  if (g_fixtureType == FIXTURE_MOTOR2B_TEST)
  {
    static int x = 0;
    LEDOn(x++); 
    if (x >= 12)
      x = 0;
  }
}

// Return true if device is detected on contacts
bool MotorDetect(void)
{  
  // Spend about 2ms sampling the encoder LED
  PIN_PULL_DOWN(GPIOA, PINA_ENCLED);
  PIN_IN(GPIOA, PINA_ENCLED);
  if (GrabADC(PINA_ENCLED) > ENCODER_DETECT_VOLTAGE)
  {
    for (int i = 0; i < 11; i++)
    {
      AllOn();
      MicroWait(150);  // Extra debounce time
    }
    return true;
  }
  return false;
}


const int LED_COUNT = 12;
typedef enum { SHORT, RED, GREEN, BLUE, OPEN } VoltRange;
const VoltRange LED_MAP[LED_COUNT] =
{ GREEN, BLUE, RED,
  BLUE, GREEN, RED,
  BLUE, GREEN, RED,
  RED,  OPEN,  RED };

// Check which 'voltage range' (0-4) an LED is in
// 0:SHORT, 1:RED, 2:GREEN, 3:BLUE, 4:OPEN
VoltRange GetLEDRange(int x)
{
  x = LEDTest(x);
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

// Test LEDs if present - if not wired up, maybe pass anyway?
void TestLEDs(void)
{
  // Check the LEDs forward and backward for faults
  for (int i = 0; i < 12; i++)
    if (GetLEDRange(i) != LED_MAP[i])
      throw ERROR_BACKPACK_LED;
  for (int i = 11; i >= 0; i--)
    if (GetLEDRange(i) != LED_MAP[i])
      throw ERROR_BACKPACK_LED;
}

// Test encoder (not motor)
const int MIN_ENC_ON = 2300, MAX_ENC_OFF = 500;   // In millivolts (with padding)
const int ENC_US = 120;   // Rise/fall time should be 120uS per Bryan
void TestEncoders(void)
{
  // Read encoder when turned on and off
  int ona, onb, offa, offb;
  ReadEncoder(true, ENC_US, ona, onb);
  ReadEncoder(false, ENC_US, offa, offb);
  ConsolePrintf("encoders,%d,%d,%d,%d\r\n", ona, onb, offa, offb);
  
  if (ona < MIN_ENC_ON || onb < MIN_ENC_ON)
    throw ERROR_ENCODER_FAULT;
  if (offa > MAX_ENC_OFF || offb > MAX_ENC_OFF)
    throw ERROR_ENCODER_FAULT;  
}

// Count encoder ticks, check direction, and collect min/max a/b values
const int OVERSAMPLE = 13;    // 8192 samples
const int ENC_LOW = 800, ENC_HIGH = 2500;   // Low/high threshold
int MeasureMotor(int speed)
{
  int mina = 2800, minb = 2800, maxa = 0, maxb = 0;
  bool ahi = 0, bhi = 0;
  int aticks = 0, bticks = 0;
  
  MotorMV(speed);
  MicroWait(250000);  // Spin up time

  // Collect samples at full speed
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
  }
  ConsolePrintf("motortest,%d,%d,%d,%d,%d,%d,%d\r\n", speed, aticks, bticks, mina, maxa, minb, maxb);
  MotorMV(0);
  
  int diff = aticks-bticks;
  if (diff > 2 || diff < -2)
    throw ERROR_ENCODER_FAULT;
  if (aticks < 0)
    throw ERROR_MOTOR_BACKWARD;
  
  return aticks;
}

// Test motor with encoders
const int MOTOR_LOW_MV = 1000, MOTOR_FULL_MV = 5000;   // In millivolts
void TestMotorA(void)
{
  const int TICKS_SLOW = 10;
  const int TICKS_FAST = 80;
  if (MeasureMotor(MOTOR_LOW_MV) < TICKS_SLOW)
    throw ERROR_MOTOR_SLOW;
  if (MeasureMotor(MOTOR_FULL_MV) < TICKS_FAST)
    throw ERROR_MOTOR_FAST;    
}

// Motor B (head motor) makes more ticks
void TestMotorB(void)
{
  const int TICKS_SLOW = 20;
  const int TICKS_FAST = 160;
  if (MeasureMotor(MOTOR_LOW_MV) < TICKS_SLOW)
    throw ERROR_MOTOR_SLOW;
  if (MeasureMotor(MOTOR_FULL_MV) < TICKS_FAST)
    throw ERROR_MOTOR_FAST;    
}

// List of all functions invoked by the test, in order
TestFunction* GetMotor1TestFunctions(void)
{
  static TestFunction functions[] =
  {
    TestEncoders,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetMotor2ATestFunctions(void)
{
  static TestFunction functions[] =
  {
    TestMotorA,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetMotor2BTestFunctions(void)
{
  static TestFunction functions[] =
  {
    TestLEDs,
    TestMotorB,
    NULL
  };

  return functions;
}
