/**
* File: behaviorExternalInterface.h
*
* Author: Kevin M. Karol
* Created: 08/30/17
*
* Description: Interface that behaviors use to interact with the rest of
* the Cozmo system
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorExternalInterface_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorExternalInterface_H__


#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"

#include "clad/types/behaviorComponent/activityTypes.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "clad/types/offTreadsStates.h"

#include "util/random/randomGenerator.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class AIComponent;
class BehaviorContainer;
class BlockWorld;
class DelegationComponent;
class FaceWorld;
class IActivity;
class ICozmoBehavior;
class IExternalInterface;
class NeedsManager;
class MoodManager;
class ProgressionUnlockComponent;
class PublicStateBroadcaster;
class Robot;
class BehaviorEventComponent;
  
namespace Audio {
class BehaviorAudioComponent;
}

namespace ComponentWrappers{
struct BehaviorExternalInterfaceComponents{
  BehaviorExternalInterfaceComponents(Robot& robot,
                                      AIComponent& aiComponent,
                                      const BehaviorContainer& behaviorContainer,
                                      BlockWorld& blockWorld,
                                      FaceWorld& faceWorld,
                                      BehaviorEventComponent& behaviorEventComponent)
  :_robot(robot)
  ,_aiComponent(aiComponent)
  ,_behaviorContainer(behaviorContainer)
  ,_blockWorld(blockWorld)
  ,_faceWorld(faceWorld)
  ,_behaviorEventComponent(behaviorEventComponent){}
  
  Robot&                   _robot;
  AIComponent&             _aiComponent;
  const BehaviorContainer& _behaviorContainer;
  const BlockWorld&        _blockWorld;
  const FaceWorld&         _faceWorld;
  BehaviorEventComponent&    _behaviorEventComponent;
};
}
  
class BehaviorExternalInterface {
public:
  BehaviorExternalInterface();
  
  void Init(Robot& robot,
            AIComponent& aiComponent,
            const BehaviorContainer& behaviorContainer,
            BlockWorld& blockWorld,
            FaceWorld& faceWorld,
            BehaviorEventComponent& behaviorEventComponent);
  
  void SetOptionalInterfaces(DelegationComponent* delegationComponent,
                             MoodManager* moodManager,
                             NeedsManager* needsManager,
                             ProgressionUnlockComponent* progressionUnlockComponent,
                             PublicStateBroadcaster* publicStateBroadcaster);
  
  virtual ~BehaviorExternalInterface(){};
  
  // Access components which the BehaviorSystem can count on will always exist
  // when making decisions
  AIComponent&             GetAIComponent()             const { assert(_beiComponents); return _beiComponents->_aiComponent;}
  const FaceWorld&         GetFaceWorld()               const { assert(_beiComponents); return _beiComponents->_faceWorld;}
  const BlockWorld&        GetBlockWorld()              const { assert(_beiComponents); return _beiComponents->_blockWorld;}
  const BehaviorContainer& GetBehaviorContainer()       const { assert(_beiComponents); return _beiComponents->_behaviorContainer;}
  BehaviorEventComponent& GetStateChangeComponent()       const { assert(_beiComponents); return _beiComponents->_behaviorEventComponent;}

  // Give behaviors/activities access to robot
  // THIS IS DEPRECATED
  // THIS FUNCTION IS SOLEY TO FACILITATE THE TRANISITON BETWEEN COZMO BEHAVIORS
  // AND VICTOR BEHAVIORS
  Robot& GetRobot() { assert(_beiComponents); return _beiComponents->_robot;}
  const Robot& GetRobot() const { assert(_beiComponents); return _beiComponents->_robot;}

  // Access components which may or may not exist - you must call
  // has before get or you may hit a nullptr assert
  inline bool HasDelegationComponent() const { return _delegationComponent != nullptr;}
  inline DelegationComponent& GetDelegationComponent() const {assert(_delegationComponent); return *_delegationComponent;}
  
  inline bool HasPublicStateBroadcaster() const { return _publicStateBroadcaster != nullptr;}
  PublicStateBroadcaster& GetRobotPublicStateBroadcaster() const {assert(_publicStateBroadcaster); return *_publicStateBroadcaster;}
  
  inline bool HasProgressionUnlockComponent() const { return _progressionUnlockComponent != nullptr;}
  ProgressionUnlockComponent& GetProgressionUnlockComponent() const {assert(_progressionUnlockComponent); return *_progressionUnlockComponent;}
  
  inline bool HasMoodManager() const { return _moodManager != nullptr;}
  MoodManager& GetMoodManager()  const {assert(_moodManager); return *_moodManager;}
  
  inline bool HasNeedsManager() const { return _needsManager != nullptr;}
  NeedsManager& GetNeedsManager() const {assert(_needsManager); return *_needsManager;}

  // Util functions
  OffTreadsState GetOffTreadsState() const;
  Util::RandomGenerator& GetRNG();

private:
  std::unique_ptr<ComponentWrappers::BehaviorExternalInterfaceComponents> _beiComponents;
  
  DelegationComponent*        _delegationComponent;
  MoodManager*                _moodManager;
  NeedsManager*               _needsManager;
  ProgressionUnlockComponent* _progressionUnlockComponent;
  PublicStateBroadcaster*     _publicStateBroadcaster;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorExternalInterface_H__
