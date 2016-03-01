/*
 * File: robotAudioAnimationOnRobot.h
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation On Robot is a sub-class of RobotAudioAnimation, it provides robot audio messages
 *              which are attached to Animation Frames. Each frame contains the synced audio data for that frame.
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Audio_RobotAudioAnimationOnRobot_H__
#define __Basestation_Audio_RobotAudioAnimationOnRobot_H__


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
class RobotAudioMessageStream;



class RobotAudioAnimationOnRobot : public RobotAudioAnimation {
  
public:
  
  // Default Constructor
  RobotAudioAnimationOnRobot( Animation* anAnimation, RobotAudioClient* audioClient );
  
  // Call at the beginning of the update loop to update the animation's state for the upcoming loop cycle
  virtual AnimationState Update( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms ) override;
  
  // Pop the front EngineToRobot audio message
  // Will set out_RobotAudioMessagePtr to Null if there are no messages for provided current time. Use this to identify
  // when to send a AudioSilence message.
  // Note: EngineToRobot pointer memory needs to be manage or it will leak memory.
  virtual void PopRobotAudioMessage( RobotInterface::EngineToRobot*& out_RobotAudioMessagePtr,
                                    TimeStamp_t startTime_ms,
                                    TimeStamp_t streamingTime_ms ) override;


protected:
  
  // Perform specific preparation to animation
  virtual void PrepareAnimation() override;
  
  // All the animations events have been completed and Audio Buffer is empty
  virtual bool IsAnimationDone() const override;
  
  
private:
  
  // Use in Update method to perform actions for the BufferLoading state
  void UpdateBufferLoading( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
  
  // Use in Update method to perform actions for the BufferReady state
  void UpdateBufferReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
  
  // Begin to load audio buffer with frames by scheduling all audio events to be posted in relevant time to each other
  void BeginBufferingAudioOnRobotMode();
  
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioAnimationOnRobot_H__ */
