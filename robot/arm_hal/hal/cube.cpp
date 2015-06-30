#include <string.h>

#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"

#include "cube.h"

#define OLD_CUBE_EXPERIMENT 1 // for testing

#define MAX_CUBES 4

LEDPacket         g_LedStatus[MAX_CUBES];
AcceleratorPacket g_AccelStatus[MAX_CUBES];

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
      uint8_t EnqueueLightUpdate(volatile LEDPacket* packet) {
        #ifndef OLD_CUBE_EXPERIMENT
        static int updateCube = 0;
               
        updateCube = (updateCube+1) % MAX_CUBES;
        memcpy((void*)packet, &g_LedStatus[updateCube], sizeof(LEDPacket));
        return updateCube;
        #else
        return 0;
        #endif
      }

      #ifndef OLD_CUBE_EXPERIMENT
      #define NUM_BLOCK_LEDS 4

      Result SetBlockLight(const u8 blockID, const u32* onColor, const u32* offColor,
                           const u32* onPeriod_ms, const u32* offPeriod_ms,
                           const u32* transitionOnPeriod_ms, const u32* transitionOffPeriod_ms)
      {
        if (blockID >= MAX_CUBES) {
          return RESULT_FAIL;
        }
       
        uint8_t *light  = g_LedStatus[blockID].ledStatus;
        uint8_t *status = (uint8_t*) onColor;
        uint32_t sum = 0;
        
        for (int c = 0; c < NUM_BLOCK_LEDS * 4; c++, status++) {
          // Skip every 4th byte
          if (!(~c & 0x3)) {
            continue ;
          }
          
          uint32_t bright = *status;
          sum += bright*bright;
          *(light++) = bright;
        }

        uint32_t sq_sum = isqrt(sum);
        
        g_LedStatus[blockID].ledDark = (sq_sum & ~0xFF) ? 0 : (0x255 - sq_sum);
        
        return RESULT_OK;
      }
#else
      #define NUM_BLOCK_LEDS 8

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
