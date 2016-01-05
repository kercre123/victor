#ifndef __DAC_H
#define __DAC_H

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace DAC {
        void Init(void);
        void Tone(void);
        void EnableAudio(bool enable);
        void Feed(uint8_t* samples, int length);
      }
    }
  }
}

#endif
