#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "uart.h"
#include "hal/portable.h"
#include "messages.h"
#include "cube.h"
#include "spine.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/robotInterface/messageToActiveObject.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "uart.h"

AcceleratorPacket g_AccelStatus[MAX_CUBES];
uint16_t g_LedStatus[MAX_CUBES][NUM_BLOCK_LEDS];
uint32_t g_SlotId[MAX_CUBES];

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      extern void GetBackpack(uint16_t* colors);

      Result AssignCubeSlots(int total_ids, const uint32_t* ids) {
        int slots = MIN(total_ids, MAX_CUBES);

        for (int i = 0; i < slots; i++) {
          g_SlotId[i] = ids[i];
        }

        Cube::SendPropIds();

        return RESULT_OK;
      }
      
      // ONE DAY THIS WILL DO BETTER STUFF
      void Cube::SpineIdle(SpineProtocol& msg) {
        static int index = 0;
        static int prop = 0;

        if (prop < MAX_CUBES && g_SlotId[prop]) {
          msg.opcode = SET_PROP_STATE;
          msg.SetPropState.slot = prop;
          memcpy((void*)&msg.SetPropState.colors, &g_LedStatus[prop], sizeof(msg.SetPropState.colors));
        } else {
          msg.opcode = SET_BACKPACK_STATE;
          GetBackpack(msg.SetBackpackState.colors);
        }

        if (prop++ > MAX_CUBES) {
          prop = 0;
        }
      }
      
      void Cube::SendPropIds(void) {
        for (int i = 0; i < MAX_CUBES; i++) {
          if (g_SlotId[i]) {
            SpineProtocol msg;

            msg.opcode = ASSIGN_PROP;
            msg.AssignProp.slot = i;
            msg.AssignProp.prop_id = g_SlotId[i];
            
            Spine::Enqueue(msg);
          }
        }
      }
      
      void DiscoverProp(uint32_t id) {
        ObjectDiscovered m;
        m.factory_id = id;
        RobotInterface::SendMessage(m);
      }
      
      void GetPropState(int id, int x, int y, int z, int shocks) {
        // Tap detection
        if (id >= MAX_CUBES) {
          return ;
        }

        uint8_t count = shocks - g_AccelStatus[id].shockCount;

        g_AccelStatus[id].x = x;
        g_AccelStatus[id].y = y;
        g_AccelStatus[id].z = z;
        g_AccelStatus[id].shockCount = shocks;

        //DisplayStatus(id);

        if (count > 0 && count < 16) {
          ObjectTapped m;
          m.timestamp = HAL::GetTimeStamp();
          m.numTaps = count;
          m.objectID = id;
          RobotInterface::SendMessage(m);
        }

        // Detect if block moved from accel data
        const u8 START_MOVING_COUNT_THRESH = 5;  // Determines number of motion tics that much be observed before Moving msg is sent
        const u8 STOP_MOVING_COUNT_THRESH = 20;  // Determines number of no-motion tics that much be observed before StoppedMoving msg is sent
        static u8 movingTimeoutCtr[MAX_CUBES] = {0};
        static bool isMoving[MAX_CUBES] = {false};
        static UpAxis prevUpAxis[MAX_CUBES] = {Unknown};

        s8 ax = g_AccelStatus[id].x;
        s8 ay = g_AccelStatus[id].y;
        s8 az = g_AccelStatus[id].z;


        // Compute upAxis
        // Send ObjectMoved message if upAxis changes
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


        // Compute acceleration due to movement
        s32 accSqrd = ax*ax + ay*ay + az*az;
        bool isMovingNow = !NEAR(accSqrd, 64*64, 500);  // 64 == 1g

        if (isMovingNow) {
          if (movingTimeoutCtr[id] < STOP_MOVING_COUNT_THRESH) {
            ++movingTimeoutCtr[id];
          }
        } else if (movingTimeoutCtr[id] > 0) {
          --movingTimeoutCtr[id];
        }

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

      Result SetBlockLight(const u32 blockID, const u16* colors)
      {
        if (blockID >= MAX_CUBES) {
          return RESULT_FAIL;
        }
        
        memcpy(g_LedStatus[blockID], colors, sizeof(g_LedStatus[blockID]));
        
        return RESULT_OK;
      }
    }
  }
}
