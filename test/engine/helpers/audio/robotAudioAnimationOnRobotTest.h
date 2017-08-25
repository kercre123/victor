//
//  robotAudioAnimationOnRobotTest.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 6/17/16.
//
//

#ifndef robotAudioAnimationOnRobotTest_hpp
#define robotAudioAnimationOnRobotTest_hpp

#include "engine/audio/robotAudioAnimationOnRobot.h"

namespace Anki {
namespace Cozmo {
namespace Audio {

class RobotAudioAnimationOnRobotTest : public RobotAudioAnimationOnRobot {
  
public:
  
  // Default Constructor
  RobotAudioAnimationOnRobotTest( Animation* anAnimation, RobotAudioClient* audioClient );
  
  // Test helper
  size_t GetCurrentStreamFrameCount();
  
protected:
  
  // Begin to load audio buffer with frames by scheduling all audio events to be posted in relevant time to each other
  virtual void BeginBufferingAudioOnRobotMode() override;

};

} // Audio
} // Cozmo
} // Anki


#endif /* robotAudioAnimationOnRobotTest_hpp */
