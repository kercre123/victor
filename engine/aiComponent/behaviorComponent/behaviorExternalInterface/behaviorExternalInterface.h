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

#include "clad/types/offTreadsStates.h"

#include "util/random/randomGenerator.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class AIComponent;
class AnimationComponent;
class BehaviorContainer;
class BehaviorEventComponent;
class BEIRobotInfo;
class BlockWorld;
class BodyLightComponent;
class CubeAccelComponent;
class CubeLightComponent;
class DelegationComponent;
class FaceWorld;
class ICozmoBehavior;
class IExternalInterface;
class NeedsManager;
class MapComponent;
class MicDirectionHistory;
class MoodManager;
class ObjectPoseConfirmer;
class PetWorld;
class ProgressionUnlockComponent;
class PublicStateBroadcaster;
class Robot;
class TouchSensorComponent;
class VisionComponent;
  
namespace Audio {
class EngineRobotAudioClient;
}

namespace ComponentWrappers{
struct BehaviorExternalInterfaceComponents{
  BehaviorExternalInterfaceComponents(BEIRobotInfo&& robotInfo,
                                      AIComponent& aiComponent,
                                      const BehaviorContainer& behaviorContainer,
                                      BlockWorld& blockWorld,
                                      FaceWorld& faceWorld,
                                      PetWorld& petWorld,
                                      BehaviorEventComponent& behaviorEventComponent);
  
  std::unique_ptr<BEIRobotInfo> _robotInfo;
  AIComponent&                  _aiComponent;
  const BehaviorContainer&      _behaviorContainer;
  BlockWorld&                   _blockWorld;
  FaceWorld&                    _faceWorld;
  const PetWorld&               _petWorld;
  BehaviorEventComponent&       _behaviorEventComponent;
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
            PetWorld& petWorld,
            BehaviorEventComponent& behaviorEventComponent);
  
  void SetOptionalInterfaces(DelegationComponent*           delegationComponent,
                             MoodManager*                   moodManager,
                             NeedsManager*                  needsManager,
                             ProgressionUnlockComponent*    progressionUnlockComponent,
                             PublicStateBroadcaster*        publicStateBroadcaster,
                             TouchSensorComponent*          touchSensorComponent,
                             VisionComponent*               visionComponent,
                             MapComponent*                  mapComponent,
                             CubeLightComponent*            cubeLightComponent,
                             ObjectPoseConfirmer*           objectPoseConfirmer,
                             CubeAccelComponent*            cubeAccelComponent,
                             AnimationComponent*            animationComponent,
                             Audio::EngineRobotAudioClient* robotAudioClient,
                             BodyLightComponent*            bodyLightComponent,
                             MicDirectionHistory*           micDirectionHistory);
  
  virtual ~BehaviorExternalInterface();
  
  // Access components which the BehaviorSystem can count on will always exist
  // when making decisions
  AIComponent&             GetAIComponent()               const { assert(_beiComponents); return _beiComponents->_aiComponent;}
  const FaceWorld&         GetFaceWorld()                 const { assert(_beiComponents); return _beiComponents->_faceWorld;}
  FaceWorld&               GetFaceWorldMutable()          const { assert(_beiComponents); return _beiComponents->_faceWorld;}
  const PetWorld&          GetPetWorld()                  const { assert(_beiComponents); return _beiComponents->_petWorld;}
  const BlockWorld&        GetBlockWorld()                const { assert(_beiComponents); return _beiComponents->_blockWorld;}
  BlockWorld&              GetBlockWorld()                      { assert(_beiComponents); return _beiComponents->_blockWorld;}
  const BehaviorContainer& GetBehaviorContainer()         const { assert(_beiComponents); return _beiComponents->_behaviorContainer;}
  BehaviorEventComponent&  GetStateChangeComponent()      const { assert(_beiComponents); return _beiComponents->_behaviorEventComponent;}

  // Give behaviors/activities access to information about robot
  BEIRobotInfo& GetRobotInfo() { assert(_beiComponents); return *_beiComponents->_robotInfo;}
  const BEIRobotInfo& GetRobotInfo() const { assert(_beiComponents); return *_beiComponents->_robotInfo;}

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

  inline bool HasTouchSensorComponent() const { return _touchSensorComponent != nullptr;}
  TouchSensorComponent& GetTouchSensorComponent() const {assert(_touchSensorComponent); return *_touchSensorComponent;}

  inline bool HasVisionComponent() const { return _visionComponent != nullptr;}
  VisionComponent& GetVisionComponent() const{assert(_visionComponent); return *_visionComponent;}

  inline bool HasMapComponent() const { return _mapComponent != nullptr;}
  MapComponent& GetMapComponent() const{assert(_mapComponent); return *_mapComponent;}

  inline bool HasCubeLightComponent() const { return _mapComponent != nullptr;}
  CubeLightComponent& GetCubeLightComponent() const{assert(_cubeLightComponent); return *_cubeLightComponent;}

  inline bool HasObjectPoseConfirmer() const { return _objectPoseConfirmer != nullptr;}
  ObjectPoseConfirmer& GetObjectPoseConfirmer() const{assert(_objectPoseConfirmer); return *_objectPoseConfirmer;}

  inline bool HasCubeAccelComponent() const { return _cubeAccelComponent != nullptr;}
  CubeAccelComponent& GetCubeAccelComponent() const{assert(_cubeAccelComponent); return *_cubeAccelComponent;}

  inline bool HasAnimationComponent() const { return _animationComponent != nullptr;}
  AnimationComponent& GetAnimationComponent() const{assert(_animationComponent); return *_animationComponent;}

  inline bool HasRobotAudioClient() const { return _robotAudioClient != nullptr;}
  Audio::EngineRobotAudioClient& GetRobotAudioClient() const{assert(_robotAudioClient); return *_robotAudioClient;}

  inline bool HasBodyLightComponent() const { return _bodyLightComponent != nullptr;}
  BodyLightComponent& GetBodyLightComponent() const{assert(_bodyLightComponent); return *_bodyLightComponent;}
  
  inline bool HasMicDirectionHistory() const { return _micDirectionHistory != nullptr;}
  const MicDirectionHistory& GetMicDirectionHistory() const {assert(_micDirectionHistory); return *_micDirectionHistory;}

  // Util functions
  OffTreadsState GetOffTreadsState() const;
  Util::RandomGenerator& GetRNG();

private:
  std::unique_ptr<ComponentWrappers::BehaviorExternalInterfaceComponents> _beiComponents;
  
  DelegationComponent*           _delegationComponent;
  MoodManager*                   _moodManager;
  NeedsManager*                  _needsManager;
  ProgressionUnlockComponent*    _progressionUnlockComponent;
  PublicStateBroadcaster*        _publicStateBroadcaster;
  TouchSensorComponent*          _touchSensorComponent;
  VisionComponent*               _visionComponent;
  MapComponent*                  _mapComponent;
  MicDirectionHistory*           _micDirectionHistory;
  CubeLightComponent*            _cubeLightComponent;
  ObjectPoseConfirmer*           _objectPoseConfirmer;
  CubeAccelComponent*            _cubeAccelComponent;
  AnimationComponent*            _animationComponent;
  Audio::EngineRobotAudioClient* _robotAudioClient;
  BodyLightComponent*            _bodyLightComponent;


};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorExternalInterface_H__
