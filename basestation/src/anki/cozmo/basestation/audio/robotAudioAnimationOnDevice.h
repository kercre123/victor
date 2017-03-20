/*
 * File: robotAudioAnimationOnDevice.h
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation On Device is a subclass of RobotAudioAnimation. It posts Audio events when the
 *              corresponding frame is being buffered. This is not synced to the animation due to the reality we don’t
 *              know when the robot is playing this frame, in the future we hope to fix this. It is intended to use when
 *              working with Webots as your simulated robot.
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __Basestation_Audio_RobotAudioAnimationOnDevice_H__
#define __Basestation_Audio_RobotAudioAnimationOnDevice_H__


#include "anki/cozmo/basestation/audio/robotAudioAnimation.h"


namespace Anki {
namespace Cozmo {

class Animation;

namespace RobotInterface {
class EngineToRobot;
}

namespace Audio {

class RobotAudioClient;
class RobotAudioBuffer;
class RobotAudioFrameStream;


class RobotAudioAnimationOnDevice : public RobotAudioAnimation {
  
public:

  // Default Constructor
  RobotAudioAnimationOnDevice( Animation* anAnimation,
                               RobotAudioClient* audioClient,
                               AudioMetaData::GameObjectType gameObject,
                               Util::RandomGenerator* randomGenerator );

  // Call at the beginning of the update loop to update the animation's state for the upcoming loop cycle
  virtual AnimationState Update( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms ) override;

  // Perform audio events in current frame
  // Will set out_RobotAudioMessagePtr to Null to identify steamer to send a AudioSilence message.
  virtual void PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                     TimeStamp_t startTime_ms,
                                     TimeStamp_t streamingTime_ms ) override;
  
protected:
  
  // Perform specific preparation to animation
  virtual void PrepareAnimation() override;
  
  // All the animations events have and completed
  virtual bool IsAnimationDone() const override;
  
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioAnimationOnDevice_H__ */
