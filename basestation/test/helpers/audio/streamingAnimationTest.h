/**
 * File: streamingAnimationTest.h
 *
 * Author: Jordan Rivas
 * Created: 10/20/16
 *
 * Description: Extend Streaming Animation for Unit Test
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __Test_Helpers_Audio_StreamingAnimationTest_H__
#define __Test_Helpers_Audio_StreamingAnimationTest_H__

#include "anki/cozmo/basestation/animations/streamingAnimation.h"


namespace Anki {
namespace Cozmo {
namespace RobotAnimation {

class StreamingAnimationTest : public StreamingAnimation
{
public:
  
  StreamingAnimationTest(const std::string& name,                   // Animation Name
                         AudioMetaData::GameObjectType gameObject,  // Game Object to buffer audio with
                         Audio::RobotAudioBuffer* audioBuffer,      // Corresponding buffer for Game Object
                         Audio::RobotAudioClient& audioClient);     // Audio Client ref to perform audio work
  
  
  Audio::RobotAudioBuffer* GetAudioBuffer() const { return _audioBuffer; }
  void GenerateAudioEventList_TEST() { GenerateAudioEventList(); }
  void IncrementCompletedEventCount_TEST() { IncrementCompletedEventCount(); }
  
};
  
}
}
}


#endif /* __Test_Helpers_Audio_StreamingAnimationTest_H__ */
