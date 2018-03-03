#pragma once

#include <mutex>
#include <queue>

namespace Anki{
  namespace AudioEngine {
    struct StandardWaveDataContainer;
  }
  namespace Cozmo {
    namespace Audio {
      class CozmoAudioController;
    }
  }
}

class DoomAudioMixer {
public:
  
  using WaveContainer = Anki::AudioEngine::StandardWaveDataContainer;
  
  // clears whatever is playing and plays something new. todo: multiple wwise wave plugins
  void Play( const WaveContainer* container, bool looping);
  
  using AudioController = Anki::Cozmo::Audio::CozmoAudioController;
  void SetAudioController(AudioController* audioController){ _audioController = audioController; }
  
  void FlushPlayQueue();
private:
  
  void PlayInternal( const WaveContainer* container, bool looping);
  
  struct QueueEntry {
    const Anki::AudioEngine::StandardWaveDataContainer* container;
    bool looping;
  };
  std::mutex _mutex;
  std::queue<QueueEntry> _playQueue;
  
  AudioController* _audioController = nullptr;
  const WaveContainer* _lastContainer = nullptr;
  
};

