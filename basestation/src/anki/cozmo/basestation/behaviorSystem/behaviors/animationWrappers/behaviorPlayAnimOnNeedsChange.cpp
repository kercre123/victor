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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimOnNeedsChange.h"

#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"

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
bool BehaviorPlayAnimOnNeedsChange::IsRunnableAnimSeqInternal(const BehaviorPreReqRobot& preReqData) const
{
  return ShouldGetInBePlayed(preReqData.GetRobot());
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayAnimOnNeedsChange::StopInternal(Robot& robot)
{
  if(ShouldGetInBePlayed(robot)){
    robot.GetAIComponent().GetWhiteboard().SetSevereNeedExpression(_params._need);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayAnimOnNeedsChange::ShouldGetInBePlayed(const Robot& robot) const
{
  const AIWhiteboard& whiteboard = robot.GetAIComponent().GetWhiteboard();
  const bool isSevereExpressed = (_params._need == whiteboard.GetSevereNeedExpression());
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool shouldSevereNeedBeExpressed =
          currNeedState.IsNeedAtBracket(_params._need, NeedBracketId::Critical);
  
  return !isSevereExpressed && shouldSevereNeedBeExpressed;
}


} // namespace Cozmo
} // namespace Anki
