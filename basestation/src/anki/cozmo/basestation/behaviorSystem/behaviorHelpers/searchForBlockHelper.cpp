/**
 * File: searchForBlockHelper.cpp
 *
 * Author: Kevin M. Karol
 * Created: 5/8/17
 *
 * Description: Handles searching under three different paradigms
 *   1) Search for a specific ID
 *   2) Search until a certain number of blocks are seen
 *   3) Search a given amount regardless of what you see
 * There are also three levels of intensity with which each of these types
 * of searches can be performed covering increasingly large search areas
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/searchForBlockHelper.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"


namespace Anki {
namespace Cozmo {  

namespace{
static const f32 kDriveBackDist_mm = 20;
static const f32 kDriveBackSpeed_mmps = 20;
static const int kSearchAdditionalSegments = 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SearchForBlockHelper::SearchForBlockHelper(Robot& robot,
                                           IBehavior& behavior,
                                           BehaviorHelperFactory& helperFactory,
                                           const SearchParameters& params)
: IHelper("SearchForBlockHelper", robot, behavior, helperFactory)
, _params(params)
, _nextSearchIntensity(SearchIntensity::QuickSearch)
{

  auto observedObjectCallback =
  [this, &robot](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event){
    const ObjectID& objSeen = event.GetData().Get_RobotObservedObject().objectID;
    
    if(_params.searchingForID.IsSet()){
      // Search should stop if we either 1) see the cube we're looking for and have
      // therefore updated its known pose or 2) see an object that may have been
      // obstructing the robots view without its knowledge previously
      if(objSeen == _params.searchingForID){
        _status = BehaviorStatus::Complete;
      }else if(_objectsSeenDuringSearch.count(objSeen) == 0){
        if(!ShouldBeAbleToFindTarget(robot)){
          _status = BehaviorStatus::Failure;
        }
        _objectsSeenDuringSearch.insert(objSeen);
      }
    }else if((_params.numberOfBlocksToLocate > 0) &&
               (_objectsSeenDuringSearch.size() >= _params.numberOfBlocksToLocate)){
      // Search can also stop if we've seen the requested number of blocks
      _status = BehaviorStatus::Complete;
    }
  };
  
  if(robot.HasExternalInterface()){
    using namespace ExternalInterface;
    _eventHandlers.push_back(robot.GetExternalInterface()->Subscribe(
                    ExternalInterface::MessageEngineToGameTag::RobotObservedObject,
                    observedObjectCallback));
  }
  
  
  // Three options - search without a purpose, search for a located block
  // or search until x many blocks are acquired - but you're not allowed to
  // have multiple goals for the helper
  DEV_ASSERT(!(params.searchingForID.IsSet() &&
                (params.numberOfBlocksToLocate != 0)), "SearchForBlockHelper.SpecifiedBothObjIDAndCount");
  
  // This helper should only be used to search for blocks that we think should be
  // in the area or to search in general to acquire blocks - it should not
  // be used to search for blocks by ID that currently are not located - that
  // will result in an unnecessary nuke of block world
  const auto& locatedObj = robot.GetBlockWorld().GetLocatedObjectByID(params.searchingForID);
  DEV_ASSERT((params.searchingForID.IsUnknown()) ||
             (locatedObj != nullptr), "SearchForBlockHelper.ObjectSpecifiedIsNotLocated");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SearchForBlockHelper::~SearchForBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SearchForBlockHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus SearchForBlockHelper::Init(Robot& robot)
{
  _nextSearchIntensity = SearchIntensity::QuickSearch;
  _robotCameraAtSearchStart = robot.GetVisionComponent().GetCamera();
  _objectsSeenDuringSearch.clear();
  
  SearchForBlock(ActionResult::NOT_STARTED, robot);

  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus SearchForBlockHelper::UpdateWhileActiveInternal(Robot& robot)
{
  if(_status != BehaviorStatus::Running){
    StopActing(false);
  }
  
  return _status;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SearchForBlockHelper::SearchForBlock(ActionResult result, Robot& robot)
{
  
  const auto& targetID = _params.searchingForID;
  
  if(targetID.IsSet()){
    const ObservableObject* targetObj = robot.GetBlockWorld().GetLocatedObjectByID(targetID);
    if(targetObj == nullptr) {
      _status = BehaviorStatus::Failure;
      return;
    }
  }
  
  if(_nextSearchIntensity > _params.searchIntensity){
    SearchFinishedWithoutInterruption(robot);
    return;
  }
  

  // Increment search level
  // Search for nearby - turn counter clockwise 90 deg - search for nearby
  // Turn clockwise 90 (from start) - search for nearby - nuclear option
  switch(_nextSearchIntensity){
    case SearchIntensity::QuickSearch:
    {
      auto searchNearby = new SearchForNearbyObjectAction(robot, targetID);
      
      CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
      compoundAction->AddAction(new TurnTowardsObjectAction(robot, targetID), true);
      compoundAction->AddAction(searchNearby);
      StartActing(compoundAction, &SearchForBlockHelper::SearchForBlock);
      _nextSearchIntensity = SearchIntensity::StandardSearch;
      break;
    }
    case SearchIntensity::StandardSearch:
    {
      const bool ignoreFailure = true;
      CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
      compoundAction->AddAction(new DriveStraightAction(robot,
                                                        -kDriveBackDist_mm,
                                                        kDriveBackSpeed_mmps,
                                                        false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(robot, M_PI_4, false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(robot, M_PI_4, false), ignoreFailure);
      
      if(targetID.IsSet()){
        compoundAction->AddAction(new SearchForNearbyObjectAction(robot, targetID), ignoreFailure);
      }

      compoundAction->AddAction(new DriveStraightAction(robot,
                                                        -kDriveBackDist_mm,
                                                        kDriveBackSpeed_mmps,
                                                        false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(robot, -( M_PI_2 + M_PI_4), false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false), ignoreFailure);
      
      if(targetID.IsSet()){
        compoundAction->AddAction(new SearchForNearbyObjectAction(robot, targetID), ignoreFailure);
      }
      
      StartActing(compoundAction, &SearchForBlockHelper::SearchForBlock);
      _nextSearchIntensity = SearchIntensity::ExhaustiveSearch;
      break;
    }
    case SearchIntensity::ExhaustiveSearch:
    {
      CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
      
      for(int i = 0; i < kSearchAdditionalSegments; i++){
        compoundAction->AddAction(new DriveStraightAction(robot,
                                                          -kDriveBackDist_mm,
                                                          kDriveBackSpeed_mmps,
                                                          false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false));
        
        if(targetID.IsSet()){
          compoundAction->AddAction(new SearchForNearbyObjectAction(robot, targetID));
        }
      }
      
      auto exhaustiveSearchFailure = [this](ActionResult res, Robot& robot){
        SearchFinishedWithoutInterruption(robot);
      };
      
      StartActing(compoundAction, exhaustiveSearchFailure);
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SearchForBlockHelper::SearchFinishedWithoutInterruption(Robot& robot)
{
  // In the first two cases we finished a complete search without finding what we wanted
  // so we failed
  // In the third case, we wanted to complete the full search, so it's a success
  if(_params.searchingForID.IsSet()){
    const ObservableObject* targetObj = robot.GetBlockWorld().GetLocatedObjectByID(_params.searchingForID);
    if((targetObj != nullptr) &&
       targetObj->IsPoseStateKnown() &&
       (_params.searchIntensity >= SearchIntensity::StandardSearch)){
      PRINT_NAMED_ERROR("SearchForBlockHelper.SearchForBlock.GoingNuclear",
                        "Failed to find known block - wiping");
      BlockWorldFilter filter;
      filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame); // not necessary, just to be explicit
      robot.GetBlockWorld().DeleteLocatedObjects(filter);
    }
    _status = BehaviorStatus::Failure;
  }else if((_params.numberOfBlocksToLocate > 0) &&
           (_objectsSeenDuringSearch.size() < _params.numberOfBlocksToLocate)){
    _status = BehaviorStatus::Failure;
  }else{
    _status = BehaviorStatus::Complete;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SearchForBlockHelper::ShouldBeAbleToFindTarget(Robot& robot)
{
  const ObservableObject* targetObj = robot.GetBlockWorld().GetLocatedObjectByID(_params.searchingForID);
  if(targetObj != nullptr){
    // check if the known object should no longer be visible given the robot's
    // camera while at the pre-dock pose
    static constexpr float kMaxNormalAngle = DEG_TO_RAD(45); // how steep of an angle we can see // ANDREW: is this true?
    static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it, size doesn't matter
    
    Vision::KnownMarker::NotVisibleReason reason =
          targetObj->IsVisibleFromWithReason(_robotCameraAtSearchStart,
                                             kMaxNormalAngle,
                                             kMinImageSizePix,
                                             false);
    
    return reason != Vision::KnownMarker::NotVisibleReason::OCCLUDED;
  }
  
  return false;
}

  
} // namespace Cozmo
} // namespace Anki

