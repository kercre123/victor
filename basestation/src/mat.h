#ifndef __Products_Cozmo__MatMarker2d__
#define __Products_Cozmo__MatMarker2d__

#include "anki/common/basestation/math/pose.h"
#include "anki/vision/basestation/marker2d.h"

namespace Anki {
  
  namespace Cozmo {

    class RobotMessage;
    
    class Mat {
    public:
      static const Point2f Size;
      
    }; // class Mat
    
    class MatMarker2d //: public Marker2d<5,2>
    {
    public:
      static const float SquareWidth;
            
      //using Marker2d<5,2>::BitString;
      
      // For if/when we want to actually decode a string:
      //MatMarker2d(const BitString &str, const Quad2f &corners);
      
      MatMarker2d(const RobotMessage &msg);
      
      //void encodeIDs(void);
      //void decodeIDs(const BitString &bitString);
      int     get_xSquare() const;
      int     get_ySquare() const;
      Radians get_upAngle() const;
      
      const Pose2d& get_imagePose() const;
      
    protected:
      
      int xSquare, ySquare;
      
      Radians upAngle;
      
      Pose2d imgPose; // x,y,angle in image coordinates
      
    }; // class MatMarker2d
    
    inline int MatMarker2d::get_xSquare() const
    { return this->xSquare; }
    
    inline int MatMarker2d::get_ySquare() const
    { return this->ySquare; }
    
    inline Radians MatMarker2d::get_upAngle() const
    { return this->upAngle; }
    
    inline const Pose2d& MatMarker2d::get_imagePose() const
    { return this->imgPose; }
    
    
  } // namespace Cozmo

} // namespace Anki

#endif // __Products_Cozmo__MatMarker2d__
