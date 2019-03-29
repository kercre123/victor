/**
 * File: BehaviorSDKInterface.h
 *
 * Author: Michelle Sintov
 * Created: 2018-05-21
 *
 * Description: Interface for SDKs including C# and Python
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSDKInterface__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSDKInterface__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorDriveOffCharger;
class IGatewayInterface;
class UserIntentComponent; 
namespace external_interface {
  class DriveOffChargerRequest;
  class DriveOnChargerRequest;
  class FindFacesRequest;
  class LookAroundInPlaceRequest;
  class RollBlockRequest;
}
  
class BehaviorSDKInterface : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorSDKInterface(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void HandleWhileActivated(const AppToEngineEvent& event) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;

private:
  void DriveOffChargerRequest(const external_interface::DriveOffChargerRequest& driveOffChargerRequest);
  void DriveOnChargerRequest(const external_interface::DriveOnChargerRequest& driveOnChargerRequest);
  void FindFacesRequest(const external_interface::FindFacesRequest& findFacesRequest);
  void LookAroundInPlaceRequest(const external_interface::LookAroundInPlaceRequest& lookAroundInPlaceRequest);
  void RollBlockRequest(const external_interface::RollBlockRequest& rollBlockRequest);

  void HandleDriveOffChargerComplete();
  void HandleDriveOnChargerComplete();
  void HandleFindFacesComplete();
  void HandleLookAroundInPlaceComplete();
  void HandleRollBlockComplete();

  // Use this to prevent (or allow) raw movement commands from the SDK. We only want to allow these when the SDK
  // behavior is activated and _not_ delegating to another behavior.
  void SetAllowExternalMovementCommands(const bool allow);
  void ProcessUserIntents();

  struct InstanceConfig {
    InstanceConfig();

    int behaviorControlLevel;
    bool disableCliffDetection;

    std::string driveOffChargerBehaviorStr;
    std::string findAndGoToHomeBehaviorStr;
    std::string findFacesBehaviorStr;
    std::string lookAroundInPlaceBehaviorStr;
    std::string rollBlockBehaviorStr;
    ICozmoBehaviorPtr driveOffChargerBehavior;
    ICozmoBehaviorPtr findAndGoToHomeBehavior;
    ICozmoBehaviorPtr findFacesBehavior;
    ICozmoBehaviorPtr lookAroundInPlaceBehavior;
    ICozmoBehaviorPtr rollBlockBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  AnkiEventMgr<external_interface::GatewayWrapper> _eventMgr;
};
} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSDKInterface__
