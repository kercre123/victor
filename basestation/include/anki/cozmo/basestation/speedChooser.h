/**
 * File: speedChooser.h
 *
 * Author: Al Chaussee
 * Date:   4/20/2016
 *
 * Description: Creates a path motion profile based on robot state
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_SPEED_CHOOSER_H
#define ANKI_COZMO_SPEED_CHOOSER_H

#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/shared/cozmoConfig_common.h"
#include "clad/types/pathMotionProfile.h"
#include "util/random/randomGenerator.h"

namespace Anki {
  namespace Cozmo {
    
    class Robot;
    
    class SpeedChooser
    {
      public:
        SpeedChooser(Robot& robot);
      
        // Generates a path motion profile based on the distance to the goal pose
        PathMotionProfile GetPathMotionProfile(const Pose3d& goal);
      
        // Generates a path motion profile based on the distance to the closest goal
        PathMotionProfile GetPathMotionProfile(const std::vector<Pose3d>& goals);
      
      private:
        Robot& _robot;
      
        // Max speed a generated motion profile can have
        const int maxSpeed_mmps = MAX_WHEEL_SPEED_MMPS;
      
        // Min speed a generated motion profile can have
        const int minSpeed_mmps = 40;
      
        const int distToObjectForMaxSpeed_mm = 500;
      
        Util::RandomGenerator rng;
    };
  }
}

#endif // ANKI_COZMO_SPEED_CHOOSER_H