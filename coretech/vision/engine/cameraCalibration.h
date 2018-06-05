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
#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/shared/radians.h"
#include <cmath>

namespace Anki {
  
  // Forward declaration
  template<MatDimType, typename> class SmallSquareMatrix;
  
  namespace Vision {
    
    class CameraCalibration
    {
    public:
      
      using DistortionCoeffs = std::vector<f32>;
      
      // Constructors:
      CameraCalibration() = delete;
      
      CameraCalibration(u16 nrows,    u16 ncols,
                        f32 fx,       f32 fy,
                        f32 center_x, f32 center_y,
                        f32 skew = 0.f);
      
      // Construct with distortion coeffs provided in some kind of container which supports std::copy
      template<class DistCoeffContainer>
      CameraCalibration(u16 nrows,    u16 ncols,
                        f32 fx,       f32 fy,
                        f32 center_x, f32 center_y,
                        f32 skew,
                        const DistCoeffContainer& distCoeffs);
      
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
      
      void    SetFocalLength(f32 fx, f32 fy);
      void    SetCenter(const Point2f& center);
      
      // Compute vertical/horizontal FOV angles.
      // (These are full field of view, not half field of view.)
      Radians ComputeVerticalFOV() const;
      Radians ComputeHorizontalFOV() const;
      
      const DistortionCoeffs& GetDistortionCoeffs() const;
      
      // Adjust focal length and center for new resolution, leave skew and distortion coefficents alone.
      CameraCalibration& Scale(u16 nrows, u16 ncols);           // in place, returning reference to "this"
      CameraCalibration  GetScaled(u16 nrows, u16 ncols) const; // return new
      
      // Returns the 3x3 camera calibration matrix:
      // [fx   skew*fx   center_x;
      //   0      fy     center_y;
      //   0       0         1    ]
      template<typename PRECISION = f32>
      SmallSquareMatrix<3,PRECISION> GetCalibrationMatrix() const;
      
      // Returns the inverse calibration matrix (e.g. for computing
      // image rays)
      template<typename PRECISION = f32>
      SmallSquareMatrix<3,PRECISION> GetInvCalibrationMatrix() const;
      
      void CreateJson(Json::Value& jsonNode) const;
      
      bool operator==(const CameraCalibration& other) const;
      bool operator!=(const CameraCalibration& other) const {return !(*this == other);}
      
    protected:
      
      u16     _nrows, _ncols;
      f32     _focalLength_x, _focalLength_y;
      Point2f _center;
      f32     _skew;
      
      DistortionCoeffs _distortionCoeffs; // radial distortion coefficients
      
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
    
    inline void CameraCalibration::SetFocalLength(f32 fx, f32 fy) {
      _focalLength_x = fx;
      _focalLength_y = fy;
    }

    inline void CameraCalibration::SetCenter(const Point2f& center) {
      _center = center;
    }
    
    inline const CameraCalibration::DistortionCoeffs& CameraCalibration::GetDistortionCoeffs() const {
      return _distortionCoeffs;
    }
    
    template<class DistCoeffContainer>
    CameraCalibration::CameraCalibration(u16 nrows,    u16 ncols,
                                         f32 fx,       f32 fy,
                                         f32 center_x, f32 center_y,
                                         f32 skew,
                                         const DistCoeffContainer& distCoeffs)
    : CameraCalibration(nrows, ncols, fx, fy, center_x, center_y, skew)
    {
      std::copy(distCoeffs.begin(), distCoeffs.end(), std::back_inserter(_distortionCoeffs));
    }
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_CAMERA_CALIBRATION_H_
