/**
 * File: AIGoal
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: High level goal: a group of behaviors that make sense to run together towards a common objective
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoal.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategyFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategies/iAIGoalStrategy.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kBehaviorChooserConfigKey = "behaviorChooser";
static const char* kStrategyConfigKey = "goalStrategy";
static const char* kRequiresSparkKey = "requireSpark";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoal::AIGoal()
: _requiredSpark(UnlockId::Count)
, _lastTimeGoalStartedSecs(-1.0f)
, _lastTimeGoalStoppedSecs(-1.0f)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoal::~AIGoal()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoal::Init(Robot& robot, const Json::Value& config)
{
  // read info from config

  // - - - - - - - - - -
  // Needs Sparks
  // - - - - - - - - - -
  
  std::string sparkString;
  if( JsonTools::GetValueOptional(config,kRequiresSparkKey,sparkString) )
  {
    _requiredSpark = UnlockIdsFromString(sparkString.c_str());
  }
  
  // configure chooser and set in out pointer
  const Json::Value& chooserConfig = config[kBehaviorChooserConfigKey];
  IBehaviorChooser* newChooser = BehaviorChooserFactory::CreateBehaviorChooser(robot, chooserConfig);
  _behaviorChooserPtr.reset( newChooser );
  
  // strategy
  const Json::Value& strategyConfig = config[kStrategyConfigKey];
  IAIGoalStrategy* newStrategy = AIGoalStrategyFactory::CreateAIGoalStrategy(robot, strategyConfig);
  _strategy.reset( newStrategy );
  
  // other params
  _priority = JsonTools::ParseUint8(config, "priority", "AIGoal.Init");
  _name = JsonTools::ParseString(config, "name", "AIGoal.Init");

  const bool success = (nullptr != _behaviorChooserPtr);
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoal::Enter()
{
  _lastTimeGoalStartedSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoal::Exit()
{
  _lastTimeGoalStoppedSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* AIGoal::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  // at the moment delegate on chooser. At some point we'll have intro/outro and other reactions
  // note we pass
  IBehavior* ret = _behaviorChooserPtr->ChooseNextBehavior(robot, currentRunningBehavior);
  return ret;
}


} // namespace Cozmo
} // namespace Anki
