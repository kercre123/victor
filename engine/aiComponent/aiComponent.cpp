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
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
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
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotStateHistory.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::AIComponent()
: _suddenObstacleDetected(false)
, _aiInformationAnalyzer(new AIInformationAnalyzer() )
, _freeplayDataTracker(new FreeplayDataTracker() )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::~AIComponent()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Init(Robot& robot, BehaviorComponent*& customBehaviorComponent)
{
  {
    _aiComponents.reset(new ComponentWrappers::AIComponentComponents(robot));
    _objectInteractionInfoCache.reset(new ObjectInteractionInfoCache(robot));
    _whiteboard.reset(new AIWhiteboard(robot) );
    _workoutComponent.reset(new WorkoutComponent(robot) );
    _doATrickSelector.reset(new DoATrickSelector(robot.GetContext()->GetDataLoader()->GetDoATrickWeightsConfig()));
    _severeNeedsComponent.reset(new SevereNeedsComponent(robot));
  }
  
  
  const CozmoContext* context = robot.GetContext();

  if(context == nullptr ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }
  
  if(customBehaviorComponent != nullptr) {
    _behaviorComponent.reset(customBehaviorComponent);
    customBehaviorComponent = nullptr;
  }else{
    _behaviorComponent = std::make_unique<BehaviorComponent>();
    _behaviorComponent->Init(BehaviorComponent::GenerateComponents(robot));
  }
  
  if(USE_BSM){
    // Toggle flag to "start" the tracking process - legacy assumption that freeeplay is not
    // active on app start
    _freeplayDataTracker->SetFreeplayPauseFlag(true, FreeplayPauseFlag::OffTreads);
    _freeplayDataTracker->SetFreeplayPauseFlag(false, FreeplayPauseFlag::OffTreads);
  }
  
  _requestGameComponent = std::make_unique<RequestGameComponent>(robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                                                                 robot.GetContext()->GetDataLoader()->GetGameRequestWeightsConfig());
  
  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();
  
  assert(_severeNeedsComponent);
  _severeNeedsComponent->Init();
  
  RobotDataLoader* dataLoader = nullptr;
  if(context){
    dataLoader = robot.GetContext()->GetDataLoader();
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
  _aiInformationAnalyzer->Update(robot);

  _whiteboard->Update();
  _severeNeedsComponent->Update();
  
  _behaviorComponent->Update(robot, currentActivityName, behaviorDebugStr);

  _freeplayDataTracker->Update();

  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotDelocalized()
{
  GetWhiteboard().OnRobotDelocalized();
  _behaviorComponent->OnRobotDelocalized();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotRelocalized()
{
  GetWhiteboard().OnRobotRelocalized();
}

// Support legacy code until move helper comp into delegate component
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BehaviorHelperComponent& AIComponent::GetBehaviorHelperComponent() const
{
  assert(_behaviorComponent);
  return _behaviorComponent->GetBehaviorHelperComponent();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperComponent& AIComponent::GetBehaviorHelperComponent()
{
  assert(_behaviorComponent);
  return _behaviorComponent->GetBehaviorHelperComponent();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& AIComponent::GetBehaviorContainer() {
  return _behaviorComponent->GetBehaviorContainer();
}


} // namespace Cozmo
} // namespace Anki
