//
//  robot.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__robot__
#define __Products_Cozmo__robot__

#include "anki/math/pose.h"

#include "anki/vision/camera.h"

namespace Anki {
  namespace Cozmo {

    class Robot
    {
    public:
      Robot();
      
      inline const Pose3d& get_pose() const;
      inline const Camera& get_camDown() const;
      inline const Camera& get_camFront() const;
      
    protected:
      Camera camDown, camFront;
      
      Pose3d pose;
      
    }; // class Robot

    // Inline accessors:
    const Pose3d& Robot::get_pose(void) const
    { return this->pose; }
    
    const Camera& Robot::get_camDown(void) const
    { return this->camDown; }
    
    const Camera& Robot::get_camFront(void) const
    { return this->camFront; }
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot__
