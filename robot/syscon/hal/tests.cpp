#include "nrf.h"
#include "nrf_gpio.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "battery.h"
#include "motors.h"
#include "head.h"
#include "cubes.h"
#include "timer.h"
#include "lights.h"
#include "backpack.h"

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

static bool runMotorTest = false;

void TestFixtures::manage() {
  static const int LOW_BAT_TIME = CYCLES_MS(500);
  static int dirChangeTime = 0;
  static int direction = 1;

  int ticks = dirChangeTime - GetCounter();

  if (!runMotorTest || ticks >= 0) return ;
  
  dirChangeTime = GetCounter() + LOW_BAT_TIME;

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
// These generally disrupt correct robot operation and leave things in a variety of bad states
void TestFixtures::dispatch(uint8_t test, uint8_t param)
{
  #ifdef FACTORY
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
      if (param == 0x55) {
        g_powerOffTime = GetCounter() + (4<<23); // 4 seconds from now
        g_turnPowerOff = true;    // Enable power-down
      } else if (param == 0xA5)
        g_turnPowerOff = false;   // Last until battery dies
      else if (param == 0x5A)
        Battery::powerOff();      // Kill battery immediately
      else {
        param = param > 254 ? 254 : param; //limit offset from counter val for modulo operations
        g_powerOffTime = GetCounter() + ((param+1)<<23);  // Last N+1 seconds longer
        g_turnPowerOff = true;    // Enable power-down
      }
      break;    // Reply "OK"
    
    case TEST_RADIOTX:
      // This no longer exists
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
      Battery::setOperatingMode(BODY_ACCESSORY_OPERATING_MODE);
      motorOverride = true;
      runMotorTest = true;
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
      Battery::setOperatingMode(BODY_ACCESSORY_OPERATING_MODE);

      motorOverride = true;
      for (int i = 0; i < 4; i++)
        Motors::setPower(i, 0);
      int motor = param & 3;              // Motor number in LSBs
      int power = ((s8)param & ~3) << 8;  // Motor direction/power in MSBs (-124..124 for MAX power)
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
      Lights::setHeadlight(param & 0x10); 
      break;    // Reply "OK"
    
    case TEST_ADC:
    {
      s32 data[2] = {vBat, vExt};
      SendDown(sizeof(data), (u8*)data);
      return;   // Already replied
    }
    
    case TEST_BACKBUTTON:
    {
      u8 data = Backpack::button_pressed;
      SendDown(sizeof(data), (u8*)&data);
      return;   // Already replied
    }
    
    case TEST_BACKPULLUP:
    {
      //__disable_irq();
      Backpack::detachTimer();  //disable bp control & high-z pins
      
      //Reset power-off counter. If backpack pull-up is missing, robot may try to turn itself off after ~5s.
      Battery::hookButton(false);
      
      //discharge button input signal
      nrf_gpio_pin_clear(PIN_BUTTON_SENSE);
      nrf_gpio_cfg_output(PIN_BUTTON_SENSE);
      MicroWait(10);
      
      //float and test time it takes to go high again.
      u32 rtc_ticks, start = GetCounter();
      nrf_gpio_cfg_input(PIN_BUTTON_SENSE, NRF_GPIO_PIN_NOPULL);
      do {
        rtc_ticks = (GetCounter() - start) >> 8;  //T[rtc]=30.6us
        if( nrf_gpio_pin_read(PIN_BUTTON_SENSE) ) //break when signal goes high
          break;
      } while(rtc_ticks < 5); //timeout ~150us
      
      Backpack::detachTimer();  //disable bp control & high-z pins
      Backpack::useTimer();     //re-enable backpack control
      //__enable_irq();
      
      SendDown(sizeof(rtc_ticks), (u8*)&rtc_ticks);
      return;   // Already replied
    }
    
    case TEST_ENCODERS:
    {
      //Breif: BODY1, check that the encoders are populated and working properly.
      //No gearbox etc installed at this point to interfere.
      
      __disable_irq();
      //maybe save and restore VDDs state after our test?
      
      // Encoder and LED power (enabled)
      nrf_gpio_pin_clear(PIN_VDDs_EN);
      nrf_gpio_cfg_output(PIN_VDDs_EN);
      MicroWait(100);
      u32 state_on = NRF_GPIO->IN;
      
      // Encoder and LED power (disable)
      nrf_gpio_pin_set(PIN_VDDs_EN);
      MicroWait(100);
      u32 state_off = NRF_GPIO->IN;
      
      __enable_irq();
      
      u8 result[4] = {
        (state_on  & (1 << PIN_ENCODER_LEFT) ) > 0,
        (state_off & (1 << PIN_ENCODER_LEFT) ) > 0,
        (state_on  & (1 << PIN_ENCODER_RIGHT)) > 0,
        (state_off & (1 << PIN_ENCODER_RIGHT)) > 0};
      
      SendDown(sizeof(result), (u8*)&result[0]);
      return;   // Already replied
    }
  }
  
  // By default, send down an "OK" message
  SendDown(0, NULL);
  #endif
}
