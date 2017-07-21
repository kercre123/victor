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
  
protected:
  virtual IBehaviorPtr ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior) override;

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
  float _lastStageChangeTime_s;
  float _timeFaceSearchShouldEnd_s;
  
  std::map<ObjectID, std::unique_ptr<FeedingCubeController>> _cubeControllerMap;
  // Set of objects Cozmo couldn't find and shouldn't search for again
  std::set<ObjectID> _cubesSearchCouldntFind;
  
  // The object cozmo should eat once he's seen that it's fully charged
  ObjectID _interactID;
  ObjectID _searchingForID;
  
  // Bool that will be set by a behavior listener callback when the behavior has
  bool _eatingComplete;
  
  bool _severeAnimsSet;
  
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  // Chooser which manages universal response behaviors
  IBehaviorChooser* _universalResponseChooser;
  
  // Behaviors that the chooser calls directly
  IBehaviorPtr _searchingForFaceBehavior;
  IBehaviorPtr _searchingForFaceBehavior_Severe;
  IBehaviorPtr _turnToFaceBehavior;
  std::shared_ptr<BehaviorFeedingSearchForCube> _searchForCubeBehavior;
  std::shared_ptr<BehaviorFeedingEat> _eatFoodBehavior;
  std::shared_ptr<BehaviorPlayArbitraryAnim> _behaviorPlayAnimation;
  
  
  // DAS info trackers
  int _DASCubesPerFeeding = 0;
  int _DASFeedingSessionsPerAppRun = 0;
  float _DASTimeLastFeedingStageStarted = -1.0f;
  int   _DASMostCubesInParallel = 0;
  
  void UpdateActivityStage(FeedingActivityStage newStage, const std::string& newStageName);
  
  // Updates the activity stage to the best thing Cozmo can do right now
  // e.g. wait for a cube to shake, search for a shaken cube etc.
  // Returns the behavior to run this tick since this is frequently used
  // in the middle of the ChooseNextBehavior switch statement
  IBehaviorPtr TransitionToBestActivityStage(Robot& robot);
  
  // Handle object observations
  void RobotObservedObject(const ObjectID& objID);
  
  void UpdateAnimationToPlay(AnimationTrigger animTrigger);
  
  void HandleObjectConnectionStateChange(Robot& robot, const ObjectConnectionState& connectionState);
  
  void ClearSevereAnims(Robot& robot);
  
  bool HasSingleBehaviorStageStarted(IBehaviorPtr behavior);
  
  void SetupSevereAnims(Robot& robot);

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityFeeding_H__
