/**
 * File: aiComponent.h
 *
 * Author: Brad Neuman
 * Created: 2016-12-07
 *
 * Description: Component for all aspects of the higher level AI
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_AiComponent_H__
#define __Cozmo_Basestation_Behaviors_AiComponent_H__

#include "anki/common/types.h"
#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <string>

namespace Anki {
namespace Cozmo {

// Forward declarations
class AIInformationAnalyzer;
class AIWhiteboard;
class BehaviorHelperComponent;
class Robot;
class WorkoutComponent;

class AIComponent : private Util::noncopyable
{
public:
  
  explicit AIComponent(Robot& robot);
  ~AIComponent();

  ////////////////////////////////////////////////////////////////////////////////
  // Components
  ////////////////////////////////////////////////////////////////////////////////

  inline const AIWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard& GetNonConstWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard&       GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
  
  inline const WorkoutComponent& GetWorkoutComponent() const { assert(_workoutComponent); return *_workoutComponent; }
  inline WorkoutComponent&       GetWorkoutComponent()       { assert(_workoutComponent); return *_workoutComponent; }

  inline const AIInformationAnalyzer& GetAIInformationAnalyzer() const {
    assert(_aiInformationAnalyzer);
    return *_aiInformationAnalyzer;
  }
  inline AIInformationAnalyzer& GetAIInformationAnalyzer() {
    assert(_aiInformationAnalyzer);
    return *_aiInformationAnalyzer;
  }
  
  inline const BehaviorHelperComponent& GetBehaviorHelperComponent() const { assert(_behaviorHelperComponent); return *_behaviorHelperComponent; }
  inline BehaviorHelperComponent&       GetBehaviorHelperComponent()       { assert(_behaviorHelperComponent); return *_behaviorHelperComponent; }
  

  ////////////////////////////////////////////////////////////////////////////////
  // Update and init
  ////////////////////////////////////////////////////////////////////////////////

  Result Init();
  Result Update();

  ////////////////////////////////////////////////////////////////////////////////
  // Message handling / dispatch
  ////////////////////////////////////////////////////////////////////////////////

  void OnRobotDelocalized();
  void OnRobotRelocalized();

private:

  Robot& _robot;
  
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  std::unique_ptr<AIWhiteboard>            _whiteboard;

  // component for tracking cozmo's work-out behaviors
  std::unique_ptr< WorkoutComponent >      _workoutComponent;

  // module to analyze information for the AI in processes common to more than one behavior, for example
  // border calculation
  std::unique_ptr<AIInformationAnalyzer>   _aiInformationAnalyzer;
  
  // component which behaviors can delegate to for automatic action error handling
  std::unique_ptr<BehaviorHelperComponent> _behaviorHelperComponent;
};

}
}

#endif
