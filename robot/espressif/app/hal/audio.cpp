extern "C" {
  #include "client.h"
  #include "osapi.h"
  #include "user_interface.h"
  #include "driver/i2spi.h"
}
#include "anki/cozmo/robot/hal.h"
#include "clad/types/animationKeyFrames.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
    
      bool AudioReady()
      {
        if (i2spiGetAudioSilenceSamples() > 0) return false;
        else return i2spiGetAudioBufferAvailable() >= AUDIO_SAMPLE_SIZE;
      }
      
      void AudioPlaySilence()
      {
        i2spiSetAudioSilenceSamples(AUDIO_SAMPLE_SIZE);
      }
      
      void AudioPlayFrame(AnimKeyFrame::AudioSample *msg)
      {
        i2spiBufferAudio(msg->sample, AUDIO_SAMPLE_SIZE);
      }
    
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki
