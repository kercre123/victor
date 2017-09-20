/**
 * File: aiComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-12-07
 *
 * Description: Component for all aspects of the higher level AI
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/aiComponent.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorSystem/behaviorContainer.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorSystem/behaviorManager.h"
#include "engine/aiComponent/behaviorSystem/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/doATrickSelector.h"
#include "engine/aiComponent/feedingSoundEffectManager.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/workoutComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotStateHistory.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::AIComponent(Robot& robot)
: _robot(robot)
, _suddenObstacleDetected(false)
, _aiInformationAnalyzer( new AIInformationAnalyzer() )
, _behaviorEventAnimResponseDirector(new BehaviorEventAnimResponseDirector())
, _behaviorHelperComponent( new BehaviorHelperComponent())
, _behaviorContainer(new BehaviorContainer(robot.GetContext()->GetDataLoader()->GetBehaviorJsons()))
, _objectInteractionInfoCache(new ObjectInteractionInfoCache(robot))
, _whiteboard( new AIWhiteboard(robot) )
, _workoutComponent( new WorkoutComponent(robot) )
, _doATrickSelector(new DoATrickSelector(robot.GetContext()->GetDataLoader()->GetDoATrickWeightsConfig()))
, _feedingSoundEffectManager(new FeedingSoundEffectManager())
, _freeplayDataTracker( new FreeplayDataTracker() )
, _severeNeedsComponent( new SevereNeedsComponent(robot))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::~AIComponent()
{
  // This needs to happen before ActionList is destroyed, because otherwise behaviors will try to respond
  // to actions shutting down
  _behaviorMgr.reset();
  _behaviorSysMgr.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Init()
{
  const CozmoContext* context = _robot.GetContext();

  if(context == nullptr ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }

  _behaviorExternalInterface = std::make_unique<BehaviorExternalInterface>(_robot,
                                                                           _robot.GetAIComponent(),
                                                                           *_behaviorContainer,
                                                                           _robot.GetBlockWorld(),
                                                                           _robot.GetFaceWorld());
  
  _behaviorExternalInterface->SetOptionalInterfaces(&_robot.GetMoodManager(),
                                                    _robot.GetContext()->GetNeedsManager(),
                                                    &_robot.GetProgressionUnlockComponent(),
                                                    &_robot.GetPublicStateBroadcaster(),
                                                    _robot.HasExternalInterface() ? _robot.GetExternalInterface() : nullptr
                                                    );
  _behaviorMgr = std::make_unique<BehaviorManager>();
  _behaviorSysMgr = std::make_unique<BehaviorSystemManager>();
  
  _requestGameComponent = std::make_unique<RequestGameComponent>(*_behaviorExternalInterface,
                                                                 _robot.GetContext()->GetDataLoader()->GetGameRequestWeightsConfig());
  
  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();
  
  assert(_severeNeedsComponent);
  _severeNeedsComponent->Init();
  
  assert(_behaviorContainer);
  _behaviorContainer->Init(*_behaviorExternalInterface, !static_cast<bool>(USE_BSM));

  
  RobotDataLoader* dataLoader = nullptr;
  if(context){
    dataLoader = _robot.GetContext()->GetDataLoader();
  }
  
  // Configure behavior manager
  {
    Json::Value blankActivitiesConfig;
    
    const Json::Value& oldActivitesConfig = (dataLoader != nullptr) ?
             dataLoader->GetRobotActivitiesConfig() : blankActivitiesConfig;
    
    const Json::Value& reactionTriggerConfig = (dataLoader != nullptr) ?
             dataLoader->GetReactionTriggerMap() : blankActivitiesConfig;
    
    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
    dataLoader->GetBehaviorSystemConfig() : blankActivitiesConfig;
    
    _behaviorMgr->InitConfiguration(*_behaviorExternalInterface,
                                    oldActivitesConfig);
    _behaviorMgr->InitReactionTriggerMap(*_behaviorExternalInterface,
                                         reactionTriggerConfig);
    
    if(USE_BSM){
      _behaviorSysMgr->InitConfiguration(*_behaviorExternalInterface,
                                         behaviorSystemConfig);
    }
  }
  
  // initialize workout component
  if(dataLoader != nullptr){
    assert( _workoutComponent );
    const Json::Value& workoutConfig = dataLoader->GetRobotWorkoutConfig();

    const Result res = _workoutComponent->InitConfiguration(workoutConfig);
    if( res != RESULT_OK ) {
      PRINT_NAMED_ERROR("AIComponent.Init.FailedToInitWorkoutComponent",
                        "Couldn't init workout component, deleting");
      return res;      
    }
  }
  
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Update(Robot& robot, std::string& currentActivityName,
                                         std::string& behaviorDebugStr)
{
  // information analyzer should run before behaviors so that they can feed off its findings
  _aiInformationAnalyzer->Update(_robot);

  _whiteboard->Update();
  _severeNeedsComponent->Update();
  
  _behaviorHelperComponent->Update(*_behaviorExternalInterface);

  _freeplayDataTracker->Update();

  return UpdateBehaviorManager(robot, currentActivityName, behaviorDebugStr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::UpdateBehaviorManager(Robot& robot, std::string& currentActivityName,
                                                        std::string& behaviorDebugStr)
{
  // https://ankiinc.atlassian.net/browse/COZMO-1242 : moving too early causes pose offset
  static int ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 = 60;
  if(ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 <=0)
  {
    IBehaviorPtr currentBehavior;
    if(USE_BSM){
      _behaviorSysMgr->Update(*_behaviorExternalInterface);
      
      currentActivityName = "ACTIVITY NAME TBD";
      
      behaviorDebugStr = currentActivityName;
      
      currentBehavior = _behaviorSysMgr->GetCurrentBehavior();
    }else{
      Result res = _behaviorMgr->Update(*_behaviorExternalInterface);
      if(res == RESULT_OK){
        currentActivityName = _behaviorMgr->GetCurrentActivity()->GetIDStr();
      
        behaviorDebugStr = currentActivityName;
      
        currentBehavior = _behaviorMgr->GetCurrentBehavior();
      }
    }
    
    if(currentBehavior != nullptr) {
      behaviorDebugStr += " ";
      behaviorDebugStr +=  BehaviorIDToString(currentBehavior->GetID());
      const std::string& stateName = currentBehavior->GetDebugStateName();
      if (!stateName.empty())
      {
        behaviorDebugStr += "-" + stateName;
      }
    }
    
  } else {
    --ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242;
  }
  
  robot.GetContext()->GetVizManager()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::MAGENTA,
                                         "%s", behaviorDebugStr.c_str());
  
  robot.GetContext()->SetSdkStatus(SdkStatusType::Behavior,
                             std::string(currentActivityName) + std::string(":") + behaviorDebugStr);
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotDelocalized()
{
  GetWhiteboard().OnRobotDelocalized();
  _behaviorMgr->OnRobotDelocalized();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotRelocalized()
{
  GetWhiteboard().OnRobotRelocalized();
}


}
}
