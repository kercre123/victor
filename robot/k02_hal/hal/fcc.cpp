#include "fcc.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"

#include "anki/cozmo/robot/spineData.h"


#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo::RobotInterface;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

struct DTM_Mode_Settings {
  bool    motor_drive;
  uint8_t mode;
  uint8_t channel;
};

static const DTM_Mode_Settings DTM_MODE[] = {
  { false, DTM_LISTEN, 3 },
  { false, DTM_TRANSMIT, 3 },
  { false, DTM_TRANSMIT, 42 },
  { false, DTM_TRANSMIT, 81 },
  { false, DTM_CARRIER, 3 },
  { false, DTM_CARRIER, 42 },
  { false, DTM_CARRIER, 81 },
  { true,  DTM_DISABLED, 0 }
};

static const int DTM_MODE_COUNT  = sizeof(DTM_MODE) / sizeof(DTM_Mode_Settings);

static const int abs(int x) {
  return (x < 0) ? -x : x;
}

  // This make the head select the target mode for the FCC test
  static int current_mode = -1;
  static int target_mode = 0;


static void configureTest(int mode) {
  SetDTMParameters msg;
  msg.channel = DTM_MODE[mode].channel;
  msg.mode = DTM_MODE[mode].mode;
    
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
  SetBodyRadioMode msg;
  msg.radioMode = BODY_DTM_OPERATING_MODE;
  
  SendMessage(msg);
  
  configureTest(0);
}


void Anki::Cozmo::HAL::FCC::mainExecution(void) {
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

  displayNum = !displayNum;
  if (displayNum) {
    OLED::DisplayDigit(target_mode, 48, 2);
  } else {
    OLED::DisplayDigit(current_mode, 48, 4);
  }

  runTest(current_mode);
}
