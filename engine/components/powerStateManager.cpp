/**
 * File: powerStateManager.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-06-27
 *
 * Description: Central engine component to manage power states (i.e. "power save mode")
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/powerStateManager.h"

#include "engine/components/mics/micComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "osState/osState.h"
#include "platform/camera/cameraService.h"
#include "util/console/consoleInterface.h"
#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/helpers/boundedWhile.h"
#include "util/logging/logging.h"

#include "clad/types/lcdTypes.h"

namespace Anki {
namespace Vector {

namespace {

#define CONSOLE_GROUP "PowerSave"

CONSOLE_VAR( bool, kPowerSave_CalmMode, CONSOLE_GROUP, true);
CONSOLE_VAR( bool, kPowerSave_Camera, CONSOLE_GROUP, true);
CONSOLE_VAR( bool, kPowerSave_LCDBacklight, CONSOLE_GROUP, true);
CONSOLE_VAR( bool, kPowerSave_ThrottleCPU, CONSOLE_GROUP, true);
CONSOLE_VAR( bool, kPowerSave_ProxSensorMap, CONSOLE_GROUP, true);

static constexpr const LCDBrightness kLCDBrightnessLow = LCDBrightness::LCDLevel_5mA;
static constexpr const LCDBrightness kLCDBrightnessNormal = LCDBrightness::LCDLevel_10mA;

static constexpr const DesiredCPUFrequency kCPUFreqSaveLow = DesiredCPUFrequency::Manual200MHz;
static constexpr const DesiredCPUFrequency kCPUFreqSaveHigh = DesiredCPUFrequency::Manual400Mhz;
static constexpr const DesiredCPUFrequency kCPUFreqNormal = DesiredCPUFrequency::Automatic;

static constexpr const float kMicBufferLowMark = 0.1f;
static constexpr const float kMicBufferHighMark = 0.6f;

}

PowerStateManager::PowerStateManager()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::PowerStateManager)
  , UnreliableComponent<BCComponentID>(this, BCComponentID::PowerStateManager)
{
}

void PowerStateManager::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _context = dependentComps.GetComponent<ContextWrapper>().context;
}

void PowerStateManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  if( !ANKI_VERIFY( _context != nullptr, "PowerStateManager.Update.NoContext", "" ) ) {
    return;
  }

  const bool shouldBeInPowerSave = !_powerSaveRequests.empty();
  if( shouldBeInPowerSave != _inPowerSaveMode ) {
    if( shouldBeInPowerSave ) {
      EnterPowerSave(dependentComps);
    }
    else {
      ExitPowerSave(dependentComps);
    }
  }

  if( _cameraState == CameraState::ShouldDelete ) {
    auto& visionComponent = dependentComps.GetComponent<VisionComponent>();
    if( visionComponent.TryReleaseInternalImages() ) {
      CameraService::getInstance()->DeleteCamera();
      _cameraState = CameraState::Deleted;
    }
  }

  if( _inPowerSaveMode && kPowerSave_ThrottleCPU ) {
    // if we are getting backed up with mic data, flip into the higher (but still low) CPU frequency
    const float micBufferLevel = dependentComps.GetComponent<MicComponent>().GetBufferFullness();

    if( _cpuThrottleLow && micBufferLevel >= kMicBufferHighMark ) {
      PRINT_CH_DEBUG("PowerStates", "PowerStateManager.Update.CPU.High",
                     "Mic buffer fullness %f, kicking into higher CPU mode",
                     micBufferLevel);
      OSState::getInstance()->SetDesiredCPUFrequency(kCPUFreqSaveHigh);
      _cpuThrottleLow = false;
    }
    else if( !_cpuThrottleLow && micBufferLevel <= kMicBufferLowMark ) {
      PRINT_CH_DEBUG("PowerStates", "PowerStateManager.Update.CPU.Low",
                     "Mic buffer fullness %f, kicking into lower CPU mode",
                     micBufferLevel);
      OSState::getInstance()->SetDesiredCPUFrequency(kCPUFreqSaveLow);
      _cpuThrottleLow = true;
    }
  }
}

void PowerStateManager::RequestPowerSaveMode(const std::string& requester)
{
  PRINT_CH_DEBUG("PowerStates", "PowerStateManager.Update.AddRequest",
                 "Adding power save request from '%s'",
                 requester.c_str());
  _powerSaveRequests.insert(requester);
}

bool PowerStateManager::RemovePowerSaveModeRequest(const std::string& requester)
{
  const size_t numRemoved = _powerSaveRequests.erase(requester);

  PRINT_CH_DEBUG("PowerStates", "PowerStateManager.Update.RemoveRequest",
                 "Removed %zu requests for '%s'",
                 numRemoved,
                 requester.c_str());

  return (numRemoved > 0);
}


void PowerStateManager::TogglePowerSaveSetting( const RobotCompMap& components,
                                                PowerSaveSetting setting,
                                                bool savePower )
{
  const bool currentlyEnabled = _enabledSettings.find(setting) != _enabledSettings.end();

  if( savePower && currentlyEnabled ) {
    PRINT_NAMED_WARNING("PowerStateManager.Toggle.DoubleEnable",
                        "Attempting to enable power save mode twice");
    // TODO:(bn) enum to string
    return;
  }
  if( !savePower && !currentlyEnabled ) {
    PRINT_NAMED_WARNING("PowerStateManager.Toggle.DoubleDisable",
                        "Attempting to disable power save mode twice");
    return;
  }

  bool result = true;

  switch( setting ) {
    case PowerSaveSetting::CalmMode: {
      const bool calibOnDisable = true;
      Result sendResult = _context->GetRobotManager()->GetMsgHandler()->SendMessage(
        RobotInterface::EngineToRobot(RobotInterface::CalmPowerMode(savePower, calibOnDisable)));
      result = (sendResult == RESULT_OK);
      break;
    }

    case PowerSaveSetting::Camera: {
      if( !CameraService::hasInstance() ) {
        PRINT_NAMED_WARNING("PowerStateManager.Toggle.CameraService.NoInstance",
                            "Trying to interact with camera service, but it doesn't exist");
        result = false;
      }
      else {
        auto& visionComponent = components.GetComponent<VisionComponent>();
        if( savePower ) {
          visionComponent.Pause(true);

          if( _cameraState != CameraState::Deleted ) {
            _cameraState = CameraState::ShouldDelete;
          }
        }
        else {
          if( _cameraState == CameraState::Deleted ) {
            if( CameraService::getInstance()->InitCamera() == RESULT_OK ) {
              _cameraState = CameraState::Running;
            }
            else {
              PRINT_NAMED_ERROR("PowerStateManager.Toggle.FailedToInitCamera",
                                "Camera service init failed! Camera may be in a bad state");
            }
          }
          else {
            _cameraState = CameraState::Running;
          }

          visionComponent.Pause(false);
        }
      }

      break;
    }

    case PowerSaveSetting::LCDBacklight: {
      const auto power = savePower ? kLCDBrightnessLow : kLCDBrightnessNormal;
      Result sendResult = _context->GetRobotManager()->GetMsgHandler()->SendMessage(
        RobotInterface::EngineToRobot( RobotInterface::SetLCDBrightnessLevel( power ) ) );

      PRINT_CH_INFO("PowerStates", "PowerStateManager.Toggle.LCD.Set",
                    "%s (%d) result=%d",
                    EnumToString(power),
                    Util::EnumToUnderlying(power),
                    Util::EnumToUnderlying(sendResult));

      result = (sendResult == RESULT_OK);
      break;
    }

    case PowerSaveSetting::ThrottleCPU: {

      // there are two frequencies used in power save mode, one higher and one lower. The idea is to spend
      // most of the time in the lower frequency state, but we may need to jump up to the higher state,
      // e.g. to drain the mic data buffer if it gets backed up. Start out in the higher state, and the code
      // in UpdateDependent will toggle back and forth as needed
      const auto freq = savePower ? kCPUFreqSaveHigh : kCPUFreqNormal;
      OSState::getInstance()->SetDesiredCPUFrequency(freq);
      _cpuThrottleLow = false;

      PRINT_CH_INFO("PowerStates", "PowerStateManager.Toggle.CPUFreq.Set", "set cpu frequency");
      break;
    }

    case PowerSaveSetting::ProxSensorNavMap: {
      const bool proxEnabled = !savePower;
      components.GetComponent<ProxSensorComponent>().SetEnabled(proxEnabled);
      PRINT_CH_DEBUG("PowerStates", "PowerStateManager.Toggle.ProxSensor",
                     "prox sensor nav map updates %s",
                     proxEnabled ? "ENABLED" : "DISABLED");
      break;
    }
  }

  if( result && savePower ) {
    _enabledSettings.insert(setting);
  }
  else if( result && !savePower ) {
    _enabledSettings.erase(setting);
  }
  else {
    PRINT_NAMED_WARNING("PowerStateManager.Toggle.Fail",
                        "Failed to %s setting %d",
                        savePower ? "enable" : "disable",
                        Util::EnumToUnderlying(setting));
  }
}


void PowerStateManager::EnterPowerSave(const RobotCompMap& components)
{
  PRINT_CH_INFO("PowerStates", "PowerStateManager.Enter",
                "Entering power save mode");

  if( kPowerSave_CalmMode ) {
    TogglePowerSaveSetting( components, PowerSaveSetting::CalmMode, true );
  }

  if( kPowerSave_Camera ) {
    TogglePowerSaveSetting( components, PowerSaveSetting::Camera, true );
  }

  if( kPowerSave_LCDBacklight ) {
    TogglePowerSaveSetting( components, PowerSaveSetting::LCDBacklight, true );
  }

  if( kPowerSave_ThrottleCPU ) {
    TogglePowerSaveSetting( components, PowerSaveSetting::ThrottleCPU, true );
  }
  
  if( kPowerSave_ProxSensorMap ) {
    TogglePowerSaveSetting( components, PowerSaveSetting::ProxSensorNavMap, true );
  }

  _inPowerSaveMode = true;
}

void PowerStateManager::ExitPowerSave(const RobotCompMap& components)
{
  PRINT_CH_INFO("PowerStates", "PowerStateManager.Exit",
                "Exiting power save mode");

  const size_t limit = _enabledSettings.size() + 1;

  BOUNDED_WHILE( limit, !_enabledSettings.empty() ) {
    const auto setting = *_enabledSettings.begin();
    TogglePowerSaveSetting( components, setting, false );
  }

  _inPowerSaveMode = false;
}

}
}
