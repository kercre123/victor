//
//  cameraCalibration.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/vision/basestation/cameraCalibration.h"

#include "anki/common/basestation/jsonTools.h"

#include "anki/common/basestation/math/matrix_impl.h"
#include "anki/common/basestation/math/point_impl.h"

/*
#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif
*/



namespace Anki {
  
  namespace Vision {
    
    CameraCalibration::CameraCalibration()
    : _nrows(480)
    , _ncols(640)
    , _focalLength_x(1.f)
    , _focalLength_y(1.f)
    , _center(0.f,0.f)
    , _skew(0.f)
    {
      /*
       std::fill(this->distortionCoeffs.begin(),
       this->distortionCoeffs.end(),
       0.f);
       */
    }
    
    CameraCalibration::CameraCalibration(const u16 nrowsIn, const u16 ncolsIn,
                                         const f32 fx,    const f32 fy,
                                         const f32 cenx,  const f32 ceny,
                                         const f32 skew_in)
    : _nrows(nrowsIn)
    , _ncols(ncolsIn)
    , _focalLength_x(fx)
    , _focalLength_y(fy)
    , _center(cenx, ceny)
    , _skew(skew_in)
    {
      /*
       std::fill(this->distortionCoeffs.begin(),
       this->distortionCoeffs.end(),
       0.f);
       */
    }
    
    CameraCalibration::CameraCalibration(const Json::Value &jsonNode)
    {
      CORETECH_ASSERT(Set(jsonNode) == RESULT_OK);
    }
    
    Result CameraCalibration::Set(const Json::Value& jsonNode)
    {
      if(!jsonNode.isMember("nrows")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _nrows = JsonTools::GetValue<u16>(jsonNode["nrows"]);
      
      if(!jsonNode.isMember("ncols")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _ncols = JsonTools::GetValue<u16>(jsonNode["ncols"]);
      
      if(!jsonNode.isMember("focalLength_x")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _focalLength_x = JsonTools::GetValue<f32>(jsonNode["focalLength_x"]);
      
      if(!jsonNode.isMember("focalLength_y")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _focalLength_y = JsonTools::GetValue<f32>(jsonNode["focalLength_y"]);
      
      if(!jsonNode.isMember("center_x")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _center.x() = JsonTools::GetValue<f32>(jsonNode["center_x"]);
      
      if(!jsonNode.isMember("center_y")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _center.y() = JsonTools::GetValue<f32>(jsonNode["center_y"]);
      
      if(!jsonNode.isMember("skew")) {
        return RESULT_FAIL_INVALID_PARAMETER;
      }
      _skew = JsonTools::GetValue<f32>(jsonNode["skew"]);

      // TODO: Add distortion coefficients
      
      return RESULT_OK;
    } // Set()

    
    void CameraCalibration::CreateJson(Json::Value& jsonNode) const
    {
      jsonNode["nrows"]         = _nrows;
      jsonNode["ncols"]         = _ncols;
      jsonNode["focalLength_x"] = _focalLength_x;
      jsonNode["focalLength_y"] = _focalLength_y;
      jsonNode["center_x"]      = _center.x();
      jsonNode["center_y"]      = _center.y();
      jsonNode["skew"]          = _skew;
    }
    
    bool CameraCalibration::operator==(const CameraCalibration& other) const
    {
      return (_nrows == other._nrows
              &&  _ncols == other._ncols
              && _focalLength_x == other._focalLength_x
              && _focalLength_y == other._focalLength_y
              && _center == other._center
              && _skew == other._skew);

    }
    
    template<typename PRECISION>
    SmallSquareMatrix<3,PRECISION> CameraCalibration::GetCalibrationMatrix() const
    {
      const PRECISION K_data[9] = {
        static_cast<PRECISION>(_focalLength_x), static_cast<PRECISION>(_focalLength_x*_skew), static_cast<PRECISION>(_center.x()),
        PRECISION(0),                           static_cast<PRECISION>(_focalLength_y),       static_cast<PRECISION>(_center.y()),
        PRECISION(0),                           PRECISION(0),                                 PRECISION(1)};
      
      return SmallSquareMatrix<3,PRECISION>(K_data);
    } // get_calibrationMatrix()
    
    template<typename PRECISION>
    SmallSquareMatrix<3,PRECISION> CameraCalibration::GetInvCalibrationMatrix() const
    {
      const PRECISION invK_data[9] = {
        static_cast<PRECISION>(1.f/_focalLength_x),
        static_cast<PRECISION>(-_skew/_focalLength_y),
        static_cast<PRECISION>(_center.y()*_skew/_focalLength_y - _center.x()/_focalLength_x),
        PRECISION(0),    static_cast<PRECISION>(1.f/_focalLength_y),    static_cast<PRECISION>(-_center.y()/_focalLength_y),
        PRECISION(0),    PRECISION(0),                                 PRECISION(1)
      };
      
      return SmallSquareMatrix<3,PRECISION>(invK_data);
    }

    // Explicit instantiation for single and double precision
    template SmallSquareMatrix<3,f64>  CameraCalibration::GetCalibrationMatrix() const;
    template SmallSquareMatrix<3,f32>  CameraCalibration::GetCalibrationMatrix() const;
    template SmallSquareMatrix<3,f64>  CameraCalibration::GetInvCalibrationMatrix() const;
    template SmallSquareMatrix<3,f32>  CameraCalibration::GetInvCalibrationMatrix() const;
    
  } // namespace Vision
} // namespace Anki