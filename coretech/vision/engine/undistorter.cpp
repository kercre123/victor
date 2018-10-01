/**
 * File: undistorter.cpp
 *
 * Author: Andrew Stein
 * Date:   07/31/2018
 *
 * Description: Handles undistorting images, given a camera calibration.
 *              Computes and caches undistortion maps by image size on first use, then
 *              uses those for subsequent calls for matching image sizes.
 *
 * Copyright: Anki, Inc. 2018
 **/


#include "coretech/common/engine/array2d_impl.h"
#include "coretech/vision/engine/cameraCalibration.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/undistorter.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace Anki {
namespace Vision {

Undistorter::Undistorter(const std::shared_ptr<CameraCalibration>& calib)
: _calib(calib)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Undistorter::~Undistorter()
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct Undistorter::MapPair
{
  // map1: The first map of either (x,y) points or just x values having the type CV_16SC2, CV_32FC1,
  //       or CV_32FC2
  // map2: The second map of y values having the type CV_16UC1, CV_32FC1, or none (empty map if map1
  //       is (x,y) points), respectively.
  // For more details, see cv::convertMaps:
  // https://docs.opencv.org/2.4/modules/imgproc/doc/geometric_transformations.html?#convertmaps
  
  cv::Mat map1;
  cv::Mat map2;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Undistorter::CacheUndistortionMaps(s32 nrows, s32 ncols)
{
  MapPair* dummy = nullptr;
  return CacheUndistortionMaps(nrows, ncols, dummy);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Undistorter::CacheUndistortionMaps(s32 nrows, s32 ncols, MapPair* &mapPair)
{
  mapPair = nullptr;
  
  // See if we already have a cached pair of maps for this resolution
  const ResolutionKey resolutionKey(nrows,ncols);
  auto iter = _mapCache.find(resolutionKey);
  if(iter != _mapCache.end())
  {
    mapPair = iter->second.get();
  }
  
  // Compute the maps (and cache them) if we didn't find them
  if(mapPair == nullptr)
  {
    auto emplaceResult = _mapCache.emplace(resolutionKey, std::make_unique<MapPair>());
    DEV_ASSERT(emplaceResult.second, "Undistorter.CacheDistortionMaps.DidNotAddNewMapPairAsExpected");
    mapPair = emplaceResult.first->second.get();
    
    // This follows the same process as in cv::undistort, which is really just a wrapper
    // for cv::initUndistortRectifyMap() and cv::remap().
    mapPair->map1.create(nrows, ncols, CV_16SC2);
    mapPair->map2.create(nrows, ncols, CV_16UC1);
    
    const auto& I = cv::Mat_<double>::eye(3,3);
    const auto& scaledCalib = _calib->GetScaled(nrows, ncols);
    
    // NOTE: has to be double, thus the conversion on the next line
    cv::Mat_<double> K;
    cv::Mat(scaledCalib.GetCalibrationMatrix().get_CvMatx_()).convertTo(K, CV_64F);
    
    // NOTE: has to be double
    const cv::Mat_<double> distCoeffs(cv::Mat(scaledCalib.GetDistortionCoeffs()));
    
    try {
      cv::initUndistortRectifyMap(K, distCoeffs, I, K,
                                  cv::Size(ncols, nrows),
                                  mapPair->map1.type(),
                                  mapPair->map1, mapPair->map2);
    } catch (cv::Exception& e) {
      PRINT_NAMED_ERROR("Undistorter.CacheDistortionMaps.OpenCvInitUndistortRectifyMapFailed",
                        "%s", e.what());
      return RESULT_FAIL;
    }
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
Result Undistorter::UndistortImageHelper(const ImageBase<T>& img, ImageBase<T>& undistortedImage)
{
  if(!_calib)
  {
    PRINT_NAMED_ERROR("Undistorter.UndistortImageHelper.NoCalibration", "");
    return RESULT_FAIL;
  }
  
  MapPair* mapPair = nullptr;
  Result cacheResult = CacheUndistortionMaps(img.GetNumRows(), img.GetNumCols(), mapPair);
  if(RESULT_OK != cacheResult)
  {
    PRINT_NAMED_ERROR("Undistorter.UndistortImageHelper.CacheFail", "");
    return cacheResult;
  }
  
  // Use the computed/cached maps to do the undistortion
  undistortedImage.Allocate(img.GetNumRows(), img.GetNumCols());
  try {
    cv::remap(img.get_CvMat_(), undistortedImage.get_CvMat_(), mapPair->map1, mapPair->map2, CV_INTER_LINEAR);
  } catch (cv::Exception& e) {
    PRINT_NAMED_ERROR("Undistorter.UndistortImageHelper.OpenCvRemapFailed",
                      "%s", e.what());
    return RESULT_FAIL;
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Undistorter::UndistortImage(const ImageRGB& img, ImageRGB& undistortedImage)
{
  return UndistortImageHelper(img, undistortedImage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Undistorter::UndistortImage(const Image&    img, Image&    undistortedImage)
{
  return UndistortImageHelper(img, undistortedImage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Undistorter::UndistortPoints(const std::shared_ptr<CameraCalibration>& calib,
                                    const s32 nrows, const s32 ncols,
                                    const std::vector<Point2f>& distortedPointsIn,
                                    std::vector<Point2f>& undistortedPointsOut)
{
  std::vector<cv::Point2f> distortedPoints;
  distortedPoints.reserve(distortedPointsIn.size());
  for(auto const& point : distortedPointsIn)
  {
    distortedPoints.emplace_back(point.x(), point.y());
  }
  std::vector<cv::Point2f> undistortedPoints(distortedPoints.size());
  
  auto const& scaledCalib = calib->GetScaled(nrows, ncols);
  cv::Matx<f32,3,3> K = scaledCalib.GetCalibrationMatrix().get_CvMatx_();
  const std::vector<f32>& distCoeffs = scaledCalib.GetDistortionCoeffs();
  
  try
  {
    cv::undistortPoints(distortedPoints, undistortedPoints, K, distCoeffs, cv::noArray(), K);
  }
  catch(const cv::Exception& e)
  {
    PRINT_NAMED_ERROR("Undistorter.UndistortPoints.OpenCvUndistortFailed",
                      "OpenCV Error: %s", e.what());
    return RESULT_FAIL;
  }
  
  undistortedPointsOut.reserve(undistortedPoints.size());
  for(auto const& cvPoint : undistortedPoints)
  {
    undistortedPointsOut.emplace_back(cvPoint.x, cvPoint.y);
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Undistorter::UndistortPoint(const std::shared_ptr<CameraCalibration>& calib,
                                   const s32 nrows, const s32 ncols,
                                   const Point2f& distortedPointIn,
                                   Point2f& undistortedPointOut)
{
  std::vector<Point2f> undistortedPoints; // dummy vector to hold the single point
  const Result result = UndistortPoints(calib, nrows, ncols, {distortedPointIn}, undistortedPoints);
  if(RESULT_OK == result)
  {
    if(!ANKI_VERIFY(undistortedPoints.size() == 1, "Undistorter.UndistortPoint.ExpectingSinglePoint",
                    "Got %zu", undistortedPoints.size()))
    {
      return RESULT_FAIL;
    }
    undistortedPointOut = undistortedPoints.front();
  }
  
  return result;
}
  
} // namespace Vision
} // namespace Anki
