//
//  robotAudioAnimationOnRobotTest.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 6/17/16.
//
//

#include "engine/audio/robotAudioFrameStream.h"
#include "helpers/audio/robotAudioAnimationOnRobotTest.h"


namespace Anki {
namespace Cozmo {
namespace Audio {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioAnimationOnRobotTest::RobotAudioAnimationOnRobotTest( Animation* anAnimation, RobotAudioClient* audioClient )
{
  InitAnimation( anAnimation, audioClient );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t RobotAudioAnimationOnRobotTest::GetCurrentStreamFrameCount()
{
  if ( nullptr != _currentBufferStream ) {
    return _currentBufferStream->AudioFrameCount();
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioAnimationOnRobotTest::BeginBufferingAudioOnRobotMode()
{
  // Don't post events to engine
  SetAnimationState( AnimationState::LoadingStream );
}


} // Audio
} // Cozmo
} // Anki
