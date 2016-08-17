/*
 * File: animationControllerTypes_Internal.h
 *
 * Author: Jordan Rivas
 * Created: 07/17/16
 *
 * Description:
 *
 * #include "anki/cozmo/basestation/animations/animationControllerTypes_Internal.h"
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_AnimationControllerTypes_Internal_H__
#define __Basestation_Animations_AnimationControllerTypes_Internal_H__

#include "anki/cozmo/basestation/animations/animationControllerTypes.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include <list>

namespace Anki {
namespace Cozmo {
namespace RobotAnimation {
  
class StreamingAnimation;


using KeyframeList = std::list<IKeyFrame*>;

using EngineToRobotMessageList = std::list<const RobotInterface::EngineToRobot*>;



  
  
} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki


#endif /* __Basestation_Animations_AnimationControllerTypes_Internal_H__ */
