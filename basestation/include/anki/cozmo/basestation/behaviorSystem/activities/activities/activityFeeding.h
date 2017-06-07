/**
 * File: activityFeeding.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity for building a pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iFeedingListener.h"
#include "anki/cozmo/basestation/components/bodyLightComponentTypes.h"
#include "anki/common/basestation/objectIDs.h"

#include "util/signals/simpleSignal_fwd.h"

namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {
  
// forward declarations
class BehaviorFeedingEat;
class BehaviorPlayArbitraryAnim;
class FeedingCubeController;

class ActivityFeeding : public IActivity, public IFeedingListener
{
public:
  ActivityFeeding(Robot& robot, const Json::Value& config);
  ~ActivityFeeding();
  
  virtual Result Update(Robot& robot) override;
  
  // Implementation of IFeedingListener
  virtual void StartedEating(Robot& robot, const int duration_s) override;
  
protected:
  virtual IBehavior* ChooseNextBehaviorInternal(Robot& robot, const IBehavior* currentRunningBehavior) override;

  virtual void OnSelectedInternal(Robot& robot) override;
  virtual void OnDeselectedInternal(Robot& robot) override;
  
private:
  enum class FeedingActivityStage{
    None,
    SearchForFace,
    TurnToFace,
    WaitingForShake,
    ReactingToShake,
    WaitingForFullyCharged,
    ReactingToFullyCharged,
    SearchingForCube,
    ReactingToCube,
    EatFood
  };
  
  
  FeedingActivityStage _chooserStage;
  float _timeFaceSearchShouldEnd_s;
  
  std::map<ObjectID, std::unique_ptr<FeedingCubeController>> _cubeControllerMap;
  
  // Control cozmo's backpack lights throughout feeding
  BackpackLightDataLocator  _bodyLightDataLocator{};
  
  // The object cozmo should eat once he's seen that it's fully charged
  ObjectID _interactID;
  
  // Bool that will be set by a behavior listener callback when the behavior has
  bool _eatingComplete;
  
  bool _idleAndDrivingSet;
  
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  // Behaviors that the chooser calls directly
  IBehavior* _searchingForFaceBehavior              = nullptr;
  IBehavior* _turnToFaceBehavior                    = nullptr;
  IBehavior* _searchForCubeBehavior                 = nullptr;
  BehaviorFeedingEat* _eatFoodBehavior              = nullptr;
  BehaviorPlayArbitraryAnim* _behaviorPlayAnimation = nullptr;
  
  void UpdateActivityStage(FeedingActivityStage newStage, const std::string& stageName);
  
  // Sets backpack lights etc for getting into hunger loop
  void TransitionToBestActivityStage(Robot& robot);
  
  // Handle object observations
  void RobotObservedObject(const ObjectID& objID);
  
  void UpdateAnimationToPlay(AnimationTrigger animTrigger, int repetitions);

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__
