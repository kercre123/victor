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

#include "engine/aiComponent/objectInteractionInfoCache.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace {
static const float kInvalidObjectCacheUpdateTime_s = -1.0f;
static const float kTimeObjectInvalidAfterStackFailure_sec = 3.0f;
static const Radians kAngleToleranceAfterFailure_radians = M_PI;
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
  
// stack dependencies
static const std::set<ObjectInteractionIntention> stackBottomDependentAxis =
                                {ObjectInteractionIntention::StackTopObjectAxisCheck};
static const std::set<ObjectInteractionIntention> stackBottomDependentNoAxis =
                                {ObjectInteractionIntention::StackTopObjectNoAxisCheck};
  
constexpr DependentIntentionArray kDependentIntentionMap = {
  {ObjectInteractionIntention::PickUpObjectNoAxisCheck,           &empty},
  {ObjectInteractionIntention::PickUpObjectAxisCheck,             &empty},
  {ObjectInteractionIntention::StackBottomObjectAxisCheck,        &stackBottomDependentAxis},
  {ObjectInteractionIntention::StackBottomObjectNoAxisCheck,      &stackBottomDependentNoAxis},
  {ObjectInteractionIntention::StackTopObjectAxisCheck,           &empty},
  {ObjectInteractionIntention::StackTopObjectNoAxisCheck,         &empty},
  {ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck, &empty},
  {ObjectInteractionIntention::RollObjectWithDelegateAxisCheck,   &empty},
  {ObjectInteractionIntention::PopAWheelieOnObject,               &empty}
};
    
static_assert(IsSequentialArray(kDependentIntentionMap),
                "DependentIntentionMap missing missing entry or not ordered");
  


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectInteractionCache
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectInteractionInfoCache::ObjectInteractionInfoCache(const Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::ObjectInteractionInfoCache) 
, _robot(robot)
{
  
  // Pickup no axis check
  BlockWorldFilter *pickupAnyFilter = new BlockWorldFilter;
  pickupAnyFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  pickupAnyFilter->AddFilterFcn( std::bind(&ObjectInteractionInfoCache::CanPickupNoAxisCheck,
                                           this, std::placeholders::_1));
  
  // Pickup with axis check
  BlockWorldFilter *pickupWithAxisFilter = new BlockWorldFilter;
  pickupWithAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  pickupWithAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanPickupAxisCheck,
                                               this, std::placeholders::_1));
  
  
  // Stack top no axis check
  BlockWorldFilter *stackTopFilter = new BlockWorldFilter;
  stackTopFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  stackTopFilter->AddFilterFcn( std::bind(&ObjectInteractionInfoCache::CanUseAsStackTopNoAxisCheck,
                                          this, std::placeholders::_1));
  
  // Stack top with axis check
  BlockWorldFilter *stackTopWithAxisFilter = new BlockWorldFilter;
  stackTopWithAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  stackTopWithAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanUseAsStackTopAxisCheck,
                                                 this, std::placeholders::_1));
  
  // stack bottom no axis check
  BlockWorldFilter *stackBottomFilter = new BlockWorldFilter;
  stackBottomFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  stackBottomFilter->AddFilterFcn( std::bind(&ObjectInteractionInfoCache::CanUseAsStackBottomNoAxisCheck,
                                             this, std::placeholders::_1));
  
  // stack bottom axis check
  BlockWorldFilter *stackBottomWithAxis = new BlockWorldFilter;
  stackBottomWithAxis->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  stackBottomWithAxis->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanUseAsStackBottomAxisCheck,
                                              this, std::placeholders::_1));
  
  
  // Roll object with delegate - no axis check
  BlockWorldFilter *rollNoAxisFilter = new BlockWorldFilter;
  rollNoAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  rollNoAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanRollObjectDelegateNoAxisCheck,
                                           this, std::placeholders::_1));
  
  // Roll object with delegate - axis check
  BlockWorldFilter *rollWithAxisFilter = new BlockWorldFilter;
  rollWithAxisFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  rollWithAxisFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanRollObjectDelegateAxisCheck,
                                             this, std::placeholders::_1));
  
  // Function for selecting the best object for rolling
  auto rollObjectBestObjectFunc = std::bind(&ObjectInteractionInfoCache::RollBlockBestObjectFunction,
                                            this, std::placeholders::_1);
  
  // Pop A Wheelie check
  BlockWorldFilter *popFilter = new BlockWorldFilter;
  popFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  popFilter->AddFilterFcn(std::bind(&ObjectInteractionInfoCache::CanUseForPopAWheelie,
                                    this, std::placeholders::_1));
  
  FullValidInteractionArray validInteractionFilters = {
    {ObjectInteractionIntention::PickUpObjectNoAxisCheck,           pickupAnyFilter},
    {ObjectInteractionIntention::PickUpObjectAxisCheck,             pickupWithAxisFilter},
    {ObjectInteractionIntention::StackBottomObjectAxisCheck,        stackBottomWithAxis},
    {ObjectInteractionIntention::StackBottomObjectNoAxisCheck,      stackBottomFilter},
    {ObjectInteractionIntention::StackTopObjectAxisCheck,           stackTopWithAxisFilter},
    {ObjectInteractionIntention::StackTopObjectNoAxisCheck,         stackTopFilter},
    {ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck, rollNoAxisFilter},
    {ObjectInteractionIntention::RollObjectWithDelegateAxisCheck,   rollWithAxisFilter},
    {ObjectInteractionIntention::PopAWheelieOnObject,               popFilter}
  };
  
  // Function for selecting the best object given distance/position in configurations
  auto defaultBestObjectFunc = std::bind(&ObjectInteractionInfoCache::DefaultBestObjectFunction,
                                            this, std::placeholders::_1);

  
  FullBestInteractionArray findBestObjectFunctions = {
    {ObjectInteractionIntention::PickUpObjectNoAxisCheck,           defaultBestObjectFunc},
    {ObjectInteractionIntention::PickUpObjectAxisCheck,             defaultBestObjectFunc},
    {ObjectInteractionIntention::StackBottomObjectAxisCheck,        defaultBestObjectFunc},
    {ObjectInteractionIntention::StackBottomObjectNoAxisCheck,      defaultBestObjectFunc},
    {ObjectInteractionIntention::StackTopObjectAxisCheck,           defaultBestObjectFunc},
    {ObjectInteractionIntention::StackTopObjectNoAxisCheck,         defaultBestObjectFunc},
    {ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck, rollObjectBestObjectFunc},
    {ObjectInteractionIntention::RollObjectWithDelegateAxisCheck,   rollObjectBestObjectFunc},
    {ObjectInteractionIntention::PopAWheelieOnObject,               defaultBestObjectFunc}
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
    filterCanUseObject |= entry.second.ObjectTapInteractionOccurred(objectID);
  }
  
  // None of the action intention filters can currently use objectID but still
  // we still have a tap intention object because ReactToDoubleTap can run and
  // determine if we can actually use the the tapped object
  if(!filterCanUseObject)
  {
    PRINT_CH_INFO("ObjectInteractionInfoCache", "SetObjectTapInteration.NoFilter",
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
    case ObjectInteractionIntention::PickUpObjectNoAxisCheck:    { return "PickUpObjectNoAxisCheck"; }
    case ObjectInteractionIntention::PickUpObjectAxisCheck:      { return "PickUpObjectAxisCheck"; }
    case ObjectInteractionIntention::StackBottomObjectAxisCheck:   { return "StackBottomObjectAxisCheck";}
    case ObjectInteractionIntention::StackBottomObjectNoAxisCheck: { return "StackBottomObjectNoAxisCheck";}
    case ObjectInteractionIntention::StackTopObjectAxisCheck:    { return "StackTopObjectAxisCheck";}
    case ObjectInteractionIntention::StackTopObjectNoAxisCheck:  { return "StackTopObjectNoAxisCheck";}
    case ObjectInteractionIntention::RollObjectWithDelegateAxisCheck: { return "RollObjectWithDelegateAxisCheck"; }
    case ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck: { return "RollObjectWithDelegateNoAxisCheck"; }
    case ObjectInteractionIntention::PopAWheelieOnObject: { return "PopAWheelieOnObject"; }
    case ObjectInteractionIntention::Count:
      { DEV_ASSERT(false, "ObjectInteractionInfoCache.InvalidIntention"); return "";}
  };
  
  // should never get here, assert if it does (programmer error specifying intention enum class)
  DEV_ASSERT(false, "ObjectInteractionInfoCache.ObjectUseIntentionToString.InvalidIntention");
  return "UNDEFINED_ERROR";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanPickupNoAxisCheck(const ObservableObject* object) const
{
  if( nullptr == object ) {
    PRINT_NAMED_ERROR("ObjectInteractionInfoCache.CanPickupNoAxisCheck.NullObject", "object was null");
    return false;
  }
  
  // check for recent failures
  auto& whiteboard = _robot.GetAIComponent().GetComponent<AIWhiteboard>();
  const bool recentlyFailed = whiteboard.DidFailToUse(object->GetID(),
                                           {{ ObjectActionFailure::PickUpObject, ObjectActionFailure::RollOrPopAWheelie }},
                                           DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                           object->GetPose().GetWithRespectToRoot(),
                                           DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                           kAngleToleranceAfterFailure_radians);
  
  const bool canPickUp = _robot.GetDockingComponent().CanPickUpObject(*object);
  return !recentlyFailed && canPickUp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanPickupAxisCheck(const ObservableObject* object) const
{
  if( ! CanPickupNoAxisCheck(object) ) {
    return false;
  }
  
  // check the up axis
  const bool upAxisOk = object->GetPose().GetWithRespectToRoot().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;
  
  return upAxisOk;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsStackTopNoAxisCheck(const ObservableObject* object) const
{
  if(_robot.GetCarryingComponent().IsCarryingObject()) {
    return object == _robot.GetBlockWorld().GetLocatedObjectByID(_robot.GetCarryingComponent().GetCarryingObject());
  }else{
    return CanPickupNoAxisCheck(object);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsStackTopAxisCheck(const ObservableObject* object) const
{
  if(_robot.GetCarryingComponent().IsCarryingObject()) {
    const bool isCarriedObj = (object == _robot.GetBlockWorld().GetLocatedObjectByID(_robot.GetCarryingComponent().GetCarryingObject()));
    const bool isCarriedUpright = (object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS);
    return isCarriedObj && isCarriedUpright;
  }else{
    return CanPickupAxisCheck(object);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsStackBottomHelper(const ObservableObject* object, ObjectInteractionIntention intention)
{
  // already in use as the top object
  if(object->GetID() == GetBestObjectForIntention(intention)){
    return false;
  }
  
  const bool hasFailedRecently = _robot.GetAIComponent().GetComponent<AIWhiteboard>().DidFailToUse(
               object->GetID(),
               ObjectActionFailure::StackOnObject,
               kTimeObjectInvalidAfterStackFailure_sec,
               object->GetPose(),
               DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
               kAngleToleranceAfterFailure_radians);
  
  
  bool ret = (!hasFailedRecently &&
              (object->GetFamily() == ObjectFamily::LightCube) &&
              _robot.GetDockingComponent().CanStackOnTopOfObject( *object ));
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsStackBottomNoAxisCheck(const ObservableObject* object)
{
  return CanUseAsStackBottomHelper(object, ObjectInteractionIntention::StackTopObjectNoAxisCheck);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseAsStackBottomAxisCheck(const ObservableObject* object)
{
  if(CanUseAsStackBottomHelper(object, ObjectInteractionIntention::StackTopObjectAxisCheck)){
    const bool isUpright = (object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS);
    return isUpright;
  }

  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanUseForPopAWheelie(const ObservableObject* object) const
{
  auto& whiteboard = _robot.GetAIComponent().GetComponent<AIWhiteboard>();

  const bool hasFailedToPopAWheelie = whiteboard.DidFailToUse(object->GetID(),
                                                   ObjectActionFailure::RollOrPopAWheelie,
                                                   DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                                   object->GetPose(),
                                                   DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                                   kAngleToleranceAfterFailure_radians);
  
  return (!hasFailedToPopAWheelie && _robot.GetDockingComponent().CanPickUpObjectFromGround(*object));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanRollObjectDelegateNoAxisCheck(const ObservableObject* object) const
{
  auto& whiteboard = _robot.GetAIComponent().GetComponent<AIWhiteboard>();
  
  const bool hasFailedToRoll = whiteboard.DidFailToUse(object->GetID(),
                                            ObjectActionFailure::RollOrPopAWheelie,
                                            DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                            object->GetPose(),
                                            DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                            kAngleToleranceAfterFailure_radians);
  
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
    
    Pose3d relPos;
    // check if we can transform to robot space
    if ( !object->GetPose().GetWithRespectTo(_robot.GetPose(), relPos) ) {
      return false;
    }
    
    // check if it's too high to interact with
    if ( object->IsPoseTooHigh(relPos, 2.f, STACKED_HEIGHT_TOL_MM, 0.5f) ) {
      return false;
    }
    
    return true;
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectInteractionInfoCache::CanRollObjectDelegateAxisCheck(const ObservableObject* object) const
{
  const Pose3d p = object->GetPose().GetWithRespectToRoot();
  return (CanRollObjectDelegateNoAxisCheck(object) &&
          (p.GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ObjectInteractionInfoCache::DefaultBestObjectFunction(const std::set<ObjectID>& validObjects)
{
  // save some computation if we can
  if(validObjects.size() == 0){
    return {};
  }else if(validObjects.size() == 1){
    return *validObjects.begin();
  }
  
  const ObservableObject* bestObj = nullptr;
  f32 shortestDistSQ = FLT_MAX;
  f32 currentDistSQ = FLT_MAX;

  for(const auto& objID: validObjects){
    const ObservableObject* obj =  _robot.GetBlockWorld().GetLocatedObjectByID(objID);
    if(obj == nullptr){
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
ObjectID ObjectInteractionInfoCache::RollBlockBestObjectFunction(const std::set<ObjectID>& validObjects)
{
  std::set<ObjectID> objsOnSideNothingOnTop;
  for(const auto& objID : validObjects){
    const ObservableObject* validObj =  _robot.GetBlockWorld().
                                                 GetLocatedObjectByID(objID);
    if(validObj == nullptr){
      continue;
    }
    
    
    if(_robot.GetDockingComponent().CanPickUpObject(*validObj) &&
       (validObj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>()
                                              != AxisName::Z_POS)){
      objsOnSideNothingOnTop.insert(objID);
    }
  }
  
  if(objsOnSideNothingOnTop.empty()){
    return DefaultBestObjectFunction(validObjects);
  }else{
    return DefaultBestObjectFunction(objsOnSideNothingOnTop);
  }
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
    
    const bool bestIsStillValid = _validObjects.find(_bestObject) != _validObjects.end();
    // Only update _bestObject if the old best object is no longer valid
    if(!bestIsStillValid)
    {
      _bestObject = _bestObjFunc(_validObjects);
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
      PRINT_CH_INFO("ObjectInteractionInfoCache", "SetBestObjectForTap",
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
