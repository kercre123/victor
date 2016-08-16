/**
 * File: behaviorExploreCliff
 *
 * Author: Raul
 * Created: 02/03/16
 *
 * Description: Behavior that looks for nearby areas marked as cliffs and tries to follow along their line
 * in order to identify the borders or the platform the robot is currently on.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorExploreCliff_H__
#define __Cozmo_Basestation_Behaviors_BehaviorExploreCliff_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"

#include "anki/common/basestation/math/pose.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorExploreCliff : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreCliff(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // destructor
  virtual ~BehaviorExploreCliff();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // true if currently there are cliffs that Cozmo would like to explore
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // notified of an action being completed
  void HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg);
  
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
  
  // select the border segments we want to explore. It queries the robot's nav memory map to retrieve borders
  // and then selects a few of them among them, returning them in the outGoals vector
  void PickGoals(Robot& robot, BorderScoreVector& outGoals) const;

  // given a set of border goals, generate the vantage points for the robot to observe/clear those borders
  void GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const;
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // set of points the robot is interested in visiting towards clearing borders
  VantagePointVector _currentVantagePoints;

  // tag for the current move action we have ordered
  uint32_t _currentActionTag;  
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
