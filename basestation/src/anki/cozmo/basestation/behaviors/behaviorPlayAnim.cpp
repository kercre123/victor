/**
 * File: behaviorPlayAnim
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorPlayAnim.h"
#include "anki/cozmo/basestation/actions/animActions.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const char* kAnimNameKey = "animName";
static const char* kLoopsKey = "num_loops";

BehaviorPlayAnim::BehaviorPlayAnim(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _animationName()
{
  SetDefaultName("PlayAnim");
  
  if (!config.isNull())
  {
    {
      const Json::Value& valueJson = config[kAnimNameKey];
      if (valueJson.isString())
      {
        _animationName = valueJson.asCString();
      }
    }
  }

  _numLoops = config.get(kLoopsKey, 1).asInt();
}
    
BehaviorPlayAnim::~BehaviorPlayAnim()
{  
}
  
bool BehaviorPlayAnim::IsRunnable(const Robot& robot) const
{
  const bool retVal = true;
  return retVal;
}

Result BehaviorPlayAnim::InitInternal(Robot& robot)
{
  StartActing(new PlayAnimationAction(robot, _animationName, _numLoops));
  
  return Result::RESULT_OK;
}  

} // namespace Cozmo
} // namespace Anki
