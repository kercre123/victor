/**
 * File: behaviorLookForFaces.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "LookForFaces" behavior, which searches for people's
 *              faces and tracks/interacts with them if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"

namespace Anki {
namespace Cozmo {

  LookForFacesBehavior::LookForFacesBehavior(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    
  }
  
} // namespace Cozmo
} // namespace Anki