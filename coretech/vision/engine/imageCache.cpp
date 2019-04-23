/**
 * File: imageCache.cpp
 *
 * Author:  Andrew Stein
 * Created: 06/05/2017
 *
 * Description: Caches resized/gray/color version of an image so that different parts of the vision system
 *              can re-use already-requested sizes/colorings of an image without needing to know about each other.
 *              Thread safe
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "coretech/vision/engine/imageCache.h"

#include "coretech/common/shared/array2d.h"

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
inline const std::shared_ptr<const Image> ImageCache::ResizedEntry::Get<Image>()
{
  std::lock_guard<std::mutex> lock(_mutex);

  // Don't already have gray
  if(!_hasValidGray)
  {
    // If buffer is valid use that to get gray
    if(_buffer.HasValidData())
    {
      if(_gray == nullptr)
      {
        _gray.reset(new Image());
      }

      _hasValidGray = _buffer.GetGray(*_gray, _size);
    }

    // If we failed to get gray from buffer
    if(!_hasValidGray)
    {
      if(!_hasValidRGB)
      {
        // GetGray from buffer failed so try RGB
        // Buffer had better be valid because _rgb is not valid and we don't already have Gray
        DEV_ASSERT(_buffer.HasValidData(), "ImageCache.ResizeEntry.GetGray.NoBufferOrRGB");

        if(_rgb == nullptr)
        {
          _rgb.reset(new ImageRGB());
        }

        // ImageBuffer has more support for getting rgb so do that first
        // and then fill gray from rgb
        _hasValidRGB = _buffer.GetRGB(*_rgb, _size);
        DEV_ASSERT(_hasValidRGB, "ImageCache.ResizeEntry.GetGray.FailedToGetFromBuffer");
      }

      if(_gray == nullptr)
      {
        _gray.reset(new Image());
      }

      _rgb->FillGray(*_gray, ImageRGB::RGBToGrayMethod::GreenChannel);
      _hasValidGray = true;
    }
  }

  DEV_ASSERT(_hasValidGray, "ImageCache.ResizedEntry.GetGray.InvalidEntry");
  return _gray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
inline const std::shared_ptr<const ImageRGB> ImageCache::ResizedEntry::Get<ImageRGB>()
{
  std::lock_guard<std::mutex> lock(_mutex);

  // Don't already have RGB
  if(!_hasValidRGB)
  {
    if(_buffer.HasValidData())
    {
      if(_rgb == nullptr)
      {
        _rgb.reset(new ImageRGB());
      }

      _hasValidRGB = _buffer.GetRGB(*_rgb, _size);
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
      _rgb->SetFromGray(*_gray);
      _hasValidRGB = true;
    }
  }

  DEV_ASSERT(_hasValidRGB, "ImageCache.ResizedEntry.GetRGB.InvalidEntry");
  return _rgb;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<> inline bool ImageCache::ResizedEntry::IsValid<Image>() const
{
  std::lock_guard<std::mutex> lock(_mutex);
  return _hasValidGray;
}

template<> inline bool ImageCache::ResizedEntry::IsValid<ImageRGB>() const
{
  std::lock_guard<std::mutex> lock(_mutex);
  return _hasValidRGB;
}

template<> inline bool ImageCache::ResizedEntry::IsValid<ImageBuffer>() const
{
  std::lock_guard<std::mutex> lock(_mutex);
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
template<class ImageType>
static void ResizeHelper(const ImageType& origImg, ImageCacheSize size,
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
    origImg.Resize(resizedImg_out, ResizeMethod::Linear);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ImageCache::ResizedEntry::Update(const Image& origImg, ImageCacheSize size)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if(_gray == nullptr)
  {
    _gray.reset(new Image());
  }
  ResizeHelper(origImg, size, *_gray);
  _hasValidGray = true;
  _size = size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ImageCache::ResizedEntry::Update(const ImageRGB& origImg, ImageCacheSize size)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if(_rgb == nullptr)
  {
    _rgb.reset(new ImageRGB());
  }
  ResizeHelper(origImg, size, *_rgb);
  _hasValidRGB = true;
  _size = size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ImageCache::ResizedEntry::Update(const ImageBuffer& origImg, ImageCacheSize size)
{
  std::lock_guard<std::mutex> lock(_mutex);
  _buffer = origImg;
  _size = size;
}


// =====================================================================================================================
//                                  IMAGE CACHE
// =====================================================================================================================

ImageCache::ImageCache()
: _sensorNumRows(0)
, _sensorNumCols(0)
, _hasColor(false)
, _timeStamp(0)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::ReleaseMemory()
{
  // Protect access to _resizedVersions
  _resizedMutex.lock();
  _resizedVersions.clear();
  _resizedMutex.unlock();

  // These are all atomic variables, no need to mutex lock
  _sensorNumRows = 0;
  _sensorNumCols = 0;
  _hasColor = false;
  _timeStamp = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageCacheSize ImageCache::GetSize(s32 subsample)
{
  ImageCacheSize size = ImageCacheSize::Full;

  if(subsample != 1)
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
  }

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const kScaleFullString    = "full";
const char* const kScaleHalfString    = "half";
const char* const kScaleQuarterString = "quarter";
const char* const kScaleEighthString  = "eighth";
ImageCacheSize ImageCache::StringToSize(const std::string& scaleStr)
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

  return ImageCache::GetSize(scale);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
void ImageCache::ResetHelper(const ImageType& img)
{
  VERBOSE_DEBUG_PRINT(kLogChannelName, "ImageCache.ResetHelper", "Resetting with %s image at t=%ums",
                      GetColorStr(img), img.GetTimestamp());

  _resizedMutex.lock();

  // Invalidate all cache entries (but don't necessarily free their memory, since we are likely
  // to reuse them and can potentially reuse that memory)
  for(auto & entry : _resizedVersions)
  {
    entry.second.Invalidate();
  }

  auto iter = _resizedVersions.find(ImageCacheSize::Full);
  if(iter == _resizedVersions.end())
  {
    _resizedVersions.emplace(std::piecewise_construct,
                             std::forward_as_tuple(ImageCacheSize::Full),
                             std::forward_as_tuple(img, ImageCacheSize::Full));
  }
  else
  {
    iter->second.Update(img, ImageCacheSize::Full);
  }

  _resizedMutex.unlock();


  _sensorNumRows = img.GetNumRows();
  _sensorNumCols = img.GetNumCols();

  _hasColor = (img.GetNumChannels() != 1);

  _timeStamp = img.GetTimestamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const Image& imgGray, ResizeMethod method)
{
  _bufferMutex.lock();

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
  _buffer.SetResizeMethod(method);

  ResetHelper(_buffer);

  _bufferMutex.unlock();

  // This is basically a no-op from an image creation standpoint, will just wrap an Image around the
  // ImageBuffer's data but it will have the side effect of "caching" the image in the Sensor ResizedEntry.
  // If this is not done then the first call to get the Sensor sized gray image will think it had to
  // "ComputeFromExisting" instead of having "FullyCached" image
  _resizedMutex.lock();

  auto iter = _resizedVersions.find(ImageCacheSize::Full);
  const auto endIter = _resizedVersions.end();
  DEV_ASSERT(iter != endIter, "ImageCache.Reset.Image.ExpectingToHaveSensor");
  (void)iter->second.Get<Image>();

  _resizedMutex.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const ImageRGB& imgColor, ResizeMethod method)
{
  _bufferMutex.lock();

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
  _buffer.SetResizeMethod(method);

  ResetHelper(_buffer);

  _bufferMutex.unlock();

  // This is basically a no-op from an image creation standpoint, will just wrap an ImageRGB around the
  // ImageBuffer's data but it will have the side effect of "caching" the image in the Sensor ResizedEntry.
  // If this is not done then the first call to get the Sensor sized rgb image will think it had to
  // "ComputeFromExisting" instead of having "FullyCached" image
  _resizedMutex.lock();

  auto iter = _resizedVersions.find(ImageCacheSize::Full);
  const auto endIter = _resizedVersions.end();
  DEV_ASSERT(iter != endIter, "ImageCache.Reset.ImageRGB.ExpectingToHaveSensor");
  (void)iter->second.Get<ImageRGB>();

  _resizedMutex.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCache::Reset(const ImageBuffer& buffer)
{
  _bufferMutex.lock();

  // Note: This is a copy but is totally fine as ImageBuffer is just a wrapper around image data
  // so no images are actually copied
  _buffer = buffer;
  ResetHelper(_buffer);

  _bufferMutex.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::shared_ptr<const Image> ImageCache::GetGray(ImageCacheSize size, GetType* getType)
{
  GetType dummy;
  auto imgGray = GetImageHelper<Image>(size, (getType == nullptr ? dummy : *getType));
  DEV_ASSERT(!imgGray->IsEmpty(), "ImageCache.GetGray.EmptyImage");
  return imgGray;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::shared_ptr<const ImageRGB> ImageCache::GetRGB(ImageCacheSize size, GetType* getType)
{
  GetType dummy;
  auto imgRGB = GetImageHelper<ImageRGB>(size, (getType == nullptr ? dummy : *getType));
  DEV_ASSERT(!imgRGB->IsEmpty(), "ImageCache.GetRGB.EmptyImage");
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
const std::shared_ptr<const ImageType> ImageCache::GetImageHelper(ImageCacheSize size, GetType& getType)
{
  _resizedMutex.lock();

  DEV_ASSERT(!_resizedVersions.empty(), "ImageCache.GetImageHelper.EmptyCache");

  auto iter = _resizedVersions.find(size);
  const auto endIter = _resizedVersions.end();

  // We should compute a new entry (either "completely new" or reusing an invalidated entry at the same size) if:
  //  - there is no entry at the requested size, OR
  //  - the entry at the requested size is not valid for either ImageType, OR
  //  - there is a valid entry at the requested (non-sensor) size, but color data is being requested and the cache
  //     was originally reset with color data from which we could resize (instead of computing from gray)
  const bool shouldComputeNewValidEntry = (iter == endIter ||
                                           !iter->second.IsValid<void>() ||
                                           ((ImageCacheSize::Full != size) &&
                                            HasColor() && IsRequestingColor<ImageType>()));
  if(shouldComputeNewValidEntry)
  {
    if(iter == endIter)
    {
      // Insert a completely new entry
      _bufferMutex.lock();

      if(_buffer.HasValidData())
      {
        auto insertion = _resizedVersions.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(size),
                                                  std::forward_as_tuple(_buffer, size));
        _bufferMutex.unlock();

        DEV_ASSERT(insertion.second, "ImageCache.GetImageHelper.NewEntryNotInserted");

        const std::shared_ptr<const ImageType> img = insertion.first->second.Get<ImageType>();

        _resizedMutex.unlock();

        getType = GetType::NewEntry;

        return img;
      }
      else
      {
        _bufferMutex.unlock();

        GetType dummy;
        const auto origImg = GetImageHelper<ImageType>(ImageCacheSize::Full, dummy);

        auto insertion = _resizedVersions.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(size),
                                                  std::forward_as_tuple(*origImg, size));

        DEV_ASSERT(insertion.second, "ImageCache.GetImageHelper.NewEntryNotInserted");

        const std::shared_ptr<const ImageType> img = insertion.first->second.template Get<ImageType>();

        _resizedMutex.unlock();

        getType = (img->GetDataPointer() == origImg->GetDataPointer() ? GetType::FullyCached : GetType::NewEntry);

        return img;
      }
    }
    else
    {
      // Resize directly into the existing (but invalid) entry's image
      ResizedEntry& entry = iter->second;

      _bufferMutex.lock();

      if(_buffer.HasValidData())
      {
        entry.Update(_buffer, size);

        _bufferMutex.unlock();
      }
      else
      {
        _bufferMutex.unlock();

        GetType dummy;

        const std::shared_ptr<const ImageType> origImg = GetImageHelper<ImageType>(ImageCacheSize::Full, dummy);

        entry.Update(*origImg, size);

        _resizedMutex.lock();
      }
      getType = GetType::ResizeIntoExisting;

      auto ret = entry.Get<ImageType>();

      _resizedMutex.unlock();

      return ret;
    }
  }
  else
  {
    // Valid entry already available at this ImageCacheSize.
    ResizedEntry& entry = iter->second;

    const bool wasValid = entry.IsValid<ImageType>();

    // If the entry is already valid for the requested ImageType then it is FullyCached otherwise
    // we will be computing it from the existing entry
    getType = (wasValid ? GetType::FullyCached : GetType::ComputeFromExisting);

    const std::shared_ptr<const ImageType> img = entry.Get<ImageType>();

    _resizedMutex.unlock();

    VERBOSE_DEBUG_PRINT("ImageCache", "ImageCache.GetImageHelper.UsingCached",
                        "%s %s image from existing entry at t=%ums and scaleFactor=%.2f",
                        (entry.IsValid<ImageType>() ? "Returning existing" : "Computing new"),
                        GetColorStr(img), img.GetTimestamp(), scaleFactor);

    return img;
  }
}

bool ImageCache::GetResizeMethod(ResizeMethod& method) const
{
  std::lock_guard<std::mutex> lock(_bufferMutex);

  if(_buffer.HasValidData())
  {
    method = _buffer.GetResizeMethod();
    return true;
  }
  return false;
}

bool ImageCache::AreEntriesInUse() const
{
  _resizedMutex.lock();

  for(const auto& entry : _resizedVersions)
  {
    if(entry.second.IsInUse())
    {
      _resizedMutex.unlock();
      return true;
    }
  }

  _resizedMutex.unlock();
  return false;
}

} // namespace Vision
} // namespace Anki
