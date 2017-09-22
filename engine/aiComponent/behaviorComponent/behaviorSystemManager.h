/**
* File: behaviorSystemManager.h
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Manages and enforces the lifecycle and transitions
* of partso of the behavior system
*
* Copyright: Anki, Inc. 2017
**/

#ifndef COZMO_BEHAVIOR_SYSTEM_MANAGER_H
#define COZMO_BEHAVIOR_SYSTEM_MANAGER_H

#include "anki/common/types.h"
#include "anki/common/basestation/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior_fwd.h"
#include "engine/robotDataLoader.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "json/json-forwards.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorContainer;
class BehaviorExternalInterface;
class IActivity;
class Robot;

struct BehaviorRunningInfo;

class BehaviorSystemManager
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorSystemManager();
  ~BehaviorSystemManager();
  
  // initialize this behavior manager from the given Json config
  Result InitConfiguration(BehaviorExternalInterface& behaviorExternalInterface,
                           const Json::Value& behaviorSystemConfig);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  void Update(BehaviorExternalInterface& behaviorExternalInterface);
  
  // returns nullptr if there is no current behavior
  const IBehaviorPtr GetCurrentBehavior() const;
  
  IBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  IBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;
  
private:
  void UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface);
  
  
  // Functions which mediate direct access to running/resume info so that the robot
  // can respond appropriately to switching between reactions/resumes
  BehaviorRunningInfo& GetRunningInfo() const {assert(_runningInfo); return *_runningInfo;}
  void SetRunningInfo(const BehaviorRunningInfo& newInfo);
  
  
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  bool _isInitialized;
  
  // - - - - - - - - - - - - - - -
  // current running behavior
  // - - - - - - - - - - - - - - -
  
  // PLEASE DO NOT ACCESS OR ASSIGN TO DIRECTLY - use functions above
  std::unique_ptr<BehaviorRunningInfo> _runningInfo;
  
  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  
  std::unique_ptr<IActivity> _baseActivity;
  

  
  BehaviorExternalInterface* _behaviorExternalInterface;
  
}; // class BehaviorSystemManager

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_SYSTEM_MANAGER_H
