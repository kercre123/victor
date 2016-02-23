/**
 * File: pathMotionProfileHelpers.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-02-22
 *
 * Description: Helpers for motion profile clad struct
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/pathMotionProfileHelpers.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/types/pathMotionProfile.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

void LoadPathMotionProfileFromJson(PathMotionProfile& profile, const Json::Value& config)
{
  profile.speed_mmps = config.get("speed_mmps", DEFAULT_PATH_SPEED_MMPS).asFloat();
  profile.accel_mmps2 = config.get("accel_mmps2", DEFAULT_PATH_ACCEL_MMPS2).asFloat();
  profile.decel_mmps2 = config.get("decel_mmps2", DEFAULT_PATH_DECEL_MMPS2).asFloat();
  profile.pointTurnSpeed_rad_per_sec = config.get("pointTurnSpeed_rad_per_sec",
                                                  DEFAULT_PATH_POINT_TURN_SPEED_RAD_PER_SEC).asFloat();
  profile.pointTurnAccel_rad_per_sec2 = config.get("pointTurnAccel_rad_per_sec2",
                                                   DEFAULT_PATH_POINT_TURN_ACCEL_RAD_PER_SEC2).asFloat();
  profile.pointTurnDecel_rad_per_sec2 = config.get("pointTurnDecel_rad_per_sec2",
                                                   DEFAULT_PATH_POINT_TURN_DECEL_RAD_PER_SEC2).asFloat();
  profile.dockSpeed_mmps = config.get("dockSpeed_mmps", DEFAULT_DOCK_SPEED_MMPS).asFloat();
  profile.dockAccel_mmps2 = config.get("dockAccel_mmps2", DEFAULT_DOCK_ACCEL_MMPS2).asFloat();
  profile.reverseSpeed_mmps = config.get("reverseSpeed_mmps", DEFAULT_PATH_REVERSE_SPEED_MMPS).asFloat();
}

}
}
