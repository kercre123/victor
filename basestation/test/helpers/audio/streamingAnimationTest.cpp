/**
 * File: streamingAnimationTest.cpp
 *
 * Author: Jordan Rivas
 * Created: 10/20/16
 *
 * Description: Extend Streaming Animation for Unit Test
 *
 * Copyright: Anki, Inc. 2016
 */

#include "helpers/audio/streamingAnimationTest.h"


namespace Anki {
namespace Cozmo {
namespace RobotAnimation {

StreamingAnimationTest::StreamingAnimationTest(const std::string& name,
                                               AudioMetaData::GameObjectType gameObject,
                                               Audio::RobotAudioBuffer* audioBuffer,
                                               Audio::RobotAudioClient& audioClient)
: Anki::Cozmo::RobotAnimation::StreamingAnimation(name, gameObject, audioBuffer, audioClient)
{
}

}
}
}
