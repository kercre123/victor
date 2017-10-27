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

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimOnNeedsChange.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayAnimOnNeedsChange::BehaviorPlayAnimOnNeedsChange(const Json::Value& config)
: BehaviorPlayAnimSequenceWithFace(config)
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
bool BehaviorPlayAnimOnNeedsChange::WantsToBeActivatedAnimSeqInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return ShouldGetInBePlayed(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimOnNeedsChange::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(ShouldGetInBePlayed(behaviorExternalInterface)){
    behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().SetSevereNeedExpression(_params._need);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimOnNeedsChange::ShouldGetInBePlayed(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const SevereNeedsComponent& severeNeedsComponent = behaviorExternalInterface.GetAIComponent().GetNonConstSevereNeedsComponent();
  const bool isSevereExpressed = (_params._need == severeNeedsComponent.GetSevereNeedExpression());
  
  bool shouldSevereNeedBeExpressed = false;
  if(behaviorExternalInterface.HasNeedsManager()){
    auto& needsManager = behaviorExternalInterface.GetNeedsManager();
    NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();
    shouldSevereNeedBeExpressed =
            currNeedState.IsNeedAtBracket(_params._need, NeedBracketId::Critical);
  }
  return !isSevereExpressed && shouldSevereNeedBeExpressed;
}


} // namespace Cozmo
} // namespace Anki
