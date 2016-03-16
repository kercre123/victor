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

#if defined(DO_MOTOR_TESTING)
static void TestMotors(void* discard) {
  static int direction = 1;
  
  for (int i = 0; i < 2; i++)
    Motors::setPower(i, 0x4000 * direction);
  for (int i = 2; i < 4; i++)
    Motors::setPower(i, 0x2800 * direction);
  direction *= -1;
}
#endif

#if defined(DO_LIGHTS_TESTING)
static void TestLights(void* data) {	
	static const LightState colors[] = {
		{ 0x0000, 0xFFFF, 0x40, 0x40, 0x40, 0x40 },
		{ 0x0000, 0x001F, 0x40, 0x40, 0x40, 0x40 },
		{ 0x0000, 0x03E0, 0x40, 0x40, 0x40, 0x40 },
		{ 0x0000, 0x7C00, 0x40, 0x40, 0x40, 0x40 }
	};
	
	static int index = 0;
	
	for (int i = 0; i < TOTAL_LIGHTS; i++) {
		Lights::update(i, &colors[index]);
	}
	
	index = (index + 1) % 4;
}
#endif

#if defined(DO_ENCODER_TESTING)
static void TestEncoders(void* userdata) {
  uint32_t data[4];
	
	Motors::getRawValues(data);
	for (int i = 0; i < 4; i++) {
		LightState colors = { data[i], data[i] };
		Lights::update(i, &colors);
	}
}
#endif

void TestFixtures::run() {
#if defined(DO_ENCODER_TESTING)
  RTOS::schedule(TestEncoders, CYCLES_MS(5.0f));
#endif

#if defined(DO_MOTOR_TESTING)
  RTOS::schedule(TestMotors, CYCLES_MS(1000.0f));
#endif

#if defined(DO_LIGHTS_TESTING)
  RTOS::schedule(TestLights, CYCLES_MS(5000.0f));
#endif
}
