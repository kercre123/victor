/**
* File: behaviorPlayAnimOnNeedsChange.cpp
*
* Author: Kevin M. Karol
* Created: 6/30/17
*
* Description: Behavior which plays an animation and transitions
* cozmo into expressing needs state
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimOnNeedsChange.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kNeedIDKey = "need";

}

BehaviorPlayAnimOnNeedsChange::BehaviorPlayAnimOnNeedsChange(Robot& robot, const Json::Value& config)
: BehaviorPlayAnimSequenceWithFace(robot, config)
{
  _params._need = NeedIdFromString(
        JsonTools::ParseString(config,
                               kNeedIDKey,
                               "BehaviorPlayAnimOnNeedsChange.ConfigError.SevereNeedExpressed"));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayAnimOnNeedsChange::~BehaviorPlayAnimOnNeedsChange()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimOnNeedsChange::IsRunnableAnimSeqInternal(const Robot& robot) const
{
  return ShouldGetInBePlayed(robot);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimOnNeedsChange::StopInternal(Robot& robot)
{
  if(ShouldGetInBePlayed(robot)){
    robot.GetAIComponent().GetSevereNeedsComponent().SetSevereNeedExpression(_params._need);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimOnNeedsChange::ShouldGetInBePlayed(const Robot& robot) const
{
  const SevereNeedsComponent& severeNeedsComponent = robot.GetAIComponent().GetNonConstSevereNeedsComponent();
  const bool isSevereExpressed = (_params._need == severeNeedsComponent.GetSevereNeedExpression());
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool shouldSevereNeedBeExpressed =
          currNeedState.IsNeedAtBracket(_params._need, NeedBracketId::Critical);
  
  return !isSevereExpressed && shouldSevereNeedBeExpressed;
}


} // namespace Cozmo
} // namespace Anki
