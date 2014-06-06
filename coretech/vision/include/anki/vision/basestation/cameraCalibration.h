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

//#include <array>

namespace Anki {
  
  // Forward declaration
  template<size_t, typename> class SmallSquareMatrix;
  
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
      
      
      // Accessors:
      u16     get_nrows()         const;
      u16     get_ncols()         const;
      f32     get_focalLength_x() const;
      f32     get_focalLength_y() const;
      f32     get_center_x()      const;
      f32     get_center_y()      const;
      f32     get_skew()          const;
      const Point2f& get_center() const;
      //const   DistortionCoeffVector& get_distortionCoeffs() const;
      
      // Returns the 3x3 camera calibration matrix:
      // [fx   skew*fx   center_x;
      //   0      fy     center_y;
      //   0       0         1    ]
      template<typename PRECISION = float>
      SmallSquareMatrix<3,PRECISION> get_calibrationMatrix() const;
      
      // Returns the inverse calibration matrix (e.g. for computing
      // image rays)
      template<typename PRECISION = float>
      SmallSquareMatrix<3,PRECISION> get_invCalibrationMatrix() const;
      
      void CreateJson(Json::Value& jsonNode) const;
      
    protected:
      
      u16  nrows, ncols;
      f32  focalLength_x, focalLength_y;
      Point2f center;
      f32  skew;
      //DistortionCoeffVector distortionCoeffs; // radial distortion coefficients
      
    }; // class CameraCalibration
    
    
    // Inline accessor definitions:
    inline u16 CameraCalibration::get_nrows() const
    { return this->nrows; }
    
    inline u16 CameraCalibration::get_ncols() const
    { return this->ncols; }
    
    inline f32 CameraCalibration::get_focalLength_x() const
    { return this->focalLength_x; }
    
    inline f32 CameraCalibration::get_focalLength_y() const
    { return this->focalLength_y; }
    
    inline f32 CameraCalibration::get_center_x() const
    { return this->center.x(); }
    
    inline f32 CameraCalibration::get_center_y() const
    { return this->center.y(); }
    
    inline const Point2f& CameraCalibration::get_center() const
    { return this->center; }
    
    inline f32  CameraCalibration::get_skew() const
    { return this->skew; }
    
    
  } // namesapce Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_CAMERA_CALIBRATION_H_
