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

#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorEventAnimResponseDirector.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/behaviorSystem/workoutComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::AIComponent(Robot& robot)
: _robot(robot)
, _aiInformationAnalyzer( new AIInformationAnalyzer() )
, _behaviorEventAnimResponseDirector(new BehaviorEventAnimResponseDirector())
, _behaviorHelperComponent( new BehaviorHelperComponent())
, _objectInteractionInfoCache(new ObjectInteractionInfoCache(robot))
, _whiteboard( new AIWhiteboard(robot) )
, _workoutComponent( new WorkoutComponent(robot) )
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
  
  _behaviorHelperComponent->Update(_robot);
   
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
