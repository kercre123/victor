#include "nrf.h"
#include "nrf_gpio.h"

#include "battery.h"
#include "motors.h"
#include "head.h"
#include "debug.h"
#include "radio.h"
#include "timer.h"
#include "lights.h"
#include "rtos.h"
#include "anki/cozmo/robot/spineData.h"

#include "hardware.h"

#include "tests.h"

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

static void TestMotors(void* discard) {
  static int direction = 1;
  
  for (int i = 0; i < 2; i++)
    Motors::setPower(i, 0x4000 * direction);
  for (int i = 2; i < 4; i++)
    Motors::setPower(i, 0x2800 * direction);
  direction *= -1;
}

static void TestLights(void* data) {
  uint8_t* colors = (uint8_t*) data;
  static int j = 1;
  
  bool valid[] = {
    false, true, true, false, 
    true, true, true, false, 
    true, true, true, false,
    true, true, true, false
  };

  colors[j] += 4;
  
  if (!colors[j]) {
    j++;
    while (!valid[j]) 
      j = (j + 1) % (sizeof(valid) / sizeof(bool));
  }
}

static void TestEncoders(void* data) {
  Motors::getRawValues((uint32_t*) data);
}

void TestFixtures::run() {
#if defined(DO_ENCODER_TESTING)
  RTOS::schedule(TestEncoders, CYCLES_MS(5.0f), &g_dataToBody.backpackColors);
#elif defined(DO_MOTOR_TESTING)
  RTOS::schedule(TestMotors, CYCLES_MS(1000.0f));
#elif defined(DO_LIGHTS_TESTING)
  RTOS::schedule(TestLights, CYCLES_MS(5.0f), &g_dataToBody.backpackColors);
#else
  return ;
#endif

  // This only happens if a test is running
  RTOS::run();
}
