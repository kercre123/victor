#include <string.h>

#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"

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
          Messages::ActiveObjectTapped m;
          m.numTaps = count;
          m.objectID = id;
          RadioSendMessage(GET_MESSAGE_ID(Messages::ActiveObjectTapped), &m);
        }
        #endif
      }

      #ifndef OLD_CUBE_EXPERIMENT

      Result SetBlockLight(const u8 blockID, const u32* onColor, const u32* offColor,
                           const u32* onPeriod_ms, const u32* offPeriod_ms,
                           const u32* transitionOnPeriod_ms, const u32* transitionOffPeriod_ms)
      {
        if (blockID >= MAX_CUBES) {
          return RESULT_FAIL;
        }
       
        uint8_t *light  = g_LedStatus[blockID].ledStatus;
        uint32_t sum = 0;
        int order[] = {0,3,2,1};
        
        for (int c = 0; c < NUM_BLOCK_LEDS; c++) {
          uint32_t color = onColor[order[c]];
          
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
      #else

      Result SetBlockLight(const u8 blockID, const u32* onColor, const u32* offColor,
                           const u32* onPeriod_ms, const u32* offPeriod_ms,
                           const u32* transitionOnPeriod_ms, const u32* transitionOffPeriod_ms)
      {
         u8 buffer[256];
         const u32 size = Messages::GetSize(Messages::SetBlockLights_ID);
         Anki::Cozmo::Messages::SetBlockLights m;
         m.blockID = blockID;

         for (int i=0; i<NUM_BLOCK_LEDS; ++i) {
           m.onColor[i] = onColor[i];
           m.offColor[i] = offColor[i];
           m.onPeriod_ms[i] = onPeriod_ms[i];
           m.offPeriod_ms[i] = offPeriod_ms[i];
           m.transitionOnPeriod_ms[i] = (transitionOnPeriod_ms == NULL ? 0 : transitionOnPeriod_ms[i]);
           m.transitionOffPeriod_ms[i] = (transitionOffPeriod_ms == NULL ? 0 : transitionOffPeriod_ms[i]);
         }
         buffer[0] = Messages::SetBlockLights_ID;
         memcpy(buffer + 1, &m, size);
         return RadioSendPacket(buffer, size+1, blockID + 1) ? RESULT_OK : RESULT_FAIL;
      }
#endif
    }
  }
}
