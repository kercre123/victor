/**
 * File: activityFeeding.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity for feeding
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__

#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/components/bodyLightComponentTypes.h"
#include "anki/common/basestation/objectIDs.h"

#include "util/signals/simpleSignal_fwd.h"

namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {
  
// forward declarations
class BehaviorExternalInterface;
class BehaviorFeedingEat;
class BehaviorFeedingSearchForCube;
class FeedingCubeController;
struct ObjectConnectionState;

class ActivityFeeding : public IActivity, public IFeedingListener
{
public:
  ActivityFeeding(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  ~ActivityFeeding();
  
  virtual Result Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // Implementation of IFeedingListener
  virtual void StartedEating(BehaviorExternalInterface& behaviorExternalInterface, const int duration_s) override;
  virtual void EatingComplete(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void EatingInterrupted(BehaviorExternalInterface& behaviorExternalInterface) override;
  
protected:
  virtual ICozmoBehaviorPtr GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior) override;

  virtual void OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  enum class FeedingActivityStage{
    SearchForFace = 0,
    SearchForFace_Severe,
    TurningToFace,
    TurningToFace_Severe,
    WaitingForShake,
    ReactingToShake,
    ReactingToShake_Severe,
    WaitingForFullyCharged,
    ReactingToFullyCharged,
    ReactingToFullyCharged_Severe,
    SearchingForCube,
    ReactingToSeeCharged,
    ReactingToSeeCharged_Severe,
    EatFood
  };
  
  FeedingActivityStage _activityStage;
  float _lastStageChangeTime_s;
  float _timeFaceSearchShouldEnd_s;
  
  std::map<ObjectID, std::unique_ptr<FeedingCubeController>> _cubeControllerMap;
  // Set of objects Cozmo couldn't find and shouldn't search for again
  std::set<ObjectID> _cubesSearchCouldntFind;
  
  // The object cozmo should eat once he's seen that it's fully charged
  ObjectID _cubeIDToEat;
  ObjectID _cubeIDToSearchFor;
  
  // Bool that will be set by a behavior listener callback when the behavior has
  bool _eatingComplete;
  
  bool _severeBehaviorLocksSet;

  AnimationTrigger _currIdle;
  bool _hasSetIdle;
  
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  // Chooser which manages universal response behaviors
  std::unique_ptr<IBehaviorChooser> _universalResponseChooser;
  
  // Behaviors that the chooser calls directly
  ICozmoBehaviorPtr _searchingForFaceBehavior;
  ICozmoBehaviorPtr _searchingForFaceBehavior_Severe;
  ICozmoBehaviorPtr _turnToFaceBehavior;
  ICozmoBehaviorPtr _turnToFaceBehavior_Severe;
  std::shared_ptr<BehaviorFeedingSearchForCube> _searchForCubeBehavior;
  std::shared_ptr<BehaviorFeedingEat> _eatFoodBehavior;
  
  ICozmoBehaviorPtr _waitBehavior;
  ICozmoBehaviorPtr _reactCubeShakeBehavior;
  ICozmoBehaviorPtr _reactCubeShakeBehavior_Severe;
  ICozmoBehaviorPtr _reactFullCubeBehavior;
  ICozmoBehaviorPtr _reactFullCubeBehavior_Severe;
  ICozmoBehaviorPtr _reactSeeCharged;
  ICozmoBehaviorPtr _reactSeeCharged_Severe;

  
  
  // DAS info trackers
  int _DASCubesPerFeeding = 0;
  int _DASFeedingSessionsPerConnectedSession = 0;
  int _DASMostCubesInParallel = 0;

  void UpdateCurrentStage(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateCubeToEat(BehaviorExternalInterface& behaviorExternalInterface);
  
  void SetActivityStage(BehaviorExternalInterface& behaviorExternalInterface, FeedingActivityStage newStage, const std::string& newStageName);

  void SetIdleForCurrentStage(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Updates the activity stage to the best thing Cozmo can do right now
  // e.g. wait for a cube to shake, search for a shaken cube etc.
  void TransitionToBestActivityStage(BehaviorExternalInterface& behaviorExternalInterface);

  ICozmoBehaviorPtr GetBestBehaviorFromMap() const;
  
  // Handle object connection state issues
  void HandleObjectConnectionStateChange(BehaviorExternalInterface& behaviorExternalInterface, const ObjectConnectionState& connectionState);
  
  void ClearSevereAnims(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool HasSingleBehaviorStageStarted(ICozmoBehaviorPtr behavior);

  bool HasFaceToTurnTo(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  void SetupSevereAnims(BehaviorExternalInterface& behaviorExternalInterface);

  void SendCubeDasEventsIfNeeded();
  
  // Setup map
  std::map<FeedingActivityStage, ICozmoBehaviorPtr> _stageToBehaviorMap;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__
