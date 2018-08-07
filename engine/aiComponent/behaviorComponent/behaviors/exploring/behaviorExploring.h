/**
 * File: behaviorExploring.h
 *
 * Author: ross
 * Created: 2018-03-25
 *
 * Description: exploration behavior
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploring__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploring__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/engine/math/pose.h"
#include "util/random/rejectionSamplerHelper_fwd.h"

namespace Anki {
namespace Vector {
  
class BehaviorExploringExamineObstacle;
class INavMap;
struct PathMotionProfile;
namespace RobotPointSamplerHelper {
  class RejectIfNotInRange;
  class RejectIfWouldCrossCliff;
  class RejectIfCollidesWithMemoryMap;
}

class BehaviorExploring : public ICozmoBehavior
{
public: 
  virtual ~BehaviorExploring();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorExploring(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void TransitionToDriving();
  void TransitionToArrived();
  
  // helper to decide state once a delegated behavior finishes
  void RegainedControl();
  
  // choose several goal poses based on multiple criteria, either in free space or near an unexplored obstacle,
  // and have the planner choose which one to drive to
  void SampleAndDrive();
  
  // check raw prox data for validity and put the lift down if in the way
  void PrepRobotForProx();
  
  // after calling a driveToPoseAction, and a plan has been computed, this will leave only the selected
  // pose and remove all the others from the cache
  void AdjustCachedPoses();
  
  // euclidean dist to goal[idx], which must exist
  float CalcDistToCachedGoal(int idx) const;
  
  // object helpers
  
  // get the charger, or nullptr
  const ObservableObject* GetCharger() const;
  // returns true if the charger position is in blockworld in the robot frame
  bool IsChargerPositionKnown() const;
  // returns true if the cube is within some distance from the charger (obviously the charger pos must be known)
  bool IsCubeNearCharger() const;
  
  // sampling helpers
  
  // samples points that satisfy certain criteria using the below helpers
  std::vector<Pose3d> SampleVisitLocations() const;
  
  // samples UP TO kNumPosesForSearch*kNumAnglesAtPose points in an annulus around the robot that
  // also lie within some distance to the charger, using rejection sampling. Some other criteria apply,
  // such as not being on the other side of a cliff obstacle, etc. After kNumSampleSteps samples,
  // retPoses contains however many samples have been accepted (which might be none!)
  void SampleVisitLocationsOpenSpace( const INavMap* memoryMap,
                                      bool tooFarFromCharger,
                                      bool chargerEqualsRobot,
                                      const Point2f& chargerPos,
                                      const Point2f& robotPos,
                                      std::vector<Pose3d>& retPoses ) const;
  
  // samples UP TO kNumProxPoses that are slightly offset from an unexplored prox obstacle and facing it
  void SampleVisitLocationsFacingObstacle( const INavMap* memoryMap,
                                           const ObservableObject* charger,
                                           const Point2f& robotPos,
                                           std::vector<Pose3d>& retPoses ) const;
  
  
  enum class State : uint8_t {
    Invalid=0,
    Driving,    // driving to point
    Arrived,    // arrived at sample point, looking around
    Complete,
  };

  struct InstanceConfig {
    bool allowNoCharger;
    float minSearchRadius_m;
    float maxSearchRadius_m;
    float maxChargerDistance_m;
    float pAcceptKnownAreas;
    
    std::shared_ptr<BehaviorExploringExamineObstacle> examineBehavior;
    ICozmoBehaviorPtr confirmChargerBehavior;
    ICozmoBehaviorPtr confirmCubeBehavior;
    
    ICozmoBehaviorPtr referenceHumanBehavior;
    
    std::unique_ptr<PathMotionProfile> customMotionProfile;
    
    std::unique_ptr<Util::RejectionSamplerHelper<Point2f>> openSpacePointEvaluator;
    std::unique_ptr<Util::RejectionSamplerHelper<Poly2f>> openSpacePolyEvaluator;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfNotInRange> condHandleNearCharger;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfWouldCrossCliff> condHandleCliffs;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfCollidesWithMemoryMap> condHandleCollisions;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfCollidesWithMemoryMap> condHandleUnknowns;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    State state;
    
    std::vector<Pose3d> sampledPoses;
    bool posesHaveBeenPruned; // true if poses now contains only the selected goal
    float distToGoal_mm; // the distance to the selected goal, if posesHaveBeenPruned, otherwise negative
    int numDriveAttemps;
    bool hasTakenPitStop;
    float timeFinishedConfirmCharger_s;
    float timeFinishedConfirmCube_s;
    
    size_t devWarnIfNotInterruptedByTick;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploring__
