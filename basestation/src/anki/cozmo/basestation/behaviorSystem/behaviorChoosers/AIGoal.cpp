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

#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategyFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategies/iAIGoalStrategy.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

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
: _priority(0)
, _driveStartAnimTrigger(AnimationTrigger::Count)
, _driveLoopAnimTrigger(AnimationTrigger::Count)
, _driveEndAnimTrigger(AnimationTrigger::Count)
, _requiredSpark(UnlockId::Count)
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
  
  // driving animation triggers
  std::string animTriggerStr;
  if ( JsonTools::GetValueOptional(config, "driveStartAnimTrigger", animTriggerStr) ) {
    _driveStartAnimTrigger = animTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerStr.c_str());
  }
  if ( JsonTools::GetValueOptional(config, "driveLoopAnimTrigger", animTriggerStr) ) {
    _driveLoopAnimTrigger = animTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerStr.c_str());
  }
  if ( JsonTools::GetValueOptional(config, "driveEndAnimTrigger", animTriggerStr) ) {
    _driveEndAnimTrigger = animTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerStr.c_str());
  }
  // check that triggers are all or nothing
  const bool hasAnyDrivingAnim = (_driveStartAnimTrigger != AnimationTrigger::Count) ||
                                 (_driveLoopAnimTrigger  != AnimationTrigger::Count) ||
                                 (_driveEndAnimTrigger   != AnimationTrigger::Count);
  const bool hasAllDrivingAnim = (_driveStartAnimTrigger != AnimationTrigger::Count) &&
                                 (_driveLoopAnimTrigger  != AnimationTrigger::Count) &&
                                 (_driveEndAnimTrigger   != AnimationTrigger::Count);
  ASSERT_NAMED(hasAllDrivingAnim || !hasAnyDrivingAnim, "AIGoal.Init.InvalidDrivingAnimTriggers_AllOrNothing");
  
  // information analyzer process
  std::string inanProcessStr;
  JsonTools::GetValueOptional(config, "infoAnalyzerProccess", inanProcessStr);
  _infoAnalysisProcess = inanProcessStr.empty() ?
    AIInformationAnalysis::EProcess::Invalid :
    AIInformationAnalysis::EProcessFromString(inanProcessStr.c_str());
  
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
void AIGoal::Enter(Robot& robot)
{
  _lastTimeGoalStartedSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _behaviorChooserPtr->OnSelected();
  
  // Update Ai Goal state to change music theme
  // Note: This only applies to freeplay goals, spark goals do nothing
  robot.GetRobotAudioClient()->UpdateAiGoalMusicState(_name);
  
  // set driving animations for this goal if specified in config
  const bool hasDrivingAnims = HasDrivingAnimTriggers();
  if ( hasDrivingAnims ) {
    robot.GetDrivingAnimationHandler().PushDrivingAnimations( {_driveStartAnimTrigger,
                                                               _driveLoopAnimTrigger,
                                                               _driveEndAnimTrigger} );
  }
  
  // request analyzer process
  if ( _infoAnalysisProcess != AIInformationAnalysis::EProcess::Invalid ) {
    robot.GetAIInformationAnalyzer().AddEnableRequest(_infoAnalysisProcess, GetName());
  }
  
  // log event to das
  Util::sEventF("robot.freeplay_goal_started", {}, "%s", _name.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoal::Exit(Robot& robot)
{
  _lastTimeGoalStoppedSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _behaviorChooserPtr->OnDeselected();
  
  // clear driving animations for this goal if specified in config
  const bool hasDrivingAnims = HasDrivingAnimTriggers();
  if ( hasDrivingAnims ) {
    robot.GetDrivingAnimationHandler().PopDrivingAnimations();
  }
  
  // (un)request analyzer process
  if ( _infoAnalysisProcess != AIInformationAnalysis::EProcess::Invalid ) {
    robot.GetAIInformationAnalyzer().RemoveEnableRequest(_infoAnalysisProcess, GetName());
  }

  // log event to das
  Util::sEventF("robot.freeplay_goal_ended", {{DDATA, TO_DDATA_STR(static_cast<int>(_lastTimeGoalStoppedSecs - _lastTimeGoalStartedSecs))}}, "%s", _name.c_str());
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
