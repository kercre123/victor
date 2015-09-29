#include "battery.h"
#include "motors.h"
#include "head.h"
#include "debug.h"
#include "timer.h"
#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "anki/cozmo/robot/spineData.h"

#include "tests.h"

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Execute a particular test - indexed by cmd with 16-bit signed arg
void TestRun(u8 cmd, s16 arg)
{
  const u8 REPSTART = 0xFC, REPEND = 0xFD;
  UART::print("%c\n\r", REPSTART);

  // Execute motor setpower commands (0-3)
  if (cmd <= 4)
  {
    Motors::setPower(cmd, arg);
    Motors::update();
    MicroWait(5000);
    Motors::printEncoder(cmd);
    Motors::update();
    MicroWait(5000);
  }
  // Execute read sensors command (4)
  if (4 == cmd)
  {
  }

  UART::print("%c\n\r", REPEND);
}

// Test mode enables direct control of the platform via the rear UART
// All commands start with a request from the host and send a reply to the client
// Standard requests are 5 bytes long - 0xFA header, command, arg0, arg1, 0xFB footer
// Standard replies are 2 bytes long - 0xFC command started, 0xFD command complete
// A few replies will return data between the command started and command complete
void TestMode(void)
{
  const int REQLEN = 5; // Requests are 5 bytes long
  const u8 REQSTART = 0xFA, REQEND = 0xFB;

  u8 req[REQLEN]; // Current request
  u8 reqLen = 0;  // Number of request bytes received

  // Read requests byte by byte and dispatch them
  while (1)
  {
    int c = UART::get();

    // Wait for a character to arrive
    if (-1 == c)
      continue;

    req[reqLen++] = c;

    // First byte must be REQSTART, else reset the request
    if (1 == reqLen && REQSTART != c) {
      reqLen = 0;

    // If last byte is REQEND, dispatch the request
    } else if (REQLEN == reqLen) {
      reqLen = 0;
      if (REQEND == c)
        TestRun(req[1], req[2] | (req[3] << 8));
    }
  }
}

static void TestMotors() {
  // Motor testing, loop forever
  //  nrf_gpio_pin_clear(PIN_LED1);
  //  nrf_gpio_pin_set(PIN_LED2);
  MicroWait(2000000);
  while (1)
  {
    #ifdef DO_FIXED_DISTANCE_TESTING
      MicroWait(3000000);
      while(Motors::debugWheelsGetTicks(0) <= 3987) // go 0.25m
      {
        Motors::setPower(0, 0x2000);
        Motors::setPower(1, 0x2000);
        Motors::update();
        MicroWait(5000);
        Motors::update();
      }
      Motors::setPower(0, 0);
      Motors::setPower(1, 0);
      Motors::update();
      MicroWait(5000);
      Motors::update();

      MicroWait(3000000);
      while(Motors::debugWheelsGetTicks(0) >= 0) // go back 0.25m
      {
        Motors::setPower(0, -0x2000);
        Motors::setPower(1, -0x2000);
        Motors::update();
        MicroWait(5000);
        Motors::update();
      }
      Motors::setPower(0, 0);
      Motors::setPower(1, 0);
      Motors::update();
      MicroWait(5000);
      Motors::update();
    #endif

    UART::print("\r\nForward: ");
    #ifndef DO_GEAR_RATIO_TESTING
    for (int i = 0; i < 2; i++)
      Motors::setPower(i, 0x4000);
    for (int i = 2; i < 4; i++)
      Motors::setPower(i, 0x2800);
    #endif
    Motors::update();
    MicroWait(5000);
    Motors::update();

    // Let motors run, then print the deltas
    MicroWait(500000);
    Motors::printEncodersRaw();

    UART::print("  Backward: ");
    #ifndef DO_GEAR_RATIO_TESTING
    for (int i = 0; i < 2; i++)
      Motors::setPower(i, -0x4000);
    for (int i = 2; i < 4; i++)
      Motors::setPower(i, -0x2800);
    #endif
    Motors::update();
    MicroWait(5000);
    Motors::update();

    // Let motors run, then print the deltas
    MicroWait(500000);
    Motors::printEncodersRaw();

    Battery::update();
  }
}

static void TestLights() {
  for(int j = 0; ;j++)
  {
    for(int i = 0; i < 200; i++)
    {
      u32 timerStart = GetCounter();
      g_dataToBody.backpackColors[(j % 4)] = 0x00FF0000;
      g_dataToBody.backpackColors[(j+1) % 4] = 0x0000FF00;
      g_dataToBody.backpackColors[(j+2) % 4] = 0x000000FF;
      g_dataToBody.backpackColors[(j+3) % 4] = 0x00FF00FF;
      
      while ((GetCounter() - timerStart) < 41666)
        Lights::manage(g_dataToBody.backpackColors);
    }
  }
}

static void TestEncoders() {
  for(;;) {
    uint32_t values[4];
    Motors::getRawValues(values);
    Lights::manage(values);
  }
}

void TestFixtures::run() {
#if defined(DO_ENCODER_TESTING)
  TestEncoders();
#elif defined(DO_MOTOR_TESTING)
  TestMotors();
#elif defined(DO_GEAR_RATIO_TESTING)
#elif defined(DO_FIXED_DISTANCE_TESTING)
#elif defined(DO_LIGHTS_TESTING)
  TestLights();
#endif
}
