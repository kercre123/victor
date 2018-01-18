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


#include "engine/aiComponent/behaviorComponent/behaviorHelpers/searchForBlockHelper.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "coretech/common/engine/utils/timer.h"


namespace Anki {
namespace Cozmo {  

namespace{
static const f32 kDriveBackDist_mm = 20;
static const f32 kDriveBackSpeed_mmps = 20;
static const int kSearchAdditionalSegments = 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SearchForBlockHelper::SearchForBlockHelper(ICozmoBehavior& behavior,
                                           BehaviorHelperFactory& helperFactory,
                                           const SearchParameters& params)
: IHelper("SearchForBlockHelper", behavior, helperFactory)
, _params(params)
, _nextSearchIntensity(SearchIntensity::QuickSearch)
{
  GetBEI().GetBehaviorEventComponent().SubscribeToTags(this,
  {
    ExternalInterface::MessageEngineToGameTag::RobotObservedObject
  });
  
  // Three options - search without a purpose, search for a located block
  // or search until x many blocks are acquired - but you're not allowed to
  // have multiple goals for the helper
  DEV_ASSERT(!(params.searchingForID.IsSet() &&
                (params.numberOfBlocksToLocate != 0)), "SearchForBlockHelper.SpecifiedBothObjIDAndCount");
  
  // This helper should only be used to search for blocks that we think should be
  // in the area or to search in general to acquire blocks - it should not
  // be used to search for blocks by ID that currently are not located - that
  // will result in an unnecessary nuke of block world
  const auto& locatedObj = GetBEI().GetBlockWorld().GetLocatedObjectByID(params.searchingForID);
  DEV_ASSERT((params.searchingForID.IsUnknown()) ||
             (locatedObj != nullptr), "SearchForBlockHelper.ObjectSpecifiedIsNotLocated");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SearchForBlockHelper::~SearchForBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SearchForBlockHelper::ShouldCancelDelegates() const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus SearchForBlockHelper::InitBehaviorHelper()
{
  _nextSearchIntensity = SearchIntensity::QuickSearch;
  if(GetBEI().HasVisionComponent()){
    _robotCameraAtSearchStart = GetBEI().GetVisionComponent().GetCamera();
  }
  _objectsSeenDuringSearch.clear();
  
  SearchForBlock(ActionResult::NOT_STARTED);

  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus SearchForBlockHelper::UpdateWhileActiveInternal()
{
  // Event handles
  const auto& stateChangeComp = GetBEI().GetBehaviorEventComponent();
  for(const auto& event: stateChangeComp.GetEngineToGameEvents()){
    if(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::RobotObservedObject){
      const ObjectID& objSeen = event.GetData().Get_RobotObservedObject().objectID;
      
      if(_params.searchingForID.IsSet()){
        // Search should stop if we either 1) see the cube we're looking for and have
        // therefore updated its known pose or 2) see an object that may have been
        // obstructing the robots view without its knowledge previously
        if(objSeen == _params.searchingForID){
          _status = IHelper::HelperStatus::Complete;
        }else if(_objectsSeenDuringSearch.count(objSeen) == 0){
          if(!ShouldBeAbleToFindTarget()){
            _status = IHelper::HelperStatus::Failure;
          }
          _objectsSeenDuringSearch.insert(objSeen);
        }
      }else if((_params.numberOfBlocksToLocate > 0) &&
               (_objectsSeenDuringSearch.size() >= _params.numberOfBlocksToLocate)){
        // Search can also stop if we've seen the requested number of blocks
        _status = IHelper::HelperStatus::Complete;
      }
    }
  }
  
  
  if(_status != IHelper::HelperStatus::Running){
    CancelDelegates(false);
  }
  
  return _status;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SearchForBlockHelper::SearchForBlock(ActionResult result)
{
  
  const auto& targetID = _params.searchingForID;
  
  if(targetID.IsSet()){
    const ObservableObject* targetObj = GetBEI().GetBlockWorld().GetLocatedObjectByID(targetID);
    if(targetObj == nullptr) {
      _status = IHelper::HelperStatus::Failure;
      return;
    }
  }
  
  if(_nextSearchIntensity > _params.searchIntensity){
    SearchFinishedWithoutInterruption();
    return;
  }
  

  // Increment search level
  // Search for nearby - turn counter clockwise 90 deg - search for nearby
  // Turn clockwise 90 (from start) - search for nearby - nuclear option
  switch(_nextSearchIntensity){
    case SearchIntensity::QuickSearch:
    {
      auto searchNearby = new SearchForNearbyObjectAction(targetID);
      
      CompoundActionSequential* compoundAction = new CompoundActionSequential();
      if(targetID.IsSet()){
        compoundAction->AddAction(new TurnTowardsObjectAction(targetID), true);
      }
      compoundAction->AddAction(searchNearby);
      DelegateIfInControl(compoundAction, &SearchForBlockHelper::SearchForBlock);
      _nextSearchIntensity = SearchIntensity::StandardSearch;
      break;
    }
    case SearchIntensity::StandardSearch:
    {
      const bool ignoreFailure = true;
      CompoundActionSequential* compoundAction = new CompoundActionSequential();
      compoundAction->AddAction(new DriveStraightAction(-kDriveBackDist_mm,
                                                        kDriveBackSpeed_mmps,
                                                        false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(M_PI_4, false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(M_PI_4, false), ignoreFailure);
      
      if(targetID.IsSet()){
        compoundAction->AddAction(new SearchForNearbyObjectAction(targetID), ignoreFailure);
      }

      compoundAction->AddAction(new DriveStraightAction(-kDriveBackDist_mm,
                                                        kDriveBackSpeed_mmps,
                                                        false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(-( M_PI_2 + M_PI_4), false), ignoreFailure);
      compoundAction->AddAction(new TurnInPlaceAction(-M_PI_4, false), ignoreFailure);
      
      if(targetID.IsSet()){
        compoundAction->AddAction(new SearchForNearbyObjectAction(targetID), ignoreFailure);
      }
      
      DelegateIfInControl(compoundAction, &SearchForBlockHelper::SearchForBlock);
      _nextSearchIntensity = SearchIntensity::ExhaustiveSearch;
      break;
    }
    case SearchIntensity::ExhaustiveSearch:
    {
      CompoundActionSequential* compoundAction = new CompoundActionSequential();
      
      for(int i = 0; i < kSearchAdditionalSegments; i++){
        compoundAction->AddAction(new DriveStraightAction(-kDriveBackDist_mm,
                                                          kDriveBackSpeed_mmps,
                                                          false));
        compoundAction->AddAction(new TurnInPlaceAction(-M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(-M_PI_4, false));
        
        if(targetID.IsSet()){
          compoundAction->AddAction(new SearchForNearbyObjectAction(targetID));
        }
      }
      
      auto exhaustiveSearchFailure = [this](ActionResult res){
        SearchFinishedWithoutInterruption();
      };
      
      DelegateIfInControl(compoundAction, exhaustiveSearchFailure);
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SearchForBlockHelper::SearchFinishedWithoutInterruption()
{
  // In the first two cases we finished a complete search without finding what we wanted
  // so we failed
  // In the third case, we wanted to complete the full search, so it's a success
  if(_params.searchingForID.IsSet()){
    const ObservableObject* targetObj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_params.searchingForID);
    if((targetObj != nullptr) &&
       targetObj->IsPoseStateKnown() &&
       (_params.searchIntensity >= SearchIntensity::StandardSearch)){
      PRINT_NAMED_ERROR("SearchForBlockHelper.SearchForBlock.GoingNuclear",
                        "Failed to find known block - wiping");
      BlockWorldFilter filter;
      filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame); // not necessary, just to be explicit
      GetBEI().GetBlockWorld().DeleteLocatedObjects(filter);
    }
    _status = IHelper::HelperStatus::Failure;
  }else if((_params.numberOfBlocksToLocate > 0) &&
           (_objectsSeenDuringSearch.size() < _params.numberOfBlocksToLocate)){
    _status = IHelper::HelperStatus::Failure;
  }else{
    _status = IHelper::HelperStatus::Complete;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SearchForBlockHelper::ShouldBeAbleToFindTarget()
{
  const ObservableObject* targetObj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_params.searchingForID);
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

