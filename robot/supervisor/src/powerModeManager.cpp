#include "powerModeManager.h"
#include "headController.h"
#include "imuFilter.h"
#include "liftController.h"
#include "wheelController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/DAS.h"
#include "clad/types/motorTypes.h"

namespace Anki {
namespace Vector {
namespace PowerModeManager {

namespace {

  // Whether or not active mode (i.e. opposite of calm mdoe) is enabled
  bool enable_        = false;

  // Whether or not motors should be calibrated when transitioning from calm to active
  bool calibOnEnable_ = false;

  // Whether or not we should go into calm mode because on charger
  // in order to prevent battery overheating
  bool autoDisable_   = false;
  TimeStamp_t autoDisableOnChargerTime_ms_ = 0;
  const u32 TIME_TO_AUTODISABLE_ON_CHARGER_MS = 10 * 1000;

} // "private" members


void EnableActiveMode(bool enable, bool calibOnEnable)
{
  enable_ = enable;
  calibOnEnable_ = calibOnEnable;
}

bool IsActiveModeEnabled()
{
  return enable_;
}

void Update() 
{
  // Check if any of the motors are being commanded
  // If so, active mode must be temporarily re-enabled (if it's currently disabled)
  const bool wheelsMoving = WheelController::AreWheelsPowered() || WheelController::AreWheelsMoving();
  const bool headMoving   = !HeadController::IsInPosition() || HeadController::IsMoving() || HeadController::IsBracing();
  const bool liftMoving   = !LiftController::IsInPosition() || LiftController::IsMoving() || LiftController::IsBracing();
  const bool shouldEnableInternally = wheelsMoving || headMoving || liftMoving;

  const bool isHALActiveModeEnabled = HAL::PowerGetDesiredMode() == HAL::POWER_MODE_ACTIVE;
  
  const bool shouldEnable = (enable_ && !autoDisable_) || shouldEnableInternally;
  if (shouldEnable && !isHALActiveModeEnabled) {
    // Active mode should be enabled, but it's not.
    // Enable it!
    AnkiInfo("PowerModeManager.EnableActiveMode", 
             "enable: %d (%d), autoDisable: %d, calib: %d", 
             enable_, shouldEnableInternally, autoDisable_, calibOnEnable_);
    DASMSG(power_mode_manager_enable_active_mode, "PowerModeManager.EnableActiveMode", "Enabling syscon active mode");
    DASMSG_SET(i1, enable_, "Enable (external)");
    DASMSG_SET(i2, shouldEnableInternally, "Should enable internally because of commanded motors");
    DASMSG_SET(i3, autoDisable_, "Auto calm mode");
    DASMSG_SET(i4, calibOnEnable_, "Calibrate on enable");
    DASMSG_SEND();
    
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_ACTIVE);

    if (calibOnEnable_ && enable_) {
      AnkiInfo("PowerModeManager.CalibrateOnActiveMode", "");
      LiftController::StartCalibrationRoutine(true, EnumToString(MotorCalibrationReason::PowerSaveEnteringActiveMode));
      HeadController::StartCalibrationRoutine(true, EnumToString(MotorCalibrationReason::PowerSaveEnteringActiveMode));
    }
  } else if (!shouldEnable && isHALActiveModeEnabled) {

    AnkiInfo("PowerModeManager.DisableActiveMode", 
             "enable: %d (%d), autoDisable: %d", 
             enable_, shouldEnableInternally, autoDisable_);
    DASMSG(power_mode_manager_disable_active_mode, "PowerModeManager.DisableActiveMode", "Disabling syscon active mode");
    DASMSG_SET(i1, enable_, "Enable (external)");
    DASMSG_SET(i2, shouldEnableInternally, "Should enable internally because of commanded motors");
    DASMSG_SET(i3, autoDisable_, "Auto calm mode");
    DASMSG_SET(i4, calibOnEnable_, "Calibrate on enable");
    DASMSG_SEND();
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_CALM);

    if (autoDisable_ && enable_) {
      AnkiWarn("PowerModeManager.AutoCalmMode", "Going into calm mode because on charger for TIME_TO_AUTODISABLE_ON_CHARGER_MS");
      DASMSG(power_mode_manager_auto_calm_mode, "power_mode_manager.auto_calm_mode", "Automatically going into calm mode due to on charger");
      DASMSG_SEND_WARNING();
    }
  }

  // Check for any activity that should clear, and reset the timeout for, autoDisable 
  static bool wasOnCharger          = false;
  const bool  isOnCharger           = HAL::BatteryIsOnCharger();
  const bool  onChargerStateChanged = isOnCharger != wasOnCharger;
              wasOnCharger          = isOnCharger;

  auto now_ms = HAL::GetTimeStamp();
  if (onChargerStateChanged) {
    if (isOnCharger) {
      autoDisableOnChargerTime_ms_ = now_ms + TIME_TO_AUTODISABLE_ON_CHARGER_MS;
    } else {
      autoDisableOnChargerTime_ms_ = 0;
      autoDisable_ = false;
    }
  }
  if (autoDisableOnChargerTime_ms_ > 0 && now_ms > autoDisableOnChargerTime_ms_) {
    autoDisable_ = true;
    calibOnEnable_ = false;
  }

}


} // namespace PowerModeManager
} // namespace Vector
} // namespace Anki
