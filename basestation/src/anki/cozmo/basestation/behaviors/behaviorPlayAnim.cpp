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
static const char* kAnimGroupNameKey = "animGroupName";
static const char* kLoopsKey = "num_loops";

BehaviorPlayAnim::BehaviorPlayAnim(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _animationName()
{
  SetDefaultName("PlayAnim");
  
  if (!config.isNull())
  {
    const Json::Value& animNameJson = config[kAnimNameKey];
    const Json::Value& animGroupNameJson = config[kAnimGroupNameKey];

    if( animNameJson.isString() ) {
      _animationName = animNameJson.asCString();
    }
    if( animGroupNameJson.isString() ) {
      _animationGroupName = animGroupNameJson.asCString();
      _useGroup = true;
    }
      
    if( animNameJson.isString() && animGroupNameJson.isString() ) {
      PRINT_NAMED_WARNING("BehaviorPlayAnim.ConfigError",
                          "config for behavior '%s' specified keys '%s' and '%s', the group will be used",
                          GetName().c_str(),
                          kAnimNameKey,
                          kAnimGroupNameKey);
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
  if( _useGroup ) {
    StartActing(new PlayAnimationGroupAction(robot, _animationGroupName, _numLoops));
  }
  else {
    StartActing(new PlayAnimationAction(robot, _animationName, _numLoops));
  }
  
  return Result::RESULT_OK;
}  

} // namespace Cozmo
} // namespace Anki
