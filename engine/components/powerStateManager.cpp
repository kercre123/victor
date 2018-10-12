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
#include "util/logging/DAS.h"
#include "util/logging/logging.h"

#include "coretech/common/engine/utils/timer.h"
#include "webServerProcess/src/webService.h"

#include "clad/types/lcdTypes.h"

namespace Anki {
namespace Vector {

namespace {

#define CONSOLE_GROUP "PowerSave"

CONSOLE_VAR( bool, kPowerSave_CalmMode, CONSOLE_GROUP, true);
CONSOLE_VAR( bool, kPowerSave_Camera, CONSOLE_GROUP, true);
CONSOLE_VAR( bool, kPowerSave_CameraStopCameraStream, CONSOLE_GROUP, false);
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

// The minimum amount of time that we must be in active mode before we can
// go to power save mode, in order to limit fast/expensive mode toggling.
static constexpr const float kMinActiveModeDuration_s = 10.f;
  
const std::string kWebVizPowerModule = "power";
const f32 kSendWebVizDataPeriod_sec = 0.5f;

}

PowerStateManager::PowerStateManager()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::PowerStateManager)
  , UnreliableComponent<BCComponentID>(this, BCComponentID::PowerStateManager)
{
}

void PowerStateManager::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _context = dependentComps.GetComponent<ContextWrapper>().context;

  // webviz controls for externally setting power save
  if (ANKI_DEV_CHEATS) {
    if (_context != nullptr) {
      auto* webService = _context->GetWebService();
      if (webService != nullptr) {
        auto onData = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
          if( data["enablePowerSave"].asString().compare("true")==0 ) {
            RequestPowerSaveMode("WebViz");
          } else {
            RemovePowerSaveModeRequest("WebViz");
          }
        };
        _signalHandles.emplace_back(webService->OnWebVizData(kWebVizPowerModule).ScopedSubscribe(onData));
      }
    }
  }
}

void PowerStateManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  if( !ANKI_VERIFY( _context != nullptr, "PowerStateManager.Update.NoContext", "" ) ) {
    return;
  }

  // Check if power save mode needs to be toggled
  const bool shouldBeInPowerSave = !_powerSaveRequests.empty();
  if( shouldBeInPowerSave != _inPowerSaveMode ) {
    if( shouldBeInPowerSave ) {
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if (currTime_s - _timePowerSaveToggled_s > kMinActiveModeDuration_s) {
        EnterPowerSave(dependentComps);
      }
    }
    else {
      ExitPowerSave(dependentComps);
    }
  }

  if( _cameraState == CameraState::ShouldDelete )
  {
    auto& visionComponent = dependentComps.GetComponent<VisionComponent>();
    Result res = RESULT_OK;
    // It is only safe to delete the camera once we are done
    // processing images
    if( !visionComponent.IsProcessingImages() )
    {
      if(kPowerSave_CameraStopCameraStream)
      {
        res = CameraService::getInstance()->DeleteCamera();
      }
      else
      {
        visionComponent.EnableImageCapture(false);
      }
        
      if(res == RESULT_OK)
      {
        _cameraState = CameraState::Deleted;
      }
    }
    
  }
  else if(_cameraState == CameraState::ShouldInit)
  {
    auto& visionComponent = dependentComps.GetComponent<VisionComponent>();
    Result res = RESULT_OK;
    if(kPowerSave_CameraStopCameraStream)
    {
      // Should InitCamera fail, it will get called again next tick which may end up working
      // This is just a hunch that camera init sometimes can fail but will work if called again
      // I've seen CameraService get into a mismatched state after InitCamera (should be fixed)
      // and restarting engine (calls InitCamera) fixes things
      // Worst case InitCamera will never work and VisionComponent will show a fault code
      // due to not receiving any images for some amount of time
      res = CameraService::getInstance()->InitCamera();
    }
    else
    {
      visionComponent.EnableImageCapture(true);
    }
    
    if(res == RESULT_OK)
    {
      visionComponent.Pause(false);
      _cameraState = CameraState::Running;
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
  
  
  // Send info to web viz occasionally
  if (ANKI_DEV_CHEATS) {
    const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (now_sec > _nextSendWebVizDataTime_sec) {
      if (_context != nullptr) {
        auto* webService = _context->GetWebService();
        if (webService!= nullptr && webService->IsWebVizClientSubscribed(kWebVizPowerModule)) {
          Json::Value toSend;
          toSend["powerSaveEnabled"] = InPowerSaveMode() ? "true" : "false";
          
          std::stringstream ss;
          ss << "[";
          for(auto& requester : _powerSaveRequests) {
            ss << requester << ", ";
          }
          ss << "]";
          toSend["powerSaveRequesters"] = ss.str();
          webService->SendToWebViz(kWebVizPowerModule, toSend);
        }
      }
      _nextSendWebVizDataTime_sec = now_sec + kSendWebVizDataPeriod_sec;
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
        else
        {
          if(_cameraState != CameraState::Running)
          {
            _cameraState = CameraState::ShouldInit;
          }
          else
          {
            PRINT_NAMED_WARNING("PowerStateManager.ToggleOn.CameraServiceAlreadyRunning",
                                "CameraState:%d not initing",
                                _cameraState);
          }
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

  if( !_inPowerSaveMode ) {
    DASMSG(power_save_start, "engine.power_save.start", "Sent when engine beigns trying to save power");
    DASMSG_SEND();
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  _inPowerSaveMode = true;
  _timePowerSaveToggled_s = currTime_s;
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

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( _inPowerSaveMode && _timePowerSaveToggled_s >= 0.0f ) {
    const int delta_s = (int) std::round( currTime_s - _timePowerSaveToggled_s );
    DASMSG(power_save_end, "engine.power_save.end", "Engine stopped trying to save power");
    DASMSG_SET(i1, delta_s, "Number of seconds spent in power save");
    DASMSG_SEND();
  }

  _inPowerSaveMode = false;
  _timePowerSaveToggled_s = currTime_s;
}


void PowerStateManager::NotifyOfRobotState(const RobotState& msg)
{
  _inSysconCalmMode = static_cast<bool>(msg.status & (uint32_t)RobotStatusFlag::CALM_POWER_MODE);
}


}
}
