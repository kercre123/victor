/*
 * File: robotAudioAnimationOnRobot.h
 *
 * Author: Jordan Rivas
 * Created: 02/29/16
 *
 * Description: Robot Audio Animation On Robot is a subclass of RobotAudioAnimation. It provides robot audio messages
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
class RobotAudioFrameStream;



class RobotAudioAnimationOnRobot : public RobotAudioAnimation {
  
public:
  
  // Default Constructor
  RobotAudioAnimationOnRobot( Animation* anAnimation,
                              RobotAudioClient* audioClient,
                              AudioMetaData::GameObjectType gameObject,
                              Util::RandomGenerator* randomGenerator );
  
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
  
  // For Test
  RobotAudioAnimationOnRobot();
  
  // Perform specific preparation to animation
  virtual void PrepareAnimation() override;
  
  // Begin to load audio buffer with frames by scheduling all audio events to be posted in relevant time to each other
  virtual void BeginBufferingAudioOnRobotMode();
  
  
private:
  
  // Use in Update method to perform actions for the current state
  void UpdateLoadingStream( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
  void UpdateLoadingStreamFrames( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
  void UpdateAudioFramesReady( TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
  
  // Track the time the first stream was created and audio event to calculate the streams relevant animation time
  double _streamAnimationOffsetTime_ms = 0.0;
  
  // Track if the first audio stream has
  bool _didPlayFirstStream = false;
  
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioAnimationOnRobot_H__ */
