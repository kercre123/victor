/**
 *  File: BehaviorBuildPyramidBase.cpp
 *
 *  Author: Kevin M. Karol
 *  Created: 2016-08-09
 *
 *  Description: Behavior which places two blocks next to each other to form the base
 *  of a pyramid.
 *
 *  Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/namedColors/namedColors.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/types/unlockTypes.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const int kCubeSizeBuffer_mm = 12;
const float kDriveBackDistance_mm = 40.f;
const float kDriveBackSpeed_mm_s = 40.f;
const float kTimeUntilRespondToBase_s = 2.f;

RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
{
  animTrigger = AnimationTrigger::Count;
  return true;
};

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBuildPyramidBase::BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _lastBasesCount(0)
, _timeFirstBaseFormed(-1.f)
, _timeLastBaseDestroyed(-1.f)
, _continuePastBaseCallback(nullptr)
, _behaviorState(State::DrivingToBaseBlock)
, _checkForFullPyramidVisualVerifyFailure(false)
, _robot(robot)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();  
  UpdatePyramidTargets(robot);
  
  bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet() && !_topBlockID.IsSet();
  if(allSet){
    // Ensure a base does not already exist in the world
    using namespace BlockConfigurations;
    auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
    return pyramidBases.empty();
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorBuildPyramidBase::InitInternal(Robot& robot)
{
  ResetMemberVars();
  
  if(!robot.IsCarryingObject()){
    TransitionToDrivingToBaseBlock(robot);
  }else{
    TransitionToPlacingBaseBlock(robot);
  }
  
  return Result::RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorBuildPyramidBase::UpdateInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  // We need a slight delay from when a configuration is made and we respond to it
  if((_lastBasesCount == 0) && !pyramidBases.empty()){
    _timeFirstBaseFormed = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  
  if((_lastBasesCount > 0) && pyramidBases.empty() && pyramids.empty()){
    _timeLastBaseDestroyed = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const bool baseAppearedWhileNotPlacing =
                        !pyramidBases.empty() &&
                        (_behaviorState < State::ObservingBase) &&
                        FLT_GT(currentTime_s, _timeFirstBaseFormed + kTimeUntilRespondToBase_s);
  
  // if a pyramid base was destroyed, stop the behavior to build the base again
  const bool baseDestroyedWhileNotPlacing =
                        pyramidBases.empty() &&
                        pyramids.empty() &&
                        FLT_GT(_timeLastBaseDestroyed, 0) &&
                        FLT_GT(currentTime_s, _timeLastBaseDestroyed + kTimeUntilRespondToBase_s);

  if(baseAppearedWhileNotPlacing || baseDestroyedWhileNotPlacing){
    StopWithoutImmediateRepetitionPenalty();
    return IBehavior::Status::Complete;
  }
  
  // prevent against visual verify failures
  if(_checkForFullPyramidVisualVerifyFailure){
    if(!pyramids.empty()){
      StopWithoutImmediateRepetitionPenalty();
      return IBehavior::Status::Complete;
    }
  }
  
  _lastBasesCount = Util::numeric_cast<int>(pyramidBases.size());
  
  IBehavior::Status ret = IBehavior::UpdateInternal(robot);
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToDrivingToBaseBlock(Robot& robot)
{
  SET_STATE(DrivingToBaseBlock);
  std::vector<BehaviorStateLightInfo> basePersistantLight;
  basePersistantLight.push_back(
    BehaviorStateLightInfo(_baseBlockID, CubeAnimationTrigger::PyramidSingle)
  );
  SetBehaviorStateLights(basePersistantLight, false);
  
  auto success = [this](Robot& robot){
    TransitionToPlacingBaseBlock(robot);
  };

  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.animBeforeDock = AnimationTrigger::BuildPyramidSecondBlockUpright;
  
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(robot, *this,
                                                              _baseBlockID, params);
  SmartDelegateToHelper(robot, pickupHelper, success);
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToPlacingBaseBlock(Robot& robot)
{
  SET_STATE(PlacingBaseBlock);
  std::vector<BehaviorStateLightInfo> basePersistantLight;
  SetBehaviorStateLights(basePersistantLight, false);
  
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.TransitionToPlacingBaseBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  const bool relativeCurrentMarker = false;
  
  auto success = [this](Robot& robot){
    TransitionToObservingBase(robot);
  };
  
  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PlaceRelObjectParameters params;
  
  UpdateBlockPlacementOffsets(params.placementOffsetX_mm,
                              params.placementOffsetY_mm);
  params.relativeCurrentMarker = relativeCurrentMarker;
  
  HelperHandle placeRelHelper = factory.CreatePlaceRelObjectHelper(robot, *this,
                                                                   _staticBlockID,
                                                                   true, params);
  SmartDelegateToHelper(robot, placeRelHelper, success);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToObservingBase(Robot& robot)
{
  SET_STATE(ObservingBase);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new DriveStraightAction(robot, -kDriveBackDistance_mm, kDriveBackSpeed_mm_s));
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::BuildPyramidReactToBase));

  StartActing(action,
              [this, &robot]{
                if(_continuePastBaseCallback != nullptr){
                  _continuePastBaseCallback(robot);
                }
              });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::GetBaseBlockID(ObjectID& id) const
{
  if(_baseBlockID.IsSet()){
    id = _baseBlockID;
  }
  
  return _baseBlockID.IsSet();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::GetStaticBlockID(ObjectID& id) const
{
  if(_staticBlockID.IsSet()){
    id = _staticBlockID;
  }
  
  return _staticBlockID.IsSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::GetTopBlockID(ObjectID& id) const
{
  if(_topBlockID.IsSet()){
    id = _topBlockID;
  }
  
  return _topBlockID.IsSet();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::UpdateBlockPlacementOffsets(f32& xOffset, f32& yOffset) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdateBlockPlacementOffset.NullObject",
                        "Static block with id %d is NULL", _staticBlockID.GetValue());
    return;
  }
  
  // Construct a list of valid Placement poses next to the block
  f32 nearestDistanceSQ = FLT_MAX;
  const Point3f blockSize = object->GetSizeInParentFrame();
  const f32 blockXSize = blockSize.x() + kCubeSizeBuffer_mm;
  const f32 blockYSize = blockSize.y() + kCubeSizeBuffer_mm;
  
  using Offsets =  std::pair<float,float>;
  std::vector<Offsets> offsetList;
  offsetList.emplace_back(blockXSize, 0);
  offsetList.emplace_back(0, blockYSize);
  offsetList.emplace_back(0, -blockYSize);
  offsetList.emplace_back(-blockXSize, 0);
  
  // assign default values, just in case
  xOffset = offsetList[0].first;
  yOffset = offsetList[0].second;

  for(const auto& entry: offsetList){
    if(CheckBaseBlockPoseIsFree(entry.first, entry.second)){
      Pose3d zRotatedPose = object->GetZRotatedPointAboveObjectCenter(0.f);
      Pose3d potentialPose(0, Z_AXIS_3D(),
                           {entry.first, entry.second, -blockSize.z()},
                           &zRotatedPose);
      f32 newDistSquared;
      ComputeDistanceSQBetween(_robot.GetPose(), potentialPose, newDistSquared);
      
      if(newDistSquared < nearestDistanceSQ){
        nearestDistanceSQ = newDistSquared;
        xOffset = entry.first;
        yOffset = entry.second;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  const ObservableObject* placingObject = _robot.GetBlockWorld().GetLocatedObjectByID(_baseBlockID);
  if(nullptr == object || nullptr == placingObject)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.CHeckBaseBlockPoseIsFree.NullObject",
                        "Static block with id %d or base block with id %d is NULL",
                        _staticBlockID.GetValue(), _baseBlockID.GetValue());
    return false;
  }
  
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  filter.SetIgnoreIDs({_staticBlockID});
  
  // get the size of the placing object
  const Point3f rotatedSize = placingObject->GetSizeInParentFrame();
  
  const Pose3d zRotatedPose = object->GetZRotatedPointAboveObjectCenter(0.f);
  const Pose3d placePose(0, Z_AXIS_3D(), {rotatedSize.x(), rotatedSize.y(), 0}, &zRotatedPose);
  
  ObservableObject* closestObject =
    _robot.GetBlockWorld().FindLocatedObjectClosestTo(placePose.GetWithRespectToOrigin(), rotatedSize, filter);
  return (closestObject == nullptr);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::UpdatePyramidTargets(const Robot& robot) const
{
  using Intention = ObjectInteractionIntention;
  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
  auto bestBase   = objInfoCache.GetBestObjectForIntention(Intention::PyramidBaseObject);
  auto bestStatic = objInfoCache.GetBestObjectForIntention(Intention::PyramidStaticObject);
  auto bestTop    = objInfoCache.GetBestObjectForIntention(Intention::PyramidTopObject);
  const bool blockAssignmentChanged = (bestBase != _baseBlockID) ||
                                      (bestStatic != _staticBlockID) ||
                                      (bestTop != _topBlockID);
  
  _baseBlockID = std::move(bestBase);
  _staticBlockID = std::move(bestStatic);
  _topBlockID = std::move(bestTop);
  
  DEV_ASSERT(AreAllBlockIDsUnique(), "BehaviorBuildPyramidBase.IsRunnable.AllBlocksNotUnique");
  return blockAssignmentChanged;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetState_internal(State state, const std::string& stateName){
  _behaviorState = state;
  SetDebugStateName(stateName);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::AreAllBlockIDsUnique() const
{
  if(_staticBlockID.IsSet() && _baseBlockID.IsSet()){
    const bool baseIDsUnique = _staticBlockID != _baseBlockID;
    if(_topBlockID.IsSet()){
      const bool topIDUnique = _topBlockID != _staticBlockID &&
                                _topBlockID != _baseBlockID;
      return baseIDsUnique && topIDUnique;
    }
    return baseIDsUnique;
  }
  
  return true;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::ResetMemberVars()
{
  _lastBasesCount = 0;
  _timeFirstBaseFormed = -1.f;
  _timeLastBaseDestroyed = -1.f;
  
}

} //namespace Cozmo
} //namespace Anki
