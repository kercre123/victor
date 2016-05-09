#include <string.h>

#include "fcc.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"

#include "anki/cozmo/robot/spineData.h"


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
  { false,    LE_RECEIVER_TEST,  1,                   0xFF,         0xFF },
  { false, LE_TRANSMITTER_TEST,  1,          DTM_PKT_PRBS9,           31 },
  { false, LE_TRANSMITTER_TEST, 20,          DTM_PKT_PRBS9,           31 },
  { false, LE_TRANSMITTER_TEST, 40,          DTM_PKT_PRBS9,           31 },
  { false, LE_TRANSMITTER_TEST,  1, DTM_PKT_VENDORSPECIFIC, CARRIER_TEST },
  { false, LE_TRANSMITTER_TEST, 20, DTM_PKT_VENDORSPECIFIC, CARRIER_TEST },
  { false, LE_TRANSMITTER_TEST, 40, DTM_PKT_VENDORSPECIFIC, CARRIER_TEST },
  { true,             LE_RESET,  0,                   0xFF,         0xFF }
};

static const int DTM_MODE_COUNT  = sizeof(DTM_MODE) / sizeof(DTM_Mode_Settings);

static int abs(int x) {
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
  // Disable espressif test mode
  EnterFactoryTestMode eftm;
  eftm.mode = FTM_None;
  SendMessage(eftm);

  // Enable body radio mode
  SetBodyRadioMode msg;
  msg.radioMode = BODY_DTM_OPERATING_MODE;  
  SendMessage(msg);

  configureTest(0);
}



void Anki::Cozmo::HAL::FCC::mainDTMExecution(void) {
  // If the motors are not going crazy, use wheel positions for
  // selecting things
  if (!DTM_MODE[current_mode].motor_drive) {
    static int configuring_motor = g_dataToHead.positions[0];
    bool select = abs(configuring_motor - g_dataToHead.positions[0]) > 0x400;

    if (select) {
      configuring_motor = g_dataToHead.positions[0];
      
      // Set our run mode
      if (current_mode != target_mode) {
        configureTest(target_mode);
      }
    }
  }

  // Target mode is set based on the position of the left tred
  target_mode = (unsigned int)(g_dataToHead.positions[1] >> 10) % DTM_MODE_COUNT;

  // Display current mode and what we would like to test
  static bool displayNum = false;

  Anki::Cozmo::RobotInterface::DisplayNumber msg;

  msg.x = 32;
  
  displayNum = !displayNum;
  if (displayNum) {
    msg.value = target_mode;
    msg.y = 0;
  } else {
    msg.value = current_mode;
    msg.y = 32;
  }

  RobotInterface::SendMessage(msg);

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
