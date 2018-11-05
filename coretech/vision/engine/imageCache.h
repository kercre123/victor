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
#include "coretech/vision/engine/imageBuffer/imageBuffer.h"
#include "coretech/vision/engine/imageCacheSizes.h"
#include "clad/types/imageFormats.h"

#include <map>

namespace Anki {
namespace Vision {
  
class ImageCache
{
public:
  
  ImageCache();
  
  // Invalidate all cached data and start with new image data at ImageCacheSize::Full.
  // Will use 'method' when resizing the image
  // May not release associated image memory.
  void Reset(const Image& imgGray, ResizeMethod method = ResizeMethod::Linear);
  
  void Reset(const ImageRGB& imgColor, ResizeMethod method = ResizeMethod::Linear);

  void Reset(const ImageBuffer& buffer);
  
  // Invalidate the cache and release all the memory associated with it.
  void ReleaseMemory();
  
  // HasColor is true iff Reset was called with an ImageRGB (even if cached "colorized gray" data exists)
  bool HasColor() const { return _hasColor; }
  
  // Get the timestamp of the image this cache was initialized with
  TimeStamp_t GetTimeStamp() const { return _timeStamp; }  

  // Get the image dimensions of the image returned for a given Size, based on
  // the dimensions of the image with which the cache was reset.
  // NOTE: Does not actually create a cache entry for the specified size, if none exists yet. (I.e., like a "peek" op)
  s32 GetNumRows(const ImageCacheSize atSize) const;
  s32 GetNumCols(const ImageCacheSize atSize) const;

  // Returns true or false depending on whether or not 'method' was set
  bool GetResizeMethod(ResizeMethod& method) const;
  
  // Look up a Size enum, given a subsample increment
  // NOTE: subsample = 1 always means "Full" processing resolution, irrespective of the relationship
  //       between Full and Sensor resolution. Size::Sensor will never be returned.
  static ImageCacheSize GetSize(s32 subsample);
  
  // Interprets a scale string into a Size enum
  // Valid scales are: "full", "half", "quarter", and "eighth"
  static ImageCacheSize StringToSize(const std::string& scaleStr);

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
  static constexpr ImageCacheSize GetDefaultImageCacheSize() { return ImageCacheSize::Half; }
  const Image&    GetGray(ImageCacheSize size = GetDefaultImageCacheSize(), GetType* getType = nullptr);
  const ImageRGB& GetRGB(ImageCacheSize size  = GetDefaultImageCacheSize(), GetType* getType = nullptr);

  const ImageBuffer& GetBuffer() const { return _buffer; }
  
private:

  s32          _sensorNumRows = 0;
  s32          _sensorNumCols = 0;
  bool         _hasColor = false;
  TimeStamp_t  _timeStamp = 0;
  
  // When we are Reset with an image buffer, we need keep a copy of it around to
  // give to any newly created ResizedEntrys
  ImageBuffer  _buffer;

  // Container class to hold ImageBuffer, Image, and/or ImageRGB that
  // were created at a specific ImageCacheSize using a ResizeMethod
  // Is capable of converting from any one ImageType to any other it can hold
  // using Get<ImageType>()
  class ResizedEntry
  {
    ImageBuffer    _buffer;
    ImageCacheSize _size;
    
    Image     _gray;
    bool     _hasValidGray = false;

    ImageRGB _rgb;
    bool     _hasValidRGB  = false;
    
  public:
    template<class ImageType>
    ResizedEntry(const ImageType& origImg, ImageCacheSize size)
    {
      Update(origImg, size);
    }
    
    void Invalidate() { _hasValidGray = false; _hasValidRGB = false; _buffer.Invalidate(); }
    
    template<class ImageType>
    ImageType& Get();
    
    template<class ImageType>
    bool IsValid() const { return (_hasValidRGB || _hasValidGray || _buffer.HasValidData()); }

    template<class ImageType>
    void Update(const ImageType& origImg, ImageCacheSize size);
  };

  using ResizeVersionsMap = std::map<ImageCacheSize, ResizedEntry>;
  ResizeVersionsMap _resizedVersions;
  
  ResizeMethod GetMethod(ImageCacheSize size) const;
  
  template<class ImageType>
  void ResetHelper(const ImageType& img);
  
  template<class ImageType>
  const ImageType& GetImageHelper(ImageCacheSize size, GetType& getType);
  
}; // class ImageCache
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_ImageCache_H__ */
