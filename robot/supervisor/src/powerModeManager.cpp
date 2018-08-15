#include "powerModeManager.h"
#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"

namespace Anki {
namespace Vector {
namespace PowerModeManager {

namespace {

  bool enable_        = false;
  bool calibOnEnable_ = false;

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
  
  if ((enable_ || shouldEnableInternally) && !isHALActiveModeEnabled) {
    // Active mode should be enabled, but it's not.
    // Enable it!
    AnkiInfo("PowerModeManager.EnableActiveMode", "enable: %d, calib: %d", enable_, calibOnEnable_);
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_ACTIVE);

    if (calibOnEnable_ && enable_) {
      AnkiInfo("PowerModeManager.CalibrateOnActiveMode", "");
      LiftController::StartCalibrationRoutine(true);
      HeadController::StartCalibrationRoutine(true);
    }
  } else if (!enable_ && isHALActiveModeEnabled) {
    // Active mode should be disabled, but it's enabled.
    // Check if it's temporarily enabled via shouldEnableInternally.
    // If so, wait. Otherwise, enable it.
    if (!shouldEnableInternally) {
      AnkiInfo("PowerModeManager.DisableActiveMode", "");
      HAL::PowerSetDesiredMode(HAL::POWER_MODE_CALM);
    }
  }
}


} // namespace PowerModeManager
} // namespace Vector
} // namespace Anki
