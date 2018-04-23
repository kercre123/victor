/**
 * File: behaviorExploring.h
 *
 * Author: ross
 * Created: 2018-03-25
 *
 * Description: prototype exploration behavior
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploring__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploring__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/math/pose.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorExploringExamineObstacle;
class INavMap;

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
                                      const Point2f& chargerPos,
                                      const Point2f& robotPos,
                                      std::vector<Pose3d>& retPoses ) const;
  
  // samples UP TO kNumProxPoses that are slightly offset from an unexplored prox obstacle and facing it
  void SampleVisitLocationsFacingObstacle( const INavMap* memoryMap,
                                           const ObservableObject* charger,
                                           std::vector<Pose3d>& retPoses ) const;
  
  // get a vector of values in [0,1] such that all values sum to 1 and divide the unit interval
  // with a random number of segments, up to maxNumSegments
  std::vector<float> SampleUnitPartition( unsigned int maxNumSegments ) const;
  
  
  enum class State : uint8_t {
    Invalid=0,
    Driving,    // driving to point
    Arrived,    // arrived at sample point, looking around
    Complete,
  };

  struct InstanceConfig {
    float minSearchRadius_m;
    float maxSearchRadius_m;
    float maxChargerDistance_m;
    float pAcceptKnownAreas;
    
    std::shared_ptr<BehaviorExploringExamineObstacle> examineBehavior;
    ICozmoBehaviorPtr confirmChargerBehavior;
    ICozmoBehaviorPtr confirmCubeBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    State state;
    
    std::vector<Pose3d> sampledPoses;
    bool posesHaveBeenPruned; // true if poses now contains only the selected goal
    float distToGoal_mm; // the distance to the selected goal, if posesHaveBeenPruned, otherwise negative
    float angleAtArrival_rad;
    int numDriveAttemps;
    bool hasTakenPitStop;
    float timeFinishedConfirmCharger_s;
    float timeFinishedConfirmCube_s;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploring__
