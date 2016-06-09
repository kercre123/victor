#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "uart.h"
#include "hal/portable.h"
#include "messages.h"
#include "spine.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/robotInterface/messageToActiveObject.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "uart.h"

#include "anki/cozmo/robot/logging.h"

#define MAX_CUBES 7

static uint8_t g_shockCount[MAX_CUBES];

// Snapshot of sum of last accelerometer values received that 
// is within ACC_MOVE_THRESH of the current sum of values on all axes.
// Used for motion detection.
static int lastAccSum[MAX_CUBES] = {0};
static u32 lastMotionCheckTime_ms = 0;
const u32 MOVE_CHECK_PERIOD_MS = 250;

// If the sum of acceleration values on all axes changes
// by this much, the block is considered to be moving.
const int ACC_MOVE_THRESH = 3;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void GetPropState(int id, int ax, int ay, int az, int shocks) {
        // Tap detection
        if (id >= MAX_CUBES) {
          return ;
        }

        uint8_t count = shocks - g_shockCount[id];

        g_shockCount[id] = shocks;

        // Do not mis-report taps / cube movement (filtering)
        if (count > 8) {
          return ;
        }
        
        //DisplayStatus(id);

        u32 currTime_ms = HAL::GetTimeStamp();
        if (count > 0) {
          ObjectTapped m;
          m.timestamp = currTime_ms;
          m.numTaps = count;
          m.objectID = id;
          RobotInterface::SendMessage(m);
        }

        // Compute upAxis
        // Send ObjectMoved message if upAxis changes
        static UpAxis prevUpAxis[MAX_CUBES] = {Unknown};
        s8 maxAccelVal = 0;
        UpAxis upAxis = Unknown;
        if (ABS(ax) > maxAccelVal) {
          upAxis = ax > 0 ? XPositive : XNegative;
          maxAccelVal = ABS(ax);
        }
        if (ABS(ay) > maxAccelVal) {
          upAxis = ay > 0 ? YPositive : YNegative;
          maxAccelVal = ABS(ay);
        }
        if (ABS(az) > maxAccelVal) {
          upAxis = az > 0 ? ZPositive : ZNegative;
          maxAccelVal = ABS(az);
        }
        bool upAxisChanged = (prevUpAxis[id] != Unknown) && (prevUpAxis[id] != upAxis);
        prevUpAxis[id] = upAxis;

        
        
        // Detect if block moved from accel data
        const u8 START_MOVING_COUNT_THRESH = 5;  // Determines number of motion tics that much be observed before Moving msg is sent
        const u8 STOP_MOVING_COUNT_THRESH = 10;  // Determines number of no-motion tics that much be observed before StoppedMoving msg is sent
        static u8 movingTimeoutCtr[MAX_CUBES] = {0};
        static bool isMoving[MAX_CUBES] = {false};
        
        // lastAccSum is the reference value we compare current sum 
        // against to determine if there was motion. This reference value
        // is updated periodically.
        //
        // NOTE: Using sum of accelerations because it's a simple value that changes
        // with rotations since we're not getting rotations. This is not at all a
        // mathematically supported way of doing motion detection, but it seems to work
        // and it's small. When all this logic moves to the engine we can do something fancier.
        //
        s32 accSum = ax + ay + az;  // magnitude of 64 is approx 1g
        if (currTime_ms > lastMotionCheckTime_ms) {  
          lastAccSum[id] = accSum;
          lastMotionCheckTime_ms = currTime_ms + MOVE_CHECK_PERIOD_MS;
        }
        
        s32 accDiff = ABS(accSum - lastAccSum[id]);
        bool isMovingNow = accDiff >= ACC_MOVE_THRESH;
        if (isMovingNow) {
          ++movingTimeoutCtr[id];
        } else if (movingTimeoutCtr[id] > 0) {
          --movingTimeoutCtr[id];
        }
        //AnkiDebugPeriodic(10, 181, "CUBE_ACC", 482, "ax %d, ay %d, az %d, sum %d, accDiff %d", 5, ax, ay, az, accSum, accDiff);
        
        if (upAxisChanged ||
            ((movingTimeoutCtr[id] >= START_MOVING_COUNT_THRESH) && !isMoving[id])) {
          ObjectMoved m;
          m.timestamp = HAL::GetTimeStamp();
          m.objectID = id;
          m.accel.x = ax;
          m.accel.y = ay;
          m.accel.z = az;
          m.upAxis = upAxis;  // This should get processed on engine eventually
          RobotInterface::SendMessage(m);
          isMoving[id] = true;
          movingTimeoutCtr[id] = STOP_MOVING_COUNT_THRESH;
        } else if ((movingTimeoutCtr[id] == 0) && isMoving[id]) {
          ObjectStoppedMoving m;
          m.timestamp = HAL::GetTimeStamp();
          m.objectID = id;
          m.upAxis = upAxis;  // This should get processed on engine eventually
          m.rolled = 0;  // This should get processed on engine eventually
          RobotInterface::SendMessage(m);
          isMoving[id] = false;
        }

      }
    }
  }
}
