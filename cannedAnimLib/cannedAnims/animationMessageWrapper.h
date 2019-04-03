/**
 * File: animationMessageWrapper.h
 *
 * Authors: Kevin M. Karol
 * Created: 5/31/18
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef ANKI_COZMO_ANIMATION_MESSAGE_WRAPPER_H
#define ANKI_COZMO_ANIMATION_MESSAGE_WRAPPER_H

#include "clad/robotInterface/messageEngineToRobot.h"

#ifdef USES_CLAD_CPPLITE
#define CLAD(ns) CppLite::Anki::Vector::ns
#else
#define CLAD(ns) ns
#endif

#ifdef USES_CLAD_CPPLITE
namespace CppLite {
#endif
namespace Anki {
  namespace Vector {
    struct AnimationEvent;
  }
}
#ifdef USES_CLAD_CPPLITE
}
#endif

namespace Anki {

namespace Vision{
class ImageRGB565;
}

namespace Vector {

class RobotAudioKeyFrame;

struct AnimationMessageWrapper{
  AnimationMessageWrapper(Vision::ImageRGB565& img)
  : faceImg(img){}
  using ETR = CLAD(RobotInterface)::EngineToRobot;

  ETR* moveHeadMessage         = nullptr;
  ETR* moveLiftMessage         = nullptr;
  ETR* bodyMotionMessage       = nullptr;
  ETR* recHeadMessage          = nullptr;
  ETR* turnToRecHeadMessage    = nullptr;
  ETR* backpackLightsMessage   = nullptr;
  RobotAudioKeyFrame* audioKeyFrameMessage = nullptr;
  CLAD(AnimationEvent)* eventMessage = nullptr;

  bool haveFaceToSend = false;
  Vision::ImageRGB565& faceImg;
};


} // namespace Vector
} // namespace Anki

#undef CLAD

#endif // ANKI_COZMO_ANIMATION_MESSAGE_WRAPPER_H
