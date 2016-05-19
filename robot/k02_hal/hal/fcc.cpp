#include <string.h>

#include "fcc.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"

#include "anki/cozmo/robot/spineData.h"

#include "hal/portable.h"
#include "MK02F12810.h"
#include "hardware.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

// Include command constants from the nordic API
extern "C" {
  #include "syscon/components/ble/ble_dtm/ble_dtm.h"
}

using namespace Anki::Cozmo::RobotInterface;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

struct DTM_Mode_Settings {
  bool    motor_drive;
  int32_t command;
  int32_t freq;
  int32_t payload;
  int32_t length;
};

static const DTM_Mode_Settings DTM_MODE[] = {
  { false,    LE_RECEIVER_TEST,  1,                   0xFF,         0xFF },   // 0 = listen
  { false, LE_TRANSMITTER_TEST,  1,           DTM_PKT_0X55,         0xFF },   // 1 = tx ch1
  { false, LE_TRANSMITTER_TEST, 20,           DTM_PKT_0X55,         0xFF },
  { false, LE_TRANSMITTER_TEST, 40,           DTM_PKT_0X55,         0xFF },
  { false, LE_TRANSMITTER_TEST,  1, DTM_PKT_VENDORSPECIFIC, CARRIER_TEST },   // 4 = tx ch1
  { false, LE_TRANSMITTER_TEST, 20, DTM_PKT_VENDORSPECIFIC, CARRIER_TEST },
  { false, LE_TRANSMITTER_TEST, 40, DTM_PKT_VENDORSPECIFIC, CARRIER_TEST },
  { true,             LE_RESET,  0,                   0xFF,         0xFF },   // 7 = motor test
  { false,            LE_RESET,  0,                   0xFF,         0xFF },
  { false,            LE_RESET,  0,                   0xFF,         0xFF },
  { false,            LE_RESET,  0,                   0xFF,         0xFF },   // 10 = wifi ch1 (wifi modes start at 10)
  { false,            LE_RESET,  0,                   0xFF,         0xFF },
  { false,            LE_RESET,  0,                   0xFF,         0xFF },
};

static const int DTM_MODE_COUNT  = sizeof(DTM_MODE) / sizeof(DTM_Mode_Settings);

static int iabs(int x) {
  return (x < 0) ? -x : x;
}

// This make the head select the target mode for the FCC test
static int current_mode = -1;
static int target_mode = 0;


static void configureTest(int mode) {
  SendDTMCommand msg;

  msg.command = DTM_MODE[mode].command;
  msg.freq = DTM_MODE[mode].freq;
  msg.length = DTM_MODE[mode].length;
  msg.payload = DTM_MODE[mode].payload;
    
  SendMessage(msg);

  current_mode = mode;
}

static void runTest(int mode) {
  if (!DTM_MODE[mode].motor_drive) {
    return ;
  }

  static const int DIRECTION_CHANGE_COUNT = 200;
  static const int DIRECTION_POWER = 0x4000;
  static int direction_counter = 0;
  static bool direction;
  
  if (++direction_counter >= DIRECTION_CHANGE_COUNT) {
    direction_counter = 0;
    direction = !direction;
  }

  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = direction ? -DIRECTION_POWER : DIRECTION_POWER;
  }
}

void Anki::Cozmo::HAL::FCC::start(void) { 
  // Enable body radio mode
  SetBodyRadioMode msg;
  msg.radioMode = BODY_DTM_OPERATING_MODE;  
  SendMessage(msg);

  configureTest(0);
}

// See "ESP8266 Certification and Test Guide" for details on the command format
static const char* WIFI_TESTS[] = {
  /* 0 = No Test             */  "CmdStop\r",
  /* 1 = 2412MHz  0dB 1Mbps  */  "WifiTxout 1 1 0 0\r",
  /* 2 = 2437MHz +4dB 1Mbps  */  "WifiTxout 6 1 240 0\r",
  /* 3 = 2462MHz  0dB 1Mbps  */  "WifiTxout 11 1 0 0\r",
};
#define WIFI_TEST_COUNT (sizeof(WIFI_TESTS)/sizeof(char*))
static void sendWifiCommand(int test) {
  static int running = 0, startdelay = 0;
  if (test >= WIFI_TEST_COUNT)
    test = 0;
  // Don't do anything if test is already running
  if (test == running)
    return;
  // Stop any running test first - and just wait to be called 5ms later
  if (running && test)
    test = 0;
  // If we're going to start a new test, we need to wait 100ms first
  if (test && ++startdelay < 20)
    return;
  startdelay = 0;
  running = test;
  
  // All we have is a bit-bang UART with no feedback.. so don't send commands too frequently!
  const char* s = WIFI_TESTS[test];
  const u32 DIVISOR = (32768*2560)/(115200);   // K02 weird RC clock / ESP baud rate
  
  // Send some start bits before the string
  GPIO_SET(GPIO_MISO, PIN_MISO);
  GPIO_OUT(GPIO_MISO, PIN_MISO);
  SOURCE_SETUP(GPIO_MISO, SOURCE_MISO, SourceGPIO);
  Anki::Cozmo::HAL::MicroWait(50);  
  while (*s)
  {
    u16 now, last = SysTick->VAL;
    u16 c = *s++;
    c <<= 1;      // Start bit
    c |= (3<<9);  // Stop bits (always 2)
    for (int i = 0; i < 11; i++)
    {
      // This line relies on 100MHz SysTick->LOAD in timer.cpp (despite our 84MHz actual clock)
      while (((last-(now = SysTick->VAL))&65535) < DIVISOR) // And don't forget Systick counts DOWN
        ;
      last = now;
      if (c & (1<<i))
        GPIO_SET(GPIO_MISO, PIN_MISO);
      else
        GPIO_RESET(GPIO_MISO, PIN_MISO);      
    }
  }
}

void Anki::Cozmo::HAL::FCC::mainDTMExecution(void) {
  // If the motors are not going crazy, use wheel positions for
  // selecting things
  if (!DTM_MODE[current_mode].motor_drive) {
    static int configuring_motor = 0;
    bool select = iabs(configuring_motor - g_dataToHead.positions[2]) > 0x100;

    if (select) {
      configuring_motor = g_dataToHead.positions[2];
      
      // Set our run mode
      if (current_mode != target_mode) {
        configureTest(target_mode);
      }
    }
  }

  // Target mode is set based on the position of the right tread
  target_mode = (unsigned int)(g_dataToHead.positions[0] >> 9) % DTM_MODE_COUNT;

  // Display current mode and what we would like to test
  static bool displayNum = false;

  static int updates = 0;
  
  updates = (updates+1) & 7;
  if (0 == updates)
    OLED::DisplayDigit(6, 0, target_mode / 10);
  if (1 == updates)
    OLED::DisplayDigit(12, 0, target_mode % 10);
  if (2 == updates)
    OLED::DisplayDigit(110, 0, current_mode / 10);
  if (3 == updates)
    OLED::DisplayDigit(116, 0, current_mode % 10);

  sendWifiCommand((current_mode < 10) ? 0 : current_mode-9);  // Wifi test modes start at 10
  runTest(current_mode);
}

struct LightEnable {
  int index;
  uint16_t color;
};

static const LightEnable lights[] = {
  { 0, 0x7FFF },
  { 1, 0x001F },
  { 1, 0x03E0 },
  { 1, 0x7C00 },
  { 2, 0x001F },
  { 2, 0x03E0 },
  { 2, 0x7C00 },
  { 3, 0x001F },
  { 3, 0x03E0 },
  { 3, 0x7C00 },
  { 4, 0x7FFF }
};
static int NUM_LIGHTS = sizeof(lights) / sizeof(LightEnable);

void Anki::Cozmo::HAL::FCC::mainLEDExecution(void) {
  // Select a color
  static int color = - 1;
  int select = (g_dataToHead.positions[0] >> 10) % NUM_LIGHTS;
 
  if (select == color) { return ; }
  color = select;
  
  // Tell the body to change it's colors
  Anki::Cozmo::RobotInterface::BackpackLights msg;

  memset(&msg, 0, sizeof(msg));

  int idx = lights[color].index;
  uint16_t clr = lights[color].color;
  
  msg.lights[idx].onColor = clr;
  msg.lights[idx].offColor = clr;

  SendMessage(msg);
}
