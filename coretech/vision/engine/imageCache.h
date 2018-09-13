/**
 * File: imageCache.h
 *
 * Author:  Andrew Stein
 * Created: 06/05/2017
 *
 * Description: Caches resized/gray/color version of an image so that different parts of the vision system
 *              can re-use already-requested sizes/colorings of an image without needing to know about each other.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_ImageCache_H__
#define __Anki_Cozmo_Basestation_ImageCache_H__

#include "coretech/vision/engine/image.h"

#include <map>

namespace Anki {
namespace Vision {
  
class ImageCache
{
public:
  
  ImageCache();
  
  // Invalidate all cached data and start with new image data at Size::Sensor.
  // 'fullScaleFactor' (and method) specifies the scaling applied (and resize method), if any, to
  // get Size::Full from this input image. It is expected to be on the interval (0.f, 1.f].
  // By default, "Full processing resolution" is the same as Sensor resolution, but this allows them to
  // be different.
  // May not release associated image memory.
  void Reset(const Image&    imgGray,  const f32 fullScaleFactor = 1.f,
             const ResizeMethod fullScaleMethod = ResizeMethod::Linear);
  
  void Reset(const ImageRGB& imgColor, const f32 fullScaleFactor = 1.f,
             const ResizeMethod fullScaleMethod = ResizeMethod::Linear);
  
  // Invalidate the cache and release all the memory associated with it.
  void ReleaseMemory();
  
  // HasColor is true iff Reset was called with an ImageRGB (even if cached "colorized gray" data exists)
  bool HasColor()       const { return _hasColor;    }
  
  // Get the timestamp of the image this cache was initialized with
  TimeStamp_t GetTimeStamp() const { return _timeStamp; }
  
  // For requesting different sizes and resize interpolation methods
  enum class Size : u8
  {
    // Full *sensor* resolution with which this cache was Reset
    Sensor,
    
    // Full *processing* resolution (depending on Reset call above, may or may not be the same as Sensor)
    // Sizes below are relative to Full
    Full,
    
    // Nearest Neighbor
    Half_NN,
    Quarter_NN,
    Double_NN,
    
    // Linear Interpolation
    Half_Linear,
    Quarter_Linear,
    Double_Linear,

    // Average-Area Downsampling
    Half_AverageArea,
    Quarter_AverageArea,

    // TODO: add other sizes/methods (e.g. cubic interpolation)
  };

  // Get the image dimensions of the image returned for a given Size, based on
  // the dimensions of the image with which the cache was reset.
  // NOTE: Does not actually create a cache entry for the specified size, if none exists yet. (I.e., like a "peek" op)
  s32 GetNumRows(const Size atSize) const;
  s32 GetNumCols(const Size atSize) const;
  
  // Look up a Size enum, given a subsample increment and resize method
  // NOTE: subsample = 1 always means "Full" processing resolution, irrespective of the relationship
  //       between Full and Sensor resolution. Size::Sensor will never be returned.
  static Size GetSize(s32 subsample, ResizeMethod method);
  
  // Interprets a scale string and a method string into a Size enum
  // Valid scales are: "full", "half", "quarter"
  // Valid methods are: "nearest", "linear", "area" (ignored for "full")
  static Size StringToSize(const std::string& scaleStr, const std::string& methodStr);

  // For unit tests to verify expected behavior
  enum class GetType : u8
  {
    NewEntry,
    ResizeIntoExisting,
    ComputeFromExisting,
    FullyCached
  };
  
  // Get color or gray image data at a given scale factor / resize method.
  // If Reset with a color image, the gray version will be computed by averaging (and cached).
  // If Reset with a gray image, the "RGB" version will be computed by replicating channels (and cached).
  // Thus you will always get a valid, non-empty image for both of these calls.
  // If you want to do different things depending on whether there is actually color data available (not just
  // replicated gray channels), use HasColor() above to decide which of these to call.
  // Notes:
  //  * The result of HasColor is not changed by calling GetRGB.
  //  * These are non-const because they could compute a resized version on demand.
  const Image&    GetGray(Size size=Size::Full, GetType* getType = nullptr);
  const ImageRGB& GetRGB(Size size=Size::Full,  GetType* getType = nullptr);
  
private:

  s32          _sensorNumRows = 0;
  s32          _sensorNumCols = 0;
  bool         _hasColor = false;
  TimeStamp_t  _timeStamp = 0;
  f32          _fullScaleFactor = 1.f; // to compute Size::Full from Size::Sensor
  ResizeMethod _fullScaleMethod = ResizeMethod::Linear;
  
  class ResizedEntry
  {
    Image    _gray;
    ImageRGB _rgb;
    bool     _hasValidGray = false;
    bool     _hasValidRGB  = false;
    
  public:
    template<class ImageType>
    ResizedEntry(const ImageType& origImg, f32 scaleFactor, ResizeMethod method)
    {
      Update(origImg, scaleFactor, method);
    }
    
    void Invalidate() { _hasValidGray = false; _hasValidRGB = false; }
    
    template<class ImageType>
    ImageType& Get(bool computeFromOpposite = false);
    
    template<class ImageType>
    bool IsValid() const { return (_hasValidRGB || _hasValidGray); }
    
    void Update(const Image&    origImg, f32 scaleFactor, ResizeMethod method);
    void Update(const ImageRGB& origImg, f32 scaleFactor, ResizeMethod method);
  };
  
  std::map<Size, ResizedEntry> _resizedVersions;
  
  f32 GetScaleFactor(ImageCache::Size size) const;
  ResizeMethod GetMethod(ImageCache::Size size) const;
  
  template<class ImageType>
  void ResetHelper(const ImageType& img, const f32 fullScaleFactor, const ResizeMethod fullScaleMethod);
  
  template<class ImageType>
  const ImageType& GetImageHelper(Size size, GetType& getType);
  
}; // class ImageCache
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_ImageCache_H__ */
