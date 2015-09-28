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
        
        if (count) {
          ObjectTapped m;
          m.numTaps = count;
          m.objectID = id;
          RobotInterface::SendMessage(m);
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
