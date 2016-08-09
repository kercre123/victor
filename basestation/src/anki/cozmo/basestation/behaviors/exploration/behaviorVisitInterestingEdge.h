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

namespace Anki {
namespace Cozmo {
  
class BehaviorVisitInterestingEdge : public IBehavior
{
private:
  
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
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;  
  virtual void   StopInternal(Robot& robot) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void TransitionToS1_MoveToVantagePoint(Robot& robot, uint8_t retries);
  void TransitionToS2_ObserveAtVantagePoint(Robot& robot);
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // behavior configuration passed on initialization
  struct Configuration
  {
    std::string observeEdgeAnimTrigger;   // animation to play when we observe an interesting edge
    std::string goalDiscardedAnimTrigger; // animation to play when a goal is discarded
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
  
  // score/distance associated with a border
  struct BorderScore {
    BorderScore() : borderInfo(), distanceSQ(0) {}
    BorderScore(const NavMemoryMapTypes::Border& b, float dSQ) : borderInfo(b), distanceSQ(dSQ) {}
    NavMemoryMapTypes::Border borderInfo;
    float distanceSQ;
  };
  
  using BorderScoreVector = std::vector<BorderScore>;
  using VantagePointVector = std::vector<Pose3d>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Border processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // select the border segments we want to visit. It queries the robot's nav memory map to retrieve borders
  // and then selects a few of them among them, returning them in the outGoals vector
  void PickGoals(Robot& robot, BorderScoreVector& outGoals) const;
  
  // returns true if the goal position appears to be reachable from the current position by raycasting in the memory map
  bool CheckGoalReachable(Robot& robot, const Vec3f& goalPosition);

  // given a set of border goals, generate the vantage points for the robot to observe/clear those borders
  void GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const;
  
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
  
  // point we are looking at
  Vec3f _lookAtPoint;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
