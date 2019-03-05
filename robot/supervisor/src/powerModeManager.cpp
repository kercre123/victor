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

} // "private" members


void EnableActiveMode(bool enable)
{
  enable_ = enable;
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
  
  const bool shouldEnable = enable_ || shouldEnableInternally;
  if (shouldEnable && !isHALActiveModeEnabled) {
    // Active mode should be enabled, but it's not.
    // Enable it!
    AnkiInfo("PowerModeManager.EnableActiveMode", 
             "enable: %d (%d)", 
             enable_, shouldEnableInternally);
    DASMSG(power_mode_manager_enable_active_mode, "PowerModeManager.EnableActiveMode", "Enabling syscon active mode");
    DASMSG_SET(i1, enable_, "Enable (external)");
    DASMSG_SET(i2, shouldEnableInternally, "Should enable internally because of commanded motors");
    DASMSG_SEND();
    
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_ACTIVE);

    if (enable_) {
      AnkiInfo("PowerModeManager.SetMotorsUncalibrated", "");
      LiftController::SetEncoderInvalid();
      HeadController::SetEncoderInvalid();
    }
  } else if (!shouldEnable && isHALActiveModeEnabled) {

    AnkiInfo("PowerModeManager.DisableActiveMode", 
             "enable: %d (%d)", 
             enable_, shouldEnableInternally);
    DASMSG(power_mode_manager_disable_active_mode, "PowerModeManager.DisableActiveMode", "Disabling syscon active mode");
    DASMSG_SET(i1, enable_, "Enable (external)");
    DASMSG_SET(i2, shouldEnableInternally, "Should enable internally because of commanded motors");
    DASMSG_SEND();
    HAL::PowerSetDesiredMode(HAL::POWER_MODE_CALM);
  }

}


} // namespace PowerModeManager
} // namespace Vector
} // namespace Anki
