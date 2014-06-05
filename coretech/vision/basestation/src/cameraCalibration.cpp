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
    : nrows(480), ncols(640), focalLength_x(1.f), focalLength_y(1.f),
    center(0.f,0.f),
    skew(0.f)
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
    : nrows(nrowsIn), ncols(ncolsIn),
    focalLength_x(fx), focalLength_y(fy),
    center(cenx, ceny), skew(skew_in)
    {
      /*
       std::fill(this->distortionCoeffs.begin(),
       this->distortionCoeffs.end(),
       0.f);
       */
    }
    
    CameraCalibration::CameraCalibration(const Json::Value &jsonNode)
    {
      CORETECH_ASSERT(jsonNode.isMember("nrows"));
      nrows = JsonTools::GetValue<u16>(jsonNode["nrows"]);
      
      CORETECH_ASSERT(jsonNode.isMember("ncols"));
      ncols = JsonTools::GetValue<u16>(jsonNode["ncols"]);
      
      CORETECH_ASSERT(jsonNode.isMember("focalLength_x"));
      focalLength_x = JsonTools::GetValue<f32>(jsonNode["focalLength_x"]);
      
      CORETECH_ASSERT(jsonNode.isMember("focalLength_y"))
      focalLength_y = JsonTools::GetValue<f32>(jsonNode["focalLength_y"]);
      
      CORETECH_ASSERT(jsonNode.isMember("center_x"))
      center.x() = JsonTools::GetValue<f32>(jsonNode["center_x"]);
      
      CORETECH_ASSERT(jsonNode.isMember("center_y"))
      center.y() = JsonTools::GetValue<f32>(jsonNode["center_y"]);
      
      CORETECH_ASSERT(jsonNode.isMember("skew"))
      skew = JsonTools::GetValue<f32>(jsonNode["skew"]);
      
      // TODO: Add distortion coefficients
    }

    
    void CameraCalibration::CreateJson(Json::Value& jsonNode) const
    {
      jsonNode["nrows"] = nrows;
      jsonNode["ncols"] = ncols;
      jsonNode["focalLength_x"] = focalLength_x;
      jsonNode["focalLength_y"] = focalLength_y;
      jsonNode["center_x"] = center.x();
      jsonNode["center_y"] = center.y();
      jsonNode["skew"] = skew;
    }
    
    
    template<typename PRECISION>
    SmallSquareMatrix<3,PRECISION> CameraCalibration::get_calibrationMatrix() const
    {
      const PRECISION K_data[9] = {
        static_cast<PRECISION>(focalLength_x), static_cast<PRECISION>(focalLength_x*skew), static_cast<PRECISION>(center.x()),
        PRECISION(0),                          static_cast<PRECISION>(focalLength_y),      static_cast<PRECISION>(center.y()),
        PRECISION(0),                          PRECISION(0),                               PRECISION(1)};
      
      return SmallSquareMatrix<3,PRECISION>(K_data);
    } // get_calibrationMatrix()
    
    template<typename PRECISION>
    SmallSquareMatrix<3,PRECISION> CameraCalibration::get_invCalibrationMatrix() const
    {
      const PRECISION invK_data[9] = {
        static_cast<PRECISION>(1.f/focalLength_x),
        static_cast<PRECISION>(-skew/focalLength_y),
        static_cast<PRECISION>(center.y()*skew/focalLength_y - center.x()/focalLength_x),
        PRECISION(0),    static_cast<PRECISION>(1.f/focalLength_y),    static_cast<PRECISION>(-center.y()/focalLength_y),
        PRECISION(0),    PRECISION(0),                                 PRECISION(1)
      };
      
      return SmallSquareMatrix<3,PRECISION>(invK_data);
    }

    // Explicit instantiation for single and double precision
    template SmallSquareMatrix<3,f64> CameraCalibration::get_calibrationMatrix() const;
    template SmallSquareMatrix<3,f32>  CameraCalibration::get_calibrationMatrix() const;
    template SmallSquareMatrix<3,f64> CameraCalibration::get_invCalibrationMatrix() const;
    template SmallSquareMatrix<3,f32>  CameraCalibration::get_invCalibrationMatrix() const;
    
  } // namespace Vision
} // namespace Anki