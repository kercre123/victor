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


#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/componentWrapper.h"
#include "util/entityComponent/entity.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include "clad/types/offTreadsStates.h"

#include "util/helpers/noncopyable.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

#include <memory>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class AIComponent;
class AnimationComponent;
class BeatDetectorComponent;
class BehaviorContainer;
class BehaviorEventComponent;
class BehaviorTimerManager;
class BEIRobotInfo;
class BlockWorld;
class BackpackLightComponent;
class CubeAccelComponent;
class CubeCommsComponent;
class CubeConnectionCoordinator;
class CubeLightComponent;
class CliffSensorComponent;
class DelegationComponent;
class FaceWorld;
class HabitatDetectorComponent;
class ICozmoBehavior;
class IExternalInterface;
class MapComponent;
class MicComponent;
class MoodManager;
class MovementComponent;
class ObjectPoseConfirmer;
class PetWorld;
class PhotographyManager;
class ProgressionUnlockComponent;
class ProxSensorComponent;
class PublicStateBroadcaster;
class SDKComponent;
class SettingsManager;
class DataAccessorComponent;
class TextToSpeechCoordinator;
class TouchSensorComponent;
class VariableSnapshotComponent;
class VisionComponent;
class VisionScheduleMediator;
  
namespace Audio {
class EngineRobotAudioClient;
}


class BEIComponentAccessGuard{
};


class BEIComponentWrapper : public ComponentWrapper {
  public:
    template<typename T>
    BEIComponentWrapper(T* component)
    : ComponentWrapper(component){}

    // Maintain a reference to the access guard in order to strip the component out of BEI
    // when the access guard falls out of scope the component will be added back into BEI
    // automatically
    std::shared_ptr<BEIComponentAccessGuard> StripComponent() const {return _accessGuard;}

    virtual bool IsComponentValidInternal() const override {return _accessGuard.use_count() == 1;}

  private:
    std::shared_ptr<BEIComponentAccessGuard> _accessGuard = std::make_shared<BEIComponentAccessGuard>();
};

class BehaviorExternalInterface : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable{
public:
  BehaviorExternalInterface()
  :IDependencyManagedComponent(this, BCComponentID::BehaviorExternalInterface) {}
  
  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};

  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override { 
    dependencies.insert(BCComponentID::BehaviorEventComponent); 
    dependencies.insert(BCComponentID::DelegationComponent);
  };
  virtual void AdditionalInitAccessibleComponents(BCCompIDSet& components) const override
  { 
    components.insert(BCComponentID::AIComponent);
    components.insert(BCComponentID::BehaviorContainer);
    components.insert(BCComponentID::BehaviorTimerManager);
    components.insert(BCComponentID::BlockWorld);
    components.insert(BCComponentID::FaceWorld);
    components.insert(BCComponentID::RobotInfo);
  }
  //////
  // end IDependencyManagedComponent functions
  //////

  void Init(AIComponent*                   aiComponent,
            AnimationComponent*            animationComponent,
            BeatDetectorComponent*         beatDetectorComponent,
            BehaviorContainer*             behaviorContainer,
            BehaviorEventComponent*        behaviorEventComponent,
            BehaviorTimerManager*          behaviorTimers,
            BlockWorld*                    blockWorld,
            BackpackLightComponent*        backpackLightComponent,
            CubeAccelComponent*            cubeAccelComponent,
            CubeCommsComponent*            cubeCommsComponent,
            CubeConnectionCoordinator*     CubeConnectionCoordinator,
            CubeLightComponent*            cubeLightComponent,
            CliffSensorComponent*          cliffSensorComponent,
            DelegationComponent*           delegationComponent,
            FaceWorld*                     faceWorld,
            HabitatDetectorComponent*      habitatDetectorComponent,
            MapComponent*                  mapComponent,
            MicComponent*                  micComponent,
            MoodManager*                   moodManager,
            MovementComponent*             movementComponent,
            ObjectPoseConfirmer*           objectPoseConfirmer,
            PetWorld*                      petWorld,
            PhotographyManager*            photographyManager,
            ProgressionUnlockComponent*    progressionUnlockComponent,
            ProxSensorComponent*           proxSensor,
            PublicStateBroadcaster*        publicStateBroadcaster,
            SDKComponent*                  sdkComponent,
            Audio::EngineRobotAudioClient* robotAudioClient,
            BEIRobotInfo*                  robotInfo,
            DataAccessorComponent*         dataAccessor,
            TextToSpeechCoordinator*       TextToSpeechCoordinator,
            TouchSensorComponent*          touchSensorComponent,
            VariableSnapshotComponent*      variableSnapshotComponent,
            VisionComponent*               visionComponent,
            VisionScheduleMediator*        visionScheduleMediator,
            SettingsManager*               settingsManager);
    
  virtual ~BehaviorExternalInterface();

  const BEIComponentWrapper& GetComponentWrapper(BEIComponentID componentID) const;
  
  // Access components which the BehaviorSystem can count on will always exist
  // when making decisions
  AIComponent&             GetAIComponent()               const { return GetComponentWrapper(BEIComponentID::AIComponent).GetComponent<AIComponent>();}
  const FaceWorld&         GetFaceWorld()                 const { return GetComponentWrapper(BEIComponentID::FaceWorld).GetComponent<FaceWorld>();}
  FaceWorld&               GetFaceWorldMutable()                { return GetComponentWrapper(BEIComponentID::FaceWorld).GetComponent<FaceWorld>();}
  const PetWorld&          GetPetWorld()                  const { return GetComponentWrapper(BEIComponentID::PetWorld).GetComponent<PetWorld>();}
  const BlockWorld&        GetBlockWorld()                const { return GetComponentWrapper(BEIComponentID::BlockWorld).GetComponent<BlockWorld>();}
  BlockWorld&              GetBlockWorld()                      { return GetComponentWrapper(BEIComponentID::BlockWorld).GetComponent<BlockWorld>();}
  const BehaviorContainer& GetBehaviorContainer()         const { return GetComponentWrapper(BEIComponentID::BehaviorContainer).GetComponent<BehaviorContainer>();}
  BehaviorEventComponent&  GetBehaviorEventComponent()    const { return GetComponentWrapper(BEIComponentID::BehaviorEvent).GetComponent<BehaviorEventComponent>();}
  BehaviorTimerManager&    GetBehaviorTimerManager()      const { return GetComponentWrapper(BEIComponentID::BehaviorTimerManager).GetComponent<BehaviorTimerManager>(); }

  // Give behaviors/activities access to information about robot
  BEIRobotInfo& GetRobotInfo() { return GetComponentWrapper(BEIComponentID::RobotInfo).GetComponent<BEIRobotInfo>();}
  const BEIRobotInfo& GetRobotInfo() const { return GetComponentWrapper(BEIComponentID::RobotInfo).GetComponent<BEIRobotInfo>();}

  // Access components which may or may not exist - you must call
  // has before get or you may hit a nullptr assert
  inline bool HasDelegationComponent() const { return GetComponentWrapper(BEIComponentID::Delegation).IsComponentValid();}
  inline DelegationComponent& GetDelegationComponent() const  { return GetComponentWrapper(BEIComponentID::Delegation).GetComponent<DelegationComponent>();}

  inline bool HasDataAccessorComponent() const { return GetComponentWrapper(BEIComponentID::DataAccessor).IsComponentValid();}
  inline DataAccessorComponent& GetDataAccessorComponent() const  { return GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();}

  inline bool HasPhotographyManager() const { return GetComponentWrapper(BEIComponentID::PhotographyManager).IsComponentValid();}
  PhotographyManager& GetPhotographyManager() const { return GetComponentWrapper(BEIComponentID::PhotographyManager).GetComponent<PhotographyManager>();}

  inline bool HasPublicStateBroadcaster() const { return GetComponentWrapper(BEIComponentID::PublicStateBroadcaster).IsComponentValid();}
  PublicStateBroadcaster& GetRobotPublicStateBroadcaster() const { return GetComponentWrapper(BEIComponentID::PublicStateBroadcaster).GetComponent<PublicStateBroadcaster>();}
  
  inline bool HasProgressionUnlockComponent() const { return GetComponentWrapper(BEIComponentID::ProgressionUnlock).IsComponentValid();}
  ProgressionUnlockComponent& GetProgressionUnlockComponent() const {return GetComponentWrapper(BEIComponentID::ProgressionUnlock).GetComponent<ProgressionUnlockComponent>();}
  
  inline bool HasMoodManager() const { return GetComponentWrapper(BEIComponentID::MoodManager).IsComponentValid();}
  MoodManager& GetMoodManager() const{ return GetComponentWrapper(BEIComponentID::MoodManager).GetComponent<MoodManager>();}
  
  inline bool HasMovementComponent() const { return GetComponentWrapper(BEIComponentID::MovementComponent).IsComponentValid();}
  MovementComponent& GetMovementComponent() const{ return GetComponentWrapper(BEIComponentID::MovementComponent).GetComponent<MovementComponent>();}
  
  inline bool HasTouchSensorComponent() const { return GetComponentWrapper(BEIComponentID::TouchSensor).IsComponentValid();}
  TouchSensorComponent& GetTouchSensorComponent() const { return GetComponentWrapper(BEIComponentID::TouchSensor).GetComponent<TouchSensorComponent>();}

  inline bool HasVisionComponent() const { return GetComponentWrapper(BEIComponentID::Vision).IsComponentValid();}
  VisionComponent& GetVisionComponent() const { return GetComponentWrapper(BEIComponentID::Vision).GetComponent<VisionComponent>();}

  inline bool HasVisionScheduleMediator() const { return GetComponentWrapper(BEIComponentID::VisionScheduleMediator).IsComponentValid();}
  VisionScheduleMediator& GetVisionScheduleMediator() const { return GetComponentWrapper(BEIComponentID::VisionScheduleMediator).GetComponent<VisionScheduleMediator>();}

  inline bool HasMapComponent() const { return GetComponentWrapper(BEIComponentID::Map).IsComponentValid();}
  MapComponent& GetMapComponent() const { return GetComponentWrapper(BEIComponentID::Map).GetComponent<MapComponent>();}

  inline bool HasCubeLightComponent() const { return GetComponentWrapper(BEIComponentID::CubeLight).IsComponentValid();}
  CubeLightComponent& GetCubeLightComponent() const { return GetComponentWrapper(BEIComponentID::CubeLight).GetComponent<CubeLightComponent>();}

  inline bool HasCubeCommsComponent() const { return GetComponentWrapper(BEIComponentID::CubeComms).IsComponentValid();}
  CubeCommsComponent& GetCubeCommsComponent() const { return GetComponentWrapper(BEIComponentID::CubeComms).GetComponent<CubeCommsComponent>();}

  inline bool HasCubeConnectionCoordinator() const { return GetComponentWrapper(BEIComponentID::CubeConnectionCoordinator).IsComponentValid();}
  CubeConnectionCoordinator& GetCubeConnectionCoordinator() const { return GetComponentWrapper(BEIComponentID::CubeConnectionCoordinator).GetComponent<CubeConnectionCoordinator>();}

  inline bool HasObjectPoseConfirmer() const { return GetComponentWrapper(BEIComponentID::ObjectPoseConfirmer).IsComponentValid();}
  ObjectPoseConfirmer& GetObjectPoseConfirmer() const { return GetComponentWrapper(BEIComponentID::ObjectPoseConfirmer).GetComponent<ObjectPoseConfirmer>();}

  inline bool HasCliffSensorComponent() const { return GetComponentWrapper(BEIComponentID::CliffSensor).IsComponentValid();}
  CliffSensorComponent& GetCliffSensorComponent() const { return GetComponentWrapper(BEIComponentID::CliffSensor).GetComponent<CliffSensorComponent>();}

  inline bool HasCubeAccelComponent() const { return GetComponentWrapper(BEIComponentID::CubeAccel).IsComponentValid();}
  CubeAccelComponent& GetCubeAccelComponent() const { return GetComponentWrapper(BEIComponentID::CubeAccel).GetComponent<CubeAccelComponent>();}

  inline bool HasAnimationComponent() const { return GetComponentWrapper(BEIComponentID::Animation).IsComponentValid();}
  AnimationComponent& GetAnimationComponent() const { return GetComponentWrapper(BEIComponentID::Animation).GetComponent<AnimationComponent>();}

  inline bool HasRobotAudioClient() const { return GetComponentWrapper(BEIComponentID::RobotAudioClient).IsComponentValid();}
  Audio::EngineRobotAudioClient& GetRobotAudioClient() const { return GetComponentWrapper(BEIComponentID::RobotAudioClient).GetComponent<Audio::EngineRobotAudioClient>();}
  
  inline bool HasBackpackLightComponent() const { return GetComponentWrapper(BEIComponentID::BackpackLightComponent).IsComponentValid();}
  BackpackLightComponent& GetBackpackLightComponent() const { return GetComponentWrapper(BEIComponentID::BackpackLightComponent).GetComponent<BackpackLightComponent>();}

  inline bool HasMicComponent() const { return GetComponentWrapper(BEIComponentID::MicComponent).IsComponentValid();}
  MicComponent& GetMicComponent() const {return GetComponentWrapper(BEIComponentID::MicComponent).GetComponent<MicComponent>();}
  
  inline bool HasHabitatDetectorComponent() const { return GetComponentWrapper(BEIComponentID::HabitatDetector).IsComponentValid();}
  HabitatDetectorComponent& GetHabitatDetectorComponent() const { return GetComponentWrapper(BEIComponentID::HabitatDetector).GetComponent<HabitatDetectorComponent>();}

  inline bool HasBeatDetectorComponent() const { return GetComponentWrapper(BEIComponentID::BeatDetector).IsComponentValid();}
  BeatDetectorComponent& GetBeatDetectorComponent() const {return GetComponentWrapper(BEIComponentID::BeatDetector).GetComponent<BeatDetectorComponent>();}

  inline bool HasTextToSpeechCoordinator() const { return GetComponentWrapper(BEIComponentID::TextToSpeechCoordinator).IsComponentValid();}
  TextToSpeechCoordinator& GetTextToSpeechCoordinator() const {return GetComponentWrapper(BEIComponentID::TextToSpeechCoordinator).GetComponent<TextToSpeechCoordinator>();}

  inline bool HasSDKComponent() const { return GetComponentWrapper(BEIComponentID::SDK).IsComponentValid();}
  SDKComponent& GetSDKComponent() const {return GetComponentWrapper(BEIComponentID::SDK).GetComponent<SDKComponent>();}

  inline bool HasSettingsManager() const { return GetComponentWrapper(BEIComponentID::SettingsManager).IsComponentValid();}
  SettingsManager& GetSettingsManager() const {return GetComponentWrapper(BEIComponentID::SettingsManager).GetComponent<SettingsManager>();}
  
  // Util functions
  OffTreadsState GetOffTreadsState() const;
  Util::RandomGenerator& GetRNG();

private:
  struct CompArrayWrapper{
    public:
      CompArrayWrapper(AIComponent*                   aiComponent,
                       AnimationComponent*            animationComponent,
                       BeatDetectorComponent*         beatDetectorComponent,
                       BehaviorContainer*             behaviorContainer,
                       BehaviorEventComponent*        behaviorEventComponent,
                       BehaviorTimerManager*          behaviorTimers,
                       BlockWorld*                    blockWorld,
                       BackpackLightComponent*        backpackLightComponent,
                       CubeAccelComponent*            cubeAccelComponent,
                       CubeCommsComponent*            cubeCommsComponent,
                       CubeConnectionCoordinator*     CubeConnectionCoordinator,
                       CubeLightComponent*            cubeLightComponent,
                       CliffSensorComponent*          cliffSensorComponent,
                       DelegationComponent*           delegationComponent,
                       FaceWorld*                     faceWorld,
                       HabitatDetectorComponent*      habitatDetectorComponent,
                       MapComponent*                  mapComponent,
                       MicComponent*                  micComponent,
                       MoodManager*                   moodManager,
                       MovementComponent*             movementComponent,
                       ObjectPoseConfirmer*           objectPoseConfirmer,
                       PetWorld*                      petWorld,
                       PhotographyManager*            photographyManager,
                       ProgressionUnlockComponent*    progressionUnlockComponent,
                       ProxSensorComponent*           proxSensor,
                       PublicStateBroadcaster*        publicStateBroadcaster,
                       SDKComponent*                  sdkComponent,
                       Audio::EngineRobotAudioClient* robotAudioClient,
                       BEIRobotInfo*                  robotInfo,
                       DataAccessorComponent*         dataAccessor,
                       TextToSpeechCoordinator*       textToSpeechCoordinator,
                       TouchSensorComponent*          touchSensorComponent,
                       VariableSnapshotComponent*      variableSnapshotComponent,
                       VisionComponent*               visionComponent,
                       VisionScheduleMediator*        visionSchedulMediator,
                       SettingsManager*               settingsManager);
      ~CompArrayWrapper(){};
      EntityFullEnumeration<BEIComponentID, BEIComponentWrapper, BEIComponentID::Count> _array;
  };
  std::unique_ptr<CompArrayWrapper> _arrayWrapper;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorExternalInterface_H__
