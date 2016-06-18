//
//  robotAudioAnimationOnRobotTest.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 6/17/16.
//
//

#include "helpers/audio/robotAudioAnimationOnRobotTest.h"


namespace Anki {
namespace Cozmo {
namespace Audio {

RobotAudioAnimationOnRobotTest::RobotAudioAnimationOnRobotTest( Animation* anAnimation, RobotAudioClient* audioClient )
//: RobotAudioAnimationOnRobot( anAnimation, audioClient )
{
  InitAnimation( anAnimation, audioClient );
}

void RobotAudioAnimationOnRobotTest::BeginBufferingAudioOnRobotMode()
{
  // Don't post events to engine
  SetAnimationState( AnimationState::LoadingStream );
}

} // Audio
} // Cozmo
} // Anki