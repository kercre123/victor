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
class BehaviorEventAnimResponseDirector;
class BehaviorHelperComponent;
class ObjectInteractionInfoCache;
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


  inline const AIInformationAnalyzer& GetAIInformationAnalyzer() const {
    assert(_aiInformationAnalyzer);
    return *_aiInformationAnalyzer;
  }
  inline AIInformationAnalyzer& GetAIInformationAnalyzer() {
    assert(_aiInformationAnalyzer);
    return *_aiInformationAnalyzer;
  }
  
  inline const BehaviorEventAnimResponseDirector& GetBehaviorEventAnimResponseDirector() const
    { assert(_behaviorEventAnimResponseDirector); return *_behaviorEventAnimResponseDirector; }
  inline BehaviorEventAnimResponseDirector&       GetBehaviorEventAnimResponseDirector()
    { assert(_behaviorEventAnimResponseDirector); return *_behaviorEventAnimResponseDirector; }
  
  inline const BehaviorHelperComponent& GetBehaviorHelperComponent() const { assert(_behaviorHelperComponent); return *_behaviorHelperComponent; }
  inline BehaviorHelperComponent&       GetBehaviorHelperComponent()       { assert(_behaviorHelperComponent); return *_behaviorHelperComponent; }
  
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache() const { assert(_objectInteractionInfoCache); return *_objectInteractionInfoCache; }
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache()       { assert(_objectInteractionInfoCache); return *_objectInteractionInfoCache; }
  
  inline const AIWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard& GetNonConstWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard&       GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
  
  inline const WorkoutComponent& GetWorkoutComponent() const { assert(_workoutComponent); return *_workoutComponent; }
  inline WorkoutComponent&       GetWorkoutComponent()       { assert(_workoutComponent); return *_workoutComponent; }
  

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
  
  // module to analyze information for the AI in processes common to more than one behavior, for example
  // border calculation
  std::unique_ptr<AIInformationAnalyzer>   _aiInformationAnalyzer;
  
  // Component which behaviors and helpers can query to find out the appropriate animation
  // to play in response to a user facing action result
  std::unique_ptr<BehaviorEventAnimResponseDirector> _behaviorEventAnimResponseDirector;
  
  // component which behaviors can delegate to for automatic action error handling
  std::unique_ptr<BehaviorHelperComponent> _behaviorHelperComponent;
  
  // Component which tracks and caches the best objects to use for certain interactions
  std::unique_ptr<ObjectInteractionInfoCache> _objectInteractionInfoCache;
  
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  std::unique_ptr<AIWhiteboard>            _whiteboard;
  
  // component for tracking cozmo's work-out behaviors
  std::unique_ptr< WorkoutComponent >      _workoutComponent;
};

}
}

#endif
