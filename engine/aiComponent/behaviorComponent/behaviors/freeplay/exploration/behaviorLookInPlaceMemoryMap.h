/**
 * File: behaviorLookInPlaceMemoryMap
 *
 * Author: Raul
 * Created: 08/03/16
 *
 * Description: Look around in place by checking the memory map and deciding which areas we still need to 
 * discover.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorLookInPlaceMemoryMap_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLookInPlaceMemoryMap_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/events/animationTriggerHelpers.h"

#include "coretech/common/engine/math/pose.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
namespace ExternalInterface {
struct RobotObservedObject;
}
class IAction;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorLookInPlaceMemoryMap
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorLookInPlaceMemoryMap : public ICozmoBehavior
{
private:
  
  using BaseClass = ICozmoBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorLookInPlaceMemoryMap(const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorLookInPlaceMemoryMap() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This behavior is activatable if when we check the memory map around the current robot position, there are still
  // undiscovered areas
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set attributes from the given config
  void LoadConfig(const Json::Value& config);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // attributes specifically for configuration of every state
  struct Configuration
  {
    // turn speeds
    float bodyTurnSpeed_degPerSec;
    float headTurnSpeed_degPerSec;
    // angles
    float bodyAngleFromSectorCenterRangeMin_deg;  // min deviation with respect to the center of the sector
    float bodyAngleFromSectorCenterRangeMax_deg;  // max deviation with respect to the center of the sector
    float t1_headAngleAbsRangeMin_deg;  // min head angle at the first stop inside a sector
    float t1_headAngleAbsRangeMax_deg;  // max head angle at the first stop inside a sector
    float t2_headAngleAbsRangeMin_deg;  // min head angle at the second stop inside a sector
    float t2_headAngleAbsRangeMax_deg;  // max head angle at the second stop inside a sector
    // sectors
    uint8_t prioritySectorCount;     // number of sectors to visit with regular scoring
    float lowPriorityScoreReduction; // score reduction applied after visiting the prioritySectorCount
    // previous locations
    float distanceThresholdForLocations_mm; // distance Cozmo has to move to explore again a location it already explored
    uint8_t maxPreviousLocationCount; // how many locations we can keep track of
    // anims
    AnimationTrigger lookInPlaceAnimTrigger;
    // std::string seeNewInterestingEdgesAnimTrigger;  //
  };
  
  enum class SectorStatus {
    NeedsChecking,   // we need to check if this sector needs visiting
    No_NeedToVisit,  // we checked, and this sector does NOT need to be visited
    Yes_NeedToVisit, // we checked, and this sector needs to be visited
    Visited         // we already visited this sector
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // accessors to mix/max distances to check in the memory map
  static float MinCircleDist();
  static float MaxCircleDist();
  
  // find the closest sector that needs visiting, and visit it (will finish if no more sectores require visiting)
  void FindAndVisitClosestVisitableSector(const int16_t lastIndex, const int16_t nextRight, const int16_t nextLeft);
  
  // udpates the status flag of the given sector (by index), by checking in the memory map if we want to visit it
  void CheckIfSectorNeedsVisit(int16_t index);
  
  // turn towards the given sector to clear its memory map, the continue onto the closest next sector
  void VisitSector(int16_t index, const int16_t nextLeft, const int16_t nextRight);

  // we visited all the sectors and are done turning in place
  void FinishedAllSectorsAtLocation();
  
  // returns true if the given sector (by index) needs to be checked, false otherwise. Asserts it's only called in non-visited (to ensure algorithm completion)
  inline bool NeedsChecking(int16_t index) const;
  // returns true if the given sector (by index) needs visiting, false otherwise. Asserts it's only called in checked and non-visited
  inline bool NeedsVisit(int16_t index) const;
  
  // returns the center of the cone that defines the sector (by index), in relative angle (meaning regardless of robot rotation)
  float GetRelativeAngleOfSectorInDegrees(int16_t index) const;
  
  // request the proper action given the parameters so that the robot turns and moves head
  IAction* CreateBodyAndHeadTurnAction(float bodyRelativeMin_deg, float bodyRelativeMax_deg,         // body min/max range relative to target (randomizes to +-range)
                                       float bodyAbsoluteTargetAngle_deg,                            // center of the body rotation range
                                       float headAbsoluteMin_deg, float headAbsoluteMax_deg,         // head min/max range absolute
                                       float bodyTurnSpeed_degPerSec,                                // body turn speed
                                       float headTurnSpeed_degPerSec);                               // head turn speed
  
  // debug render sector status
  void UpdateSectorRender();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // parsed configurations params from json
  Configuration _configParams;

  // facing direction when we start the behavior
  Radians _startingBodyFacing_rad;
  
  // sector status (it always contains up to date info and the right size)
  using SectorList = std::vector<SectorStatus>;
  SectorList _sectors;
  
  // sectors we have visited this run
  uint8_t _visitedSectorCount;
  
  // list of poses we have checked previously
  // at the moment we don't clear them based on timestamp or anything. This would have to be in sync with
  // memory map decay time, or we could simply have a timestamp here so that at least we check for new
  // borders. However with current map implementation the map won't have unknowns, so this behavior won't
  // know which angles to visit. Use full 360 behavior for that case
  using PoseList = std::list<Pose3d>;
  PoseList _previousFullLocations;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Inline methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool BehaviorLookInPlaceMemoryMap::NeedsChecking(int16_t index) const
{
  // index should be valid
  DEV_ASSERT(index>=0 && index<_sectors.size(), "BehaviorLookInPlaceMemoryMap.NeedsChecking.InvalidIndex");
  // visited indices should not be queried again
  DEV_ASSERT(_sectors[index] != SectorStatus::Visited, "BehaviorLookInPlaceMemoryMap.NeedsChecking.AlreadyVisitedSector");
  // check status
  const bool ret = (_sectors[index] == SectorStatus::NeedsChecking);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookInPlaceMemoryMap::NeedsVisit(int16_t index) const
{
  // index should be valid
  DEV_ASSERT(index>=0 && index<_sectors.size(), "BehaviorLookInPlaceMemoryMap.NeedsVisit.InvalidIndex");
  // indices that need raycast should not be queried yet
  DEV_ASSERT(_sectors[index] != SectorStatus::NeedsChecking, "BehaviorLookInPlaceMemoryMap.NeedsVisit.SectorNeedsChecking");
  // visited indices should not be queried again
  DEV_ASSERT(_sectors[index] != SectorStatus::Visited, "BehaviorLookInPlaceMemoryMap.NeedsVisit.AlreadyVisitedSector");
  // check status
  const bool ret = (_sectors[index] == SectorStatus::Yes_NeedToVisit);
  return ret;
}

} // namespace Cozmo
} // namespace Anki

#endif //
