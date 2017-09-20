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

#include "clad/types/behaviorSystem/activityTypes.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "clad/types/offTreadsStates.h"

#include "util/random/randomGenerator.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class AIComponent;
class BehaviorContainer;
class BlockWorld;
class FaceWorld;
class IActivity;
class IBehavior;
class IExternalInterface;
class NeedsManager;
class MoodManager;
class ProgressionUnlockComponent;
class PublicStateBroadcaster;
class Robot;
  

  
class BehaviorExternalInterface {
public:
  BehaviorExternalInterface(Robot& robot,
                            AIComponent& aiComponent,
                            const BehaviorContainer& behaviorContainer,
                            BlockWorld& blockWorld,
                            FaceWorld& faceWorld);
  
  void SetOptionalInterfaces(MoodManager* moodManager,
                             NeedsManager* needsManager,
                             ProgressionUnlockComponent* progressionUnlockComponent,
                             PublicStateBroadcaster* publicStateBroadcaster,
                             IExternalInterface* robotExternalInterface);
  
  virtual ~BehaviorExternalInterface(){};
  
  // Access components which the BehaviorSystem can count on will always exist
  // when making decisions
  AIComponent&             GetAIComponent()       const { return _aiComponent;}
  const FaceWorld&         GetFaceWorld()         const { return _faceWorld;}
  const BlockWorld&        GetBlockWorld()        const { return _blockWorld;}
  const BehaviorContainer& GetBehaviorContainer() const { return _behaviorContainer;}
  
  // Give behaviors/activities access to robot
  // THIS IS DEPRECATED - DO NOT ADD ANY VALUES TO THE WHITELIST IN THIS FILE
  // THIS FUNCTION IS SOLEY TO FACILITATE THE TRANISITON BETWEEN COZMO BEHAVIORS
  // AND VICTOR BEHAVIORS
  Robot& GetRobot() { return _robot;}
  const Robot& GetRobot() const { return _robot;}

  // Access components which may or may not exist - the BehaviorSystem should
  // premise no critical decisions on the existance of these components
  std::weak_ptr<PublicStateBroadcaster>     GetRobotPublicStateBroadcaster() const { return _publicStateBroadcaster; }
  std::weak_ptr<IExternalInterface>         GetRobotExternalInterface() const { return _robotExternalInterface; }
  std::weak_ptr<ProgressionUnlockComponent> GetProgressionUnlockComponent() const { return _progressionUnlockComponent; }
  std::weak_ptr<MoodManager>                GetMoodManager()  const { return _moodManager;}
  std::weak_ptr<NeedsManager>               GetNeedsManager() const { return _needsManager;}

  // Util functions
  OffTreadsState GetOffTreadsState() const;
  Util::RandomGenerator& GetRNG();

private:
  Robot&                                      _robot;
  AIComponent&                                _aiComponent;
  const BehaviorContainer&                    _behaviorContainer;
  const FaceWorld&                            _faceWorld;
  const BlockWorld&                           _blockWorld;
  std::shared_ptr<MoodManager>                _moodManager;
  std::shared_ptr<NeedsManager>               _needsManager;
  std::shared_ptr<ProgressionUnlockComponent> _progressionUnlockComponent;
  std::shared_ptr<PublicStateBroadcaster>     _publicStateBroadcaster;
  std::shared_ptr<IExternalInterface>         _robotExternalInterface;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorExternalInterface_H__
