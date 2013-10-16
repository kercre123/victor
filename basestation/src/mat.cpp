#include "anki/cozmo/basestation/mat.h"

namespace Anki {
  namespace Cozmo {
    
    const Point2f MatSection::Size = Point2f(800.f, 800.f);
    
    const float MatMarker2d::SquareWidth = 10.f; // mm
    
    MatMarker2d::MatMarker2d(const u16 xSquareIn, const u16 ySquareIn,
                             const Pose2d& imgPoseIn,
                             const MarkerUpDirection upDirection)
    : xSquare(xSquareIn), ySquare(ySquareIn), imgPose(imgPoseIn)
    {
      switch(upDirection)
      {
        case MARKER_TOP:
          upAngle = 0.f;
          break;
          
        case MARKER_BOTTOM:
          upAngle = M_PI;
          break;
          
        case MARKER_LEFT:
          upAngle = M_PI_2;
          break;
          
        case MARKER_RIGHT:
          upAngle = 3.f*M_PI_2;
          break;
          
        default:
          CORETECH_ASSERT(false); // Should never get here
          
      } // switch(upDirection)
      
    } // MatMarker2d Constructor
    
  } // namespace Cozmo
  
} // namespace Anki