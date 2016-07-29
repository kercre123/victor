#include "nrf.h"
#include "nrf_gpio.h"

#include "battery.h"
#include "motors.h"
#include "head.h"
#include "radio.h"
#include "timer.h"
#include "lights.h"
#include "rtos.h"
#include "backpack.h"
#include "anki/cozmo/robot/spineData.h"

#include "hardware.h"
#include "tests.h"

#include "messages.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

// Some internals we hijack for testing purposes
extern bool motorOverride;
extern int resultLedOn;
extern int resultLedOff;
extern int g_powerOffTime;
extern bool g_turnPowerOff;
extern Fixed vBat, vExt;

static void TestMotors(void* discard) {
  static int direction = 1;
  
  for (int i = 0; i < 2; i++)
    Motors::setPower(i, 0x7800 * direction);
  for (int i = 2; i < 4; i++)
    Motors::setPower(i, 0x5000 * direction);
  direction *= -1;
}

// Do a blocking send of a single byte out the testport, with all the horror that implies
static void SendByte(int c)
{
  NRF_UART0->TXD = c;
  while (NRF_UART0->EVENTS_TXDRDY != 1)
    ;
  NRF_UART0->EVENTS_TXDRDY = 0;
}

// Send a complete packet of result data back to test fixture
// You can't send more than 16 bytes due to bandwidth limitations
void SendDown(int len, uint8_t* result)
{
  NVIC_DisableIRQ(UART0_IRQn);
  NRF_UART0->PSELTXD = 0xFFFFFFFF;
  NRF_UART0->PSELRXD = 0xFFFFFFFF;
  NRF_UART0->BAUDRATE = NRF_BAUD(charger_baud_rate);
  NRF_UART0->TASKS_STARTTX = 1;
  
  // Extra delay for fixture turnaround/to ensure start-bit is found
  nrf_gpio_pin_clear(PIN_TX_VEXT);
  nrf_gpio_cfg_output(PIN_TX_VEXT);
  MicroWait(150);
  
  // The power system goes batshit while transmitting, so go as quick as we can
  NRF_UART0->PSELTXD = PIN_TX_VEXT;
  MicroWait(12);   // Enough time for a start bit transition
  
  SendByte(~0xBE);   // In honor of Palatucci
  SendByte(len);
  while (len--) {
    SendByte(*result++);
    SendByte(0);    // Keep robot alive with breaths of air
  }
  SendByte(~0xEF);
  
  MicroWait(12);   // Enough time for a stop bit
  NRF_UART0->PSELTXD = 0xFFFFFFFF; // PIN_TX_VEXT;
  NVIC_EnableIRQ(UART0_IRQn);
}

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

// Tests dispatched by fixture
// These generally disrupt correct RTOS/robot operation and leave things in a variety of bad states
void TestFixtures::dispatch(uint8_t test, uint8_t param)
{
  // Test modes from 0..127 are routed up to Espressif
  if (test < 128) {
    RobotInterface::EnterFactoryTestMode msg;
    msg.mode = test;
    msg.param = param;
    RobotInterface::SendMessage(msg);
    SendDown(0, NULL);    // Send OK message
    return;
  }
  
  // Tests from 128..255 are handled in body
  switch (test)
  {
    // This hack keeps the battery on for N more seconds (see main.cpp)
    // It operates as a kind of watchdog - if you stop calling it the robot will turn off
    case TEST_POWERON:
      if (param == 0xA5)
        g_turnPowerOff = false;   // Last until battery dies
      else if (param == 0x5A)
        Battery::powerOff();      // Kill battery immediately
      else
        g_powerOffTime = GetCounter() + ((param+1)<<23);  // Last N+1 seconds longer
      break;    // Reply "OK"
    
    case TEST_RADIOTX:
      Radio::sendTestPacket();
      break;    // Reply "OK"
    
    case TEST_KILLHEAD:
    {
      RobotInterface::OTA::EnterRecoveryMode msg;
      msg.mode = RobotInterface::OTA::Recovery_Mode;
      RobotInterface::SendMessage(msg);
      break;    // Reply "OK"
    }
    
    case TEST_MOTORSLAM:
    {
      motorOverride = true;
      RTOS::schedule(TestMotors, CYCLES_MS(500.0f));
      break;    // Reply "OK"
    }
    
    case TEST_PLAYTONE:
    {
      RobotInterface::GenerateTestTone msg;
      RobotInterface::SendMessage(msg);     
      break;    // Reply "OK"
    }
    
    // Get version and ESN information
    case TEST_GETVER:
    {
      SendDown(8, (u8*)0x1F010);    // Bootloader/fixture version
      return;   // Already replied
    }
    
    case TEST_RUNMOTOR:
    {
      motorOverride = true;
      for (int i = 0; i < 4; i++)
        Motors::setPower(i, 0);
      int motor = param & 3;              // Motor number in LSBs
      int power = ((s8)param & ~3) << 8;  // Motor direction/power in MSBs (-124..124 for MAX power)
      Motors::disable(false);
      Motors::setPower(motor, power);
      break;    // Reply "OK"
    }
    
    case TEST_GETMOTOR:
      SendDown(16, (u8*)g_dataToHead.positions);  // All 4 motor positions
      return;   // Already replied
    
    case TEST_DROP:
    {
      int data[2] = {resultLedOn, resultLedOff};
      SendDown(sizeof(data), (u8*)data);  // On value, off value
      return;   // Already replied
    }
    
    // param[4] = IR forward, param[0:3] = 0 (no backpack), 1-12 (backpack LED)
    case TEST_LIGHT:
      Backpack::testLight(param & 0xF);
      Battery::setHeadlight(param & 0x10); 
      break;    // Reply "OK"
    
    case TEST_ADC:
    {
      u16 data[2] = {vBat, vExt};
      SendDown(sizeof(data), (u8*)data);
      return;   // Already replied
    }
  }
  
  // By default, send down an "OK" message
  SendDown(0, NULL);
}

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
