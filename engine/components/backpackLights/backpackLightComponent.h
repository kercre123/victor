/**
 * File: backpackLightComponent.h
 *
 * Author: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Manages various lights on Cozmo's body. 
 *              Currently this includes the backpack lights and headlight.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_BackpackLightComponent_H__
#define __Anki_Cozmo_Basestation_Components_BackpackLightComponent_H__

#include "coretech/common/shared/types.h"
#include "clad/types/ledTypes.h"
#include "clad/types/backpackAnimationTriggers.h"

#include "engine/ankiEventUtil.h"
#include "engine/robotComponents_fwd.h"
#include "engine/components/backpackLights/backpackLightAnimation.h"
#include "engine/components/backpackLights/backpackLightComponentTypes.h"
#include "engine/robotComponents_fwd.h"

#include "json/json.h"
#include "util/cladHelpers/cladEnumToStringMap.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <map>
#include <memory>
#include <future>
#include <atomic>

namespace Anki {
namespace Cozmo {
  
class Robot;
class BackpackLightAnimationContainer;

class BackpackLightComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  BackpackLightComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::AIComponent);
    dependencies.insert(RobotComponentID::Battery);
  };
    
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  
  template<typename T>
  void HandleMessage(const T& msg);
    
  void ClearAllBackpackLightConfigs() { _backpackLightMap.clear(); }
  
  // Allows the caller to start a looping light pattern on Cozmo's backpack and get a handle to cancel it later.
  // Note that the source specified will be used in determining which pattern should be used when multiple have
  // been triggered.
  void StartLoopingBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights, BackpackLightSource source, BackpackLightDataLocator& lightLocator_out);
  
  // Related to the above start looping call, allows the caller to cancel a currently looping light pattern
  bool StopLoopingBackpackAnimation(const BackpackLightDataLocator& lightDataLocator);
  
  // General purpose call to set backpack lights. The light pattern will persist until this function is called again.
  // Uses a private source value with lower priority than the types specified in the above StartLooping call.
  void SetBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights);
  
  // Start the backpack lights associated with a trigger
  void SetBackpackAnimation(const BackpackAnimationTrigger& trigger, bool shouldLoop = true);

  void SetPairingLight(bool on);
  
private:
  
  Robot*                          _robot = nullptr;
  std::unique_ptr<BackpackLightAnimationContainer> _backpackLightContainer;
  Util::CladEnumToStringMap<BackpackAnimationTrigger>* _backpackTriggerToNameMap = nullptr;
  
  std::list<Signal::SmartHandle>  _eventHandles;

  // Contains overall mapping of light config sources to list of configurations
  BackpackLightMap                _backpackLightMap;
  
  // Reference to the most recently used light configuration
  BackpackLightDataRefWeak        _curBackpackLightConfig;
  
  // Locator handle for the shared light configuration associated with SetBackpackLightAnimation above
  BackpackLightDataLocator        _sharedLightConfig{};
  BackpackLightDataLocator        _criticalLightConfig{};
  
  // Note: this variable does NOT track the current trigger playing, it tracks internal state for 
  // UpdateChargingLightConfig and should not be used for any other decision making
  BackpackAnimationTrigger _internalChargeLightsTrigger = BackpackAnimationTrigger::Off;

  enum class SystemLightState
  {
    Invalid,
    Off,
    Pairing,
    Streaming,
  };
  SystemLightState _systemLightState = SystemLightState::Off;

  std::future<void> _offlineCheckFuture;
  std::atomic<TimeStamp_t> _offlineAtTime_ms;

  bool _offlineIsPulsing = true;
  TimeStamp_t _offlinePulseTime_ms = 0;
  
  Result SetBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights);
  void StartLoopingBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights, BackpackLightSourceType source, BackpackLightDataLocator& lightLocator_out);

  // Returns a sorted vector based on light source priority
  // with elements closer to the begining as having higher priorty
  static std::vector<BackpackLightSourceType> GetLightSourcePriority();

  // Returns a weak reference to the best light configuration based on priority
  BackpackLightDataRefWeak GetBestLightConfig();

  // Updates the critical backpack light config if neccessary
  void UpdateCriticalBackpackLightConfig(bool isCloudStreamOpen);

  // Updates the current system light pattern if neccessary
  void UpdateSystemLightState(bool isCloudStreamOpen);

  void UpdateOfflineCheck();
};

}
}

#endif

