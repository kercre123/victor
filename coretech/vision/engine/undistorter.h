/**
 * File: undistorter.h
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

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/array2d.h"

#include <map>
#include <memory>

namespace Anki {
namespace Vision {
  
// Forward declaration
class CameraCalibration;
template<class T> class ImageBase;
class Image;
class ImageRGB;

class Undistorter
{
public:
  Undistorter(const std::shared_ptr<CameraCalibration>& calib);
  virtual ~Undistorter();
  
  // Pre-cache undistortion maps for a given image size (does nothing if they already exist)
  Result CacheUndistortionMaps(s32 nrows, s32 ncols);
  
  // TODO: Provide ability to load from / save to disk (question: how to store per calibration?)
  //Result Save(const std::string& filename);
  //Result Load(const std::string& filename);
  
  // Will compute and cache distortion maps if needed, or used pre-cached ones if available
  Result UndistortImage(const ImageRGB& img, ImageRGB& undistortedImage);
  Result UndistortImage(const Image&    img, Image&    undistortedImage);
  
  // Undistort one or more points from an (nrows x ncols) image. Scales calibration as needed.
  Result UndistortPoint(const s32 nrows, const s32 ncols,
                        const Point2f& distortedPointIn,
                        Point2f& undistortedPointOut);
  
  Result UndistortPoints(const s32 nrows, const s32 ncols,
                         const std::vector<Point2f>& distortedPointsIn,
                         std::vector<Point2f>& undistortedPointsOut);
  
  // Static versions if you just want to pass in the calibration instead of having
  // an instantiated Undistorter laying around
  static Result UndistortPoint(const std::shared_ptr<CameraCalibration>& calib,
                               const s32 nrows, const s32 ncols,
                               const Point2f& distortedPointIn,
                               Point2f& undistortedPointOut);
  
  static Result UndistortPoints(const std::shared_ptr<CameraCalibration>& calib,
                                const s32 nrows, const s32 ncols,
                                const std::vector<Point2f>& distortedPointsIn,
                                std::vector<Point2f>& undistortedPointsOut);
private:
  
  std::shared_ptr<CameraCalibration> _calib;

  using ResolutionKey = std::pair<s32, s32>; // first=nrows, second=ncols
  struct MapPair;
  std::map<ResolutionKey, std::unique_ptr<MapPair>> _mapCache;
  
  Result CacheUndistortionMaps(s32 nrows, s32 ncols, MapPair* &mapPair);
  
  template<class T>
  Result UndistortImageHelper(const ImageBase<T>& img, ImageBase<T>& undistortedImage);
  
}; // class Undistorter
  
//
// Inlined methods
//
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline Result Undistorter::UndistortPoint(const s32 nrows, const s32 ncols,
                                          const Point2f& distortedPointIn,
                                          Point2f& undistortedPointOut)
{
  // Just call static version with member calibration
  return Undistorter::UndistortPoint(_calib, nrows, ncols, distortedPointIn, undistortedPointOut);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline Result Undistorter::UndistortPoints(const s32 nrows, const s32 ncols,
                                           const std::vector<Point2f>& distortedPointsIn,
                                           std::vector<Point2f>& undistortedPointsOut)
{
  // Just call static version with member calibration
  return Undistorter::UndistortPoints(_calib, nrows, ncols, distortedPointsIn, undistortedPointsOut);
}
  
} // namespace Vision
} // namespace Anki

