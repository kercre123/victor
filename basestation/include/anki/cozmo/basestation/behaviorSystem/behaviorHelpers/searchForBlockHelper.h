/**
 * File: searchForBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 5/8/17
 *
 * Description: Handles searching under three different paradigms
 *   1) Search for a specific ID
 *   2) Search until a certain number of blocks are seen
 *   3) Search a given amount regardless of what you see 
 * There are also three levels of intensity with which each of these types
 * of searches can be performed covering increasingly large search areas
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_SearchForBlockHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_SearchForBlockHelper_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/cozmo/basestation/preActionPose.h"
#include "anki/vision/basestation/camera.h"

namespace Anki {
namespace Cozmo {

class SearchForBlockHelper : public IHelper{
public:
  SearchForBlockHelper(Robot& robot, IBehavior& behavior,
                       BehaviorHelperFactory& helperFactory,
                       const SearchParameters& params = {});
  virtual ~SearchForBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot) override;
  
private:
  SearchParameters _params;
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  SearchIntensity _nextSearchIntensity;
  Vision::Camera _robotCameraAtSearchStart;
  std::set<ObjectID> _objectsSeenDuringSearch;
  
  
  void SearchForBlock(ActionResult result, Robot& robot);
  void SearchFinishedWithoutInterruption(Robot& robot);
  bool ShouldBeAbleToFindTarget(Robot& robot);
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_SearchForBlockHelper_H__

