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
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategies/iAIGoalStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalStrategyFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
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
static const char* kRequiresObjectTapped = "requireObjectTapped";
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersAIGoalArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersAIGoalArray),
              "Reaction triggers duplicate or non-sequential");
  
static const char* kObjectTapInteractionLock = "ObjectTapInteraction";

} // end namespace
  
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
  
  // log event to das
  int nSecs = Util::numeric_cast<int>(_lastTimeGoalStoppedSecs - _lastTimeGoalStartedSecs);
  if (nSecs < 0) { // Attempt to fix COZMO-7862
    PRINT_NAMED_ERROR("AIGoal.Exit.NegativeDuration",
                      "Negative duration (%i secs, started at %f and stopped at %f",
                      nSecs, _lastTimeGoalStartedSecs, _lastTimeGoalStoppedSecs);
    nSecs = 0;
  }
  Util::sEventF("robot.freeplay_goal_ended",
                {{DDATA, std::to_string(nSecs).c_str()}},
                "%s", _name.c_str());
  
  // If the goal requires a tapped object make sure to unset the tapped object when the goal exits
  if(_requireObjectTapped)
  {
    // Don't know which light animation was being played so stop both
    robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::DoubleTappedKnown);
    robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::DoubleTappedUnsure);
    
    
    robot.GetBehaviorManager().RemoveDisableReactionsLock(kObjectTapInteractionLock);
    
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


} // namespace Cozmo
} // namespace Anki
