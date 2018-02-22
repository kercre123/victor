// rsam: on Sept/08/2016 I changed the definition for NavMemoryMap::Borders. This behavior would have to be updated,
// but I realized that we haven't used it in some time, and that it was already outdated with respect to
// some improvements that we have added to borders or other behaviors (like behaviorVisistInterestingEdge),
// so I'm not wasting time now since I don't foresee bringing this behavior back. behaviorVisistInterestingEdge can
// actually give us most of the features we would need to make behaviorExploreMarkedCube


///**
// * File: behaviorExploreMarkedCube
// *
// * Author: Raul
// * Created: 01/22/16
// *
// * Description: Behavior that looks for a nearby marked cube that Cozmo has not fully explored (ie: seen in
// * all directions), and tries to see the sides that are yet to be discovered.
// *
// * Copyright: Anki, Inc. 2016
// *
// **/
//
//#ifndef __Cozmo_Basestation_Behaviors_BehaviorExploreMarkedCube_H__
//#define __Cozmo_Basestation_Behaviors_BehaviorExploreMarkedCube_H__
//
//#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
//#include "engine/navMemoryMap/iNavMemoryMap.h"
//
//#include "coretech/common/engine/math/pose.h"
//
//#include "clad/externalInterface/messageEngineToGame.h"
//
//namespace Anki {
//namespace Cozmo {
//  
//class BehaviorExploreMarkedCube : public ICozmoBehavior
//{
//private:
//  
//  // Enforce creation through BehaviorFactory
//  friend class BehaviorFactory;
//  BehaviorExploreMarkedCube(Robot& robot, const Json::Value& config);
//  
//public:
//
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // Initialization/destruction
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  
//  // destructor
//  virtual ~BehaviorExploreMarkedCube();
//
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // ICozmoBehavior API
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//  // true if currently there are marked cubes that Cozmo would like to explore
//  virtual bool WantsToBeActivatedBehavior() const override;
//  virtual bool CarryingObjectHandledInternally() const override { return false;}
//
//protected:
//
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // ICozmoBehavior API
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  
//  virtual void OnBehaviorActivated() override;
//  virtual void BehaviorUpdate() override;
//  virtual void OnBehaviorDeactivated() override;
//
//  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
//
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // Events
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  
//  // notified of an action being completed
//  void HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg);
//  
//private:
//
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // Types
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  
//  // score/distance associated with a border
//  struct BorderScore {
//    BorderScore() : borderInfo(), distanceSQ(0) {}
//    BorderScore(const MemoryMapTypes::Border& b, float dSQ) : borderInfo(b), distanceSQ(dSQ) {}
//    MemoryMapTypes::Border borderInfo;
//    float distanceSQ;
//  };
//  
//  using BorderScoreVector = std::vector<BorderScore>;
//  using VantagePointVector = std::vector<Pose3d>;
//  
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // Border processing
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  
//  // select the border segments we want to explore. It queries the robot's nav memory map to retrieve borders
//  // and then selects a few of them among them, returning them in the outGoals vector
//  void PickGoals(Robot& robot, BorderScoreVector& outGoals) const;
//
//  // given a set of border goals, generate the vantage points for the robot to observe/clear those borders
//  void GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const;
//    
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  // Attributes
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//  // set of points the robot is interested in visiting towards clearing borders
//  VantagePointVector _currentVantagePoints;
//
//  // tag for the current move action we have ordered
//  uint32_t _currentActionTag;
//  
//};
//  
//
//} // namespace Cozmo
//} // namespace Anki
//
//#endif //
