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

  // Whether or not we should go into calm mode because of inactivity
  // for INACTIVITY_TIMEOUT_TO_CALM_MS
  bool autoDisable_   = false;
  TimeStamp_t lastActivityTime_ms_ = HAL::GetTimeStamp();
  const u32 INACTIVITY_TIMEOUT_TO_CALM_MS = 30 * 60 * 1000;

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
  
  if (((enable_ && !autoDisable_) || shouldEnableInternally) && !isHALActiveModeEnabled) {
    // Active mode should be enabled, but it's not.
    // Enable it!
    AnkiInfo("PowerModeManager.EnableActiveMode", 
             "enable: %d (%d), autoDisable: %d, calib: %d", 
             enable_, shouldEnableInternally, autoDisable_, calibOnEnable_);
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_ACTIVE);

    if (calibOnEnable_ && enable_) {
      AnkiInfo("PowerModeManager.CalibrateOnActiveMode", "");
      LiftController::StartCalibrationRoutine(true, EnumToString(MotorCalibrationReason::PowerSaveEnteringActiveMode));
      HeadController::StartCalibrationRoutine(true, EnumToString(MotorCalibrationReason::PowerSaveEnteringActiveMode));
    }
  } else if ((!enable_ || autoDisable_) && !shouldEnableInternally && isHALActiveModeEnabled) {
    AnkiInfo("PowerModeManager.DisableActiveMode", 
             "enable: %d (%d), autoDisable: %d", 
             enable_, shouldEnableInternally, autoDisable_);
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_CALM);

    if (autoDisable_ && enable_) {
      AnkiWarn("PowerModeManager.AutoCalmMode", "This shouldn't be happening outside of DevDoNothing!");
      DASMSG(power_mode_manager_auto_calm_mode, "power_mode_manager.auto_calm_mode", "Automatically going into calm mode due to inactivity");
      DASMSG_SEND_WARNING();
    }
  }

  // Check for any activity that should clear, and reset the timeout for, autoDisable 
  static bool wasPickedUp        = false;
  const bool  isPickedUp         = IMUFilter::IsPickedUp();
  const bool  pickupStateChanged = isPickedUp != wasPickedUp;
              wasPickedUp        = isPickedUp;
  static bool wasOnCharger = false;
  const bool  isOnCharger        = HAL::BatteryIsOnCharger();
  const bool  onChargerStateChanged = isOnCharger != wasOnCharger;
              wasOnCharger       = isOnCharger;
  const bool  buttonPressed      = HAL::GetButtonState(HAL::BUTTON_POWER);
  const bool  activityDetected   = shouldEnableInternally || buttonPressed || pickupStateChanged || onChargerStateChanged;

  auto now_ms = HAL::GetTimeStamp();
  if (activityDetected || !enable_) {
    lastActivityTime_ms_ = now_ms;
    autoDisable_ = false;
    calibOnEnable_ = true;
  } else if (now_ms - lastActivityTime_ms_ > INACTIVITY_TIMEOUT_TO_CALM_MS) {
    autoDisable_ = true;
  }
}


} // namespace PowerModeManager
} // namespace Vector
} // namespace Anki
