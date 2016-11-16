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

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/namedColors/namedColors.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"

#include "clad/types/unlockTypes.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const int kCubeSizeBuffer_mm = 12;
const int kRetryCount = 3;
  
static const char * kCustomLightLockName = "BuildPyramidSetByIDLock";

RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
{
  animTrigger = AnimationTrigger::Count;
  return true;
};
  
// Lights constants
  
// Single Block
static const constexpr uint kSingleTimeOn = 500;
static const constexpr uint kSingleTimeOff = kSingleTimeOn;
static const constexpr uint kSingleTransitionOn = kSingleTimeOn;
static const constexpr uint kSingleTransitionOff = kSingleTransitionOn;

// Base Formed rotation
static const constexpr uint kBaseFormedTimeOn = kSingleTimeOn*2;
static const constexpr uint kBaseFormedTimeOff = kBaseFormedTimeOn;
static const constexpr uint kBaseFormedTransitionOn = kBaseFormedTimeOn;
static const constexpr uint kBaseFormedTransitionOff = kBaseFormedTransitionOn;
  

// Full Pyramid rotation
static const constexpr uint kFullPyramidTimeOn = kSingleTimeOn/16;
static const constexpr uint kFullPyramidTimeOff = 3 * kFullPyramidTimeOn;
static const constexpr uint kFullPyramidTransitionOn = kSingleTimeOn/32;
static const constexpr uint kFullPyramidTransitionOff = kFullPyramidTransitionOn;
  
// Pyramid flourish consts
static const constexpr float kWaitForPyramidShutdown_sec = 10000;
static const constexpr uint kPyramidFlourishTimeOn = static_cast<uint>(2* kWaitForPyramidShutdown_sec);
static const constexpr uint kPyramidFlourishTimeOff = kPyramidFlourishTimeOn; //to be sadfe
static const constexpr uint kPyramidFlourishShutoff_sec = 4*kFullPyramidTimeOn; //to be sadfe

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBuildPyramidBase::BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _lastBasesCount(0)
, _continuePastBaseCallback(nullptr)
, _robot(robot)
, _baseBlockOffsetX(0)
, _baseBlockOffsetY(0)
{
  SetDefaultName("BuildPyramidBase");
  
  SubscribeToTags({
    EngineToGameTag::RobotDelocalized
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::IsRunnableInternal(const Robot& robot) const
{
  UpdatePyramidTargets(robot);

  bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet();
  if(allSet && AreAllBlockIDsUnique()){
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
  _lastBasesCount = 0;
  SetPickupInitialBlockLights();
  if(!robot.IsCarryingObject()){
    TransitionToDrivingToBaseBlock(robot);
  }else{
    TransitionToPlacingBaseBlock(robot);
  }
  
  return Result::RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::StopInternal(Robot& robot)
{
  ResetPyramidTargets(robot);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SparksPersistantMusicLightControls(Robot& robot, int& lastPyramidBaseSeenCount, bool& wasRobotCarryingObject, bool& sparksPersistantCallbackSet)
{
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const int pyramidBaseSize = static_cast<int>(pyramidBases.size());
  
  // Did we see a new pyramid base or was a pyramid base destroyed?
  if(pyramidBaseSize > 0 && lastPyramidBaseSeenCount == 0){
    const auto& base = pyramidBases[0];
    robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
            UnlockId::BuildPyramid, static_cast<int>(MusicState::BaseFormed));
    SetPyramidBaseLightsByID(robot, base->GetBaseBlockID(), base->GetStaticBlockID());
    
  }else if(pyramidBaseSize == 0 && lastPyramidBaseSeenCount != 0){
    robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
            UnlockId::BuildPyramid, static_cast<int>(MusicState::SearchingForCube));
    robot.GetLightsComponent().ClearAllCustomPatterns();
  }
  
  // The robot picked up a block - is it going to place it as a top block, or just looking around?
  if(robot.IsCarryingObject() != wasRobotCarryingObject){
    if(pyramidBaseSize == 0){
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
              UnlockId::BuildPyramid, static_cast<int>(MusicState::InitialCubeCarry));
    }else{
      // To pick up the cube with a base in place means the full pyramid behavior is running,
      // remove all locks so that the running behavior has full control of the lights
      const auto& base = pyramidBases[0];
      robot.GetLightsComponent().ClearCustomLightPattern(base->GetBaseBlockID(), kCustomLightLockName);
      robot.GetLightsComponent().ClearCustomLightPattern(base->GetStaticBlockID(), kCustomLightLockName);
    }
  }
  
  wasRobotCarryingObject = robot.IsCarryingObject();
  lastPyramidBaseSeenCount = pyramidBaseSize;
  
  // transition states in the event of a robot delocalization
  if(!sparksPersistantCallbackSet){
    auto handlerCallback = [&robot](const EngineToGameEvent& event) {
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
                                  UnlockId::BuildPyramid, static_cast<int>(MusicState::SearchingForCube));
      robot.GetLightsComponent().ClearAllCustomPatterns();
    };
  
    static Signal::SmartHandle eventHalder = robot.GetExternalInterface()->Subscribe(EngineToGameTag::RobotDelocalized, handlerCallback);
    sparksPersistantCallbackSet = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorBuildPyramidBase::UpdateInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  if(pyramidBases.size() > 0){
    // if bases exists, and the base we're trying to build isn't one of them, cancel and re-assign blocks
    PyramidBase possibleBase = PyramidBase(_staticBlockID, _baseBlockID);
    const bool baseIsValid = std::find_if(pyramidBases.begin(), pyramidBases.end(),
                        [&possibleBase](const PyramidBasePtr p){ return *p == possibleBase;}) == pyramidBases.end();
    if(baseIsValid){
      StopWithoutImmediateRepetitionPenalty();
      return IBehavior::Status::Complete;
    }
  }
  
  // if a pyramid base was destroyed, stop the behavior to build the base again
  if(pyramidBases.size() == 0 &&
     _lastBasesCount > 0){
    StopWithoutImmediateRepetitionPenalty();
    return IBehavior::Status::Complete;
  }
  
  _lastBasesCount = static_cast<int>(pyramidBases.size());
  
  IBehavior::Status ret = IBehavior::UpdateInternal(robot);
  
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToDrivingToBaseBlock(Robot& robot)
{
  DEBUG_SET_STATE(DrivingToBaseBlock);
  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _baseBlockID);
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, kRetryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToPlacingBaseBlock(robot);
                }
              });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToPlacingBaseBlock(Robot& robot)
{
  DEBUG_SET_STATE(PlacingBaseBlock);
  UpdateAudioState(static_cast<int>(MusicState::InitialCubeCarry));
  
  // if a block has moved into the area we want to place the block, update where we are going to place the block
  if(!CheckBaseBlockPoseIsFree(_baseBlockOffsetX, _baseBlockOffsetY)){
    UpdateBlockPlacementOffsets();
  }
  
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.TransitionToPlacingBaseBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  const bool relativeCurrentMarker = false;
  DriveToPlaceRelObjectAction* driveAction = new DriveToPlaceRelObjectAction(robot, _staticBlockID, true, _baseBlockOffsetX, _baseBlockOffsetY, false, 0, false, 0.f, false, relativeCurrentMarker);
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, kRetryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  SetPyramidBaseLights();
                  TransitionToObservingBase(robot);
                }
              });

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToObservingBase(Robot& robot)
{
  DEBUG_SET_STATE(ObservingBase);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::BuildPyramidReactToBase));

  StartActing(action,
              [this, &robot]{
                if(_continuePastBaseCallback != nullptr){
                  _continuePastBaseCallback(robot);
                }
              });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::UpdatePyramidTargets(const Robot& robot) const
{
  using namespace BlockConfigurations;
  
  /// Check block config caches for pyramids and bases for fast forwarding
  
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  auto pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  if(pyramids.size() > 0){
    auto pyramid = pyramids[0];
    _baseBlockID = pyramid->GetPyramidBase().GetBaseBlockID();
    _staticBlockID = pyramid->GetPyramidBase().GetStaticBlockID();
    _topBlockID = pyramid->GetTopBlockID();
    return;
  }
  
  if(pyramidBases.size() > 0){
    auto pyramidBase = pyramidBases[0];
    _baseBlockID = pyramidBase->GetBaseBlockID();
    _staticBlockID = pyramidBase->GetStaticBlockID();
    _topBlockID.UnSet();
  }

  
  BlockList allBlocks;
  BlockWorldFilter bottomBlockFilter;
  bottomBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  robot.GetBlockWorld().FindMatchingObjects(bottomBlockFilter, allBlocks);
  
  // set the base blocks for either pyramid base or full pyramid
  if(allBlocks.size() < 2){
    ResetPyramidTargets(robot);
  }
  else if(!_baseBlockID.IsSet() ||
     !_staticBlockID.IsSet()){
    const Pose3d& robotPose = robot.GetPose().GetWithRespectToOrigin();
    
    if(robot.IsCarryingObject()){
      _baseBlockID = robot.GetCarryingObject();
    }else{
      _baseBlockID = GetNearestBlockToPose(robotPose, allBlocks);
    }
    
    const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
    if(nullptr == baseBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdatePyramidTargets.BaseBlockNull",
                          "BaseBlockID=%d", _baseBlockID.GetValue());
      _baseBlockID.UnSet();
      return;
    }
    
    SafeEraseBlockFromBlockList(_baseBlockID, allBlocks);
    auto baseBlockPose = baseBlock->GetPose().GetWithRespectToOrigin();
    
    _staticBlockID = GetNearestBlockToPose(baseBlockPose, allBlocks);
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
    if(nullptr == staticBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdatePyramidTargets.StaticBlockNull",
                          "StaticBlockID=%d", _staticBlockID.GetValue());
      _staticBlockID.UnSet();
      return;
    }
    UpdateBlockPlacementOffsets();
    
  }
  
  // If the base blocks are set, remove them as possibilites for the top block
  if(_baseBlockID.IsSet() && _staticBlockID.IsSet()){
    SafeEraseBlockFromBlockList(_baseBlockID, allBlocks);
    SafeEraseBlockFromBlockList(_staticBlockID, allBlocks);
  }
  
  // find the top block so the full pyramid construction can run
  if(!allBlocks.empty() && !_topBlockID.IsSet()){
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
    if(staticBlock){
      auto staticBlockPose = staticBlock->GetPose().GetWithRespectToOrigin();
      _topBlockID = GetNearestBlockToPose(staticBlockPose, allBlocks);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::UpdateBlockPlacementOffsets() const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdateBlockPlacementOffset.NullObject",
                        "Static block with id %d is NULL", _staticBlockID.GetValue());
    return;
  }
  
  // Construct a list of valid Placement poses next to the block
  f32 nearestDistanceSQ = FLT_MAX;
  f32 blockSize = object->GetSize().y() + kCubeSizeBuffer_mm;
  using Offsets =  std::pair<float,float>;
  std::vector<Offsets> offsetList;
  offsetList.emplace_back(blockSize, 0);
  offsetList.emplace_back(0, blockSize);
  offsetList.emplace_back(0, -blockSize);
  offsetList.emplace_back(-blockSize, 0);

  for(const auto& entry: offsetList){
    if(CheckBaseBlockPoseIsFree(entry.first, entry.second)){
      Pose3d validPose = Pose3d(0.f, Z_AXIS_3D(), {entry.first, entry.second, 0.f}, &object->GetPose());
      f32 newDistSquared;
      ComputeDistanceSQBetween(_robot.GetPose(), validPose, newDistSquared);
      
      if(newDistSquared < nearestDistanceSQ){
        nearestDistanceSQ = newDistSquared;
        _baseBlockOffsetX = entry.first;
        _baseBlockOffsetY = entry.second;
      
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  const ObservableObject* placingObject = _robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  if(nullptr == object || nullptr == placingObject)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.CHeckBaseBlockPoseIsFree.NullObject",
                        "Static block with id %d or base block with id %d is NULL", _staticBlockID.GetValue(), _baseBlockID.GetValue());
    return false;
  }
  
  
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  filter.SetIgnoreIDs({_staticBlockID});
  
  // get the size of the placing object
  const float xSize = placingObject->GetSize().x();
  const float ySize = placingObject->GetSize().y();
  const float zSize = placingObject->GetSize().z();
  
  Pose3d placePose = Pose3d(0.f, Z_AXIS_3D(), {xOffset, yOffset,0.f}, &object->GetPose());
  ObservableObject* closestObject = _robot.GetBlockWorld().FindObjectClosestTo(placePose, {xSize, ySize, zSize}, filter);
  return closestObject == nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::ResetPyramidTargets(const Robot& robot) const
{
  _staticBlockID.UnSet();
  _baseBlockID.UnSet();
  _topBlockID.UnSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorBuildPyramidBase::GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const
{
  f32 shortestDistanceSQ = -1.f;
  f32 currentDistance = -1.f;
  ObjectID nearestObject;
  
  //Find nearest block to pickup
  for(auto block: allBlocks){
    auto blockPose = block->GetPose().GetWithRespectToOrigin();
    f32 newDistSquared;
    ComputeDistanceSQBetween(pose, blockPose, newDistSquared);
    if(currentDistance < shortestDistanceSQ || shortestDistanceSQ < 0){
      shortestDistanceSQ = currentDistance;
      nearestObject = block->GetID();
    }
  }
  
  return nearestObject;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SafeEraseBlockFromBlockList(const ObjectID& objectID, BlockList& blockList) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(objectID);
  auto iter = std::find(blockList.begin(), blockList.end(), object);
  if(iter != blockList.end()){
    iter = blockList.erase(iter);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::AreAllBlockIDsUnique() const
{
  return (_topBlockID != _staticBlockID &&
          _topBlockID != _baseBlockID &&
          _staticBlockID != _baseBlockID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag()){
    case ExternalInterface::MessageEngineToGameTag::RobotDelocalized:
    {
      SmartRemoveCustomLightPattern(_staticBlockID);
      SmartRemoveCustomLightPattern(_baseBlockID);
      SmartRemoveCustomLightPattern(_topBlockID);
      break;
    }
    default:
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.HandleWhileRunning.UnknownTag",
                          "Receviede unexpected tag");
      break;
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetPickupInitialBlockLights()
{
  SmartSetCustomLightPattern(_staticBlockID, GetSingleStaticBlockLights());
  SmartSetCustomLightPattern(_baseBlockID, GetBaseFormedTopLights());

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetPyramidBaseLightsByID(Robot& robot, const ObjectID& staticID, const ObjectID& baseID)
{
  robot.GetLightsComponent().SetCustomLightPattern(baseID,
                              GetBaseFormedBaseLights(robot, staticID, baseID), kCustomLightLockName);
  robot.GetLightsComponent().SetCustomLightPattern(staticID,
                              GetBaseFormedStaticLights(robot, staticID, baseID), kCustomLightLockName);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetPyramidBaseLights()
{
  SmartRemoveCustomLightPattern(_staticBlockID);
  SmartRemoveCustomLightPattern(_baseBlockID);
  SmartRemoveCustomLightPattern(_topBlockID);
  
  SmartSetCustomLightPattern(_baseBlockID, GetBaseFormedBaseLights(_robot, _staticBlockID, _baseBlockID));
  SmartSetCustomLightPattern(_staticBlockID, GetBaseFormedStaticLights(_robot, _staticBlockID, _baseBlockID));
  SmartSetCustomLightPattern(_topBlockID, GetBaseFormedTopLights());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetFullPyramidLights()
{
  SmartRemoveCustomLightPattern(_staticBlockID);
  SmartRemoveCustomLightPattern(_baseBlockID);
  SmartRemoveCustomLightPattern(_topBlockID);
  
  SmartSetCustomLightPattern(_staticBlockID, GetFullPyramidLights(_robot));
  SmartSetCustomLightPattern(_baseBlockID, GetFullPyramidLights(_robot));
  SmartSetCustomLightPattern(_topBlockID, GetFullPyramidLights(_robot));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetPyramidFlourishLights()
{
  SmartRemoveCustomLightPattern(_staticBlockID);
  SmartRemoveCustomLightPattern(_baseBlockID);
  SmartRemoveCustomLightPattern(_topBlockID);
  
  SmartSetCustomLightPattern(_staticBlockID, GetFlourishStaticLights());
  SmartSetCustomLightPattern(_baseBlockID, GetFlourishBaseLights());
  SmartSetCustomLightPattern(_topBlockID, GetFlourishTopLights());
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetSingleStaticBlockLights()
{
  static const ObjectLights kSingleStaticBlockLights = {
    //clock of marker, marker, counter clock marker, across
    .onColors               = {{NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{kSingleTimeOn,kSingleTimeOn,kSingleTimeOn,kSingleTimeOn}},
    .offPeriod_ms           = {{kSingleTimeOff,kSingleTimeOff,kSingleTimeOff,kSingleTimeOff}},
    .transitionOnPeriod_ms  = {{kSingleTransitionOn,kSingleTransitionOn, kSingleTransitionOn,kSingleTransitionOn}},
    .transitionOffPeriod_ms = {{kSingleTransitionOff,kSingleTransitionOff,kSingleTransitionOff, kSingleTransitionOn}},
    .offset                 = {{0,kSingleTimeOn,2*kSingleTimeOn,3*kSingleTimeOn}},
    .rotationPeriod_ms      = 0,
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE,
    .relativePoint          = {0,0}
  };
  
  return kSingleStaticBlockLights;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetBaseFormedLights()
{
  static const ObjectLights kBaseFormedLights= {
    //clock of marker, marker, counter clock marker, across
    //.onColors               = {{NamedColors::GREEN, NamedColors::BLACK, NamedColors::GREEN, NamedColors::GREEN}},
    .onColors               = {{NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{kBaseFormedTimeOn,kBaseFormedTimeOn,kBaseFormedTimeOn,kBaseFormedTimeOn}},
    .offPeriod_ms           = {{kBaseFormedTimeOff,kBaseFormedTimeOff,kBaseFormedTimeOff,kBaseFormedTimeOff}},
    .transitionOnPeriod_ms  = {{kBaseFormedTransitionOn,kBaseFormedTransitionOn, kBaseFormedTransitionOn,kBaseFormedTransitionOn}},
    .transitionOffPeriod_ms = {{kBaseFormedTransitionOff,kBaseFormedTransitionOff,kBaseFormedTransitionOff, kBaseFormedTransitionOn}},
    //.offset                 = {{kBaseFormedTimeOn,0,kBaseFormedTimeOn*5,0}},
    .offset                 = {{0,0,0,0}},
    .rotationPeriod_ms      = 0,
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE,
    .relativePoint          = {0,0}
  };
  return kBaseFormedLights;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectLights BehaviorBuildPyramidBase::GetBaseFormedBaseLights(Robot& robot, const ObjectID& staticID, const ObjectID& baseID)
{
  ObjectLights baseBlockLights = GetBaseFormedLights();

  
  const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(baseID);
  if(baseBlock == nullptr){
    return baseBlockLights;
  }
  
  using namespace BlockConfigurations;
  
  // To account for block world ambiguity, use normalized offset instead of direct coordinates of other block
  PyramidBase baseForOffset = PyramidBase(baseID, staticID); // purposefully reversed for base offset
  Point2f baseOffsets = baseForOffset.GetBaseBlockIdealOffsetValues(robot);
  const float kOffsetMultiplier = 1;
  Pose3d offsetPose = Pose3d(0, Z_AXIS_3D(), {kOffsetMultiplier * baseOffsets.x(), kOffsetMultiplier * baseOffsets.y(), 0}, &baseBlock->GetPose());
  
  baseBlockLights.relativePoint = {offsetPose.GetWithRespectToOrigin().GetTranslation().x(), offsetPose.GetWithRespectToOrigin().GetTranslation().y()};
  //baseBlockLights.offset = {{kBaseFormedTimeOn*4,0,kBaseFormedTimeOn*2,kBaseFormedTimeOn*3}};
  baseBlockLights.offset = {{kBaseFormedTimeOn*2,kBaseFormedTimeOn*2,kBaseFormedTimeOn*2,kBaseFormedTimeOn*2}};

  return baseBlockLights;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectLights BehaviorBuildPyramidBase::GetBaseFormedStaticLights(Robot& robot, const ObjectID& staticID, const ObjectID& baseID)
{
  ObjectLights staticBlockLights = GetBaseFormedLights();

  const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(staticID);
  if(staticBlock == nullptr){
    return staticBlockLights;
  }
 
  using namespace BlockConfigurations;

  PyramidBase baseForOffset = PyramidBase(staticID, baseID);
  Point2f baseOffsets = baseForOffset.GetBaseBlockIdealOffsetValues(robot);
  const float kOffsetMultiplier = 1;
  Pose3d offsetPose = Pose3d(0, Z_AXIS_3D(), {kOffsetMultiplier * baseOffsets.x(), kOffsetMultiplier * baseOffsets.y(), 0}, &staticBlock->GetPose());
  
  
  staticBlockLights.relativePoint = {offsetPose.GetWithRespectToOrigin().GetTranslation().x(), offsetPose.GetWithRespectToOrigin().GetTranslation().y()};
  
  return staticBlockLights;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetBaseFormedTopLights()
{
  static ObjectLights kFullTopLights= {
    //clock of marker, marker, counter clock marker, across
    .onColors               = {{NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{kFullPyramidTimeOn,kFullPyramidTimeOn,kFullPyramidTimeOn,kFullPyramidTimeOn}},
    .offPeriod_ms           = {{0,0,0,0}},
    .transitionOnPeriod_ms  = {{0, 0, 0, 0}},
    .transitionOffPeriod_ms = {{0, 0, 0, 0}},
    .offset                 = {{0, 0, 0, 0}},
    .rotationPeriod_ms      = 0,
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE,
    .relativePoint          = {0,0}
  };
  
  return kFullTopLights;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetFullPyramidLights(Robot& robot)
{
  static ColorRGBA lightGreen =ColorRGBA(0.f, 0.5f, 0.f);
  static ObjectLights kFullBaseLights= {
    //clock of marker, marker, counter clock marker, across
    .onColors               = {{NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN}},
    .offColors              = {{lightGreen, lightGreen, lightGreen, lightGreen}},
    .onPeriod_ms            = {{kFullPyramidTimeOn,kFullPyramidTimeOn,kFullPyramidTimeOn,kFullPyramidTimeOn}},
    .offPeriod_ms           = {{kFullPyramidTimeOff,kFullPyramidTimeOff,kFullPyramidTimeOff,kFullPyramidTimeOff}},
    .transitionOnPeriod_ms  = {{kFullPyramidTransitionOn, kFullPyramidTransitionOn, kFullPyramidTransitionOn, kFullPyramidTransitionOn}},
    .transitionOffPeriod_ms = {{kFullPyramidTransitionOff, kFullPyramidTransitionOff, kFullPyramidTransitionOff, kFullPyramidTransitionOff}},
    .offset                 = {{0, kFullPyramidTimeOn, kFullPyramidTimeOn*2, kFullPyramidTimeOn*3}},
    .rotationPeriod_ms      = 0,
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_OFF,
    .relativePoint          = {0,0}
  };
  
  return kFullBaseLights;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetFlourishBaseLights() const
{
  static ObjectLights kFlourishBaseLights= {
    //clock of marker, marker, counter clock marker, across
    .onColors               = {{NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN, NamedColors::GREEN}},
    .offColors              = {{NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN}},
    .onPeriod_ms            = {{kPyramidFlourishShutoff_sec*4,kPyramidFlourishShutoff_sec*5,kPyramidFlourishShutoff_sec*6,kPyramidFlourishShutoff_sec*7}},
    .offPeriod_ms           = {{kPyramidFlourishTimeOff,kPyramidFlourishTimeOff,kPyramidFlourishTimeOff,kPyramidFlourishTimeOff}},
    .transitionOnPeriod_ms  = {{0, 0, 0, 0}},
    .transitionOffPeriod_ms = {{0, 0, 0, 0}},
    .offset                 = {{0, 0, 0, 0}},
    .rotationPeriod_ms      = 0,
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE,
    .relativePoint          = {0, 0}
  };
  

  return kFlourishBaseLights;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetFlourishStaticLights() const
{
  return GetFlourishBaseLights();
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObjectLights& BehaviorBuildPyramidBase::GetFlourishTopLights() const
{
  static ObjectLights kFlourishTopLights= GetFlourishBaseLights();
  kFlourishTopLights.onPeriod_ms = {{kPyramidFlourishShutoff_sec, kPyramidFlourishShutoff_sec*2, kPyramidFlourishShutoff_sec*3, kPyramidFlourishShutoff_sec*4}};
  
  return kFlourishTopLights;

}

} //namespace Cozmo
} //namespace Anki
