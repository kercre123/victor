/**
 * File: AIWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"

#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/rotation.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS 0

namespace Anki {
namespace Cozmo {

namespace {

// all coordinates have to be this close from their counterpart to be considered the same observation (and thus override it)
CONSOLE_VAR(float, kBW_PossibleObjectClose_mm, "AIWhiteboard", 50.0f);
CONSOLE_VAR(float, kBW_PossibleObjectClose_rad, "AIWhiteboard", M_PI_F); // current objects flip due to distance, consider 360 since we don't care
// limit to how many pending possible objects we have stored
CONSOLE_VAR(uint32_t, kBW_MaxPossibleObjects, "AIWhiteboard", 10);
CONSOLE_VAR(float, kFlatPosisbleObjectTol_deg, "AIWhiteboard", 10.0f);
CONSOLE_VAR(float, kBW_MaxHeightForPossibleObject_mm, "AIWhiteboard", 30.0f);
// debug render
CONSOLE_VAR(bool, kBW_DebugRenderPossibleObjects, "AIWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderPossibleObjectsZ, "AIWhiteboard", 35.0f);
CONSOLE_VAR(bool, kBW_DebugRenderBeacons, "AIWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderBeaconZ, "AIWhiteboard", 35.0f);
// face tracking
CONSOLE_VAR(float, kFaceTracking_HeadAngleDistFactor, "AIWhiteboard", 1.0);
CONSOLE_VAR(float, kFaceTracking_BodyAngleDistFactor, "AIWhiteboard", 3.0);

const int kMaxStackHeightReach = 2;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ObjectUseActionToString(AIWhiteboard::ObjectUseAction action)
{
  using ObjectUseAction = AIWhiteboard::ObjectUseAction;
  switch(action) {
    case ObjectUseAction::PickUpObject : { return "PickUp";  }
    case ObjectUseAction::StackOnObject: { return "StackOn"; }
    case ObjectUseAction::PlaceObjectAt: { return "PlaceAt"; }
    case ObjectUseAction::RollOrPopAWheelie: { return "RollOrPop";}
  };

  // should never get here, assert if it does (programmer error specifying action enum class)
  DEV_ASSERT(false, "AIWhiteboard.ObjectUseActionToString.InvalidAction");
  return "UNDEFINED_ERROR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ObjectUseIntentionToString(AIWhiteboard::ObjectUseIntention intention)
{
  using ObjectUseIntention = AIWhiteboard::ObjectUseIntention;
  switch(intention) {
    case ObjectUseIntention::PickUpAnyObject: { return "PickUpAnyObject"; }
    case ObjectUseIntention::PickUpObjectWithAxisCheck: { return "PickUpObjectWithAxisCheck"; }
    case ObjectUseIntention::RollObjectWithAxisCheck: { return "RollObjectWithAxisCheck"; }
    case ObjectUseIntention::RollObjectNoAxisCheck: { return "RollObjectNoAxisCheck"; }
    case ObjectUseIntention::PopAWheelieOnObject: { return "PopAWheelieOnObject"; }
    case ObjectUseIntention::PyramidBaseObject: { return "PyramidBaseObject"; }
    case ObjectUseIntention::PyramidStaticObject: { return "PyramidStaticObject"; }
    case ObjectUseIntention::PyramidTopObject: { return "PyramidTopObject"; }
  };

  // should never get here, assert if it does (programmer error specifying intention enum class)
  DEV_ASSERT(false, "AIWhiteboard.ObjectUseIntentionToString.InvalidIntention");
  return "UNDEFINED_ERROR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// return how many max entries of the given type we store. DO NOT return 0, at least there has to be 1
size_t GetObjectFailureListMaxSize(AIWhiteboard::ObjectUseAction action)
{
  using ObjectUseAction = AIWhiteboard::ObjectUseAction;
  switch(action) {
    case ObjectUseAction::PickUpObject : { return 1; }
    case ObjectUseAction::StackOnObject: { return 1; }
    case ObjectUseAction::PlaceObjectAt: { return 10; } // this can affect behaviorExploreBringCubeToBeacon's kLocationFailureDist_mm (read note there)
    case ObjectUseAction::RollOrPopAWheelie: { return 1;}
  };

  // should never get here, assert if it does (programmer error specifying action enum class)
  DEV_ASSERT(false, "AIWhiteboard.GetObjectFailureListMaxSize.InvalidAction");
  return 0;
}

} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::AIWhiteboard(Robot& robot)
: _robot(robot)
, _gotOffChargerAtTime_sec(-1.0f)
, _returnedToTreadsAtTime_sec(-1.0f)
, _edgeInfoTime_sec(-1.0f)
, _edgeInfoClosestEdge_mm(-1.0f)
{
  CreateBlockWorldFilters();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::~AIWhiteboard()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanPickupHelper(const ObservableObject* object) const
{
  if( nullptr == object ) {
    PRINT_NAMED_ERROR("AIWhiteboard.CanPickupHelper.NullObject", "object was null");
    return false;
  }
  
  // check for recent failures
  const bool recentlyFailed = DidFailToUse(object->GetID(),
                                           {{ ObjectUseAction::PickUpObject, ObjectUseAction::RollOrPopAWheelie }},
                                           DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                           object->GetPose().GetWithRespectToOrigin(),
                                           DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                           DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  const bool canPickUp = _robot.CanPickUpObject(*object);
  return !recentlyFailed && canPickUp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanPopAWheelieHelper(const ObservableObject* object) const
{
  const bool hasFailedToPopAWheelie = DidFailToUse(object->GetID(),
                                                   AIWhiteboard::ObjectUseAction::RollOrPopAWheelie,
                                                   DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                                   object->GetPose(),
                                                   DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                                   DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  return (!hasFailedToPopAWheelie && _robot.CanPickUpObjectFromGround(*object));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanRollHelper(const ObservableObject* object) const
{
  const bool hasFailedToRoll = DidFailToUse(object->GetID(),
                                            AIWhiteboard::ObjectUseAction::RollOrPopAWheelie,
                                            DefaultFailToUseParams::kTimeObjectInvalidAfterFailure_sec,
                                            object->GetPose(),
                                            DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                                            DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
  
  return (!hasFailedToRoll && _robot.CanPickUpObjectFromGround(*object));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanRollRotationImportantHelper(const ObservableObject* object) const
{
  const Pose3d p = object->GetPose().GetWithRespectToOrigin();
  return (CanRollHelper(object) &&
          (p.GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanUseAsPyramidBaseBlock(const ObservableObject* object) const
{
  const auto& pyramidBases = _robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = _robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  for(const auto& pyramid: pyramids){
    if(object->GetID() == pyramid->GetPyramidBase().GetBaseBlockID()){
      return true;
    }
  }

  for(const auto& pyramidBase: pyramidBases){
    if(object->GetID() == pyramidBase->GetBaseBlockID()){
      return true;
    }
  }
  
  // If a pyramid or base exists and this object doesn't match the base, wait to assign that object
  if(!(pyramids.empty() && pyramidBases.empty())){
    return false;
  }
  
  
  // If there is a stack of 2, the middle block should be selected as the base of the pyramid
  const auto& stacks = _robot.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetStacks();
  for(const auto& stack: stacks){
    if(stack->GetStackHeight() == kMaxStackHeightReach &&
       stack->GetMiddleBlockID() == object->GetID()){
      return _robot.CanPickUpObject(*object);
    }
  }
  
  if(!stacks.empty()){
    return false;
  }
  
  
  // If the robot is carrying a block, make that the static block
  if(_robot.IsCarryingObject()){
    return _robot.GetCarryingObject() == object->GetID();
  }
  
  // So long as we can pick the object up, it's a valid base block
  return _robot.CanPickUpObject(*object);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanUseAsPyramidStaticBlock(const ObservableObject* object) const
{
  // Base block must be set before static block can be set
  auto bestBaseBlock = GetBestObjectForAction(ObjectUseIntention::PyramidBaseObject);
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

  for(const auto& pyramidBase: pyramidBases){
    if((object->GetID() == pyramidBase->GetStaticBlockID()) &&
       (bestBaseBlock == pyramidBase->GetBaseBlockID())){
      return true;
    }
  }

  // If a pyramid or base exists and this object doesn't match the base, wait to assign that object
  if(!(pyramids.empty() && pyramidBases.empty())){
    return false;
  }
  
  
  
  
  if(!object->IsRestingAtHeight(0, BlockWorld::kOnCubeStackHeightTolerence)){
    return false;
  }
  
  const ObservableObject* onTop = _robot.GetBlockWorld().FindObjectOnTopOf(
                                           *object,
                                           BlockWorld::kOnCubeStackHeightTolerence);
  
  if(onTop != nullptr){
    return false;
  }

  return true;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::CanUseAsPyramidTopBlock(const ObservableObject* object) const
{
  // Base and static blocks must be set before top block can be set
  auto bestBaseBlock = GetBestObjectForAction(ObjectUseIntention::PyramidBaseObject);
  auto bestStaticBlock = GetBestObjectForAction(ObjectUseIntention::PyramidStaticObject);

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
void AIWhiteboard::CreateBlockWorldFilters()
{

  // Pickup no axis check
  {
    BlockWorldFilter *pickupFilter = new BlockWorldFilter;
    pickupFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    pickupFilter->AddFilterFcn( std::bind(&AIWhiteboard::CanPickupHelper, this, std::placeholders::_1) );
    _filtersPerAction[ObjectUseIntention::PickUpAnyObject].reset( pickupFilter );
  }

  // Pickup with axis check
  {
    BlockWorldFilter *pickupFilter = new BlockWorldFilter;
    pickupFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    pickupFilter->AddFilterFcn(
      [this](const ObservableObject* object)
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
      });
    _filtersPerAction.insert( std::make_pair( ObjectUseIntention::PickUpObjectWithAxisCheck,
                                              std::unique_ptr<BlockWorldFilter>(pickupFilter) ) );
  }
  
  // Roll with axis check
  {
    BlockWorldFilter *rollFilter = new BlockWorldFilter;
    rollFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    rollFilter->AddFilterFcn(std::bind(&AIWhiteboard::CanRollRotationImportantHelper, this, std::placeholders::_1));
    _filtersPerAction[ObjectUseIntention::RollObjectWithAxisCheck].reset(rollFilter);
  }
  
  // Roll no axis check
  {
    BlockWorldFilter *rollFilter = new BlockWorldFilter;
    rollFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    rollFilter->AddFilterFcn(std::bind(&AIWhiteboard::CanRollHelper, this, std::placeholders::_1));
    _filtersPerAction[ObjectUseIntention::RollObjectNoAxisCheck].reset(rollFilter);
  }
  
  // Pop A Wheelie check
  {
    BlockWorldFilter *popFilter = new BlockWorldFilter;
    popFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    popFilter->AddFilterFcn(std::bind(&AIWhiteboard::CanPopAWheelieHelper, this, std::placeholders::_1));
    _filtersPerAction[ObjectUseIntention::PopAWheelieOnObject].reset(popFilter);
  }
  
  // Pyramid Base object
  {
    BlockWorldFilter *baseFilter = new BlockWorldFilter;
    baseFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    baseFilter->AddFilterFcn(std::bind(&AIWhiteboard::CanUseAsPyramidBaseBlock, this, std::placeholders::_1));
    _filtersPerAction[ObjectUseIntention::PyramidBaseObject].reset(baseFilter);
  }
  
  // Pyramid Static object
  {
    BlockWorldFilter *staticFilter = new BlockWorldFilter;
    staticFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    staticFilter->AddFilterFcn(std::bind(&AIWhiteboard::CanUseAsPyramidStaticBlock, this, std::placeholders::_1));
    _filtersPerAction[ObjectUseIntention::PyramidStaticObject].reset(staticFilter);
  }
  
  // Pyramid Top object
  {
    BlockWorldFilter *topFilter = new BlockWorldFilter;
    topFilter->SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    topFilter->AddFilterFcn(std::bind(&AIWhiteboard::CanUseAsPyramidTopBlock, this, std::placeholders::_1));
    _filtersPerAction[ObjectUseIntention::PyramidTopObject].reset(topFilter);
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::Init()
{
  // register to possible object events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedPossibleObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotMarkedObjectPoseUnknown>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotOffTreadsStateChanged>();
  }
  else {
    PRINT_NAMED_WARNING("AIWhiteboard.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::Update()
{
  UpdateValidObjects();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OnRobotDelocalized()
{
  RemovePossibleObjectsFromZombieMaps();

  UpdatePossibleObjectRender();
  
  // TODO rsam we probably want to rematch beacons when robot relocalizes.
  _beacons.clear();
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OnRobotRelocalized()
{
  // just need to update render, otherwise they render wrt old origin
  UpdatePossibleObjectRender();

  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ProcessClearQuad(const Quad2f& quad)
{
  const Pose3d* worldOriginPtr = _robot.GetWorldOrigin();
  // remove all objects inside clear quads
  auto isObjectInsideQuad = [&quad, worldOriginPtr](const PossibleObject& possibleObj)
  {
    Pose3d relPose;
    if ( possibleObj.pose.GetWithRespectTo(*worldOriginPtr, relPose) ) {
      return quad.Contains( relPose.GetTranslation() );
    } else {
      return false;
    }
  };
  const size_t countBefore = _possibleObjects.size();
  _possibleObjects.remove_if( isObjectInsideQuad );
  const size_t countAfter = _possibleObjects.size();
  
  // update render if we removed something
  if ( countBefore != countAfter ) {
    UpdatePossibleObjectRender();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::FinishedSearchForPossibleCubeAtPose(ObjectType objectType, const Pose3d& pose)
{
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.FinishedSearch",
                      "finished search, so removing possible object");
  }
  RemovePossibleObjectsMatching(objectType, pose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::FindUsableCubesOutOfBeacons(ObjectInfoList& outObjectList) const
{
  #if ANKI_DEVELOPER_CODE
  {
    // all beacons should be in this world
    for ( const auto& beacon : _beacons ) {
      DEV_ASSERT(&beacon.GetPose().FindOrigin() == _robot.GetWorldOrigin(),
                 "AIWhiteboard.FindUsableCubesOutOfBeacons.DirtyBeacons");
    }
  }
  #endif

  outObjectList.clear();
  if ( !_beacons.empty() )
  {
    // if the robot is currently carrying a cube, insert only that one right away, regardless of whether it's inside or
    // outisde the beacon, since we would always want to drop it. Any other cube would be unusable until we drop this one
    if ( _robot.IsCarryingObject() )
    {
      const ObservableObject* const carryingObject = _robot.GetBlockWorld().GetObjectByID( _robot.GetCarryingObject() );
      if ( nullptr != carryingObject ) {
        outObjectList.emplace_back( carryingObject->GetID(), carryingObject->GetFamily() );
      } else {
        // this can be blocking under certain scenarios, since we can think we are carrying a cube, but it is not in
        // the blockworld
        PRINT_NAMED_ERROR("AIWhiteboard.FindUsableCubesOutOfBeacons.NullCarryingObject",
                          "Could not get carrying object pointer (ID=%d)",
                          _robot.GetCarryingObject().GetValue());
      }
    }
    else
    {
      // ask for all cubes we know, and if any is not inside a beacon, add to the list
      BlockWorldFilter filter;
      filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
      filter.SetFilterFcn([this, &outObjectList](const ObservableObject* blockPtr) {
        if(!blockPtr->IsPoseStateUnknown())
        {
          // check if the robot can pick up this object
          const bool canPickUp = _robot.CanPickUpObject(*blockPtr);
          if ( canPickUp )
          {
            bool isBlockInAnyBeacon = false;
            // check if the object is within any beacon
            for ( const auto& beacon : _beacons ) {
              isBlockInAnyBeacon = beacon.IsLocWithinBeacon(blockPtr->GetPose());
              if ( isBlockInAnyBeacon ) {
                break;
              }
            }
            
            // this block should be carried to a beacon
            if ( !isBlockInAnyBeacon ) {
              outObjectList.emplace_back( blockPtr->GetID(), blockPtr->GetFamily() );
            }
          }
        }
        return false; // have to return true/false, even though not used
      });
      
      _robot.GetBlockWorld().FilterObjects(filter);
    }
  }

  // do we have any usable cubes out of beacons?
  bool hasBlocksOutOfBeacons = !outObjectList.empty();
  return hasBlocksOutOfBeacons;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::FindCubesInBeacon(const AIBeacon* beacon, ObjectInfoList& outObjectList) const
{
  #if ANKI_DEVELOPER_CODE
  {
    // passed beacon should be a beacon in our list, not null and in the current origin
    bool beaconIsValid = false;
    for ( const auto& beaconIt : _beacons ) {
      beaconIsValid = beaconIsValid || (beacon == &beaconIt);
    }
    DEV_ASSERT(beaconIsValid, "AIWhiteboard.FindCubesInBeacon.NotABeacon");
    DEV_ASSERT(beacon, "AIWhiteboard.FindCubesInBeacon.NullBeacon");
    DEV_ASSERT(&beacon->GetPose().FindOrigin() == _robot.GetWorldOrigin(),
               "AIWhiteboard.FindCubesInBeacon.BeaconNotInOrigin");
  }
  #endif
  
  outObjectList.clear();
  
  {
    Robot& robotRef = _robot; // can't capture member of this for lambda, need scoped variable
    // ask for all cubes we know about inside the beacon
    BlockWorldFilter filter;
    filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    filter.SetFilterFcn([this, &robotRef, &outObjectList, beacon](const ObservableObject* blockPtr)
    {
      if(!blockPtr->IsPoseStateUnknown() && !_robot.IsCarryingObject(blockPtr->GetID()) )
      {
        const bool isBlockInBeacon = beacon->IsLocWithinBeacon(blockPtr->GetPose());
        if ( isBlockInBeacon ) {
          outObjectList.emplace_back( blockPtr->GetID(), blockPtr->GetFamily() );
        }
      }
      return false; // have to return true/false, even though not used
    });
    
    _robot.GetBlockWorld().FilterObjects(filter);
  }
  
  return !outObjectList.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::AreAllCubesInBeacons() const
{
  #if ANKI_DEVELOPER_CODE
  {
    // all beacons should be in this world
    for ( const auto& beacon : _beacons ) {
      DEV_ASSERT(&beacon.GetPose().FindOrigin() == _robot.GetWorldOrigin(),
                 "AIWhiteboard.FindUsableCubesOutOfBeacons.DirtyBeacons");
    }
  }
  #endif

  bool allInBeacon = false;
  if ( !_beacons.empty() )
  {
    // robot can't be carrying an object, otherwise they are not in beacons
    if ( !_robot.IsCarryingObject() )
    {
      allInBeacon = true;
      
      // ask for all cubes we know if they are in beacons
      BlockWorldFilter filter;
      filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
      filter.SetFilterFcn([this, &allInBeacon](const ObservableObject* blockPtr)
      {
        if(!blockPtr->IsPoseStateUnknown())
        {
          bool isBlockInAnyBeacon = false;
          // check if the object is within any beacon
          for ( const auto& beacon : _beacons ) {
            isBlockInAnyBeacon = beacon.IsLocWithinBeacon(blockPtr->GetPose());
            if ( isBlockInAnyBeacon ) {
              break;
            }
          }
        
          // this block should be carried to a beacon
          if ( !isBlockInAnyBeacon ) {
            allInBeacon = false;
          }
        }
        else
        {
          // object position is unknown, consider not in beacon
          allInBeacon = false;
        }
        return false; // have to return true/false, even though not used
      });
      
      _robot.GetBlockWorld().FilterObjects(filter);
    }
  }

  // return the result of the filter
  return allInBeacon;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetFailedToUse(const ObservableObject& object, ObjectUseAction action)
{
  // PlaceObjectAt should provide location
  DEV_ASSERT(action != ObjectUseAction::PlaceObjectAt, "AIWhiteboard.SetFailedToUse.PlaceObjectAtRequiresLocation");

  // use object current pose
  const Pose3d& objectPose = object.GetPose();
  SetFailedToUse(object, action, objectPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetFailedToUse(const ObservableObject& object, ObjectUseAction action, const Pose3d& atLocation)
{
  // simply store failure's timestamp
  ObjectFailureTable& failureTableForAction = GetObjectFailureTable(action);
  FailureList& failureListForObj = failureTableForAction[object.GetID().GetValue()];
  
  // check if we need to remove an entry because we have reached the maximum
  const size_t maxItemsInList = GetObjectFailureListMaxSize(action);
  DEV_ASSERT(maxItemsInList > 0, "AIWhiteboard.SetFailedToUse.MaxIs0");
  if ( failureListForObj.size() >= maxItemsInList )
  {
    // can't have an empty list or a list bigger than the max (so it has to be equal to the max)
    DEV_ASSERT(!failureListForObj.empty(), "AIWhiteboard.SetFailedToUse.EmptyListReachedMax");
    DEV_ASSERT(failureListForObj.size() == maxItemsInList, "AIWhiteboard.SetFailedToUse.ListBeyondMax");
    
    // print the one we are removing
    PRINT_CH_INFO("AIWhiteboard", "SetFailedToUse",
      "Removed failure on ['%s'] for object '%d' atPose (%.2f,%.2f,%.2f) atTime (%fs) to add one (we are at the max=%zu)",
      ObjectUseActionToString(action),
      object.GetID().GetValue(),
      failureListForObj.begin()->_pose.GetTranslation().x(),
      failureListForObj.begin()->_pose.GetTranslation().y(),
      failureListForObj.begin()->_pose.GetTranslation().z(),
      failureListForObj.begin()->_timestampSecs,
      maxItemsInList);
  
    // remove first
    failureListForObj.pop_front();
  }

  // always add
  {
    // insert at back (newest)
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    failureListForObj.emplace_back(atLocation, curTime);
    
    // log the new one
    PRINT_CH_INFO("AIWhiteboard", "SetFailedToUse",
      "Added failure on ['%s'] for object '%d' atPose (%.2f,%.2f,%.2f) atTime (%fs) ",
      ObjectUseActionToString(action),
      object.GetID().GetValue(),
      atLocation.GetTranslation().x(),
      atLocation.GetTranslation().y(),
      atLocation.GetTranslation().z(),
      curTime);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectUseAction action) const
{
  const bool ret = DidFailToUse(objectID, ReasonsContainer{action}, -1.f, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectUseAction action, float recentSecs) const
{
  const bool ret = DidFailToUse(objectID, ReasonsContainer{action}, recentSecs, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectUseAction action,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  const bool ret = DidFailToUse(objectID, ReasonsContainer{action}, -1.f, atPose, distThreshold_mm, angleThreshold);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectUseAction action, float recentSecs,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  const bool ret = DidFailToUse(objectID, ReasonsContainer{action}, recentSecs, atPose, distThreshold_mm, angleThreshold);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::ReasonsContainer reasons) const
{
  const bool ret = DidFailToUse(objectID, reasons, -1.f, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::ReasonsContainer reasons, float recentSecs) const
{
  const bool ret = DidFailToUse(objectID, reasons, recentSecs, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::ReasonsContainer reasons,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  const bool ret = DidFailToUse(objectID, reasons, -1.f, atPose, distThreshold_mm, angleThreshold);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::ReasonsContainer reasons, float recentSecs,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  for( const auto& reason : reasons ) {
    const ObjectFailureTable& failureTable = GetObjectFailureTable(reason);
    const bool ret = FindMatchingEntry(failureTable, objectID, recentSecs, atPose, distThreshold_mm, angleThreshold);
    if( ret ) {
      return true;
    }
  }

  // not found for any passed in reason
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::FindMatchingEntry(const ObjectFailureTable& failureTable, const int objectID,
  float recentSecs,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  // if we are looking for failures in any object
  if ( objectID == ANY_OBJECT )
  {
    // iterate all the queues for all objects
    for( const auto& pair : failureTable )
    {
      // iterate all entries in each queue
      const FailureList& failuresForObj = pair.second;
      for( const FailureInfo& failureInfo : failuresForObj)
      {
        // check if the entry matches the search
        const bool matchesSearch = EntryMatches(failureInfo, recentSecs, atPose, distThreshold_mm, angleThreshold);
        if ( matchesSearch ) {
          // it does, we found a failure we care about
          return true;
        }
      }
    }
    
    // none of the current entries for any object matched
    return false;
  }
  else
  {
    // find failures for the specific object
    const auto& failuresForObjIt = failureTable.find( objectID );
    if ( failuresForObjIt != failureTable.end() )
    {
      // iterate all entries in this queue
      const FailureList& failuresForObj = failuresForObjIt->second;
      for( const FailureInfo& failureInfo : failuresForObj)
      {
        // check if the entry matches the search
        const bool matchesSearch = EntryMatches(failureInfo, recentSecs, atPose, distThreshold_mm, angleThreshold);
        if ( matchesSearch ) {
          // it does, we found a failure we care about
          return true;
        }
      }
    }
    
    // none of the current entries for this object matched
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::EntryMatches(const FailureInfo& entry, float recentSecs, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  // if we specified a timestamp (ie: if we care about recent entries only)
  const bool checkTime = FLT_GE(recentSecs, 0.0f);
  if ( checkTime )
  {
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceFailureSecs = curTime - entry._timestampSecs;
    const bool entryIsTooOld = FLT_GE(timeSinceFailureSecs, recentSecs);
    if ( entryIsTooOld ) {
      // the entry is too old, we do not care about it
      return false;
    }
  }
  
  // passed time checks, check pose
  const bool checkLocation = FLT_GE(distThreshold_mm, 0.0f);
  if ( checkLocation )
  {
    const bool arePosesClose = entry._pose.IsSameAs(atPose, Point3f(distThreshold_mm), angleThreshold);
    if ( !arePosesClose ) {
      // the entry is too far, we do not care about it
      return false;
    }
  }
  
  // entry passed all checks, this entry does match the search
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AIWhiteboard::ObjectFailureTable& AIWhiteboard::GetObjectFailureTable(ObjectUseAction action) const
{
  switch(action) {
    case ObjectUseAction::PickUpObject : { return _pickUpFailures;  }
    case ObjectUseAction::StackOnObject: { return _stackOnFailures; }
    case ObjectUseAction::PlaceObjectAt: { return _placeAtFailures; }
    case ObjectUseAction::RollOrPopAWheelie: { return _rollOrPopFailures;}
  }

  // should never get here, assert if it does (programmer error specifying action enum class)
  {
    DEV_ASSERT(false, "AIWhiteboard.GetObjectFailureTable.InvalidAction");
    static ObjectFailureTable empty;
    return empty;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::ObjectFailureTable& AIWhiteboard::GetObjectFailureTable(ObjectUseAction action)
{
  switch(action) {
    case ObjectUseAction::PickUpObject : { return _pickUpFailures;  }
    case ObjectUseAction::StackOnObject: { return _stackOnFailures; }
    case ObjectUseAction::PlaceObjectAt: { return _placeAtFailures; }
    case ObjectUseAction::RollOrPopAWheelie: { return _rollOrPopFailures;}
  }

  // should never get here, assert if it does (programmer error specifying action enum class)
  {
    DEV_ASSERT(false, "AIWhiteboard.GetObjectFailureTable.InvalidAction");
    static ObjectFailureTable empty;
    return empty;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::set< ObjectID >& AIWhiteboard::GetValidObjectsForAction(ObjectUseIntention action) const
{
  const auto setIter = _validObjectsForAction.find(action);
  if( setIter != _validObjectsForAction.end() ) {
    return setIter->second;
  }
  else {
    // we need to return a reference, so use a static. Should only happen if this is called before our first
    // update
    PRINT_NAMED_WARNING("AIWhiteboard.GetValidObjectsForAction.NoObjects",
                        "Tried to get valid objects for action intent %d, but don't have a cache yet",
                        (int)action);
    const static std::set< ObjectID > emptyStaticSet;
    return emptyStaticSet;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::IsObjectValidForAction(ObjectUseIntention action, const ObjectID& object) const
{
  const auto setIter = _validObjectsForAction.find(action);
  if( setIter == _validObjectsForAction.end() ) {
    return false;
  }
  
  const auto objectIter = setIter->second.find(object);
  // the object is valid if and only if it is in our valid set
  return objectIter != setIter->second.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID AIWhiteboard::GetBestObjectForAction(ObjectUseIntention intent) const
{
  auto iter = _bestObjectForAction.find(intent);
  if( iter != _bestObjectForAction.end() ) {
    return iter->second;
  }
  else {
    return {};
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockWorldFilter* AIWhiteboard::GetDefaultFilterForAction(ObjectUseIntention action) const
{
  auto iter = _filtersPerAction.find(action);
  if( iter != _filtersPerAction.end() ) {
    return iter->second.get();
  }
  else {
    return nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::FaceID_t AIWhiteboard::GetBestFaceToTrack(const std::set< Vision::FaceID_t >& possibleFaces,
                                                  bool preferNamedFaces) const
{
  if( possibleFaces.empty() ) {
    return Vision::UnknownFaceID;
  }

  if( possibleFaces.size() == 1 ) {
    return *possibleFaces.begin();
  }

  // add a large penalty to any face without a name, if we are preferring named faces
  const f32 unnamedFacePenalty = preferNamedFaces ? 1000.0f : 0.0f;
  
  float bestCost = std::numeric_limits<float>::max();
  Vision::FaceID_t bestFace = Vision::UnknownFaceID;
  for( auto targetID : possibleFaces ) {

    const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(targetID);
    if( nullptr == face ) {
      continue;
    }
    
    Pose3d poseWrtRobot;    
    if( ! face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot) ) {
      // no transform, probably a different origin
      continue;
    }

    Radians absHeadTurnAngle = TurnTowardsPoseAction::GetAbsoluteHeadAngleToLookAtPose(poseWrtRobot.GetTranslation());
    Radians relBodyTurnAngle = TurnTowardsPoseAction::GetRelativeBodyAngleToLookAtPose(poseWrtRobot.GetTranslation());

    Radians relHeadTurnAngle = absHeadTurnAngle - _robot.GetHeadAngle();

    const float headTurnCost = kFaceTracking_HeadAngleDistFactor * relHeadTurnAngle.getAbsoluteVal().ToFloat();
    const float bodyTurnCost = kFaceTracking_BodyAngleDistFactor * relBodyTurnAngle.getAbsoluteVal().ToFloat();
    const float penalty = face->HasName() ? 0.0f : unnamedFacePenalty;
    
    const float cost = headTurnCost + bodyTurnCost + penalty;      

    if( cost < bestCost ) {
      
      bestFace = targetID;
      bestCost = cost;
    }
  }

  return bestFace;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const
{
  possibleObjects.clear();
  
  // iterate all possible objects
  const Pose3d* worldOriginPtr = _robot.GetWorldOrigin();
  for( const auto& possibleObject : _possibleObjects )
  {
    // if we can obtain a pose with respect to the current origin, store that output (relative pose and type)
    Pose3d poseWRTOrigin;
    if ( possibleObject.pose.GetWithRespectTo(*worldOriginPtr, poseWRTOrigin) )
    {
      possibleObjects.emplace_back( poseWRTOrigin, possibleObject.type );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::AddBeacon( const Pose3d& beaconPos )
{
  _beacons.emplace_back( beaconPos );

  // update render
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::FailedToFindLocationInBeacon(AIBeacon* beacon)
{
  // set time stamp in the beacon
  beacon->FailedToFindLocation();
  
  // update render
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AIBeacon* AIWhiteboard::GetActiveBeacon() const
{
  if ( _beacons.empty() ) {
    return nullptr;
  }
  
  return &_beacons[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIBeacon* AIWhiteboard::GetActiveBeacon()
{
  if ( _beacons.empty() ) {
    return nullptr;
  }
  
  return &_beacons[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::RemovePossibleObjectsMatching(ObjectType objectType, const Pose3d& pose)
{
  // iterate all current possible objects
  auto possibleObjectIt = _possibleObjects.begin();
  while ( possibleObjectIt != _possibleObjects.end() )
  {
    // compare type (there shouldn't be many of the same type, but can happen if we see more than one possible object
    // for the same cube)
    if ( possibleObjectIt->type == objectType )
    {
      Pose3d relPose;
      if ( possibleObjectIt->pose.GetWithRespectTo(pose, relPose) )
      {
        // compare locations
        const float distSQ = kBW_PossibleObjectClose_mm*kBW_PossibleObjectClose_mm;
        const bool isClosePossibleObject = FLT_LE(relPose.GetTranslation().LengthSq(), distSQ );
        if ( isClosePossibleObject )
        {
          if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
            PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Remove",
                              "removing possible object that was at (%f, %f, %f), matching object type %d",
                              relPose.GetTranslation().x(),
                              relPose.GetTranslation().y(),
                              relPose.GetTranslation().z(),
                              objectType);
          }

          // they are close, remove old entry
          possibleObjectIt = _possibleObjects.erase( possibleObjectIt );
          // jump up to not increment iterator after erase
          continue;
        }
      }
    }

    // not erased, increment iter
    ++possibleObjectIt;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::RemovePossibleObjectsFromZombieMaps()
{
  PossibleObjectList::iterator iter = _possibleObjects.begin();
  while( iter != _possibleObjects.end() )
  {
    const Pose3d* objOrigin = &(iter->pose.FindOrigin());
    const bool isZombie = _robot.GetBlockWorld().IsZombiePoseOrigin(objOrigin);
    if ( isZombie ) {
      if ( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
        PRINT_CH_INFO("AIWhiteboard", "RemovePossibleObjectsFromZombieMaps", "Deleted possible object because it was zombie");
      }
     iter = _possibleObjects.erase(iter);
    } else {
      ++iter;
    }
  }
  if ( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "RemovePossibleObjectsFromZombieMaps", "%zu possible objects not zombie", _possibleObjects.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg;
  
  // this is for the future. In the future, should a white board of one robot get messages from another robot? Should
  // it just ignore them? This assert will fire when this whiteboard receives a message from another robot. Make a
  // decision then
  DEV_ASSERT(_robot.GetID() == possibleObject.robotID,
             "AIWhiteboard.HandleMessage.RobotObservedObject.UnexpectedRobotID");
  
  Pose3d obsPose( msg.pose, _robot.GetPoseOriginList() );
  
  // iterate objects we previously had and remove them if we think they belong to this object
  RemovePossibleObjectsMatching(possibleObject.objectType, obsPose);

  // update render
  UpdatePossibleObjectRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotObservedPossibleObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg.possibleObject;

  // this is for the future. In the future, should a white board of one robot get messages from another robot? Should
  // it just ignore them? This assert will fire when this whiteboard receives a message from another robot. Make a
  // decision then
  DEV_ASSERT(_robot.GetID() == possibleObject.robotID,
             "AIWhiteboard.HandleMessage.RobotObservedPossibleObject.UnexpectedRobotID");
  
  Pose3d obsPose( msg.possibleObject.pose, _robot.GetPoseOriginList() );
  
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "ObservedPossible", "robot observed a possible object");
  }

  ConsiderNewPossibleObject(possibleObject.objectType, obsPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotMarkedObjectPoseUnknown& msg)
{
  const ObservableObject* obj = _robot.GetBlockWorld().GetObjectByID(msg.objectID);
  if( nullptr == obj ) {
    PRINT_NAMED_WARNING("AIWhiteboard.HandleMessage.RobotMarkedObjectPoseUnknown.NoBlock",
                        "couldnt get object with id %d",
                        msg.objectID);
    return;
  }

  PRINT_CH_INFO("AIWhiteboard", "MarkedUnknown",
                    "marked %d unknown, adding to possible objects",
                    msg.objectID);

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PoseMarkedUnknown", "considering old pose from object %d as possible object",
                      msg.objectID);
  }
  
  // its position has become Unknown. We probably were at the location where we expected this object to be,
  // and it's no there. So we don't want to keep its markers anymore
  RemovePossibleObjectsMatching(obj->GetType(), obj->GetPose());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged& msg)
{
  const bool onTreads = msg.treadsState == OffTreadsState::OnTreads;
  if ( onTreads ) {
    _returnedToTreadsAtTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ConsiderNewPossibleObject(ObjectType objectType, const Pose3d& obsPose)
{
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Consider",
                      "considering pose (%f, %f, %f)",
                      obsPose.GetTranslation().x(),
                      obsPose.GetTranslation().y(),
                      obsPose.GetTranslation().z());
  }
  
  // only add relatively flat objects
  const RotationMatrix3d Rmat = obsPose.GetRotationMatrix();
  const bool isFlat = (Rmat.GetAngularDeviationFromParentAxis<'Z'>() < DEG_TO_RAD(kFlatPosisbleObjectTol_deg));

  if( ! isFlat ) {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.NotFlat",
                        "pose isn't flat, so not adding to list (angle %fdeg)",
                        Rmat.GetAngularDeviationFromParentAxis<'Z'>().getDegrees());
    }

    return;
  }

  Pose3d relPose;
  if( ! obsPose.GetWithRespectTo(_robot.GetPose(), relPose) ) {
    PRINT_NAMED_WARNING("AIWhiteboard.PossibleObject.NoRelPose",
                        "Couldnt get pose WRT to robot");
    return;
  }

  // can't really use size since this is just a pose
  // TODO:(bn) get the size based on obectType so this can be relative to the bottom of the object?
  if( relPose.GetTranslation().z() > kBW_MaxHeightForPossibleObject_mm ) {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.TooHigh",
                        "pose is too high, not considering. relativeZ = %f",
                        relPose.GetTranslation().z());
    }

    return;
  }

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.RemovingOldObjects",
                      "removing any old possible objects that are nearby");
  }

  // remove any objects that are similar to this
  RemovePossibleObjectsMatching(objectType, obsPose);
  
  // check with the world if this a possible marker for an object we have already recognized, because in that case we
  // are not interested in storing it again.
  
  Vec3f maxLocDist(kBW_PossibleObjectClose_mm);
  Radians maxLocAngle(kBW_PossibleObjectClose_rad);
  ObservableObject* prevObservedObject =
    _robot.GetBlockWorld().FindClosestMatchingObject( objectType, obsPose, maxLocDist, maxLocAngle);
  
  // found object nearby, no need
  if ( nullptr != prevObservedObject )
  {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.NearbyObject",
                        "Already had a real(known) object nearby, not adding a new one");
    }

    return;
  }

  // if we are at the limit of stored objects, remove the front one
  if ( _possibleObjects.size() >= kBW_MaxPossibleObjects )
  {
    // we reached the limit, remove oldest entry
    DEV_ASSERT(!_possibleObjects.empty(), "AIWhiteboard.HandleMessage.ReachedLimitEmpty");
    _possibleObjects.pop_front();
    // PRINT_NAMED_INFO("AIWhiteboard.HandleMessage.BeyondObjectLimit", "Reached limit of pending objects, removing oldest one");
  }

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Add",
                      "added possible object. rot=%fdeg, z=%f",
                      Rmat.GetAngularDeviationFromParentAxis<'Z'>().getDegrees(),
                      relPose.GetTranslation().z());
  }

  
  // always add new entry
  _possibleObjects.emplace_back( obsPose, objectType );
  
  // update render
  UpdatePossibleObjectRender();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdateValidObjects()
{
  for( const auto& filterPair : _filtersPerAction ) {
    const auto& actionIntent = filterPair.first;
    const auto& filterPtr = filterPair.second;

    // update set of valid objects
    auto& validObjectIDSet = _validObjectsForAction[actionIntent];
    validObjectIDSet.clear();
      
    std::vector<const ObservableObject*> objects;
    _robot.GetBlockWorld().FindMatchingObjects(*filterPtr, objects);
    for( const ObservableObject* obj : objects ) {
      if( obj ) {
        validObjectIDSet.insert( obj->GetID() );
      }
    }

    const bool bestIsStillValid = validObjectIDSet.find(_bestObjectForAction[actionIntent]) != validObjectIDSet.end();
    
    // Only update _bestObjectForAction if we don't have a tap object, or if the tap object became invalid
    if(!_haveTapIntentionObject || !bestIsStillValid)
    {      
      // select best object
      const ObservableObject* closestObject = _robot.GetBlockWorld().FindObjectClosestTo(_robot.GetPose(),
                                                                                         * filterPair.second);
      if( closestObject != nullptr ) {

        if( _haveTapIntentionObject && _bestObjectForAction[actionIntent].IsSet() ) {
          PRINT_CH_INFO("AIWhiteboard", "UpdateValidObjects.ResetBestTapped",
                        "We have a tap intent, but object id %d is no longer valid for action %s, selecting object %d instead",
                        _bestObjectForAction[actionIntent].GetValue(),
                        ObjectUseIntentionToString(actionIntent),
                        closestObject->GetID().GetValue());
        }
        
        _bestObjectForAction[ actionIntent ] = closestObject->GetID();
      }
      else {

        if( _haveTapIntentionObject && _bestObjectForAction[actionIntent].IsSet() ) {
          PRINT_CH_INFO("AIWhiteboard", "UpdateValidObjects.ClearBestTapped",
                        "We have a tap intent, but object id %d is no longer valid, clearing best for action %s",
                        _bestObjectForAction[actionIntent].GetValue(),
                        ObjectUseIntentionToString(actionIntent));
        }

        _bestObjectForAction[ actionIntent ].UnSet();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdatePossibleObjectRender()
{
  if ( kBW_DebugRenderPossibleObjects )
  {
    const Pose3d* worldOriginPtr = _robot.GetWorldOrigin();
    _robot.GetContext()->GetVizManager()->EraseSegments("AIWhiteboard.PossibleObjects");
    for ( auto& possibleObjectIt : _possibleObjects )
    {
      // this offset should not be applied pose
      const Vec3f& zRenderOffset = (Z_AXIS_3D() * kBW_DebugRenderPossibleObjectsZ);
      
      Pose3d thisPose;
      if ( possibleObjectIt.pose.GetWithRespectTo(*worldOriginPtr, thisPose))
      {
        const float kLen = kBW_PossibleObjectClose_mm;
        Quad3f quad3({ kLen,  kLen, 0},
                     {-kLen,  kLen, 0},
                     { kLen, -kLen, 0},
                     {-kLen, -kLen, 0});
        thisPose.ApplyTo(quad3, quad3);
        quad3 += zRenderOffset;
        _robot.GetContext()->GetVizManager()->DrawQuadAsSegments("AIWhiteboard.PossibleObjects",
            quad3, NamedColors::ORANGE, false);

        Vec3f directionEndPoint = X_AXIS_3D() * kLen*0.5f;
        directionEndPoint = thisPose * directionEndPoint;
        _robot.GetContext()->GetVizManager()->DrawSegment("AIWhiteboard.PossibleObjects",
            thisPose.GetTranslation() + zRenderOffset, directionEndPoint + zRenderOffset, NamedColors::YELLOW, false);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdateBeaconRender()
{
  // re-draw all beacons since they all use the same id
  if ( kBW_DebugRenderBeacons )
  {
    const std::string renderId("AIWhiteboard.UpdateBeaconRender");
    _robot.GetContext()->GetVizManager()->EraseSegments(renderId);
  
    // iterate all beacons and render
    for( const auto& beacon : _beacons )
    {
      // currently we don't support beacons from older origins (rsam: I will soon)
      DEV_ASSERT(&beacon.GetPose().FindOrigin() == _robot.GetWorldOrigin(),
                 "AIWhiteboard.UpdateBeaconRender.BeaconFromOldOrigin");
      
      // note that since we don't know what timeout behaviors use, we can only say that it ever failed
      ColorRGBA color = NEAR_ZERO(beacon.GetLastTimeFailedToFindLocation()) ? NamedColors::DARKGREEN : NamedColors::ORANGE;
      
      Vec3f center = beacon.GetPose().GetWithRespectToOrigin().GetTranslation();
      center.z() += kBW_DebugRenderBeaconZ;
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius(), color, false);
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius()-0.5f, color, false); // double line
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius()-1.0f, color, false); // triple line (jane's request)
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetObjectTapInteraction(const ObjectID& objectID)
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(objectID);
  
  // We still want to do something with this double tapped object even if it doesn't exist in the
  // current frame
  if(object != nullptr)
  {
    bool filterCanUseObject = false;
    
    // For all intentions and their filters check if the filter can use the objectID
    // and if so set objectID as the best object for the intention
    for(const auto& filterPair : _filtersPerAction)
    {
      const auto& actionIntent = filterPair.first;
      const auto& filterPtr = filterPair.second;

      const ObjectID oldBest = _bestObjectForAction[actionIntent];      
      _bestObjectForAction[actionIntent].UnSet();
      
      if(filterPtr &&
         filterPtr->ConsiderObject(object))
      {

        if( oldBest != objectID ) {
          PRINT_CH_INFO("AIWhiteboard", "SetBestObjectForTap",
                        "Setting tapped object %d as best for intention '%s'",
                        objectID.GetValue(),
                        ObjectUseIntentionToString(actionIntent));
        }
        
        _bestObjectForAction[actionIntent] = objectID;
        filterCanUseObject = true;
      }
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

  _haveTapIntentionObject = true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ClearObjectTapInteraction()
{
  _haveTapIntentionObject = false;
  
  // Let ReactToDoubleTap be able to react if the next double tapped object is the same as the last
  // double tapped object
  _canReactToDoubleTapReactAgain = true;
  
  _suppressReactToDoubleTap = false;
  
  // Update the valid objects for each ObjectUseIntention now that we don't have a tapIntentionObject
  UpdateValidObjects();
}

} // namespace Cozmo
} // namespace Anki
