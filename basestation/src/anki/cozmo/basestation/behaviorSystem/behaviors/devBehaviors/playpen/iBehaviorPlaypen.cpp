/**
 * File: iBehaviorPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {
 
IBehaviorPlaypen::IBehaviorPlaypen(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  
}
  
}
}
