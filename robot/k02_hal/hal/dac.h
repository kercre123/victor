#ifndef __DAC_H
#define __DAC_H

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      void DACInit(void);
      void DACTone(void);
      void EnableAudio(bool enable);
      void FeedDAC(uint8_t* samples, int length);
    }
  }
}

#endif
