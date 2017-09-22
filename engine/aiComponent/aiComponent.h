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

#define USE_BSM 1

namespace Anki {
namespace Cozmo {

// Forward declarations
class AIInformationAnalyzer;
class AIWhiteboard;
class BehaviorContainer;
class BehaviorEventAnimResponseDirector;
class BehaviorExternalInterface;
class BehaviorHelperComponent;
class BehaviorManager;
class BehaviorSystemManager;
class DoATrickSelector;
class FeedingSoundEffectManager;
class FreeplayDataTracker;
class ObjectInteractionInfoCache;
class RequestGameComponent;
class Robot;
class SevereNeedsComponent;
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
  
  inline const BehaviorManager& GetBehaviorManager() const { return *_behaviorMgr; }
  inline BehaviorManager&       GetBehaviorManager()       { return *_behaviorMgr; }
  
  // For test only
  inline BehaviorContainer& GetBehaviorContainer() { return *_behaviorContainer; }
  
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache() const { assert(_objectInteractionInfoCache); return *_objectInteractionInfoCache; }
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache()       { assert(_objectInteractionInfoCache); return *_objectInteractionInfoCache; }
  
  inline const AIWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard& GetNonConstWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard&       GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
  
  inline const WorkoutComponent& GetWorkoutComponent() const { assert(_workoutComponent); return *_workoutComponent; }
  inline WorkoutComponent&       GetWorkoutComponent()       { assert(_workoutComponent); return *_workoutComponent; }
  
  inline RequestGameComponent& GetRequestGameComponent()  { assert(_requestGameComponent); return *_requestGameComponent;}
  inline RequestGameComponent& GetNonConstRequestGameComponent() const { assert(_requestGameComponent); return *_requestGameComponent;}

  inline FeedingSoundEffectManager& GetFeedingSoundEffectManager() { assert(_feedingSoundEffectManager); return  *_feedingSoundEffectManager; };
  
  inline DoATrickSelector& GetDoATrickSelector()  { assert(_doATrickSelector); return *_doATrickSelector; }

  inline FreeplayDataTracker& GetFreeplayDataTracker()  { assert(_freeplayDataTracker); return *_freeplayDataTracker; }  
  
  inline const SevereNeedsComponent& GetSevereNeedsComponent() const  { assert(_severeNeedsComponent); return *_severeNeedsComponent; }
  inline SevereNeedsComponent& GetSevereNeedsComponent()  { assert(_severeNeedsComponent); return *_severeNeedsComponent; }
  inline SevereNeedsComponent& GetNonConstSevereNeedsComponent() const { assert(_severeNeedsComponent); return *_severeNeedsComponent; }

  
  ////////////////////////////////////////////////////////////////////////////////
  // Update and init
  ////////////////////////////////////////////////////////////////////////////////

  Result Init();
  Result Update(Robot& robot, std::string& currentActivityName,
                              std::string& behaviorDebugStr);

  ////////////////////////////////////////////////////////////////////////////////
  // Message handling / dispatch
  ////////////////////////////////////////////////////////////////////////////////

  void OnRobotDelocalized();
  void OnRobotRelocalized();
  
  ////////////////////////////////////////////////////////////////////////////////
  // Accessors
  ////////////////////////////////////////////////////////////////////////////////
  
  inline bool IsSuddenObstacleDetected() const { return _suddenObstacleDetected; }

private:
  Robot& _robot;
  bool   _suddenObstacleDetected;
  
  // module to analyze information for the AI in processes common to more than one behavior, for example
  // border calculation
  std::unique_ptr<AIInformationAnalyzer>   _aiInformationAnalyzer;
  
  // Component which behaviors and helpers can query to find out the appropriate animation
  // to play in response to a user facing action result
  std::unique_ptr<BehaviorEventAnimResponseDirector> _behaviorEventAnimResponseDirector;
  
  // component which behaviors can delegate to for automatic action error handling
  std::unique_ptr<BehaviorHelperComponent> _behaviorHelperComponent;

  // Factory creates and tracks data-driven behaviors etc
  std::unique_ptr<BehaviorContainer> _behaviorContainer;
  
  // Interface that the behavior system can use to communicate with the rest of engine
  std::unique_ptr<BehaviorExternalInterface> _behaviorExternalInterface;
  
  // components which manage the behavior system
  std::unique_ptr<BehaviorManager>       _behaviorMgr;
  std::unique_ptr<BehaviorSystemManager> _behaviorSysMgr;
  
  // Component which tracks and caches the best objects to use for certain interactions
  std::unique_ptr<ObjectInteractionInfoCache> _objectInteractionInfoCache;
  
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  std::unique_ptr<AIWhiteboard>            _whiteboard;
  
  // component for tracking cozmo's work-out behaviors
  std::unique_ptr< WorkoutComponent >      _workoutComponent;
  
  // component for keeping track of what game cozmo should request next
  std::unique_ptr<RequestGameComponent> _requestGameComponent;
  
  // component which behaviors can delegate to for selecting a random Trick / Spark
  std::unique_ptr<DoATrickSelector>    _doATrickSelector;

  // Coordinates sound effect events across various feeding cubes/activities etc
  std::unique_ptr<FeedingSoundEffectManager> _feedingSoundEffectManager;
  
  // component for tracking freeplay DAS data
  std::unique_ptr<FreeplayDataTracker> _freeplayDataTracker;
  
  // component for tracking severe needs states
  std::unique_ptr<SevereNeedsComponent> _severeNeedsComponent;
  
  
  Result UpdateBehaviorManager(Robot& robot, std::string& currentActivityName,
                                             std::string& behaviorDebugStr);  
};

}
}

#endif
