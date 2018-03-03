#include "cozmoAnim/doom/doomAudioMixer.h"
#include "cozmoAnim/doom/sf/Audio.h"

#include "audioEngine/audioTools/standardWaveDataContainer.h"

DoomAudioMixer _gMixer;

// namespace is sf to avoid renaming all types. this is a vestige from SFML
namespace sf {
 

  
SoundBuffer::~SoundBuffer()
{
  delete _audioData;
}
  
//using WaveContainer = Anki::AudioEngine::StandardWaveDataContainer;
//WaveContainer SoundBuffer::GetDataCopy() const
//{
//  return WaveContainer(*_audioData);
//}
  
bool SoundBuffer::loadFromSamples(const std::vector<Int16>& samples, unsigned int channelCount, unsigned int sampleRate)
{
  using namespace Anki::AudioEngine;
  
  if( _audioData ) {
    delete _audioData;
  }

  _audioData = new StandardWaveDataContainer(sampleRate, channelCount, samples.size());
  
  // Convert waveData format into StandardWaveDataContainer's format
  const float kOneOverSHRT_MAX = 1.0f / float(SHRT_MAX);
  for (size_t sampleIdx = 0; sampleIdx < _audioData->bufferSize; ++sampleIdx) {
    _audioData->audioBuffer[sampleIdx] = samples[sampleIdx] * kOneOverSHRT_MAX;
  }
  
  return true;
}
  
void Sound::play()
{
  // currently we only play music, becuase there's one wav playing plugin
  if( !_music ) {
    return;
  }
  if( _buffer == nullptr ) {
    return;
  }
 
  _gMixer.Play( _buffer->GetData() , _looping );
}
 
                       
                       
  
void Sound::stop()
{
  
}

}
