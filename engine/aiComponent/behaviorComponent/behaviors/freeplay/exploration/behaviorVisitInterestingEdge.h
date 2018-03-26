/**
 * File: behaviorVisitInterestingEdge
 *
 * Author: Raul
 * Created: 07/19/16
 *
 * Description: Behavior to visit interesting edges from the memory map. Some decision on whether we want to
 * visit any edges found there can be done by the behavior.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorVisitInterestingEdge_H__
#define __Cozmo_Basestation_Behaviors_BehaviorVisitInterestingEdge_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/navMap/iNavMap.h"

#include "coretech/common/engine/math/pose.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class IActionRunner;
  
class BehaviorVisitInterestingEdge : public ICozmoBehavior
{
private:
  
  using BaseClass = ICozmoBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorVisitInterestingEdge(const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // destructor
  virtual ~BehaviorVisitInterestingEdge();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // true if currently there are edges that Cozmo would like to visit
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  // update internal: to handle discarding more goals while running
  virtual void BehaviorUpdate() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Logic Helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // create parallel actions to lower lift and head. Caller is responsible for releasing memory
  IActionRunner* CreateLowLiftAndLowHeadActions();
  
  // wait for new edges (while playing other actions if needed)
  void StartWaitingForEdges();
  bool IsWaitingForImages() const { return (_waitForImagesActionTag != ActionConstants::INVALID_TAG); }
  
  // squint loop needs to be stopped manually
  void StopSquintLoop();
  bool IsPlayingSquintLoop() const { return (_squintLoopAnimActionTag != ActionConstants::INVALID_TAG); }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // S1: move to a vantage point where we expect to see the border again
  void TransitionToS1_MoveToVantagePoint(uint8_t attemptsDone);
  // S2: move slowly and try to gather more accurate information of where the border is
  void TransitionToS2_GatherAccurateEdge();
  // S3: observe the border (play anim) from a safe close distance
  void TransitionToS3_ObserveFromClose();
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // behavior configuration passed on initialization
  struct Configuration
  {
    AnimationTrigger observeEdgeAnimTrigger;   // animation to play when we observe an interesting edge
    AnimationTrigger edgesNotFoundAnimTrigger; // animation to play when we reached the vantage point but can't see edges anymore
    AnimationTrigger squintStartAnimTrigger;   // animation to play when we focus on edges (start)
    AnimationTrigger squintLoopAnimTrigger;    // animation to play when we focus on edges (loop)
    AnimationTrigger squintEndAnimTrigger;     // animation to play when we focus on edges (end)
    // goal selection / vantage point calculation
    bool    allowGoalsBehindOtherEdges = true;        // if set to false, goals behind other edges won't be selected, if true any goal is valid
    float   distanceFromLookAtPointMin_mm = 0.0f;     // min value of random distance from LookAt point to generate vantage point
    float   distanceFromLookAtPointMax_mm = 0.0f;     // max value of random distance from LookAt point to generate vantage point
    float   distanceInsideGoalToLookAt_mm = 0.0f;     // the actual point to look at will be inside the interesting border by this distance (important for offsets)
    float   additionalClearanceInFront_mm = 0.0f;     // additional distance in front of the robot that we need to clear (with raycast) to be a valid vantage point
    float   additionalClearanceBehind_mm  = 0.0f;     // additional distance behind the robot that we have to clear (with raycast) to be a valid vantage point
    float   vantagePointAngleOffsetPerTry_deg = 0.0f; // how much we rotate the vantage vector every attempt
    uint8_t vantagePointAngleOffsetTries = 0;         // how many offsets we try after the vantageDirection. 1 try means +offset and -offset
    // params for focused/accurate edges
    float   accuracyDistanceFromBorder_mm = 0.0f;    // distance from the last detected border that indicates the border is as accurate as it can be
    float   observationDistanceFromBorder_mm = 0.0f; // distance from the last detected border to observe the object (normally closer than accuracyDistance)
    float   borderApproachSpeed_mmps = 0.0f;         // speed to approach the border after reaching the vantage point
    // see diagram below for forwardCone
    float forwardConeHalfWidthAtRobot_mm = 0.0f;       // half width at robot pose of the cone that flags non-interesting edges
    float forwardConeFarPlaneDistFromRobot_mm = 0.0f;  // distance of far plane with respect to the robot
    float forwardConeHalfWidthAtFarPlane_mm = 0.0f;    // half width at far plane of the cone that flags non-interesting edges
  };
  
  // enum for how the behavior is running
  enum class EOperatingState {
    Invalid,               // not set
    MovingToVantagePoint,  // Cozmo is interested in a goal, and is moving to the vantage point
    GatheringAccurateEdge, // Cozmo reached the vantage point and is gathering a more accurate position for the border
    Observing,             // Cozmo has reached a close point to the border and is observing the border
    DoneVisiting           // Cozmo is done visiting the interesting edge (either it didn't find it or it observed the object)
  };
  
  // score/distance associated with a border region
  struct BorderRegionScore {
    using BorderRegion  = MemoryMapTypes::BorderRegion;
    using BorderSegment = MemoryMapTypes::BorderSegment;
    BorderRegionScore(const BorderRegion* rPtr, const size_t idx, float dSQ);
    // is valid if any field is set (we trust all are valid when set)
    inline bool IsValid() const { return borderRegionPtr && !borderRegionPtr->segments.empty(); }
    // get selected segment
    const BorderSegment& GetSegment() const;
    // attributes
    const BorderRegion* borderRegionPtr; // region information (non-owning pointer)
    size_t idxClosestSegmentInRegion;    // index of the closest segment (to the robot) in the region
    float distanceSQ;                    // distance to closest segment (used as score for the region)
  };
  
  using BorderRegionScoreVector = std::vector<BorderRegionScore>;
  using VantagePointVector = std::vector<Pose3d>;

  // information that we cache during WantsToBeActivated, which is const but actually computes important information
  struct CacheForWantsToBeActivated {
    inline void Set(VantagePointVector&& points);
    inline void Reset() { _vantagePoints.clear(); }
    inline bool IsSet() const { return !_vantagePoints.empty(); }
    // set of points the robot is interested in visiting towards clearing borders
    VantagePointVector _vantagePoints;
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Border processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // select goals we would want to visit (big enough small and apparently reachable)
  void PickGoals(BorderRegionScoreVector& validGoals) const;
  
  // returns true if the goal position appears to be reachable from the current position by raycasting in the memory map
  bool CheckGoalReachable(const Vec3f& goalPosition) const;

  // given a set of border goals, generate the vantage points for the robot to observe/clear those borders
  void GenerateVantagePoints(const BorderRegionScore& goal, const Vec3f& lookAtPoint, VantagePointVector& outVantagePoints) const;
  
  // flags a quad in front of the robot as not interesting. The quad is defined from robot pose forward
  // Each plane (near/far) has a width defined in parameters
  //         ! ! ! ! forwardCone ! ! ! !
  //
  //                                    farPlane
  // halfWidthAtRobot*2     _______-------v
  //          v_______------              |
  //  +-------|-+                         |
  //  | Robot * |                         | halfWidthAtFarPlane*2
  //  +-------|-+                         |
  //           -------_____               |
  //                       --------_______|
  //          |<------------------------->|
  //               farPlaneDistFromRobot
  //
  static void FlagVisitedQuadAsNotInteresting(BehaviorExternalInterface& bei, float halfWidthAtRobot_mm, float farPlaneDistFromRobot_mm, float halfWidthAtFarPlane_mm);
  
  // create a quad around a goal that is not interesting anymore (note it's around the goal, not around the lookAt point)
  // The quad is aligned with the given normal
  static void FlagQuadAroundGoalAsNotInteresting(BehaviorExternalInterface& bei, const Vec3f& goalPoint, const Vec3f& goalNormal, float halfQuadSideSize_mm);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Operating state
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Cozmo is gathering accurate edge information
  void StateUpdate_GatheringAccurateEdge();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Debug helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // debug render functions
  void RenderDiscardedRegion(const MemoryMapTypes::BorderRegion& region) const;
  void RenderAcceptedRegion(const MemoryMapTypes::BorderRegion& region) const;
  void RenderChosenGoal(const BorderRegionScore& bestGoal) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Init
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set configuration values from the given config
  void LoadConfig(const Json::Value& config);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // parsed configurations params from json
  Configuration _configParams;

  // we calculate this info during WanstToBeActivated, there's no reason to recompute, store in this mutable cache (eek)
  mutable CacheForWantsToBeActivated _cache;
  
  // tag of the wait for images action when we are focused on edges
  u32                 _waitForImagesActionTag;
  
  // tag of the play anim action playing loop squint
  u32  _squintLoopAnimActionTag;
  
  // area currently considered interesting edges
  double _interestingEdgesArea_m2;
  
  // what Cozmo is doing
  EOperatingState _operatingState;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Inline functions/methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::CacheForWantsToBeActivated::Set(VantagePointVector&& points)
{
  _vantagePoints = points;
}
 

} // namespace Cozmo
} // namespace Anki

#endif //
