/**
 * File: imageCache.cpp
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

#include "coretech/vision/engine/imageCache.h"

#include "coretech/common/engine/array2d_impl.h"

#include "util/math/math.h"

#define DEBUG_IMAGE_CACHE 0

#if DEBUG_IMAGE_CACHE
#  define VERBOSE_DEBUG_PRINT PRINT_CH_DEBUG
#else
#  define VERBOSE_DEBUG_PRINT(...) {}
#endif

namespace Anki {
namespace Vision {

#if DEBUG_IMAGE_CACHE
  namespace {
    //
    // Helpers only needed if using DEBUG_IMAGE_CACHE
    //
    static const char * const kLogChannelName = "ImageCache";
    
    inline static const char * GetColorStr(const ImageRGB&)
    {
      return "RGB";
    }
    
    inline static const char * GetColorStr(const Image&)
    {
      return "Gray";
    }

    inline static const char * GetColorStr(const ImageBuffer&)
    {
      return "Buffer";
    }
  }
#endif
  
// =====================================================================================================================
//                                  RESIZED ENTRY
// =====================================================================================================================
  
template<>
inline Image& ImageCache::ResizedEntry::Get<Image>()
{
  // Don't already have gray
  if(!_hasValidGray)
  {
    // If buffer is valid use that to get gray
    if(_buffer.HasValidData())
    {
      _hasValidGray = _buffer.GetGray(_gray, _size, _resizeMethod);
    }

    // If we failed to get gray from buffer
    if(!_hasValidGray)
    {
      if(!_hasValidRGB)
      {
        // GetGray from buffer failed so try RGB
        // Buffer had better be valid because _rgb is not valid and we don't already have Gray
        DEV_ASSERT(_buffer.HasValidData(), "ImageCache.ResizeEntry.GetGray.NoBufferOrRGB");
        
        // ImageBuffer has more support for getting rgb so do that first
        // and then fill gray from rgb
        _hasValidRGB = _buffer.GetRGB(_rgb, _size, _resizeMethod);
        DEV_ASSERT(_hasValidRGB, "ImageCache.ResizeEntry.GetGray.FailedToGetFromBuffer");
      }
      
      _rgb.FillGray(_gray, ImageRGB::RGBToGrayMethod::GreenChannel);
      _hasValidGray = true;
    }
  }
  
  DEV_ASSERT(_hasValidGray, "ImageCache.ResizedEntry.GetGray.InvalidEntry");
  return _gray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
inline ImageRGB& ImageCache::ResizedEntry::Get<ImageRGB>()
{
  // Don't already have RGB
  if(!_hasValidRGB)
  {
    if(_buffer.HasValidData())
    {
      _hasValidRGB = _buffer.GetRGB(_rgb, _size, _resizeMethod);
    }

    // Buffer is either invalid or GetRGB failed but we have a valid gray image
    // already so use it
    if(!_hasValidRGB && _hasValidGray)
    {
      // This is not expected to happen and is likely not what the caller intended to happen
      // This should never happen, the ImageBuffer should always be valid.
      // There is also the additional (HasColor() && IsRequestingColor()) check in GetImageHelper()
      // which should also prevent this from happening
      PRINT_NAMED_WARNING("ImageCache.ResizedEntry.GetRGB.ComputingFromGray", "");
      _rgb.SetFromGray(_gray);
      _hasValidRGB = true;
    }
  }
  
  DEV_ASSERT(_hasValidRGB, "ImageCache.ResizedEntry.GetRGB.InvalidEntry");
  return _rgb;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<> inline bool ImageCache::ResizedEntry::IsValid<Image>() const
{
  return _hasValidGray;
}

template<> inline bool ImageCache::ResizedEntry::IsValid<ImageRGB>() const
{
  return _hasValidRGB;
}

template<> inline bool ImageCache::ResizedEntry::IsValid<ImageBuffer>() const
{
  return _buffer.HasValidData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s32 ImageCache::GetNumRows(const ImageCacheSize atSize) const
{
  return std::round(ImageCacheSizeToScaleFactor(atSize)*(f32)_sensorNumRows);
}
  
s32 ImageCache::GetNumCols(const ImageCacheSize atSize) const
{
  return std::round(ImageCacheSizeToScaleFactor(atSize)*(f32)_sensorNumCols);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ResizeMethod ImageCache::GetMethod(ImageCacheSize size) const
{
  switch(size)
  {
    case ImageCacheSize::Full:
      return _fullScaleMethod;
      
    case ImageCacheSize::Half:
    case ImageCacheSize::Quarter:
    case ImageCacheSize::Eighth:
      return ResizeMethod::Linear;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
static void ResizeHelper(const ImageType& origImg, ImageCacheSize size, ResizeMethod method,
                         ImageType& resizedImg_out)
{
  f32 scaleFactor = ImageCacheSizeToScaleFactor(size);
  if(Util::IsNear(scaleFactor, 1.f))
  {
    resizedImg_out = origImg;
  }
  else
  {
    const s32 resizedNumRows = std::round((f32)origImg.GetNumRows() * scaleFactor);
    const s32 resizedNumCols = std::round((f32)origImg.GetNumCols() * scaleFactor);
    resizedImg_out.Allocate(resizedNumRows, resizedNumCols);
    origImg.Resize(resizedImg_out, method);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ImageCache::ResizedEntry::Update(const Image& origImg, ImageCacheSize size, ResizeMethod method)
{
  ResizeHelper(origImg, size, method, _gray);
  _hasValidGray = true;
  _size = size;
  _resizeMethod = method;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ImageCache::ResizedEntry::Update(const ImageRGB& origImg, ImageCacheSize size, ResizeMethod method)
{
  ResizeHelper(origImg, size, method, _rgb);
  _hasValidRGB = true;
  _size = size;
  _resizeMethod = method;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ImageCache::ResizedEntry::Update(const ImageBuffer& origImg, ImageCacheSize size, ResizeMethod method)
{
  _buffer = origImg;
  _size = size;
  _resizeMethod = method;
}


// =====================================================================================================================
//                                  IMAGE CACHE
// =====================================================================================================================
  
ImageCache::ImageCache()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::ReleaseMemory()
{
  _resizedVersions.clear();
  _sensorNumRows = 0;
  _sensorNumCols = 0;
  _hasColor = false;
  _timeStamp = 0;
  _fullScaleMethod = ResizeMethod::Linear;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageCacheSize ImageCache::GetSize(s32 subsample, Vision::ResizeMethod method)
{
  ImageCacheSize size = ImageCacheSize::Full;
  
  if(subsample != 1)
  {
    switch(method)
    {
      case Vision::ResizeMethod::Linear:
      case Vision::ResizeMethod::NearestNeighbor:
      case Vision::ResizeMethod::AverageArea:
      {
        switch(subsample)
        {
          case 2:
            size = ImageCacheSize::Half;
            break;
            
          case 4:
            size = ImageCacheSize::Quarter;
            break;

          case 8:
            size = ImageCacheSize::Eighth;
            break;
            
          default:
            DEV_ASSERT(false, "ImageCache.GetSize.UnsupportedSubsample");
            break;
        }
        break;
      }
      
      default:
        PRINT_NAMED_ERROR("ImageCache.GetSize.UnsupportedMethod", "");
        break;
    }
  }

  return size;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const kScaleFullString     = "full";
const char* const kScaleHalfString     = "half";
const char* const kScaleQuarterString  = "quarter";
const char* const kScaleEighthString   = "eighth";
const char* const kMethodNearestString = "nearest";
const char* const kMethodLinearString  = "linear";
const char* const kMethodAreaString    = "average_area";
ImageCacheSize ImageCache::StringToSize(const std::string& scaleStr, const std::string& methodStr)
{
  s32 scale = 0;
  if(kScaleFullString == scaleStr)
  {
    scale = 1;
  }
  else if(kScaleHalfString == scaleStr)
  {
    scale = 2;
  }
  else if(kScaleQuarterString == scaleStr)
  {
    scale = 4;
  }
  else if(kScaleEighthString == scaleStr)
  {
    scale = 8;
  }
  else
  {
    PRINT_NAMED_ERROR("ImageCache.StringToSize.InvalidScale", "%s", scaleStr.c_str());
  }

  Vision::ResizeMethod method = Vision::ResizeMethod::Linear;
  if(kMethodNearestString == methodStr)
  {
    method = Vision::ResizeMethod::NearestNeighbor;
  }
  else if(kMethodLinearString == methodStr)
  {
    method = Vision::ResizeMethod::Linear;
  }
  else if(kMethodAreaString == methodStr)
  {
    method = Vision::ResizeMethod::AverageArea;
  }
  else
  {
    PRINT_NAMED_ERROR("ImageCache.StringToSize.InvalidMethod", "%s", methodStr.c_str());
  }

  return ImageCache::GetSize(scale, method);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
void ImageCache::ResetHelper(const ImageType& img, const ResizeMethod fullScaleMethod)
{
  VERBOSE_DEBUG_PRINT(kLogChannelName, "ImageCache.ResetHelper", "Resetting with %s image at t=%ums",
                      GetColorStr(img), img.GetTimestamp());
  
  // Invalidate all cache entries (but don't necessarily free their memory, since we are likely
  // to reuse them and can potentially reuse that memory)
  for(auto & entry : _resizedVersions)
  {
    entry.second.Invalidate();
  }
  
  const ResizeMethod kSensorMethod = ResizeMethod::NearestNeighbor; // not used
  
  auto iter = _resizedVersions.find(ImageCacheSize::Full);
  if(iter == _resizedVersions.end())
  {
    _resizedVersions.emplace(ImageCacheSize::Full, ResizedEntry(img, ImageCacheSize::Full, kSensorMethod));
  }
  else
  {
    iter->second.Update(img, ImageCacheSize::Full, kSensorMethod);
  }
  
  _sensorNumRows = img.GetNumRows();
  _sensorNumCols = img.GetNumCols();
  
  _hasColor = (img.GetNumChannels() != 1);
  
  _timeStamp = img.GetTimestamp();
  
  _fullScaleMethod = fullScaleMethod;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const Image& imgGray, const ResizeMethod fullScaleMethod)
{
  // This is kind of gross but this function should only ever be called from unit tests.
  // The tests were written when ImageCache was Reset with a Full image instead of a Sensor image.
  // Calling ResetHelper with an Image would have caused the image to be resized incorrectly
  // compared to what the unit tests were expecting. Wrapping the Image in an ImageBuffer
  // allows ImageBuffer to deal with the special resizing/scaleFactor for Images
  // Might get better with VIC-8052
  _buffer = ImageBuffer(const_cast<u8*>(imgGray.GetRawDataPointer()),
                        imgGray.GetNumRows(),
                        imgGray.GetNumCols(),
                        ImageEncoding::RawGray,
                        imgGray.GetTimestamp(),
                        imgGray.GetImageId());
  
  ResetHelper(_buffer, fullScaleMethod);

  // This is basically a no-op from an image creation standpoint, will just wrap an Image around the
  // ImageBuffer's data but it will have the side effect of "caching" the image in the Sensor ResizedEntry.
  // If this is not done then the first call to get the Sensor sized gray image will think it had to
  // "ComputeFromExisting" instead of having "FullyCached" image
  auto iter = _resizedVersions.find(ImageCacheSize::Full);
  DEV_ASSERT(iter != _resizedVersions.end(), "ImageCache.Reset.Image.ExpectingToHaveSensor");
  (void)iter->second.Get<Image>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const ImageRGB& imgColor, const ResizeMethod fullScaleMethod)
{
  // This is kind of gross but this function should only ever be called from unit tests.
  // The tests were written when ImageCache was Reset with a Full image instead of a Sensor image.
  // Calling ResetHelper with an ImageRGB would have caused the image to be resized incorrectly
  // compared to what the unit tests were expecting. Wrapping the ImageRGB in an ImageBuffer
  // allows ImageBuffer to deal with the special resizing/scaleFactor for ImageRGBs
  // Might get better with VIC-8052
  _buffer = ImageBuffer(const_cast<u8*>(reinterpret_cast<const u8*>(imgColor.GetDataPointer())),
                        imgColor.GetNumRows(),
                        imgColor.GetNumCols(),
                        ImageEncoding::RawRGB,
                        imgColor.GetTimestamp(),
                        imgColor.GetImageId());
  
  ResetHelper(_buffer, fullScaleMethod);

  // This is basically a no-op from an image creation standpoint, will just wrap an ImageRGB around the
  // ImageBuffer's data but it will have the side effect of "caching" the image in the Sensor ResizedEntry.
  // If this is not done then the first call to get the Sensor sized rgb image will think it had to
  // "ComputeFromExisting" instead of having "FullyCached" image
  auto iter = _resizedVersions.find(ImageCacheSize::Full);
  DEV_ASSERT(iter != _resizedVersions.end(), "ImageCache.Reset.ImageRGB.ExpectingToHaveSensor");
  (void)iter->second.Get<ImageRGB>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const ImageBuffer& buffer, const ResizeMethod fullScaleMethod)
{
  // Note: This is a copy but is totally fine as ImageBuffer is just a wrapper around image data
  // so no images are actually copied
  _buffer = buffer;
  ResetHelper(_buffer, fullScaleMethod);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Image& ImageCache::GetGray(ImageCacheSize size, GetType* getType)
{
  GetType dummy;
  const Image& imgGray = GetImageHelper<Image>(size, (getType == nullptr ? dummy : *getType));
  DEV_ASSERT(!imgGray.IsEmpty(), "ImageCache.GetGray.EmptyImage");
  return imgGray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ImageRGB& ImageCache::GetRGB(ImageCacheSize size, GetType* getType)
{
  GetType dummy;
  const ImageRGB& imgRGB = GetImageHelper<ImageRGB>(size, (getType == nullptr ? dummy : *getType));
  DEV_ASSERT(!imgRGB.IsEmpty(), "ImageCache.GetRGB.EmptyImage");
  return imgRGB;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
static inline bool IsRequestingColor() {
  return false;
}

template<>
inline bool IsRequestingColor<ImageRGB>() {
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
const ImageType& ImageCache::GetImageHelper(ImageCacheSize size, GetType& getType)
{
  DEV_ASSERT(!_resizedVersions.empty(), "ImageCache.GetImageHelper.EmptyCache");
   
  auto iter = _resizedVersions.find(size);
  
  // We should compute a new entry (either "completely new" or reusing an invalidated entry at the same size) if:
  //  - there is no entry at the requested size, OR
  //  - the entry at the requested size is not valid for either ImageType, OR
  //  - there is a valid entry at the requested (non-sensor) size, but color data is being requested and the cache
  //     was originally reset with color data from which we could resize (instead of computing from gray)
  const bool shouldComputeNewValidEntry = (iter == _resizedVersions.end() ||
                                           !iter->second.IsValid<void>() ||
                                           ((ImageCacheSize::Full != size) &&
                                            HasColor() && IsRequestingColor<ImageType>()));
  if(shouldComputeNewValidEntry)
  {
    // No valid entry available at this ImageCacheSize, so compute, cache, and return it.
    const ResizeMethod method = GetMethod(size);
    
    if(iter == _resizedVersions.end())
    {
      // Insert a completely new entry
      if(_buffer.HasValidData())
      {
        auto insertion = _resizedVersions.emplace(size, ResizedEntry(_buffer, size, method));

        DEV_ASSERT(insertion.second, "ImageCache.GetImageHelper.NewEntryNotInserted");
        
        const ImageType& img = insertion.first->second.Get<ImageType>();
        getType = GetType::NewEntry;
        
        return img;
      }
      else
      {
        GetType dummy;
        const auto origImg = GetImageHelper<ImageType>(ImageCacheSize::Full, dummy);
        auto insertion = _resizedVersions.emplace(size, ResizedEntry(origImg, size, method));

        DEV_ASSERT(insertion.second, "ImageCache.GetImageHelper.NewEntryNotInserted");
        
        const ImageType& img = insertion.first->second.Get<ImageType>();
        getType = (img.GetDataPointer() == origImg.GetDataPointer() ? GetType::FullyCached : GetType::NewEntry);
        
        return img;
      }
    }
    else
    {
      // Resize directly into the existing (but invalid) entry's image
      ResizedEntry& entry = iter->second;
      if(_buffer.HasValidData())
      {
        entry.Update(_buffer, size, method);
      }
      else
      {
        GetType dummy;
        const auto& origImg = GetImageHelper<ImageType>(ImageCacheSize::Full, dummy);
        entry.Update(origImg, size, method);
      }
      getType = GetType::ResizeIntoExisting;
      return entry.Get<ImageType>();
    }
  }
  else
  {
    // Valid entry already available at this ImageCacheSize.
    ResizedEntry& entry = iter->second;
    
    // If the entry is already valid for the requested ImageType then it is FullyCached otherwise
    // we will be computing it from the existing entry
    getType = (entry.IsValid<ImageType>() ? GetType::FullyCached : GetType::ComputeFromExisting);
    
    ImageType& img = entry.Get<ImageType>();

    VERBOSE_DEBUG_PRINT("ImageCache", "ImageCache.GetImageHelper.UsingCached",
                        "%s %s image from existing entry at t=%ums and scaleFactor=%.2f",
                        (entry.IsValid<ImageType>() ? "Returning existing" : "Computing new"),
                        GetColorStr(img), img.GetTimestamp(), scaleFactor);
    
    return img;
  }
}
  
} // namespace Vision
} // namespace Anki
