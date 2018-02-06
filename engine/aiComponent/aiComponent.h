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

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/robotComponents_fwd.h"
#include "engine/entity.h"
#include "engine/dependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <string>

namespace Anki {
namespace Cozmo {

// Forward declarations
class AIInformationAnalyzer;
class AIWhiteboard;
class BEIRobotInfo;
class BehaviorComponent;
class BehaviorContainer;
class BehaviorHelperComponent;
class ContinuityComponent;
class DoATrickSelector;
class FaceSelectionComponent;
class FeedingSoundEffectManager;
class FreeplayDataTracker;
class ObjectInteractionInfoCache;
class PuzzleComponent;
class RequestGameComponent;
class Robot;
class SevereNeedsComponent;
class WorkoutComponent;

namespace ComponentWrappers{
class AIComponentComponents{
public:
  AIComponentComponents(Robot&                      robot,
                        BehaviorComponent*&         behaviorComponent,
                        ContinuityComponent*        continuityComponent,
                        DoATrickSelector*           doATrickSelector,
                        FaceSelectionComponent*     faceSelectionComponent,
                        FeedingSoundEffectManager*  feedingSoundEFfectManager,
                        FreeplayDataTracker*        freeplayDataTracker,
                        AIInformationAnalyzer*      infoAnalyzer,
                        ObjectInteractionInfoCache* objectInteractionInfoCache,
                        PuzzleComponent*            puzzleComponent,
                        RequestGameComponent*       requestGameComponent,
                        SevereNeedsComponent*       severeNeedsComponent,
                        AIWhiteboard*               aiWhiteboard,
                        WorkoutComponent*           workoutComponent);
  virtual ~AIComponentComponents();
  
  Robot& _robot;
  EntityFullEnumeration<AIComponentID, ComponentWrapper, AIComponentID::Count> _components;

};
}

  // AIComponent is updated at the robot component level, same as BehaviorComponent
  // Therefore BCComponents (which are managed by BehaviorComponent) can't declare dependencies on AIComponent 
  // since when it's Init/Update relative to BehaviorComponent must be declared by BehaviorComponent explicitly, 
  // not by individual components within BehaviorComponent
class AIComponent :  public UnreliableComponent<BCComponentID>, 
                     public IDependencyManagedComponent<RobotComponentID>,  
                     private Util::noncopyable
{
public:
  explicit AIComponent();
  virtual ~AIComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override final{};
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void UpdateDependent(const RobotCompMap& dependentComponents) override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};

  // Prevent hiding function warnings by exposing the (valid) unreliable component methods
  using UnreliableComponent<BCComponentID>::InitDependent;
  using UnreliableComponent<BCComponentID>::GetInitDependencies;
  using UnreliableComponent<BCComponentID>::UpdateDependent;
  using UnreliableComponent<BCComponentID>::GetUpdateDependencies;
  //////
  // end IDependencyManagedComponent functions
  //////


  ////////////////////////////////////////////////////////////////////////////////
  // Components
  ////////////////////////////////////////////////////////////////////////////////
  template<typename T>
  T& GetComponent(AIComponentID componentID) const {assert(_aiComponents); return _aiComponents->_components.GetComponent(componentID).GetValue<T>();}


  inline const AIInformationAnalyzer& GetAIInformationAnalyzer() const { return GetComponent<AIInformationAnalyzer>(AIComponentID::InformationAnalyzer);}
  inline AIInformationAnalyzer& GetAIInformationAnalyzer() { return GetComponent<AIInformationAnalyzer>(AIComponentID::InformationAnalyzer);}
  
  
  inline const BehaviorComponent& GetBehaviorComponent() const {  return GetComponent<BehaviorComponent>(AIComponentID::BehaviorComponent); }
  inline BehaviorComponent&       GetBehaviorComponent()       {  return GetComponent<BehaviorComponent>(AIComponentID::BehaviorComponent); }
  
  // Support legacy code until move helper comp into delegate component
  const BehaviorHelperComponent& GetBehaviorHelperComponent() const;
  BehaviorHelperComponent&       GetBehaviorHelperComponent();
  // For test only
  BehaviorContainer& GetBehaviorContainer();
  
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache() const {  return GetComponent<ObjectInteractionInfoCache>(AIComponentID::ObjectInteractionInfoCache); }
  inline ObjectInteractionInfoCache& GetObjectInteractionInfoCache()       {  return GetComponent<ObjectInteractionInfoCache>(AIComponentID::ObjectInteractionInfoCache); }
  
  inline const AIWhiteboard& GetWhiteboard()   const {  return GetComponent<AIWhiteboard>(AIComponentID::Whiteboard); }
  inline AIWhiteboard& GetNonConstWhiteboard() const {  return GetComponent<AIWhiteboard>(AIComponentID::Whiteboard); }
  inline AIWhiteboard&       GetWhiteboard()         {  return GetComponent<AIWhiteboard>(AIComponentID::Whiteboard); }
  
  inline const PuzzleComponent& GetPuzzleComponent() const { return GetComponent<PuzzleComponent>(AIComponentID::Puzzle);}
  inline PuzzleComponent&       GetPuzzleComponent()       { return GetComponent<PuzzleComponent>(AIComponentID::Puzzle);}

  inline const FaceSelectionComponent& GetFaceSelectionComponent() const {return GetComponent<FaceSelectionComponent>(AIComponentID::FaceSelection);}

  inline const WorkoutComponent& GetWorkoutComponent() const {  return GetComponent<WorkoutComponent>(AIComponentID::Workout); }
  inline WorkoutComponent&       GetWorkoutComponent()       {  return GetComponent<WorkoutComponent>(AIComponentID::Workout); }
  
  inline RequestGameComponent& GetRequestGameComponent()               {  return GetComponent<RequestGameComponent>(AIComponentID::RequestGame);}
  inline RequestGameComponent& GetNonConstRequestGameComponent() const {  return GetComponent<RequestGameComponent>(AIComponentID::RequestGame);}

  inline FeedingSoundEffectManager& GetFeedingSoundEffectManager() {  return GetComponent<FeedingSoundEffectManager>(AIComponentID::FeedingSoundEffect); };
  
  inline DoATrickSelector& GetDoATrickSelector()  {  return GetComponent<DoATrickSelector>(AIComponentID::DoATrick); }

  inline FreeplayDataTracker& GetFreeplayDataTracker()  {  return GetComponent<FreeplayDataTracker>(AIComponentID::FreeplayDataTracker); }  
  
  inline const SevereNeedsComponent& GetSevereNeedsComponent() const   {  return GetComponent<SevereNeedsComponent>(AIComponentID::SevereNeeds); }
  inline SevereNeedsComponent& GetSevereNeedsComponent()               {  return GetComponent<SevereNeedsComponent>(AIComponentID::SevereNeeds); }
  inline SevereNeedsComponent& GetNonConstSevereNeedsComponent() const {  return GetComponent<SevereNeedsComponent>(AIComponentID::SevereNeeds); }

  
  ////////////////////////////////////////////////////////////////////////////////
  // Update and init
  ////////////////////////////////////////////////////////////////////////////////

  // Pass in behavior component if you have a custom component already set up
  // If nullptr is passed in AIComponent will generate a default behavior component
  // AIComponent takes over contrrol of the customBehaviorComponent's memory
  Result Init(Robot* robot, BehaviorComponent*& customBehaviorComponent);
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
  
  void CheckForSuddenObstacle(Robot& robot);
};

}
}

#endif
