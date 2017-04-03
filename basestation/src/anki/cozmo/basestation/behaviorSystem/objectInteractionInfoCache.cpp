/**
 * File: objectInteractionInfoCache.cpp
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Cache which keeps track of the valid and best objects to use
 * with certain object interaction intentions.  Updated lazily for performance
 * improvements
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"

#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
const int kMaxStackHeightReach = 2;
}
  
using ObjectActionFailure = AIWhiteboard::ObjectActionFailure;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Consts for mapping ObjectInteractionIntentions to their filters
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
using DependentIntentionArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray
                     <ObjectInteractionIntention,
                      const std::set<ObjectInteractionIntention>*,
                      ObjectInteractionIntention::Count>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray;
  
static const std::set<ObjectInteractionIntention> empty;
static const std::set<ObjectInteractionIntention> pyramidStaticDependent =
                                {ObjectInteractionIntention::PyramidBaseObject};
static const std::set<ObjectInteractionIntention> pyramidTopDependent =
                                {ObjectInteractionIntention::PyramidBaseObject,
                                 ObjectInteractionIntention::PyramidStaticObject};

constexpr DependentIntentionArray kDependentIntentionMap = {
  {ObjectInteractionIntention::PickUpAnyObject,                   &empty},
  {ObjectInteractionIntention::PickUpObjectWithAxisCheck,         &empty},
  {ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck, &empty},
  {ObjectInteractionIntention::RollObjectWithDelegateAxisCheck,   &empty},
  {ObjectInteractionIntention::PopAWheelieOnObject,               &empty},
  {ObjectInteractionIntention::PyramidBaseObject,                 &empty},
  {ObjectInteractionIntention::PyramidStaticObject,               &pyramidStaticDependent},
  {ObjectInteractionIntention::PyramidTopObject   ,               &pyramidTopDependent}
};
    
static_assert(IsSequentialArray(kDependentIntentionMap),
                "DependentIntentionMap missing missing entry or not ordered");
  
static const float kInvalidObjectCacheUpdateTime_s = -1.0f;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCache
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectInteractionInfoCache::ObjectInteractionInfoCache(const Robot& robot)
: _robot(robot)
{
  
  // Pickup no axis check
  BlockWorldFilter *pickupAnyFilter = new BlockWorldFilter;
  pickupAnyFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  pickupAnyFilter->AddFilterFcn( std::bind(&ObjectInteractionInfoCache::CanPickupHelper,
                                           this, std::placeholders::_1));
  
  // Pickup with axis check
  BlockWorldFilter *pickupWithAxisFilter = new BlockWorldFilter;
  pickupWithAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  pickupWithAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanPickupWithAxisHelper,
                                               this, std::placeholders::_1));
  
  // Roll object with delegate - no axis check
  BlockWorldFilter *rollNoAxisFilter = new BlockWorldFilter;
  rollNoAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  rollNoAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanRollObjectDelegateNoAxisHelper,
                                           this, std::placeholders::_1));
  
  // Roll object with delegate - axis check
  BlockWorldFilter *rollWithAxisFilter = new BlockWorldFilter;
  rollWithAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  rollWithAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanRollObjectDelegateWithAxisHelper,
                                             this, std::placeholders::_1));
  
  // Function for selecting the best object for rolling
  auto rollObjectBestObjectFunc = std::bind(&ObjectInteractionInfoCache::RollBlockBestObjectFilter,
                                            this, std::placeholders::_1);
  
  // Pop A Wheelie check
  BlockWorldFilter *popFilter = new BlockWorldFilter;
  popFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  popFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanPopAWheelieHelper,
                                    this, std::placeholders::_1));
  
  // Pyramid Base object
  BlockWorldFilter *baseFilter = new BlockWorldFilter;
  baseFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  baseFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanUseAsBuildPyramidBaseBlock,
                                     this, std::placeholders::_1));
  
  // Pyramid Static object
  BlockWorldFilter *staticFilter = new BlockWorldFilter;
  staticFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  staticFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanUseAsBuildPyramidStaticBlock,
                                       this, std::placeholders::_1));
  
  // Pyramid Top object
  BlockWorldFilter *topFilter = new BlockWorldFilter;
  topFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  topFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanUseAsBuildPyramidTopBlock,
                                    this, std::placeholders::_1));
  
  
  FullValidInteractionArray validInteractionFilters = {
    {ObjectInteractionIntention::PickUpAnyObject,                   pickupAnyFilter},
    {ObjectInteractionIntention::PickUpObjectWithAxisCheck,         pickupWithAxisFilter},
    {ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck, rollNoAxisFilter},
    {ObjectInteractionIntention::RollObjectWithDelegateAxisCheck,   rollWithAxisFilter},
    {ObjectInteractionIntention::PopAWheelieOnObject,               popFilter},
    {ObjectInteractionIntention::PyramidBaseObject,                 baseFilter},
    {ObjectInteractionIntention::PyramidStaticObject,               staticFilter},
    {ObjectInteractionIntention::PyramidTopObject,                  topFilter}
  };
  
  // Function for selecting the best object given distance/position in configurations
  auto defaultBestObjectFunc = std::bind(&ObjectInteractionInfoCache::DefaultBestObjectFilter,
                                            this, std::placeholders::_1);

  
  FullBestInteractionArray findBestObjectFunctions = {
    {ObjectInteractionIntention::PickUpAnyObject,                   defaultBestObjectFunc},
    {ObjectInteractionIntention::PickUpObjectWithAxisCheck,         defaultBestObjectFunc},
    {ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck, rollObjectBestObjectFunc},
    {ObjectInteractionIntention::RollObjectWithDelegateAxisCheck,   rollObjectBestObjectFunc},
    {ObjectInteractionIntention::PopAWheelieOnObject,               defaultBestObjectFunc},
    {ObjectInteractionIntention::PyramidBaseObject,                 defaultBestObjectFunc},
    {ObjectInteractionIntention::PyramidStaticObject,               defaultBestObjectFunc},
    {ObjectInteractionIntention::PyramidTopObject,                  defaultBestObjectFunc}
  };

  
  ConfigureObjectInteractionFilters(robot, validInteractionFilters, findBestObjectFunctions);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectInteractionInfoCache::ConfigureObjectInteractionFilters(const Robot& robot,
                                                                   FullValidInteractionArray validInteractions,
                                                                   FullBestInteractionArray  bestInteractions)
{
  auto interactionEnum = 0;
  while(static_cast<ObjectInteractionIntention>(interactionEnum) != ObjectInteractionIntention::Count){
    auto validFilter = validInteractions[interactionEnum];
    auto bestFilter  = bestInteractions[interactionEnum];
    _trackers.emplace(validFilter.EnumValue(),
                      ObjectInteractionCacheEntry(robot,
                                                  ObjectUseIntentionToString(validFilter.EnumValue()),
                                                  validFilter.Value(),
                                                  bestFilter.Value()));
    interactionEnum++;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ObjectInteractionInfoCache::GetBestObjectForIntention(
                                        ObjectInteractionIntention intention)
{
  EnsureInformationValid(intention);
  return _trackers.find(intention)->second.GetBestObject();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<ObjectID> ObjectInteractionInfoCache::GetValidObjectsForIntention(
                                        ObjectInteractionIntention intention)
{
  EnsureInformationValid(intention);
  return _trackers.find(intention)->second.GetValidObjects();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockWorldFilter& ObjectInteractionInfoCache::GetDefaultFilterForIntention(
                                          ObjectInteractionIntention intention)
{
  return _trackers.find(intention)->second.GetValidObjectsFilter();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::IsObjectValidForInteraction(ObjectInteractionIntention intention,
                                                             const ObjectID& object)
{
  auto validObjs = GetValidObjectsForIntention(intention);
  
  const auto objectIter = validObjs.find(object);
  // the object is valid if and only if it is in our valid set
  return objectIter != validObjs.end();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectInteractionInfoCache::EnsureInformationValid(ObjectInteractionIntention intention)
{
  // Ensure all intentions this intention depends on are valid
  const int intentionIdx = Util::numeric_cast<int>(intention);
  auto dependentIntentions = *(kDependentIntentionMap[intentionIdx].Value());
  for(const auto& dependentIntention : dependentIntentions){
    EnsureInformationValid(dependentIntention);
  }
  
  _trackers.find(intention)->second.EnsureInformationValid();
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectInteractionInfoCache::ObjectTapInteractionOccurred(const ObjectID& objectID)
{
  bool filterCanUseObject = false;

  for(auto& entry: _trackers){
    filterCanUseObject = filterCanUseObject ||
             entry.second.ObjectTapInteractionOccurred(objectID);
  }
  
  // None of the action intention filters can currently use objectID but still
  // we still have a tap intention object because ReactToDoubleTap can run and
  // determine if we can actually use the the tapped object
  if(!filterCanUseObject)
  {
    PRINT_CH_INFO("AIWhiteBoard", "SetObjectTapInteration.NoFilter",
                  "No actionIntent filter can currently use object %u",
                  objectID.GetValue());
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void  ObjectInteractionInfoCache::InvalidateAllIntents()
{
  for(auto& entry: _trackers){
    entry.second.Invalidate();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ObjectInteractionInfoCache::ObjectUseIntentionToString(ObjectInteractionIntention intention)
{
  switch(intention) {
    case ObjectInteractionIntention::PickUpAnyObject: { return "PickUpAnyObject"; }
    case ObjectInteractionIntention::PickUpObjectWithAxisCheck: { return "PickUpObjectWithAxisCheck"; }
    case ObjectInteractionIntention::RollObjectWithDelegateAxisCheck: { return "RollObjectWithDelegateAxisCheck"; }
    case ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck: { return "RollObjectWithDelegateNoAxisCheck"; }
    case ObjectInteractionIntention::PopAWheelieOnObject: { return "PopAWheelieOnObject"; }
    case ObjectInteractionIntention::PyramidBaseObject: { return "PyramidBaseObject"; }
    case ObjectInteractionIntention::PyramidStaticObject: { return "PyramidStaticObject"; }
    case ObjectInteractionIntention::PyramidTopObject: { return "PyramidTopObject"; }
    case ObjectInteractionIntention::Count:
      { DEV_ASSERT(false, "ObjectInteractionInfoCache.InvalidIntention"); return "";}
  };
  
  // should never get here, assert if it does (programmer error specifying intention enum class)
  DEV_ASSERT(false, "AIWhiteboard.ObjectUseIntentionToString.InvalidIntention");
  return "UNDEFINED_ERROR";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanPickupHelper(const ObservableObject* object) const
{
  if( nullptr == object ) {
    PRINT_NAMED_ERROR("AIWhiteboard.CanPickupHelper.NullObject", "object was null");
    return false;
  }
  
  // check for recent failures
  auto& whiteboard = _robot.GetAIComponent().GetWhiteboard();
  const bool recentlyFailed = whiteboard.DidFailToUse(object->GetID(),
                                           {{ ObjectActionFailure::PickUpObject, ObjectActionFailure::RollOrPopAWheelie }},
                                           DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                           object->GetPose().GetWithRespectToOrigin(),
                                           DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                           DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  const bool canPickUp = _robot.CanPickUpObject(*object);
  return !recentlyFailed && canPickUp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanPickupWithAxisHelper(const ObservableObject* object) const
{
  if( ! CanPickupHelper(object) ) {
    return false;
  }
  
  // check the up axis
  const bool forFreeplay = true;
  const bool isRollingUnlocked = _robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube,forFreeplay);
  const bool upAxisOk = ! isRollingUnlocked ||
  object->GetPose().GetWithRespectToOrigin().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;
  
  return upAxisOk;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanPopAWheelieHelper(const ObservableObject* object) const
{
  auto& whiteboard = _robot.GetAIComponent().GetWhiteboard();

  const bool hasFailedToPopAWheelie = whiteboard.DidFailToUse(object->GetID(),
                                                   AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie,
                                                   DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                                   object->GetPose(),
                                                   DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                                   DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  return (!hasFailedToPopAWheelie && _robot.CanPickUpObjectFromGround(*object));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanRollObjectDelegateNoAxisHelper(const ObservableObject* object) const
{
  auto& whiteboard = _robot.GetAIComponent().GetWhiteboard();
  
  const bool hasFailedToRoll = whiteboard.DidFailToUse(object->GetID(),
                                            AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie,
                                            DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                            object->GetPose(),
                                            DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                            DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  if(!hasFailedToRoll){
    // Logic from can interact with - unfortunately those helpers check for on top of
    // which isn't relevant for roll delegate so we check the relevant properties
    // directly here
    if( object->GetFamily() != ObjectFamily::Block &&
       object->GetFamily() != ObjectFamily::LightCube ) {
      return false;
    }
    
    // Only roll blocks that are resting flat
    if( !object->IsRestingFlat() ) {
      return false;
    }
    
    Pose3d wasted;
    // check if we can transform to robot space
    if ( !object->GetPose().GetWithRespectTo(_robot.GetPose(), wasted) ) {
      return false;
    }
    
    return true;
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanRollObjectDelegateWithAxisHelper(const ObservableObject* object) const
{
  const Pose3d p = object->GetPose().GetWithRespectToOrigin();
  return (CanRollObjectDelegateNoAxisHelper(object) &&
          (p.GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS));
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsBuildPyramidBaseBlock(const ObservableObject* object) const
{
  const auto& pyramidBases = _robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = _robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  for(const auto& pyramid: pyramids){
    if(object->GetID() == pyramid->GetPyramidBase().GetBaseBlockID()){
      return true;
    }
  }
  
  // If a pyramid exists and this object doesn't match the base, wait to assign that object
  if(!pyramids.empty()){
    return false;
  }
  
  for(const auto& pyramidBase: pyramidBases){
    if(object->GetID() == pyramidBase->GetBaseBlockID()){
      return true;
    }
  }
  
  // If a base exists and this object doesn't match the base, wait to assign that object
  if(!pyramidBases.empty()){
    return false;
  }
  
  
  // If there is a stack of 2, the top block should be selected as the base of the pyramid
  const auto& stacks = _robot.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetStacks();
  for(const auto& stack: stacks){
    if(stack->GetStackHeight() == kMaxStackHeightReach &&
       stack->GetTopBlockID() == object->GetID()){
      return _robot.CanPickUpObject(*object);
    }
  }
  
  // If the robot is carrying a block, make that the static block
  if(_robot.IsCarryingObject()){
    return _robot.GetCarryingObject() == object->GetID();
  }
  
  if(!stacks.empty()){
    return false;
  }
  
  // So long as we can pick the object up, it's a valid base block
  return _robot.CanPickUpObject(*object);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsBuildPyramidStaticBlock(const ObservableObject* object)
{
  // Base block must be set before static block can be set
  auto bestBaseBlock = GetBestObjectForIntention(ObjectInteractionIntention::PyramidBaseObject);
  if(!bestBaseBlock.IsSet() || (bestBaseBlock == object->GetID())){
    return false;
  }
  
  const auto& pyramidBases = _robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = _robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  for(const auto& pyramid: pyramids){
    if((object->GetID() == pyramid->GetPyramidBase().GetStaticBlockID()) &&
       (bestBaseBlock == pyramid->GetPyramidBase().GetBaseBlockID())){
      return true;
    }
  }
  
  // If a pyramid exists and this object doesn't match the static, wait to assign that object
  if(!pyramids.empty()){
    return false;
  }
  
  for(const auto& pyramidBase: pyramidBases){
    if((object->GetID() == pyramidBase->GetStaticBlockID()) &&
       (bestBaseBlock == pyramidBase->GetBaseBlockID())){
      return true;
    }
  }
  
  // If a base exists and this object doesn't match the static, wait to assign that object
  if(!pyramidBases.empty()){
    return false;
  }
  
  if(!object->IsRestingAtHeight(0, BlockWorld::kOnCubeStackHeightTolerance)){
    return false;
  }
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsBuildPyramidTopBlock(const ObservableObject* object)
{
  // Base and static blocks must be set before top block can be set
  auto bestBaseBlock = GetBestObjectForIntention(ObjectInteractionIntention::PyramidBaseObject);
  auto bestStaticBlock = GetBestObjectForIntention(ObjectInteractionIntention::PyramidStaticObject);
  
  if(!bestBaseBlock.IsSet() ||
     !bestStaticBlock.IsSet() ||
     (bestBaseBlock == object->GetID()) ||
     (bestStaticBlock == object->GetID())){
    return false;
  }
  
  // If the robot is carrying a block, which is not needed for the base
  // make that the TopBlock
  if(_robot.IsCarryingObject()){
    return _robot.GetCarryingObject() == object->GetID();
  }
  
  return _robot.CanPickUpObject(*object);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ObjectInteractionInfoCache::DefaultBestObjectFilter(const std::set<ObjectID>& validObjects)
{
  const ObservableObject* bestObj = nullptr;
  f32 shortestDistSQ = FLT_MAX;
  f32 currentDistSQ = FLT_MAX;
  
  // Give preference to blocks on top of the stack rather than the bottom
  const auto& stacks = _robot.GetBlockWorld().GetBlockConfigurationManager().
                                                 GetStackCache().GetStacks();
  for(const auto& stack: stacks){
    const ObservableObject* topBlock =  _robot.GetBlockWorld().GetLocatedObjectByID(
                                                     stack->GetTopBlockID());

    if(ComputeDistanceSQBetween(_robot.GetPose(), topBlock->GetPose(), currentDistSQ) &&
       (currentDistSQ < shortestDistSQ)){
      bestObj = topBlock;
      shortestDistSQ = currentDistSQ;
    }
  }
  
  

  for(const auto& objID: validObjects){
    const ObservableObject* obj =  _robot.GetBlockWorld().GetLocatedObjectByID(objID);
    if(obj == nullptr){
      continue;
    }
    
    // if the block is the base of a stack and we have any other object already
    // set, give the other object preference
    bool isBottomBlock = false;
    for(const auto& stack: stacks){
      if(obj->GetID() == stack->GetBottomBlockID()){
        isBottomBlock = true;
        break;
      }
    }
    if(bestObj != nullptr && isBottomBlock){
      continue;
    }
    
    // If this block is the closest block, use it as the best available
    if(ComputeDistanceSQBetween(_robot.GetPose(), obj->GetPose(), currentDistSQ) &&
       (currentDistSQ < shortestDistSQ)){
      bestObj = obj;
      shortestDistSQ = currentDistSQ;
    }
  }
  
  if(bestObj != nullptr){
    return bestObj->GetID();
  }else{
    return {};
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ObjectInteractionInfoCache::RollBlockBestObjectFilter(const std::set<ObjectID>& validObjects)
{
  // Use the lowest cost object as a default
  ObjectID lowestCostID = DefaultBestObjectFilter(validObjects);
  const ObservableObject* lowestCostObj =  _robot.GetBlockWorld().
                                             GetLocatedObjectByID(lowestCostID);
  
  // If the lowest cost object is upright, see if any other objects are
  // non-upright and prefer those instead
  const bool lowestCostUpright = lowestCostObj == nullptr ||
    (lowestCostObj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>()
                                                            == AxisName::Z_POS);
  
  if(lowestCostUpright){
    f32 shortestDistSQ = FLT_MAX;
    f32 currentDistSQ = FLT_MAX;
    
    for(const auto& objID: validObjects){
      const ObservableObject* validObj =  _robot.GetBlockWorld().
                                             GetLocatedObjectByID(objID);
      if(validObj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>()
                                                            == AxisName::Z_POS){
        continue;
      }
    
      if(ComputeDistanceSQBetween(_robot.GetPose(), validObj->GetPose(), currentDistSQ) &&
         (currentDistSQ < shortestDistSQ)){
        lowestCostID = validObj->GetID();
        shortestDistSQ = currentDistSQ;
      }
    }
  }
  
  return lowestCostID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCacheEntry
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectInteractionCacheEntry::ObjectInteractionCacheEntry(const Robot& robot,
                                                         const std::string& debugName,
                                                         BlockWorldFilter* validFilter,
                                                         BestObjectFunction bestObjFunc)
: _robot(robot)
, _debugName(debugName)
, _blockWorldFilterValidBlocks(validFilter)
, _bestObjFunc(bestObjFunc)
, _timeUpdated_s(kInvalidObjectCacheUpdateTime_s)
{
  DEV_ASSERT(_blockWorldFilterValidBlocks != nullptr,
             "ObjectInteractionCacheEntry.InvalidValidFilterPtr");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectInteractionCacheEntry::EnsureInformationValid()
{
  const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(!FLT_NEAR(_timeUpdated_s, currentTimeInSeconds)){
    _timeUpdated_s = currentTimeInSeconds;
    _validObjects.clear();
    
    std::vector<const ObservableObject*> objects;
    const auto& blockWorld = _robot.GetBlockWorld();
    blockWorld.FindLocatedMatchingObjects(*_blockWorldFilterValidBlocks, objects);
    for( const ObservableObject* obj : objects ) {
      if( obj ) {
        _validObjects.insert(obj->GetID());
      }
    }
    
    const auto& whiteboard = _robot.GetAIComponent().GetWhiteboard();
    const bool haveTapIntentionObject = whiteboard.HasTapIntent();
    const bool bestIsStillValid = _validObjects.find(_bestObject) != _validObjects.end();
    
    // Only update _bestObjectForAction if we don't have a tap object,
    // or if the tap object became invalid
    if(!haveTapIntentionObject || !bestIsStillValid)
    {
      // select best object
      const ObjectID bestObjID = _bestObjFunc(_validObjects);
      if( bestObjID.IsSet() ) {
        
        if( haveTapIntentionObject && _bestObject.IsSet() ) {
          PRINT_CH_INFO("AIWhiteboard", "EnsureInformationValid.ResetBestTapped",
                        "We have a tap intent, but object id %d is no longer valid \
                        for action %s, selecting object %d instead",
                        _bestObject.GetValue(),
                        _debugName.c_str(),
                        bestObjID.GetValue());
        }
        
        _bestObject = bestObjID;
      }
      else {
        
        if( haveTapIntentionObject && _bestObject.IsSet() ) {
          PRINT_CH_INFO("AIWhiteboard", "EnsureInformationValid.ClearBestTapped",
                        "We have a tap intent, but object id %d is no longer valid, \
                        clearing best for action %s",
                        _bestObject.GetValue(),
                        _debugName.c_str());
        }
        
        _bestObject.UnSet();
      }
    }
    
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ObjectInteractionCacheEntry::GetBestObject() const
{
#if ANKI_DEV_CHEATS
  const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  DEV_ASSERT(FLT_NEAR(_timeUpdated_s, currentTimeInSeconds),
             "ObjectInteractionCachEntry.GetBestObjects.\
             AttemptingToAccessObjectWithoutEnsuringInformationValid");
#endif
  return _bestObject;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set< ObjectID > ObjectInteractionCacheEntry::GetValidObjects() const
{
#if ANKI_DEV_CHEATS
  const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  DEV_ASSERT(FLT_NEAR(_timeUpdated_s, currentTimeInSeconds),
             "ObjectInteractionCachEntry.GetValidObjects.\
             AttemptingToAccessObjectWithoutEnsuringInformationValid");
#endif
  
  return _validObjects;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionCacheEntry::ObjectTapInteractionOccurred(const ObjectID& objectID)
{
  const ObservableObject* locatedObject =
  _robot.GetBlockWorld().GetLocatedObjectByID(objectID);
  
  const ObjectID oldBest = _bestObject;
  _bestObject.UnSet();
  
  // consider located instance only, if we don't know where it is we can't use it
  if((nullptr != locatedObject) &&
     _blockWorldFilterValidBlocks->ConsiderObject(locatedObject))
  {
    
    if( oldBest != objectID ) {
      PRINT_CH_INFO("AIWhiteboard", "SetBestObjectForTap",
                    "Setting tapped object %d as best for intention %s",
                    objectID.GetValue(),
                    _debugName.c_str());
    }
    
    _bestObject = objectID;
    return true;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectInteractionCacheEntry::Invalidate()
{
  _timeUpdated_s = kInvalidObjectCacheUpdateTime_s;
}


} // namespace Cozmo
} // namespace Anki
