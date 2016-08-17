/*
 * File: animationControllerTypes.h
 *
 * Author: Jordan Rivas
 * Created: 07/17/16
 *
 * Description:
 *
 * #include "anki/cozmo/basestation/animations/animationControllerTypes.h"
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_AnimationControllerTypes_H__
#define __Basestation_Animations_AnimationControllerTypes_H__

#include <functional>
#include <cstdint>

namespace Anki {
namespace Cozmo {
namespace RobotAnimation {

#define K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL 2
static constexpr char KAnimationControllerLogChannel[] = "AnimationController";



} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki

#endif /* __Basestation_Animations_AnimationControllerTypes_H__ */
