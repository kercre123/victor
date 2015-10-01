#include <string.h>

#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"
#include "messages.h"
#include "clad/types/activeObjectTypes.h"

//#define OLD_CUBE_EXPERIMENT 1 // for testing

#define MAX_CUBES 20
#define NUM_BLOCK_LEDS 4

static LEDPacket         g_LedStatus[MAX_CUBES];
static AcceleratorPacket g_AccelStatus[MAX_CUBES];

uint32_t isqrt(uint32_t a_nInput)
{
    uint32_t op  = a_nInput;
    uint32_t res = 0;
    uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type


    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
    {
        one >>= 2;
    }

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      extern volatile ONCHIP GlobalDataToHead g_dataToHead;
      extern volatile ONCHIP GlobalDataToBody g_dataToBody;

      void DisplayStatus(int id) {
        static char cnt[4];
        char msg[512];
        char *wr = msg;
        
        cnt[id]++;
        
        for (int b = 0; b < 6; b++)
          wr += sprintf(wr, "%2x ", ((u8*)&g_dataToHead.cubeStatus)[b]);
        
        for (int i = 0; i < 4; i++) {
          wr += sprintf(wr, "\n%2x:", cnt[i]);
          for (int b = 0; b < 6; b++) {
            wr += sprintf(wr, " %2x", ((u8*)&g_AccelStatus[i])[b]);
          }
        }

        FacePrintf(msg);
      }
        
      
      void ManageCubes(void) {
        #ifndef OLD_CUBE_EXPERIMENT
        // LED status
        static int blockID = 0;

        blockID = (blockID+1) % MAX_CUBES;
        memcpy((void*)&g_dataToBody.cubeStatus, &g_LedStatus[blockID], sizeof(LEDPacket));
        g_dataToBody.cubeToUpdate = blockID;

        // Tap detection
        if (g_dataToHead.common.source != SPI_SOURCE_BODY){
          return ;
        }

        uint8_t id = g_dataToHead.cubeToUpdate;
        
        if (id >= MAX_CUBES) {
          return ;
        }

        uint8_t shocks = g_dataToHead.cubeStatus.shockCount;
        uint8_t count = shocks - g_AccelStatus[id].shockCount;
        memcpy(&g_AccelStatus[id], (void*)&g_dataToHead.cubeStatus, sizeof(AcceleratorPacket));
        
        //DisplayStatus(id);

        if (count > 0 && count < 16) {
          ObjectTapped m;
          m.numTaps = count;
          m.objectID = id;
          RobotInterface::SendMessage(m);
        }        
        
        
        // Detect if block moved from accel data
        const u8 START_MOVING_COUNT_THRESH = 5;  // Determines number of motion tics that much be observed before Moving msg is sent
        const u8 STOP_MOVING_COUNT_THRESH = 20;  // Determines number of no-motion tics that much be observed before StoppedMoving msg is sent
        static u8 movingTimeoutCtr[MAX_CUBES] = {0};
        static bool isMoving[MAX_CUBES] = {false};
        static UpAxis prevUpAxis[MAX_CUBES] = {UP_AXIS_UNKNOWN};
        
        s8 ax = g_AccelStatus[id].x;
        s8 ay = g_AccelStatus[id].y;
        s8 az = g_AccelStatus[id].z;
        
        
        // Compute upAxis
        // Send ObjectMoved message if upAxis changes
        s8 maxAccelVal = 0;
        UpAxis upAxis = UP_AXIS_UNKNOWN;
        if (abs(ax) > maxAccelVal) {
          upAxis = ax > 0 ? UP_AXIS_Xpos : UP_AXIS_Xneg;
          maxAccelVal = abs(ax);
        }
        if (abs(ay) > maxAccelVal) {
          upAxis = ay > 0 ? UP_AXIS_Ypos : UP_AXIS_Yneg;
          maxAccelVal = abs(ay);
        }
        if (abs(az) > maxAccelVal) {
          upAxis = az > 0 ? UP_AXIS_Zpos : UP_AXIS_Zneg;
          maxAccelVal = abs(az);
        }
        bool upAxisChanged = (prevUpAxis[id] != UP_AXIS_UNKNOWN) && (prevUpAxis[id] != upAxis);
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
        
        if (upAxisChanged || ((movingTimeoutCtr[id] >= START_MOVING_COUNT_THRESH) && !isMoving[id])) {
          ActiveObjectMoved m;
          m.objectID = id;
          m.xAccel = ax;
          m.yAccel = ay;
          m.zAccel = az;
          m.upAxis = upAxis;  // This should get processed on engine eventually
          RobotInterface::SendMessage(m);
          isMoving[id] = true;
          movingTimeoutCtr[id] = STOP_MOVING_COUNT_THRESH;
        } else if ((movingTimeoutCtr[id] == 0) && isMoving[id]) {
          Messages::ActiveObjectStoppedMoving m;
          m.objectID = id;
          m.upAxis = upAxis;  // This should get processed on engine eventually
          m.rolled = 0;  // This should get processed on engine eventually
          RobotInterface::SendMessage(m);
          isMoving[id] = false;
        }
        #endif
      }

      Result SetBlockLight(const u32 blockID, const LightState* lights)
      {
        if (blockID >= MAX_CUBES) {
          return RESULT_FAIL;
        }
       
        uint8_t *light  = g_LedStatus[blockID].ledStatus;
        uint32_t sum = 0;
        static const int order[] = {0,3,2,1};
        
        for (int c = 0; c < NUM_BLOCK_LEDS; c++) {
          uint32_t color = lights[order[c]].onColor;
          
          for (int ch = 24; ch > 0; ch -= 8) {
            uint8_t status = (color >> ch) & 0xFF;
            uint32_t bright = status;
            sum += bright * bright;
            *(light++) = bright;
          }
        }

        uint32_t sq_sum = isqrt(sum);
        
        g_LedStatus[blockID].ledDark = (sq_sum & ~0xFF) ? 0 : (0x255 - sq_sum);
        
        return RESULT_OK;
      }
    }
  }
}
