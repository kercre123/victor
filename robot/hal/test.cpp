#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#include "uart.h"
#include "dac.h"
#include "oled.h"
#include "imu.h"

// These error numbers should be in a file shared with fixture.h
// To aid memorization, error numbers are grouped logically:
//  3xy - motor errors - where X is the motor 1 (left), 2 (right), 3 (lift), or 4 (head) - and Y is the problem (reversed, encoder, etc)
//  5xy - head errors - where X is the component (0 = CPU) and Y is the problem
//  6xy - body errors - where X is the component (0 = CPU) and Y is the problem
enum STARTUP_ERROR {
  ERROR_NONE = 0,
  ERROR_IMU_ID = 520,           // 520 for IMU errors
  ERROR_CLIFF_SENSOR = 620,     // 620 for cliff sensor errors (fixture could distinguish LED error from photodiode error)
  ERROR_MOTOR_NOFORWARD = 310,  // Motor did not move forward
  ERROR_MOTOR_NOBACKWARD = 311, // Motor did not move backward
  ERROR_MOTOR_REVERSED = 312,   // Encoder showed motor moving in reverse direction
};

// Magical numbers for self test
// Relaxed by Nathan for EP1
static const int DROP_THRESHOLD = 0x80;
static const int MOTOR_THRESHOLD[] = {
  0x80, 0x80, 0x4000, 0x400
};

int RunTests(void);

static inline void TestPause(const int duration) {
  using namespace Anki::Cozmo::HAL;

  for (int i = duration * 20; i > 0; i--) {
    UART::WaitForSync();
  }
}

extern "C" void LeaveFacePrintf(void);

void StartupSelfTest(void) {
  using namespace Anki::Cozmo::HAL;

  int result = RunTests();
  
  if (result != ERROR_NONE) {
    OLED::DisplayNumber(result, 88, 0);
  }
}

int RunTests(void) {
  using namespace Anki::Cozmo::HAL;

  uint8_t imuid = IMU::ReadID();
  
  if (imuid != 0xD1) {
    return ERROR_IMU_ID;
  }
  
  // DAC Tone has moved to main loop so FTM0 can be reinitalized to something else
  TestPause(1);
  
  // Measure cliff sensor now, but don't report trouble until motors are okay
  uint32_t cliffLevel = g_dataToHead.cliffLevel;

  // Wind the lift and head motor back enough to allow some travel in the tests ahead
  for (int i = 2; i < 4; i++) {
    g_dataToBody.motorPWM[i] = -0x3FFF;
  }
  TestPause(3);
  
  for (int i = 2; i < 4; i++) {
    g_dataToBody.motorPWM[i] = 0;
  }
  TestPause(1);
  
  // Record start position, drive forward
  Fixed start[4], forward[4], backward[4];
  memcpy(start, g_dataToHead.positions, sizeof(g_dataToHead.positions));
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = 0x3FFF;
  }
  TestPause(3);
  
  // Stop the motors and let them settle
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = 0;
  }
  TestPause(1);

  // Record forward position, drive backward
  memcpy(forward, g_dataToHead.positions, sizeof(g_dataToHead.positions));
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = -0x3FFF;
  }
  TestPause(3);

  memcpy(backward, g_dataToHead.positions, sizeof(g_dataToHead.positions));
  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = 0;
  }
  
  // This helps debug failing lifts - looks like they were moving about 0x8000 on average
  //FacePrintf("%x %x", forward[2] - start[2], forward[2] - backward[2]);
  
  // Leave head up for ease of display
  g_dataToBody.motorPWM[3] = 0x7fff;  
  TestPause(3);
  g_dataToBody.motorPWM[3] = 0x0;
  
  // Light test!
  /*
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
  */

  for (int i = 0; i < 4; i++) {
    if (MOTOR_THRESHOLD[i] > forward[i] - start[i]) {
      if (-MOTOR_THRESHOLD[i] > forward[i] - start[i])
        return ERROR_MOTOR_REVERSED + i*10;
      else
        return ERROR_MOTOR_NOFORWARD + i*10;
    }

    if (MOTOR_THRESHOLD[i] > forward[i] - backward[i]) {
      if (-MOTOR_THRESHOLD[i] > forward[i] - backward[i])
        return ERROR_MOTOR_REVERSED + i*10;
      else
        return ERROR_MOTOR_NOBACKWARD + i*10;
    }
  }
  
  if (cliffLevel < DROP_THRESHOLD) {
    return ERROR_CLIFF_SENSOR;
  }
  
  return ERROR_NONE;
}
