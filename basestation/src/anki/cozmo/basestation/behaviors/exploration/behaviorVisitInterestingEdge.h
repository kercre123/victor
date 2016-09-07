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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"

#include "anki/common/basestation/math/pose.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorVisitInterestingEdge : public IBehavior
{
private:
  
  using BaseClass = IBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorVisitInterestingEdge(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // destructor
  virtual ~BehaviorVisitInterestingEdge();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // true if currently there are edges that Cozmo would like to visit
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;  
  virtual void   StopInternal(Robot& robot) override;

  // update internal: to handle discarding more goals while running
  virtual Status UpdateInternal(Robot& robot) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void TransitionToS1_MoveToVantagePoint(Robot& robot, uint8_t retries);
  void TransitionToS2_ObserveAtVantagePoint(Robot& robot);
  
  // evaluates what would be the best goal if we were to select one now, and if it's not reachable it discards it,
  // without starting new actions or changing Cozmo. This can run while Cozmo is performing other actions
  // returns true if it discarded a goal, false if either there are no more goals or the next one won't be discarded
  bool DiscardNextUnreachableGoal(Robot& robot) const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // behavior configuration passed on initialization
  struct Configuration
  {
    AnimationTrigger observeEdgeAnimTrigger;   // animation to play when we observe an interesting edge
    AnimationTrigger goalDiscardedAnimTrigger; // animation to play when a goal is discarded
    // goal selection / vantage point calculation
    bool    allowGoalsBehindOtherEdges = true;        // if set to false, goals behind other edges won't be selected, if true any goal is valid
    float   distanceFromLookAtPointMin_mm = 0.0f;     // min value of random distance from LookAt point to generate vantage point
    float   distanceFromLookAtPointMax_mm = 0.0f;     // max value of random distance from LookAt point to generate vantage point
    float   distanceInsideGoalToLookAt_mm = 0.0f;     // the actual point to look at will be inside the interesting border by this distance (important for offsets)
    float   additionalClearanceInFront_mm = 0.0f;     // additional distance in front of the robot that we need to clear (with raycast) to be a valid vantage point
    float   additionalClearanceBehind_mm  = 0.0f;     // additional distance behind the robot that we have to clear (with raycast) to be a valid vantage point
    float   vantagePointAngleOffsetPerTry_deg = 0.0f; // how much we rotate the vantage vector every attempt
    uint8_t vantagePointAngleOffsetTries = 0;         // how many offsets we try after the vantageDirection. 1 try means +offset and -offset
    // see diagram below for vantageCone
    float vantageConeHalfWidthAtRobot_mm = 0.0f;       // half width at robot pose of the cone that flags non-interesting edges
    float vantageConeFarPlaneDistFromLookAt_mm = 0.0f; // distance of far plane with respect to the lookAt point (=goal + distanceInsideGoalToLookAt_mm)
    float vantageConeHalfWidthAtFarPlane_mm = 0.0f;    // half width at far plane of the cone that flags non-interesting edges
    // quad that clears around a goal for which we couldn't calculate a good vantage point
    float noVantageGoalHalfQuadSideSize_mm = 50.0f; // when we can't find a goal, half of the side size of the quad centered around the goal that flags as not interesting (optional)
  };
  
  // enum for how the behavior is running
  enum class EOperatingState {
    Invalid,          // not set
    DiscardingGoals,  // Cozmo found a non reachable edge, so he is going to be thinking about it and discarding others
    DoneDiscarding,   // Cozmo was discarding goals, but there are no more goals or the next one would be reachable
    VisitingGoal      // Cozmo is interested in a goal, and is moving there to visit it
  };
  
  // score/distance associated with a border
  struct BorderScore {
    BorderScore() : borderInfo(), distanceSQ(FLT_MAX) {}
    // clear info and distance
    inline void Reset() {
      borderInfo = NavMemoryMapTypes::Border();
      distanceSQ = FLT_MAX;
    }
    // set info and distance
    void Set(const NavMemoryMapTypes::Border& b, float dSQ) {
      borderInfo = b;
      distanceSQ = dSQ;
    }
    inline bool IsValid() const { return !FLT_NEAR(distanceSQ, FLT_MAX); }
    // attributes
    NavMemoryMapTypes::Border borderInfo;
    float distanceSQ;
  };
  
  using VantagePointVector = std::vector<Pose3d>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Border processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // select the best goal that we would want to visit (based on euclidean distance or other scoring)
  void PickBestGoal(Robot& robot, BorderScore& bestGoal) const;
  
  // returns true if the goal position appears to be reachable from the current position by raycasting in the memory map
  bool CheckGoalReachable(const Robot& robot, const Vec3f& goalPosition) const;

  // given a set of border goals, generate the vantage points for the robot to observe/clear those borders
  void GenerateVantagePoints(Robot& robot, const BorderScore& goal, const Vec3f& lookAtPoint, VantagePointVector& outVantagePoints) const;
  
  // flags a quad in front of the robot as not interesting. The quad is defined from robot pose to the look at point, plus
  // an additional distance from the lookAt point. Each plane (at robot and at lookAt+dist) has a width defined in parameters
  //         ! ! ! ! vantageCone ! ! ! !
  //
  //                                    farPlane
  // halfWidthAtRobot*2     _______-------v
  //          v_______------     goal     |
  //  +-------|-+                  v      |
  //  | Robot * |                  $  $   | halfWidthAtFarPlane*2
  //  +-------|-+                     ^   |
  //           -------_____        lookAt |
  //                       --------_______|
  //                                 |<-->|
  //                                  farPlaneDistFromGoal
  //
  static void FlagVisitedQuadAsNotInteresting(Robot& robot, float halfWidthAtRobot_mm, const Vec3f& lookAtPoint,
    float farPlaneDistFromLookAt_mm, float halfWidthAtFarPlane_mm);
  
  // create a quad around a goal that is not interesting anymore (note it's around the goal, not around the lookAt point)
  // The quad is aligned with the given normal
  static void FlagQuadAroundGoalAsNotInteresting(Robot& robot, const Vec3f& goalPoint, const Vec3f& goalNormal, float halfQuadSideSize_mm);
  
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

  // set of points the robot is interested in visiting towards clearing borders
  VantagePointVector _currentVantagePoints;
  
  // point we are looking at (created from the goal)
  Vec3f _lookAtPoint;
  
  // what Cozmo is doing
  EOperatingState _operatingState;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
