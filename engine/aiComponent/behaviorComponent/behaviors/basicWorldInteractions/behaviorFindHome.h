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

#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/engine/math/pose.h"

#include "util/random/rejectionSamplerHelper_fwd.h"

namespace Anki {
namespace Vector {

namespace RobotPointSamplerHelper {
  class RejectIfInRange;
  class RejectIfWouldCrossCliff;
  class RejectIfCollidesWithMemoryMap;
}
struct VisionProcessingResult;
  
enum class AnimationTrigger : int32_t;
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
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

private:
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    float       minSearchAngleSweep_deg = 0.f;
    int         maxSearchTurns = 0;
    int         maxNumRecentSearches = 0;
    int         numSearchesBeforePlayingPostSearchAnim = -1; // unused if negative
    float       recentSearchWindow_sec = 0.f;
    float       minDrivingDist_mm = 0.f;
    float       maxDrivingDist_mm = 0.f;
    
    AnimationTrigger searchTurnAnimTrigger;
    AnimationTrigger postSearchAnimTrigger;
    std::unique_ptr<BlockWorldFilter> homeFilter;
    
    std::unique_ptr<Util::RejectionSamplerHelper<Point2f>> searchSpacePointEvaluator;
    std::unique_ptr<Util::RejectionSamplerHelper<Poly2f>>  searchSpacePolyEvaluator;
    
    std::shared_ptr<RobotPointSamplerHelper::RejectIfInRange> condHandleNearPrevSearch;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfWouldCrossCliff> condHandleCliffs;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfCollidesWithMemoryMap> condHandleCollisions;

    ICozmoBehaviorPtr observeChargerBehavior = nullptr;
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

    // Count of the frames of marker detection being run
    //  while this behavior is activated.
    u32 numFramesOfMarkers = 0;

    // Count of the frames where the image quality was TooDark.
    // NOTE: only counted while marker detection is being run.
    u32 numFramesOfImageTooDark = 0; 
    
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
  
  void TransitionToStartSearch();
  void TransitionToLookInPlace();
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
  

} // namespace Vector
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorFindHome_H__
