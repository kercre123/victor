/**
 * File: sdkComponent
 *
 * Author: Michelle Sintov
 * Created: 05/25/2018
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_SDKComponent_H_
#define __Engine_Components_SDKComponent_H_

#include "engine/robotComponents_fwd.h"
#include "engine/components/textToSpeech/textToSpeechCoordinator.h"
#include "engine/components/visionScheduleMediator/iVisionModeSubscriber.h"
#include "clad/types/visionModes.h"
#include "clad/types/sdkAudioTypes.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

namespace external_interface {
  class GatewayWrapper;

  class PathMotionProfile;

  class GoToPoseRequest;
  class DockWithCubeRequest;
  class DriveStraightRequest;
  class TurnInPlaceRequest;
  class SetLiftHeightRequest;
  class SetHeadAngleRequest;
  class TurnTowardsFaceRequest;
  class GoToObjectRequest;
  class RollObjectRequest;
  class PopAWheelieRequest;
  class PickupObjectRequest;
  class PlaceObjectOnGroundHereRequest;
  class MasterVolumeRequest;
}

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SDKComponent : public IDependencyManagedComponent<RobotComponentID>
                   , public IVisionModeSubscriber
{
public:

  SDKComponent();
  ~SDKComponent();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent functions

  virtual void GetInitDependencies( RobotCompIDSet& dependencies ) const override;
  virtual void GetUpdateDependencies( RobotCompIDSet& dependencies ) const override {};

  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  // Event/Message handling
  void HandleProtoMessage(const AnkiEvent<external_interface::GatewayWrapper>& event);

  // SDK behavior activation functions
  bool SDKWantsControl();
  int SDKControlLevel();
  void SDKBehaviorActivation(bool enabled);
  bool SDKWantsBehaviorLock();

  void OnActionCompleted(ExternalInterface::RobotCompletedAction msg);

  template<typename T>
  void HandleMessage(const T& msg);

private:

  Robot* _robot = nullptr;  
  bool _sdkWantsControl = false;
  bool _sdkWantsLock = false;
  bool _sdkBehaviorActivated = false;
  int _sdkControlLevel;
  uint64_t _sdkLockConnId = 0;
  uint64_t _sdkControlConnId = 0;

  bool _captureSingleImage = false;
  
  std::vector<::Signal::SmartHandle> _signalHandles;

  // Set of vision modes that we are waiting to appear/disappear from the VisionProcessingResult message
  // If bool is true then we are waiting for the mode to appear in the result message
  // If bool is false then we are waiting for the mode to disappear from the result message
  std::set<std::pair<VisionMode, bool>> _visionModesWaitingToChange;

  void OnSendAudioModeRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void IsImageStreamingEnabledRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void DispatchSDKActivationResult(bool enabled, uint64_t connectionId);
  void DispatchBehaviorLockLostResult();
  void SetBehaviorLock(uint64_t controlId);
  void DispatchAudioStreamResult();

  // Returns true if the subscription was actually updated
  bool SubscribeToVisionMode(bool subscribe, VisionMode mode, bool updateWaitingToChangeSet = true);
  void DisableMirrorMode();
  void SayText(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void SetEyeColor(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void ListAnimationTriggers(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void HandleAudioStreamPrepareRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void HandleAudioStreamChunkRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void HandleAudioStreamCompleteRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void HandleAudioStreamCancelRequest(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void HandleStreamStatusEvent(SDKAudioStreamingState streamStatusId, int audioFramesReceived, int audioFramesPlayed);
  void SetMasterVolume(const AnkiEvent<external_interface::GatewayWrapper>& event);
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_Components_SDKComponent_H_
