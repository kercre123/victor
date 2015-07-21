//
//  cameraCalibration.h
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef ANKI_CORETECH_VISION_BASESTATION_CAMERA_CALIBRATION_H_
#define ANKI_CORETECH_VISION_BASESTATION_CAMERA_CALIBRATION_H_

#include "json/json.h"
#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/shared/radians.h"
#include <cmath>

namespace Anki {
  
  // Forward declaration
  template<s32, typename> class SmallSquareMatrix;
  
  namespace Vision {
    
    class CameraCalibration
    {
    public:
      /*
       static const int NumDistortionCoeffs = 5;
       typedef std::array<float,CameraCalibration::NumDistortionCoeffs> DistortionCoeffVector;
       */
      
      // Constructors:
      CameraCalibration();
      
      CameraCalibration(const u16 nrows,    const u16 ncols,
                        const f32 fx,       const f32 fy,
                        const f32 center_x, const f32 center_y,
                        const f32 skew = 0.f);
      
      CameraCalibration(const u16 nrows,    const u16 ncols,
                        const f32 fx,       const f32 fy,
                        const f32 center_x, const f32 center_y,
                        const f32 skew,
                        const std::vector<float> &distCoeffs);
      
      // Construct from a Json node
      CameraCalibration(const Json::Value& jsonNode);
      
      // Set from a Json node
      Result Set(const Json::Value& jsonNode);
      
      // Accessors:
      u16     GetNrows()         const;
      u16     GetNcols()         const;
      f32     GetFocalLength_x() const;
      f32     GetFocalLength_y() const;
      f32     GetCenter_x()      const;
      f32     GetCenter_y()      const;
      f32     GetSkew()          const;
      const Point2f& GetCenter() const;
      
      // Compute vertical/horizontal FOV angles.
      // (These are full field of view, not half field of view.)
      Radians ComputeVerticalFOV() const;
      Radians ComputeHorizontalFOV() const;
      
      //const   DistortionCoeffVector& get_distortionCoeffs() const;
      
      // Returns the 3x3 camera calibration matrix:
      // [fx   skew*fx   center_x;
      //   0      fy     center_y;
      //   0       0         1    ]
      template<typename PRECISION = float>
      SmallSquareMatrix<3,PRECISION> GetCalibrationMatrix() const;
      
      // Returns the inverse calibration matrix (e.g. for computing
      // image rays)
      template<typename PRECISION = float>
      SmallSquareMatrix<3,PRECISION> GetInvCalibrationMatrix() const;
      
      void CreateJson(Json::Value& jsonNode) const;
      
      bool operator==(const CameraCalibration& other) const;
      bool operator!=(const CameraCalibration& other) const {return !(*this == other);}
      
    protected:
      
      u16     _nrows, _ncols;
      f32     _focalLength_x, _focalLength_y;
      Point2f _center;
      f32     _skew;
      
      //DistortionCoeffVector distortionCoeffs; // radial distortion coefficients
      
    }; // class CameraCalibration
    
    
    // Inline accessor definitions:
    inline u16 CameraCalibration::GetNrows() const
    { return _nrows; }
    
    inline u16 CameraCalibration::GetNcols() const
    { return _ncols; }
    
    inline f32 CameraCalibration::GetFocalLength_x() const
    { return _focalLength_x; }
    
    inline f32 CameraCalibration::GetFocalLength_y() const
    { return _focalLength_y; }
    
    inline f32 CameraCalibration::GetCenter_x() const
    { return _center.x(); }
    
    inline f32 CameraCalibration::GetCenter_y() const
    { return _center.y(); }
    
    inline const Point2f& CameraCalibration::GetCenter() const
    { return _center; }
    
    inline f32  CameraCalibration::GetSkew() const
    { return _skew; }
    
    inline Radians CameraCalibration::ComputeVerticalFOV() const {
      return Radians(2.f*std::atan2f(0.5f*static_cast<f32>(GetNrows()),
                                     GetFocalLength_y()));
    }
    
    inline Radians CameraCalibration::ComputeHorizontalFOV() const {
      return Radians(2.f*std::atan2f(0.5f*static_cast<f32>(GetNcols()),
                                     GetFocalLength_x()));
    }
    
  } // namesapce Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_CAMERA_CALIBRATION_H_
