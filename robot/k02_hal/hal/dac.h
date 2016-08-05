#ifndef __DAC_H
#define __DAC_H

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace DAC {
        void Init(void);
        void Mute(void);
        void EnableAudio(bool enable);
        void Feed(bool enable, uint8_t* samples);
        void Sync();
        void Tone(const float mult);
      }
    }
  }
}

#endif
