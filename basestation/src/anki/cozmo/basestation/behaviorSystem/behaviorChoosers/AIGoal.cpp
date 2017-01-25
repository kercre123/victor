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
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalPersistentUpdates/buildPyramidPersistentUpdate.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategies/iAIGoalStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategyFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
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
static const char* kPersistentUpdateComponentKey = "persistentComponentID";
static const char* kPyramidSparkUpdateFuncID = "pyramidSparkUpdate";
static const char* kRequiresObjectTapped = "requireObjectTapped";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoal::AIGoal()
: _persistentUpdateComponent(nullptr)
, _priority(0)
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
  Util::SafeDelete(_persistentUpdateComponent);
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

  #if (DEV_ASSERT_ENABLED)
  {
    // check that triggers are all or nothing
    const bool hasAnyDrivingAnim = (_driveStartAnimTrigger != AnimationTrigger::Count) ||
                                 (_driveLoopAnimTrigger  != AnimationTrigger::Count) ||
                                 (_driveEndAnimTrigger   != AnimationTrigger::Count);
    const bool hasAllDrivingAnim = (_driveStartAnimTrigger != AnimationTrigger::Count) &&
                                 (_driveLoopAnimTrigger  != AnimationTrigger::Count) &&
                                 (_driveEndAnimTrigger   != AnimationTrigger::Count);
    DEV_ASSERT(hasAllDrivingAnim || !hasAnyDrivingAnim, "AIGoal.Init.InvalidDrivingAnimTriggers_AllOrNothing");
  }
  #endif
  
  // information analyzer process
  std::string inanProcessStr;
  JsonTools::GetValueOptional(config, "infoAnalyzerProcess", inanProcessStr);
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
  std::string updateFuncID;
  if ( JsonTools::GetValueOptional(config, kPersistentUpdateComponentKey, updateFuncID) ) {
    _persistentUpdateComponent = PersistentUpdateComponentFactory(updateFuncID);
    _persistentUpdateComponent->Init(robot);
  }
  _priority = JsonTools::ParseUint8(config, "priority", "AIGoal.Init");
  _name = JsonTools::ParseString(config, "name", "AIGoal.Init");
  
  JsonTools::GetValueOptional(config, kRequiresObjectTapped, _requireObjectTapped);

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
    robot.GetAIComponent().GetAIInformationAnalyzer().AddEnableRequest(_infoAnalysisProcess, GetName());
  }
  
  // notify the persistant update function that the goal was entered
  if(_persistentUpdateComponent != nullptr){
    _persistentUpdateComponent->GoalEntered(robot);
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
    robot.GetAIComponent().GetAIInformationAnalyzer().RemoveEnableRequest(_infoAnalysisProcess, GetName());
  }

  // notify the persistant update function that the goal is exiting
  if(_persistentUpdateComponent != nullptr){
    _persistentUpdateComponent->GoalExited(robot);
  }
  
  // log event to das
  const int nSecs = static_cast<int>(_lastTimeGoalStoppedSecs - _lastTimeGoalStartedSecs);
  Util::sEventF("robot.freeplay_goal_ended",
                {{DDATA, std::to_string(nSecs).c_str()}},
                "%s", _name.c_str());
  
  // If the goal requires a tapped object make sure to unset the tapped object when the goal exits
  if(_requireObjectTapped)
  {
    // Don't know which light animation was being played so stop both
    robot.GetCubeLightComponent().StopLightAnim(CubeAnimationTrigger::DoubleTappedKnown);
    robot.GetCubeLightComponent().StopLightAnim(CubeAnimationTrigger::DoubleTappedUnsure);
    
    robot.GetBehaviorManager().RequestEnableReactionTrigger("ObjectTapInteraction", ReactionTrigger::CubeMoved, true);
    
    robot.GetBehaviorManager().LeaveObjectTapInteraction();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIGoal::Update(Robot& robot)
{
  auto result = Result::RESULT_OK;
  if(_behaviorChooserPtr != nullptr){
    result = _behaviorChooserPtr->Update(robot);
  }
  
  if(_persistentUpdateComponent != nullptr){
    _persistentUpdateComponent->Update(robot);
  }
  
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* AIGoal::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  // at the moment delegate on chooser. At some point we'll have intro/outro and other reactions
  // note we pass
  IBehavior* ret = _behaviorChooserPtr->ChooseNextBehavior(robot, currentRunningBehavior);
  return ret;
}
  
IGoalPersistentUpdate* AIGoal::PersistentUpdateComponentFactory(const std::string& updateFuncID)
{
  if(updateFuncID == kPyramidSparkUpdateFuncID){
    return new BuildPyramidPersistentUpdate;
  }
  
  return nullptr;
}



} // namespace Cozmo
} // namespace Anki
