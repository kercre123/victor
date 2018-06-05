//
//  cameraCalibration.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "coretech/vision/engine/cameraCalibration.h"

#include "coretech/common/engine/jsonTools.h"

#include "coretech/common/engine/math/matrix_impl.h"
#include "coretech/common/engine/math/point_impl.h"


namespace Anki {
namespace Vision {
  
  CameraCalibration::CameraCalibration(u16 nrows,    u16 ncols,
                                       f32 fx,       f32 fy,
                                       f32 center_x, f32 center_y,
                                       f32 skew)
  : _nrows(nrows)
  , _ncols(ncols)
  , _focalLength_x(fx)
  , _focalLength_y(fy)
  , _center(center_x, center_y)
  , _skew(skew)
  {
    
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

    // Note: unlike parameters above, if distortionCoeffs not present, they are left at 0
    JsonTools::GetVectorOptional(jsonNode, "distortionCoeffs", _distortionCoeffs);
    
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
            && _skew == other._skew
            && _distortionCoeffs == other._distortionCoeffs);

  }
  
  CameraCalibration& CameraCalibration::Scale(u16 nrows, u16 ncols)
  {
    if(ncols == _ncols && nrows == _nrows)
    {
      // Special case, early out
      return *this;
    }
    
    const f32 xscale = (f32)ncols / (f32)_ncols;
    const f32 yscale = (f32)nrows / (f32)_nrows;
    
    _focalLength_x *= xscale;
    _focalLength_y *= yscale;
    _center.x()    *= xscale;
    _center.y()    *= yscale;
    
    _nrows = nrows;
    _ncols = ncols;
    
    return *this;
  }
  
  CameraCalibration CameraCalibration::GetScaled(u16 nrows, u16 ncols) const
  {
    CameraCalibration scaledCalib(*this);
    scaledCalib.Scale(nrows,ncols);
    return scaledCalib;
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
