#pragma once

#include <vector>

#include "cozmoAnim/doom/doomAudioMixer.h"

namespace Anki {
  namespace AudioEngine {
    struct StandardWaveDataContainer;
  }
}

extern DoomAudioMixer _gMixer;

// namespace is sf to avoid renaming all types. this is a vestige from SFML
namespace sf {

typedef signed   short Int16;
typedef unsigned long long   Uint64;
  
  
  
class SoundBuffer
{
public:
  ~SoundBuffer();
  //bool loadFromSamples(const Int16* samples, Uint64 sampleCount, unsigned int channelCount, unsigned int sampleRate){ return true; }
  bool loadFromSamples(const std::vector<Int16>& data, unsigned int channelCount, unsigned int sampleRate);
  
  using WaveContainer = Anki::AudioEngine::StandardWaveDataContainer;
  const WaveContainer* GetData() const { return _audioData; }
private:
  Anki::AudioEngine::StandardWaveDataContainer* _audioData = nullptr;
};

class Sound
{
public:
  
  enum Status
  {
      Stopped,
      Paused,
      Playing
  };

  void setVolume(float volume) { _volume = volume; }
  void play();
  void stop();
  void setLoop(bool loop) { _looping = loop; }
  void setMinDistance (float distance){}
  void setAttenuation (float attenuation){}
  void setPitch(float pitch){}
  void setPosition(float x, float y, float z){}
  void setRelativeToListener(bool relative ) { _centeredOnListener = relative; }
  void SetMusic(bool music) { _music = music; }
  Status getStatus() const{ return _status; }
  void setBuffer(const SoundBuffer& buffer) { _buffer = &buffer; }
  void pause(){ stop(); } // todo: these should be different
  
private:
  
  const SoundBuffer* _buffer = nullptr;
  bool _centeredOnListener = false;
  Status _status = Status::Stopped;
  bool _looping = false;
  float _volume = 1.0f;
  bool _music = false; // currently we only play music
};

  
class Listener
{
public:
  static void setPosition (float x, float y, float z){}
  static void setDirection (float x, float y, float z){}
};
  
}
