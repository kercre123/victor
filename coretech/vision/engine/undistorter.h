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
  
private:
  
  std::shared_ptr<CameraCalibration> _calib;

  using ResolutionKey = std::pair<s32, s32>; // first=nrows, second=ncols
  struct MapPair;
  std::map<ResolutionKey, std::unique_ptr<MapPair>> _mapCache;
  
  Result CacheUndistortionMaps(s32 nrows, s32 ncols, MapPair* &mapPair);
  
  template<class T>
  Result UndistortImageHelper(const ImageBase<T>& img, ImageBase<T>& undistortedImage);
  
};
  
} // namespace Vision
} // namespace Anki

