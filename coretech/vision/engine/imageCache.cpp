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
  }
#endif
  
// =====================================================================================================================
//                                  RESIZED ENTRY
// =====================================================================================================================

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
inline Image& ImageCache::ResizedEntry::Get<Image>(bool computeFromOpposite)
{
  if(computeFromOpposite && !_hasValidGray)
  {
    DEV_ASSERT(_hasValidRGB, "ImageCache.ResizedEntry.GetGray.NoColorAvailable");
    _rgb.FillGray(_gray);
    _hasValidGray = true;
  }
  
  DEV_ASSERT(_hasValidGray, "ImageCache.ResizedEntry.GetGray.InvalidEntry");
  return _gray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
inline ImageRGB& ImageCache::ResizedEntry::Get<ImageRGB>(bool computeFromOpposite)
{
  if(computeFromOpposite && !_hasValidRGB)
  {
    DEV_ASSERT(!_gray.IsEmpty(), "ImageCache.ResizedEntry.GetRGB.NoGrayAvailable");
    _rgb.SetFromGray(_gray);
    _hasValidRGB = true;
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline f32 GetScaleFactor(ImageCache::Size size)
{
  switch(size)
  {
    case ImageCache::Size::Full:
      DEV_ASSERT(false, "ImageCache.GetScaleFactor.FullRequiresNoResize");
      return 1.f;
      
    case ImageCache::Size::Double_NN:
    case ImageCache::Size::Double_Linear:
      return 2.f;
      
    case ImageCache::Size::Half_NN:
    case ImageCache::Size::Half_Linear:
    case ImageCache::Size::Half_AverageArea:
      return 0.5f;
      
    case ImageCache::Size::Quarter_NN:
    case ImageCache::Size::Quarter_Linear:
    case ImageCache::Size::Quarter_AverageArea:
      return 0.25f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s32 ImageCache::GetNumRows(const Size atSize) const
{
  return std::round(GetScaleFactor(atSize)*(f32)GetOrigNumRows());
}
  
s32 ImageCache::GetNumCols(const Size atSize) const
{
  return std::round(GetScaleFactor(atSize)*(f32)GetOrigNumCols());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline ResizeMethod GetMethod(ImageCache::Size size)
{
  switch(size)
  {
    case ImageCache::Size::Full:
      DEV_ASSERT(false, "ImageCache.GetScaleFactor.FullRequiresNoResize");
      return ResizeMethod::NearestNeighbor;
      
    case ImageCache::Size::Double_NN:
    case ImageCache::Size::Half_NN:
    case ImageCache::Size::Quarter_NN:
      return ResizeMethod::NearestNeighbor;
      
    case ImageCache::Size::Double_Linear:
    case ImageCache::Size::Half_Linear:
    case ImageCache::Size::Quarter_Linear:
      return ResizeMethod::Linear;

    case ImageCache::Size::Half_AverageArea:
    case ImageCache::Size::Quarter_AverageArea:  
      return ResizeMethod::AverageArea;

  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
static void ResizeHelper(const ImageType& origImg, ImageCache::Size size,
                         ImageType& resizedImg_out)
{
  const f32 scaleFactor = GetScaleFactor(size);
  const s32 resizedNumRows = std::round((f32)origImg.GetNumRows() * scaleFactor);
  const s32 resizedNumCols = std::round((f32)origImg.GetNumCols() * scaleFactor);
  
  const ResizeMethod method = GetMethod(size);
  resizedImg_out.Allocate(resizedNumRows, resizedNumCols);
  origImg.Resize(resizedImg_out, method);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::ResizedEntry::Update(const Image& origImg, Size size)
{
  if(Size::Full == size)
  {
    // Special case: no resize needed. Note that this does not copy origImg's data.
    _gray = origImg;
  }
  else
  {
    ResizeHelper(origImg, size, _gray);
  }
  _hasValidRGB  = false;
  _hasValidGray = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::ResizedEntry::Update(const ImageRGB& origImg, Size size)
{
  if(Size::Full == size)
  {
    // Special case: no resize needed. Note that this does not copy origImg's data.
    _rgb = origImg;
  }
  else
  {
    ResizeHelper(origImg, size, _rgb);
  }
  _hasValidRGB  = true;
  _hasValidGray = false;
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
  _origNumRows = 0;
  _origNumCols = 0;
  _hasColor = false;
  _timeStamp = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageCache::Size ImageCache::GetSize(s32 scale, Vision::ResizeMethod method)
{
  Size size = Size::Full;
  
  if(scale != 1)
  {
    switch(method)
    {
      case Vision::ResizeMethod::NearestNeighbor:
      {
        switch(scale)
        {
          case 2:
            size = Size::Half_NN;
            break;
            
          case 4:
            size = Size::Quarter_NN;
            break;
            
          default:
            DEV_ASSERT(false, "ImageCache.GetSize.UnsupportedScaleNN");
            break;
        }
        break;
      }
        
      case Vision::ResizeMethod::Linear:
      {
        switch(scale)
        {
          case 2:
            size = Size::Half_Linear;
            break;
            
          case 4:
            size = Size::Quarter_Linear;
            break;
            
          default:
            PRINT_NAMED_ERROR("ImageCache.GetSize.UnsupportedScaleLinear", "");
            break;
        }
        break;
      }
        
      case Vision::ResizeMethod::AverageArea:
      {
        switch(scale)
        {
          case 2:
            size = Size::Half_AverageArea;
            break;
          
          case 4:
            size = Size::Quarter_AverageArea;
            break;
            
          default:
            PRINT_NAMED_ERROR("ImageCache.GetSize.UnsupportedScaleAverage", "");
            break;
        }
      }

      default:
        PRINT_NAMED_ERROR("ImageCache.GetSize.UnsupportedMethod", "");
        break;
    }
  }

  return size;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const kScaleFullString = "full";
const char* const kScaleHalfString = "half";
const char* const kScaleQuarterString = "quarter";
const char* const kMethodNearestString = "nearest";
const char* const kMethodLinearString = "linear";
const char* const kMethodAreaString = "average_area";
ImageCache::Size ImageCache::StringToSize(const std::string& scaleStr, const std::string& methodStr)
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
void ImageCache::ResetHelper(const ImageType& img)
{
  VERBOSE_DEBUG_PRINT(kLogChannelName, "ImageCache.ResetHelper", "Resetting with %s image at t=%ums",
                      GetColorStr(img), img.GetTimestamp());
  
  // Invalidate all cache entries (but don't necessarily free their memory, since we are likely
  // to reuse them and can potentially reuse that memory)
  for(auto & entry : _resizedVersions)
  {
    entry.second.Invalidate();
  }
  
  auto iter = _resizedVersions.find(Size::Full);
  if(iter == _resizedVersions.end())
  {
    _resizedVersions.emplace(Size::Full, ResizedEntry(img, Size::Full));
  }
  else
  {
    iter->second.Update(img, Size::Full);
  }
  
  _origNumRows = img.GetNumRows();
  _origNumCols = img.GetNumCols();
  
  _hasColor = (img.GetNumChannels() != 1);
  
  _timeStamp = img.GetTimestamp();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const Image& imgGray)
{
  ResetHelper(imgGray);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const ImageRGB& imgColor)
{
  ResetHelper(imgColor);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Image& ImageCache::GetGray(Size size, GetType* getType)
{
  GetType dummy;
  const Image& imgGray = GetImageHelper<Image>(size, (getType == nullptr ? dummy : *getType));
  DEV_ASSERT(!imgGray.IsEmpty(), "ImageCache.GetGray.EmptyImage");
  return imgGray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ImageRGB& ImageCache::GetRGB(Size size, GetType* getType)
{
  GetType dummy;
  const ImageRGB& imgRGB = GetImageHelper<ImageRGB>(size, (getType == nullptr ? dummy : *getType));
  DEV_ASSERT(!imgRGB.IsEmpty(), "ImageCache.GetRGB.EmptyImage");
  return imgRGB;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
const ImageType& ImageCache::GetImageHelper(Size size, GetType& getType)
{
  DEV_ASSERT(!_resizedVersions.empty(), "ImageCache.GetImageHelper.EmptyCache");
  
  auto iter = _resizedVersions.find(size);
  if(iter == _resizedVersions.end() || !iter->second.IsValid<void>())
  {
    // No valid entry available at this scale factor, so compute, cache, and return it.
    DEV_ASSERT(Size::Full != size, "ImageCache.GetImageHelper.NoOriginalVersion");
    
    // Start with the original image (at full size) for computing any new entry
    GetType dummy;
    const ImageType& origImg = GetImageHelper<ImageType>(Size::Full, dummy);
    
    VERBOSE_DEBUG_PRINT("ImageCache", "ImageCache.GetImageHelper.AddNewResizedEntry",
                        "Computing new resized %s image from original at t=%ums at scaleFactor=%.2f",
                        GetColorStr(origImg), origImg.GetTimestamp(), scaleFactor);
    
    if(iter == _resizedVersions.end())
    {
      // Insert a completely new entry
      auto insertion = _resizedVersions.emplace(size, ResizedEntry(origImg, size));
      DEV_ASSERT(insertion.second, "ImageCache.GetImageHelper.NewEntryNotInserted");
      getType = GetType::NewEntry;
      return insertion.first->second.Get<ImageType>();
    }
    else
    {
      // Resize directly into the existing (but invalid) entry's image
      ResizedEntry& entry = iter->second;
      entry.Update(origImg, size);
      getType = GetType::ResizeIntoExisting;
      return entry.Get<ImageType>();
    }
  }
  else
  {
    // Valid entry available at this scale factor.
    ResizedEntry& entry = iter->second;
    
    // If gray version not available, compute and cache it from color (or vice versa).
    const bool computeFromOpposite = !entry.IsValid<ImageType>();
    ImageType& img = entry.Get<ImageType>(computeFromOpposite);
    
    getType = (computeFromOpposite ? GetType::ComputeFromExisting : GetType::FullyCached);
    
    VERBOSE_DEBUG_PRINT("ImageCache", "ImageCache.GetImageHelper.UsingCached",
                        "%s %s image from existing entry at t=%ums and scaleFactor=%.2f",
                        (computeFromOpposite ? "Computing new" : "Returning existing"),
                        GetColorStr(img), img.GetTimestamp(), scaleFactor);
    
    return img;
  }
}
  
} // namespace Vision
} // namespace Anki
