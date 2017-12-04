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
class BehaviorComponent;
class BehaviorContainer;
class BehaviorHelperComponent;
class DoATrickSelector;
class FeedingSoundEffectManager;
class FreeplayDataTracker;
class ObjectInteractionInfoCache;
class PuzzleComponent;
class RequestGameComponent;
class Robot;
class SevereNeedsComponent;
class WorkoutComponent;
  

namespace ComponentWrappers{
struct AIComponentComponents{
  AIComponentComponents(Robot& robot)
  :_robot(robot){}
  Robot& _robot;
};
}

  
class AIComponent : private Util::noncopyable
{
public:
  
  explicit AIComponent();
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
  
  
  inline const BehaviorComponent& GetBehaviorComponent() const { assert(_behaviorComponent); return *_behaviorComponent; }
  inline BehaviorComponent&       GetBehaviorComponent()       { assert(_behaviorComponent); return *_behaviorComponent; }
  
  // Support legacy code until move helper comp into delegate component
  const BehaviorHelperComponent& GetBehaviorHelperComponent() const;
  BehaviorHelperComponent&       GetBehaviorHelperComponent();
  // For test only
  BehaviorContainer& GetBehaviorContainer();
  
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache() const { assert(_objectInteractionInfoCache); return *_objectInteractionInfoCache; }
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache()       { assert(_objectInteractionInfoCache); return *_objectInteractionInfoCache; }
  
  inline const AIWhiteboard& GetWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard& GetNonConstWhiteboard() const { assert(_whiteboard); return *_whiteboard; }
  inline AIWhiteboard&       GetWhiteboard()       { assert(_whiteboard); return *_whiteboard; }
  
  inline const PuzzleComponent& GetPuzzleComponent() const { assert(_puzzleComponent); return *_puzzleComponent; }
  inline PuzzleComponent&       GetPuzzleComponent()       { assert(_puzzleComponent); return *_puzzleComponent; }
  
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

  // Pass in behavior component if you have a custom component already set up
  // If nullptr is passed in AIComponent will generate a default behavior component
  // AIComponent takes over contrrol of the customBehaviorComponent's memory
  Result Init(Robot& robot, BehaviorComponent*& customBehaviorComponent);
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
  std::unique_ptr<ComponentWrappers::AIComponentComponents> _aiComponents;
  bool   _suddenObstacleDetected;
  
  // module to analyze information for the AI in processes common to more than one behavior, for example
  // border calculation
  std::unique_ptr<AIInformationAnalyzer>   _aiInformationAnalyzer;
  
  // component hwich manages all aspects of the AI system that relate to behaviors
  std::unique_ptr<BehaviorComponent> _behaviorComponent;
  
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
  
  // component for tracking cozmo's puzzle behaviors
  std::unique_ptr< PuzzleComponent >      _puzzleComponent;
  
  void CheckForSuddenObstacle(Robot& robot);
};

}
}

#endif
