/**
* File: BehaviorFindHome.h
*
* Author: Matt Michini
* Created: 1/31/18
*
* Description:
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_BehaviorFindHome_H__
#define __Engine_Behaviors_BehaviorFindHome_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "clad/types/animationTrigger.h"

#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/engine/math/pose.h"

#include "util/random/rejectionSamplerHelper_fwd.h"

namespace Anki {
namespace Cozmo {

namespace RobotPointSamplerHelper {
  class RejectIfInRange;
  class RejectIfWouldCrossCliff;
  class RejectIfCollidesWithMemoryMap;
}
  
class BlockWorldFilter;
  
class BehaviorFindHome : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindHome(const Json::Value& config);
  
public:
  virtual ~BehaviorFindHome() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  virtual void OnBehaviorActivated() override;

private:
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    float       minSearchAngleSweep_deg = 0.f;
    int         maxSearchTurns = 0;
    int         maxNumRecentSearches = 0;
    float       recentSearchWindow_sec = 0.f;
    float       minDrivingDist_mm = 0.f;
    float       maxDrivingDist_mm = 0.f;
    
    AnimationTrigger searchTurnAnimTrigger = AnimationTrigger::Count;
    AnimationTrigger searchTurnEndAnimTrigger = AnimationTrigger::Count;
    AnimationTrigger waitForImagesAnimTrigger = AnimationTrigger::Count;
    std::unique_ptr<BlockWorldFilter> homeFilter;
    
    std::unique_ptr<Util::RejectionSamplerHelper<Point2f>> searchSpacePointEvaluator;
    std::unique_ptr<Util::RejectionSamplerHelper<Poly2f>>  searchSpacePolyEvaluator;
    
    std::shared_ptr<RobotPointSamplerHelper::RejectIfInRange> condHandleNearPrevSearch;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfWouldCrossCliff> condHandleCliffs;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfCollidesWithMemoryMap> condHandleCollisions;
  };

  struct DynamicVariables {
    DynamicVariables() {}
    
    // Number of completed 'searches'. One 'search' means
    // spinning around in place and looking for the charger
    int numSearchesCompleted = 0;
    
    // Number of turn animations played while searching in
    // place for the charger
    int numTurnsCompleted = 0;
    
    // Cumulative angle swept while searching in place for
    // the charger
    float angleSwept_deg = 0.f;
    
    struct Persistent {
      // Map of basestation time to locations at which we have executed
      // a "search in place". Used to ensure we do not search at the same
      // locations repeatedly within a specified timeframe.
      std::map<float, Point2f> searchedLocations;
      
      // Keep track of the last time we visited the old charger's location
      float lastVisitedOldChargerTime = std::numeric_limits<float>::lowest();
    };
    Persistent persistent;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToHeadStraight();
  void TransitionToStartSearch();
  void TransitionToSearchTurn();
  void TransitionToRandomDrive();
  
  // Generate potential locations to drive to (to perform a search)
  void GenerateSearchPoses(std::vector<Pose3d>& outPoses);
  
  // Fallback method for generating a naive randomly-selected pose in
  // case the 'smarter' sampling method fails to generate any poses.
  void GetRandomDrivingPose(Pose3d& outPose);
  
  // Cull the list of searched locations to the recent window and return
  // a vector of recently searched locations.
  std::vector<Point2f> GetRecentlySearchedLocations();
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorFindHome_H__
