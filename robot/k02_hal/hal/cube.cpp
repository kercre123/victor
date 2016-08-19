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

// Motion computation thresholds and vars
static const u8 START_MOVING_COUNT_THRESH = 2; // Determines number of motion tics that much be observed before Moving msg is sent
static const u8 STOP_MOVING_COUNT_THRESH = 5;  // Determines number of no-motion tics that much be observed before StoppedMoving msg is sent
static const u8 ACC_MOVE_THRESH = 5;           // If current value differs from filtered value by this much, block is considered to be moving.
static u8 movingTimeoutCtr[MAX_CUBES] = {0};
static bool isMoving[MAX_CUBES] = {false};

static Anki::Cozmo::UpAxis prevMotionDir[MAX_CUBES] = {Anki::Cozmo::Unknown};  // Last sent motion direction
static Anki::Cozmo::UpAxis prevUpAxis[MAX_CUBES] = {Anki::Cozmo::Unknown};     // Last sent upAxis
static Anki::Cozmo::UpAxis nextUpAxis[MAX_CUBES] = {Anki::Cozmo::Unknown};     // Candidate next upAxis to send
static u8 nextUpAxisCount[MAX_CUBES] = {0};          // Number of times candidate next upAxis is observed
static const u8 UPAXIS_STABLE_COUNT_THRESH = 15;     // How long the candidate upAxis must be observed for before it is reported
static const Anki::Cozmo::UpAxis idxToUpAxis[3][2] = { {Anki::Cozmo::XPositive, Anki::Cozmo::XNegative},
                                                       {Anki::Cozmo::YPositive, Anki::Cozmo::YNegative},
                                                       {Anki::Cozmo::ZPositive, Anki::Cozmo::ZNegative} };


namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void ClearActiveObjectData()
      {
        for (int i=0; i< MAX_CUBES; ++i) {
          movingTimeoutCtr[i] = 0;
          isMoving[i] = false;
          
          prevMotionDir[i] = Unknown;
          prevUpAxis[i] = Unknown;
          nextUpAxis[i] = Unknown;
          nextUpAxisCount[i] = 0;
        }
      }
      
      void ReportUpAxisChanged(int id, UpAxis a)
      {
        ObjectUpAxisChanged m;
        m.timestamp = HAL::GetTimeStamp();
        m.objectID = id;
        m.upAxis = a;
        RobotInterface::SendMessage(m);
      }
      
      void GetPropState(uint8_t id, int ax, int ay, int az, int shocks,
                        uint8_t tapTime, int8_t tapNeg, int8_t tapPos) {
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
          m.robotID = 0;
          m.tapTime = tapTime;
          m.tapNeg = tapNeg;
          m.tapPos = tapPos;
          RobotInterface::SendMessage(m);
        }


        // Accumulate IIR filter for each axis
        static s32 acc_filt[MAX_CUBES][3] = {0};
        static const s32 filter_coeff = 20;
        acc_filt[id][0] = ((filter_coeff * ax) + ((100 - filter_coeff) * acc_filt[id][0])) / 100;
        acc_filt[id][1] = ((filter_coeff * ay) + ((100 - filter_coeff) * acc_filt[id][1])) / 100;
        acc_filt[id][2] = ((filter_coeff * az) + ((100 - filter_coeff) * acc_filt[id][2])) / 100;

        // Check for motion
        bool xMoving = ABS(acc_filt[id][0] - ax) > ACC_MOVE_THRESH;
        bool yMoving = ABS(acc_filt[id][1] - ay) > ACC_MOVE_THRESH;
        bool zMoving = ABS(acc_filt[id][2] - az) > ACC_MOVE_THRESH;
        bool isMovingNow = xMoving || yMoving || zMoving;

        if (isMovingNow) {
          // Incremenent counter if block is not yet considered moving, or
          // if it is already considered moving and the moving count is less than STOP_MOVING_COUNT_THRESH
          // so that it doesn't need to decrement so much before it's considered stop moving again.
          if (!isMoving[id] || (movingTimeoutCtr[id] < STOP_MOVING_COUNT_THRESH)) {
            ++movingTimeoutCtr[id];
          }
        } else if (movingTimeoutCtr[id] > 0) {
          --movingTimeoutCtr[id];
        }
        

//        if (id <= 2) {
//          AnkiDebugPeriodic(50, 217, "CUBE_ACC", 521, "%d: ax %d, ay %d, az %d; filt_x %d, filt_y %d, filt_z %d", 7,
//                            id, ax, ay, az, acc_filt[id][0], acc_filt[id][1], acc_filt[id][2]);
//        }
        
        // Compute motion direction change. i.e. change in dominant acceleration vector
        s8 maxAccelVal = 0;
        UpAxis motionDir = Unknown;

        for (int i=0; i < 3; ++i) {
          s8 absAccel = ABS(acc_filt[id][i]);
          if (absAccel > maxAccelVal) {
            motionDir = idxToUpAxis[i][acc_filt[id][i] > 0 ? 0 : 1];
            maxAccelVal = absAccel;
          }
        }
        bool motionDirChanged = (prevMotionDir[id] != Unknown) && (prevMotionDir[id] != motionDir);
        prevMotionDir[id] = motionDir;


        // Check for stable up axis
        // Send ObjectUpAxisChanged msg if it changes
        if (motionDir == nextUpAxis[id]) {
          //s32 accSum = ax + ay + az;  // magnitude of 64 is approx 1g

          if (nextUpAxis[id] != prevUpAxis[id]) {
            if (++nextUpAxisCount[id] >= UPAXIS_STABLE_COUNT_THRESH) {
              ReportUpAxisChanged(id, nextUpAxis[id]);
              prevUpAxis[id] = nextUpAxis[id];
            }
          } else {
            // Next is same as what was already reported so no need to accumulate count
            nextUpAxisCount[id] = 0;
          }
        } else {
          nextUpAxisCount[id] = 0;
          nextUpAxis[id] = motionDir;
        }

        // Send ObjectMoved message if object was stationary and is now moving or if motionDir changes
        if (motionDirChanged ||
            ((movingTimeoutCtr[id] >= START_MOVING_COUNT_THRESH) && !isMoving[id])) {
          ObjectMoved m;
          m.timestamp = HAL::GetTimeStamp();
          m.objectID = id;
          m.robotID = 0;
          m.accel.x = ax;
          m.accel.y = ay;
          m.accel.z = az;
          m.axisOfAccel = motionDir;
          RobotInterface::SendMessage(m);
          isMoving[id] = true;
          movingTimeoutCtr[id] = STOP_MOVING_COUNT_THRESH;
        } else if ((movingTimeoutCtr[id] == 0) && isMoving[id]) {
      
          // If there is an upAxis change to report, then report it.
          // Just make more logical sense if it gets sent before the stopped message.
          if (nextUpAxis[id] != prevUpAxis[id]) {
            ReportUpAxisChanged(id, nextUpAxis[id]);
            prevUpAxis[id] = nextUpAxis[id];
          }
          
          // Send stopped message
          ObjectStoppedMoving m;
          m.timestamp = HAL::GetTimeStamp();
          m.objectID = id;
          m.robotID = 0;
          RobotInterface::SendMessage(m);
          isMoving[id] = false;
        }

      }
    }
  }
}
