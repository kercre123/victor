/***********************************************************************************************************************
 *
 *  AudioPlaybackSystem
 *  Victor / Anim
 *
 *  Created by Jarrod Hatfield on 4/10/2018
 *
 *  Description
 *  + System to load and playback recordings/audio files/etc
 *
 **********************************************************************************************************************/

#ifndef __AnimProcess_CozmoAnim_AudioPlaybackSystem_H__
#define __AnimProcess_CozmoAnim_AudioPlaybackSystem_H__

#include "audioUtil/audioDataTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "coretech/common/shared/types.h"

#include <queue>


namespace Anki {
namespace Cozmo {
  class AnimContext;
}
}

namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioPlaybackJob;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AudioPlaybackSystem
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  AudioPlaybackSystem( const AnimContext* context );
  ~AudioPlaybackSystem();

  AudioPlaybackSystem() = delete;
  AudioPlaybackSystem( const AudioPlaybackSystem& other ) = delete;
  AudioPlaybackSystem& operator=( const AudioPlaybackSystem& other ) = delete;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  void Update( BaseStationTime_t currTime_nanosec );
  void PlaybackAudio( const std::string& path );


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool IsValidFile( const std::string& path ) const;

  void StartNextJobInQueue();
  static void StartAudioPlaybackJob( std::shared_ptr<AudioPlaybackJob> audiojob );


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data

  const AnimContext*    _animContext;

  std::shared_ptr<AudioPlaybackJob> _currentJob;
  std::queue<AudioPlaybackJob*> _jobQueue;
};

} //  namespace Audio
} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_AudioPlaybackSystem_H__
