#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#include "uart.h"
#include "dac.h"
#include "oled.h"
#include "imu.h"

enum STARTUP_ERROR {
  ERROR_NONE,
  ERROR_IMU_ID,
  ERROR_CLIFF_SENSOR,
  ERROR_ENCODER_FORWARD,
  ERROR_ENCODER_BACKWARD
};

// Magical numbers for self test
static const int DROP_THREADHOLD = 0x200;
static const int MOTOR_THRESHOLD[] = {
  0x80, 0x80, 0x10000, 0x400
};

STARTUP_ERROR RunTests(void);

static inline void TestPause(const int duration) {
  using namespace Anki::Cozmo::HAL;
  MicroWait(100000*duration);
  WaitForSync();
}

void StartupSelfTest(void) {
  using namespace Anki::Cozmo::HAL;
  
  STARTUP_ERROR result = RunTests();
  
  if (result != ERROR_NONE) {
    FacePrintf("%i FAIL", (int) result);
  }
}

STARTUP_ERROR RunTests(void) {
  using namespace Anki::Cozmo::HAL;

  FacePrintf("SELF TEST");
  
  uint8_t imuid = ReadIMUID();
  
  if (imuid != 0xD1) {
    return ERROR_IMU_ID;
  }
  
  DACTone();

  TestPause(1);
  
  if (g_dataToHead.cliffLevel < DROP_THREADHOLD) {
    return ERROR_CLIFF_SENSOR;
  }
  
  Fixed start[4], forward[4], backward[4];
  
  memcpy(start, g_dataToHead.positions, sizeof(g_dataToHead.positions));
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = 0x3FFF;
  }
  TestPause(3);
  
  memcpy(forward, g_dataToHead.positions, sizeof(g_dataToHead.positions));
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = -0x3FFF;
  }
  TestPause(3);

  memcpy(backward, g_dataToHead.positions, sizeof(g_dataToHead.positions));
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = 0;
  }
  TestPause(3);

  // Light test!
  uint32_t *lightValue = (uint32_t*) &g_dataToBody.backpackColors;
  
  memset(g_dataToBody.backpackColors, 0, sizeof(g_dataToBody.backpackColors));
  

  lightValue[0] = 0;
  for (int i = 1; i < 4; i++) lightValue[i] = 0x0000FF;
  TestPause(5);
  
  lightValue[0] = 0xFFFFFF;
  for (int i = 1; i < 4; i++) lightValue[i] = 0x00FF00;
  TestPause(5);
  
  lightValue[0] = 0;
  for (int i = 1; i < 4; i++) lightValue[i] = 0xFF0000;
  TestPause(5);

  memset(g_dataToBody.backpackColors, 0, sizeof(g_dataToBody.backpackColors));

  for (int i = 0; i < 4; i++) {
    if (MOTOR_THRESHOLD[i] > forward[i] - start[i]) {
      return ERROR_ENCODER_FORWARD;
    }

    if (MOTOR_THRESHOLD[i] > forward[i] - backward[i]) {
      return ERROR_ENCODER_BACKWARD;
    }
  }

  OLEDFlip(); 

  return ERROR_NONE;
}
