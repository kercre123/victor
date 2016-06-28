#include "nrf.h"
#include "nrf_gpio.h"

#include "battery.h"
#include "motors.h"
#include "head.h"
#include "radio.h"
#include "timer.h"
#include "lights.h"
#include "rtos.h"
#include "anki/cozmo/robot/spineData.h"

#include "hardware.h"
#include "tests.h"

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

// Do a blocking send of a single byte out the testport, with all the horror that implies
static void SendByte(int c)
{
  // The power system goes batshit while transmitting, so go as quick as we can
  NRF_UART0->PSELTXD = PIN_TX_VEXT;
  MicroWait(4);   // Enough time for a start bit
  NRF_UART0->TXD = c;
  while (NRF_UART0->EVENTS_TXDRDY != 1)
    ;
  NRF_UART0->EVENTS_TXDRDY = 0;
  NRF_UART0->PSELTXD = 0xFFFFFFFF; // PIN_TX_VEXT;
}

// Send a complete packet of result data back to test fixture
static void SendDown(int len, uint8_t* result)
{
  NVIC_DisableIRQ(UART0_IRQn);
  NRF_UART0->PSELTXD = 0xFFFFFFFF;
  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->BAUDRATE = NRF_BAUD(charger_baud_tx);
  NRF_UART0->TASKS_STARTTX = 1;
  SendByte(0xBE);   // In honor of Palatucci
  SendByte(len);
  while (len--)
    SendByte(*result++);
  SendByte(0xEF);
  NVIC_EnableIRQ(UART0_IRQn);
}

// Tests dispatched by fixture
// These generally disrupt correct RTOS/robot operation and leave things in a variety of bad states
void TestFixtures::dispatch(uint8_t test, uint8_t param)
{
  switch (test)
  {
    case TEST_POWERON:
      Battery::powerOn();      
      break;    // Reply "OK"
    
    case TEST_RADIOTX:
      Radio::sendTestPacket();
      break;    // Reply "OK"
    
    case TEST_KILLHEAD:
      RobotInterface::OTA::EnterRecoveryMode msg;
      msg.mode = RobotInterface::OTA::Recovery_Mode;
      RobotInterface::SendMessage(msg);
      break;    // Reply "OK"
    
    // Get version and ESN information
    case TEST_GETVER:
      //0x1F000
      return;   // Already replied
    
    // XXX: This test needs a timeout - the test fixture system needs a timeout
    case TEST_RUNMOTOR:
      int motor = param & 3;              // Motor number in LSBs
      int power = (s8)(param & ~3) << 8;  // Motor direction/power in MSBs (-124..124 for MAX power)
      Motors::disable(false);
      Motors::setPower(motor, power);
      break;    // Reply "OK"
    
    case TEST_STOPMOTOR:
      for (int i = 0; i < 4; i++)
        Motors::setPower(i, 0);
      // XXX
      return;   // Already replied
  }
  
  // By default, send down an "OK" message
  SendDown(0, NULL);
}

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
