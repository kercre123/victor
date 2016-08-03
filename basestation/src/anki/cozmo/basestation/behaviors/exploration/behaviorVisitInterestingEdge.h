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

  // given a set of border goals, generate the vantage points for the robot to observe/clear those borders
  void GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const;
  
  // flags a quad in front of the robot as not interesting. The quad is defined from robot pose to the goal point plus
  // an additional distance from the goal. Each plane (at robot and at goal+dist) has a width defined in parameters
  //
  //                                    farPlane
  // halfWidthAtRobot*2     _______-------v
  //          v_______------              |
  //  +-------|-+                         |
  //  | Robot * |                    $    | farHalfWidth*2
  //  +-------|-+                   goal  |
  //           -------_____               |
  //                       --------_______|
  //                                 |<-->|
  //                                  farPlaneDistFromGoal
  //
  static void FlagQuadAsNotInteresting(Robot& robot, float halfWidthAtRobot_mm, const Pose3d& goalPosition,
    float farPlaneDistFromGoal_mm, float halfAtFarPlaneWidth_mm);
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // set of points the robot is interested in visiting towards clearing borders
  VantagePointVector _currentVantagePoints;
  
  // goal we are visiting
  Pose3d _goalPose;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
