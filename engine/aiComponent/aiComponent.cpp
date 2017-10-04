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
#include "engine/aiComponent/doATrickSelector.h"
#include "engine/aiComponent/feedingSoundEffectManager.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/workoutComponent.h"
#include "engine/cozmoContext.h"
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
, _objectInteractionInfoCache(new ObjectInteractionInfoCache(robot))
, _whiteboard( new AIWhiteboard(robot) )
, _workoutComponent( new WorkoutComponent(robot) )
, _requestGameComponent(new RequestGameComponent(robot))
, _doATrickSelector(new DoATrickSelector(robot))
, _feedingSoundEffectManager(new FeedingSoundEffectManager())
, _freeplayDataTracker( new FreeplayDataTracker() )
, _severeNeedsComponent( new SevereNeedsComponent(robot))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::~AIComponent()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Init()
{
  const CozmoContext* context = _robot.GetContext();

  if( !context ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }

  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();
  
  assert(_severeNeedsComponent);
  _severeNeedsComponent->Init();

  // initialize workout component
  if( context) {
    assert( _workoutComponent );
    const Json::Value& workoutConfig = context->GetDataLoader()->GetRobotWorkoutConfig();

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
Result AIComponent::Update()
{
  // information analyzer should run before behaviors so that they can feed off its findings
  _aiInformationAnalyzer->Update(_robot);

  _whiteboard->Update();
  _severeNeedsComponent->Update();
  
  _behaviorHelperComponent->Update(_robot);

  _freeplayDataTracker->Update();
   
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotDelocalized()
{
  GetWhiteboard().OnRobotDelocalized();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotRelocalized()
{
  GetWhiteboard().OnRobotRelocalized();
}


}
}
