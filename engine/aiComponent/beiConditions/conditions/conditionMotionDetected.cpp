/**
* File: conditionMotionDetected.h
*
* Author: Kevin M. Karol
* Created: 1/23/18
*
* Description: Condition which is true when motion is detected
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"


namespace Anki {
namespace Cozmo {

namespace{
const char* kMotionAreaKey = "motionArea";
const char* kMotionLevelKey = "motionLevel";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionMotionDetected::ConditionMotionDetected(const Json::Value& config)
: IBEICondition(config)
{
  const auto& areaStr = JsonTools::ParseString(config,
                                               kMotionAreaKey,
                                               "ConditionMotionDetected.ConfigError.MotionArea");
  _instanceParams.motionArea = AreaStringToEnum(areaStr);
  const auto& levelStr = JsonTools::ParseString(config,
                                                kMotionLevelKey,
                                                "ConditionMotionDetected.ConfigError.MotionLevel");
  _instanceParams.motionLevel = LevelStringToEnum(levelStr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionMotionDetected::~ConditionMotionDetected()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionMotionDetected::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _instanceParams._messageHelper.reset(new BEIConditionMessageHelper(this, behaviorExternalInterface));
  
  _instanceParams._messageHelper->SubscribeToTags({
    EngineToGameTag::RobotObservedMotion
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionMotionDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // within last tick
  return _lifetimeParams.tickCountMotionObserved + 1 >= BaseStationTimer::getInstance()->GetTickCount();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionMotionDetected::HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  using namespace ExternalInterface;
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // don't update the pounce location while we are active but go back.
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      bool observedMotion = false;
      switch(_instanceParams.motionArea){
        case MotionArea::Left:  { observedMotion = EvaluateLeft(motionObserved);   break;}
        case MotionArea::Right: { observedMotion = EvaluateRight(motionObserved);  break;}
        case MotionArea::Top:   { observedMotion = EvaluateTop(motionObserved);    break;}
        case MotionArea::Ground:{ observedMotion = EvaluateGround(motionObserved); break;}
      }
      if(observedMotion){
        _lifetimeParams.tickCountMotionObserved = BaseStationTimer::getInstance()->GetTickCount();
      }

      break;
    }
    default: {
      PRINT_NAMED_ERROR("ConditionMotionDetected.HandleEvent.InvalidEvent", "");
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionMotionDetected::EvaluateLeft(const RobotObservedMotion& motionObserved)
{
  _lifetimeParams.detectedMotionLevel = motionObserved.left_img_area;
  switch(_instanceParams.motionLevel){
    case MotionLevel::High:{ return motionObserved.left_img_area > 0.4f;}
    case MotionLevel::Low: { return motionObserved.left_img_area > 0.2f;}
    case MotionLevel::Any: { return motionObserved.left_img_area > 0.0f;}
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionMotionDetected::EvaluateRight(const RobotObservedMotion& motionObserved)
{
  _lifetimeParams.detectedMotionLevel = motionObserved.right_img_area;
  switch(_instanceParams.motionLevel){
    case MotionLevel::High: { return motionObserved.right_img_area > 0.4f;}
    case MotionLevel::Low:  { return motionObserved.right_img_area > 0.2f;}
    case MotionLevel::Any:  { return motionObserved.right_img_area > 0.0f;}
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionMotionDetected::EvaluateTop(const RobotObservedMotion& motionObserved)
{
  _lifetimeParams.detectedMotionLevel = motionObserved.top_img_area;
  switch(_instanceParams.motionLevel){
    case MotionLevel::High: { return motionObserved.top_img_area > 0.4f;}
    case MotionLevel::Low:  { return motionObserved.top_img_area > 0.2f;}
    case MotionLevel::Any:  { return motionObserved.top_img_area > 0.0f;}
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionMotionDetected::EvaluateGround(const RobotObservedMotion& motionObserved)
{
  _lifetimeParams.detectedMotionLevel = motionObserved.ground_area;
  switch(_instanceParams.motionLevel){
    case MotionLevel::High: { return motionObserved.ground_area > 0.4f;}
    case MotionLevel::Low:  { return motionObserved.ground_area > 0.2f;}
    case MotionLevel::Any:  { return motionObserved.ground_area > 0.0f;}
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionMotionDetected::MotionArea ConditionMotionDetected::AreaStringToEnum(const std::string& areaStr)
{
  if(areaStr == "Left"){
    return MotionArea::Left;
  }else if(areaStr == "Right"){
    return MotionArea::Right;
  }else if(areaStr == "Top"){
    return MotionArea::Top;
  }else if(areaStr == "Ground"){
    return MotionArea::Ground;
  }else{
    DEV_ASSERT(false, "ConditionMotionDetected.AreaStringToEnum.InvalidAreaStr");
    return MotionArea::Ground;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionMotionDetected::MotionLevel ConditionMotionDetected::LevelStringToEnum(const std::string& levelStr)
{
  if(levelStr == "High"){
    return MotionLevel::High;
  }else if(levelStr == "Low"){
    return MotionLevel::Low;
  }else if(levelStr == "Any"){
    return MotionLevel::Any;
  }else{
    DEV_ASSERT(false, "ConditionMotionDetected.LevelStringToEnum.InvalidAreaStr");
    return MotionLevel::Low;
  }
}


} // namespace
} // namespace
