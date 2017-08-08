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

#include "engine/behaviorSystem/activities/activities/iActivity.h"
#include "engine/behaviorSystem/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/components/bodyLightComponentTypes.h"
#include "anki/common/basestation/objectIDs.h"

#include "util/signals/simpleSignal_fwd.h"

namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {
  
// forward declarations
class BehaviorFeedingEat;
class BehaviorFeedingSearchForCube;
class FeedingCubeController;
struct ObjectConnectionState;

class ActivityFeeding : public IActivity, public IFeedingListener
{
public:
  ActivityFeeding(Robot& robot, const Json::Value& config);
  ~ActivityFeeding();
  
  virtual Result Update(Robot& robot) override;
  
  // Implementation of IFeedingListener
  virtual void StartedEating(Robot& robot, const int duration_s) override;
  virtual void EatingInterrupted(Robot& robot) override;
  
protected:
  virtual IBehaviorPtr ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior) override;

  virtual void OnSelectedInternal(Robot& robot) override;
  virtual void OnDeselectedInternal(Robot& robot) override;
  
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
  
  bool _severeAnimsSet;

  AnimationTrigger _currIdle;
  bool _hasSetIdle;
  
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  // Chooser which manages universal response behaviors
  std::unique_ptr<IBehaviorChooser> _universalResponseChooser;
  
  // Behaviors that the chooser calls directly
  IBehaviorPtr _searchingForFaceBehavior;
  IBehaviorPtr _searchingForFaceBehavior_Severe;
  IBehaviorPtr _turnToFaceBehavior;
  IBehaviorPtr _turnToFaceBehavior_Severe;
  std::shared_ptr<BehaviorFeedingSearchForCube> _searchForCubeBehavior;
  std::shared_ptr<BehaviorFeedingEat> _eatFoodBehavior;
  
  IBehaviorPtr _waitBehavior;
  IBehaviorPtr _reactCubeShakeBehavior;
  IBehaviorPtr _reactCubeShakeBehavior_Severe;
  IBehaviorPtr _reactFullCubeBehavior;
  IBehaviorPtr _reactFullCubeBehavior_Severe;
  IBehaviorPtr _reactSeeCharged;
  IBehaviorPtr _reactSeeCharged_Severe;

  
  
  // DAS info trackers
  int _DASCubesPerFeeding = 0;
  int _DASFeedingSessionsPerConnectedSession = 0;
  int _DASMostCubesInParallel = 0;

  void UpdateCurrentStage(Robot& robot);
  void UpdateCubeToEat(Robot& robot);
  
  void SetActivityStage(Robot& robot, FeedingActivityStage newStage, const std::string& newStageName);

  void SetIdleForCurrentStage(Robot& robot);
  
  // Updates the activity stage to the best thing Cozmo can do right now
  // e.g. wait for a cube to shake, search for a shaken cube etc.
  void TransitionToBestActivityStage(Robot& robot);

  IBehaviorPtr GetBestBehaviorFromMap() const;
  
  // Handle object connection state issues
  void HandleObjectConnectionStateChange(Robot& robot, const ObjectConnectionState& connectionState);
  
  void ClearSevereAnims(Robot& robot);
  
  bool HasSingleBehaviorStageStarted(IBehaviorPtr behavior);

  bool HasFaceToTurnTo(Robot& robot) const;
  
  void SetupSevereAnims(Robot& robot);

  void SendCubeDasEventsIfNeeded();
  
  // Setup map
  std::map<FeedingActivityStage, IBehaviorPtr> _stageToBehaviorMap;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__
