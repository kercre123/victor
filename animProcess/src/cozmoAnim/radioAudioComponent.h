/**
* File: radioAudioComponent.h
*
* Author:
*
* Description:
*
* Copyright: Anki, Inc. 2018
*
*/

#ifndef __Anki_cozmo_cozmoAnim_RadioAudioComponent_H__
#define __Anki_cozmo_cozmoAnim_RadioAudioComponent_H__

#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "audioEngine/audioTools/streamingWaveDataInstance.h"
//#include "audioEngine/audioTypes.h"
//#include "coretech/common/shared/types.h"
//#include "clad/audio/audioEventTypes.h"
//#include "clad/audio/audioGameObjectTypes.h"
//#include "clad/audio/audioSwitchTypes.h"
//#include "clad/types/textToSpeechTypes.h"
//#include "util/helpers/templateHelpers.h"
#include <deque>
#include <mutex>
#include <map>

// Forward declarations
namespace Anki {
  namespace Vector {
    class AnimContext;
    //class AudioLayerBuffer;
    class AudioDataBuffer;
    namespace Audio {
      class CozmoAudioController;
    }
  }
  namespace Util {
    namespace Dispatch {
      class Queue;
    }
  }
}

namespace Anki {
namespace Vector {

class RadioAudioComponent
{
public:
  RadioAudioComponent();
  ~RadioAudioComponent();
  
  void Init(const AnimContext* context);
  
  void Update();
  
  // adding data starts playing
  void AddData( uint8_t* data, int size );
  
  void Stop();
  
private:
  
  using StreamingWaveDataPtr = std::shared_ptr<AudioEngine::StreamingWaveDataInstance>;
  using AudioController = Audio::CozmoAudioController;
  using DispatchQueue = Util::Dispatch::Queue;
  
  // returns true if done
  void DecodeAudioData(const StreamingWaveDataPtr& data);
  
  // once anyone calls AddData(), this will be called to launch a thread, if it doesnt already exist
  void PlayIncomingAudio();
  
  //void SavePCM( float* buff, size_t size = 0 );
  void SavePCM( short* buff, size_t size = 0 );
  
  const char* const StateToString() const;

  // audio controller provided by context
  AudioController* _audioController = nullptr;
  
  // worker thread
  DispatchQueue* _dispatchQueue = nullptr;
  
  enum class State : uint8_t {
    None=0,
    Preparing,
    Playable,
    Playing,
    ClearingUp,
  };
  
  // shared
  mutable std::mutex _mutex;
  mutable std::condition_variable _cv;
  State _state = State::None;
  bool _hasData;
  bool _first;
  StreamingWaveDataPtr _waveData;
  std::unique_ptr<AudioDataBuffer> _mp3Buffer;
  

};


} // end namespace Vector
} // end namespace Anki


#endif //__Anki_cozmo_cozmoAnim_RadioAudioComponent_H__
