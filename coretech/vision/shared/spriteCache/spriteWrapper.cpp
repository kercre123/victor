/**
* File: spriteWrapper.cpp
*
* Author: Kevin M. Karol
* Created: 4/12/2018
*
* Description: Provides an interface to access a sprite's contents
* regardless of whether it's currently in memory or needs to be read in
*
* Copyright: Anki, Inc. 2018
*
**/


#include "coretech/vision/shared/spriteCache/spriteWrapper.h"

#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/engine/image.h"
#include "util/cladHelpers/cladEnumToStringMap.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(const Util::CladEnumToStringMap<SpriteName>* spriteMap,
                             SpriteName spriteName)
: _fullSpritePath(spriteMap->GetValue(spriteName))
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(const std::string& fullSpritePath)
: _fullSpritePath(fullSpritePath)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(ImageRGBA* sprite)
{
  _spriteRGBA.reset(sprite);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(Image* sprite)
{
  _spriteGrayscale.reset(sprite);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::~SpriteWrapper()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ISpriteWrapper::ImgTypeCacheSpec SpriteWrapper::IsContentCached(const HSImageHandle& hsImage) const
{
  ISpriteWrapper::ImgTypeCacheSpec whatsCached;

  if(_spriteGrayscale != nullptr){
    whatsCached.grayscale = true;
  }

  if(_spriteRGBA != nullptr){
    if(hsImage->GetHSID() == _hsID){
      whatsCached.rgba = true;
    }
  }

  return whatsCached;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageRGBA SpriteWrapper::GetSpriteContentsRGBA(const HSImageHandle& hsImage)
{
  // Return cahed RGBA if possible
  if((_spriteRGBA != nullptr) &&
     (hsImage != nullptr) &&
     (hsImage->GetHSID() == _hsID)){
    return *_spriteRGBA;
  }
  
  // See if the value channel from the SpriteRGBA can be re-used
  if(_spriteRGBA != nullptr){
    ImageRGBA outImage;
    const Image grayImg = _spriteRGBA->ToGray();
    ApplyHS(grayImg, hsImage, outImage);
    return outImage;
  }

  // Otherwise, see if hue can be applied to cached grayscale image
  if((_spriteGrayscale != nullptr) &&
     (hsImage != nullptr) &&
     (hsImage->GetHSID() != 0)){
    ImageRGBA outImage;
    ApplyHS(*_spriteGrayscale, hsImage, outImage);
    return outImage;
  }
  

  // Last resort - load from disk and apply hue/saturation directly
  ImageRGBA outImage;
  LoadSprite(outImage, hsImage);
  return outImage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Image SpriteWrapper::GetSpriteContentsGrayscale()
{ 
  // Try returning cached memory
  if(_spriteGrayscale != nullptr){
    return *_spriteGrayscale;
  }else if(_spriteRGBA != nullptr){
    Image outImage = _spriteRGBA->ToGray();
    return outImage;
  }

  // Otherwise load from disk
  Image outImage;
  LoadSprite(outImage);
  return outImage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ImageRGBA& SpriteWrapper::GetCachedSpriteContentsRGBA(const HSImageHandle& hsImage)
{
  if((_spriteRGBA == nullptr) ||
     (hsImage->GetHSID() != _hsID)){
    PRINT_NAMED_ERROR("SpriteWrapper.GetCachedGetCachedSpriteContents.InvalidContentAccess",
                      "Access to %s was requested as a reference, but sprite is not cached",
                      _fullSpritePath.c_str());
                      
    ISpriteWrapper::ImgTypeCacheSpec typesToCache = {false, true};
    CacheSprite(typesToCache, hsImage);
  }
  return *_spriteRGBA;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Image& SpriteWrapper::GetCachedSpriteContentsGrayscale()
{
  if(_spriteGrayscale == nullptr){
    PRINT_NAMED_ERROR("SpriteWrapper.GetCachedGetCachedSpriteContents.InvalidContentAccess",
                      "Access to %s was requested as a reference, but sprite is not cached",
                      _fullSpritePath.c_str());
    ISpriteWrapper::ImgTypeCacheSpec typesToCache = {true, false};
    CacheSprite(typesToCache);
  }
  return *_spriteGrayscale;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteWrapper::GetFullSpritePath(std::string& fullSpritePath)
{
  fullSpritePath = _fullSpritePath;
  if(ANKI_DEV_CHEATS && _fullSpritePath.empty()){
    PRINT_NAMED_ERROR("SpriteWrapper.GetFullSpritePath.PathIsEmpty", 
                      "The image stored in this wrapper does not reference a sprite saved on disk");
  }
  return !_fullSpritePath.empty();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteWrapper::CacheSprite(const ImgTypeCacheSpec& typesToCache, const HSImageHandle& hsImage)
{
  // Check to see if Grayscale is already cached
  if(typesToCache.grayscale &&
    _spriteGrayscale != nullptr){
    PRINT_NAMED_WARNING("SpriteWrapper.CacheSprite.GrayscaleSpriteAlreadyCached",
                        "CacheSprite called on %s which is already cached",
                        _fullSpritePath.c_str());
  }

  // Check to see if RGBA is already cached
  const bool isRGBACached = (_spriteRGBA != nullptr) && (hsImage->GetHSID() == _hsID);
  if(typesToCache.rgba &&
     isRGBACached){
    PRINT_NAMED_WARNING("SpriteWrapper.CacheSprite.RGBASpriteAlreadyCached",
                        "CacheSprite called on %s which is already cached",
                        _fullSpritePath.c_str());
    return;
  }

  // Cache Grayscale sprite if appropriate
  if(typesToCache.grayscale &&
     _spriteGrayscale == nullptr){
    LoadSprite(*_spriteGrayscale);
  }

  // Cache RGBA sprite if appropritae
  if(typesToCache.rgba &&
     !isRGBACached){
    if((_spriteGrayscale != nullptr) &&
       (hsImage != nullptr) &&
       (hsImage->GetHSID() == 0)){
      ApplyHS(*_spriteGrayscale, hsImage, *_spriteRGBA);
    }else{
      LoadSprite(*_spriteRGBA, hsImage);
    }
  } 
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteWrapper::ClearCachedSprite()
{
  if((_spriteRGBA == nullptr) && 
     (_spriteGrayscale == nullptr)){
    PRINT_NAMED_WARNING("SpriteWrapper.ClearCachedSprite.NoSpritesCached",
                        "ClearCachedSprite called on %s which is not cached",
                        _fullSpritePath.c_str());
    return;
  }

  if(_fullSpritePath.empty()){
    PRINT_NAMED_WARNING("SpriteWrapper.ClearCachedSprite.NoSprite Path",
                        "ClearCachedSprite called on SpriteWrapper which stores image directly - sprite cannot be recovered");
  }

  _spriteRGBA.reset();
  _spriteGrayscale.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteWrapper::LoadSprite(Image& outImage) const
{
  auto res = outImage.Load(_fullSpritePath.c_str());
  ANKI_VERIFY(RESULT_OK == res,
              "CompositeImage.SpriteBoxImpl.Constructor.GrayLoadFailed",
              "Failed to load sprite %s",
              _fullSpritePath.c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteWrapper::LoadSprite(ImageRGBA& outImage, const HSImageHandle& hsImage) const
{
  if((hsImage != nullptr) &&
     hsImage->GetHSID() != 0){
    // Load the image as a grayscale image and merge it with a hue image
    Image grayImg;
    grayImg.Load(_fullSpritePath.c_str());
    ApplyHS(grayImg, hsImage, outImage);
  }else if(!_fullSpritePath.empty()){
    // Load the image in as an RGB directly 
    auto res = outImage.Load(_fullSpritePath.c_str());
    ANKI_VERIFY(RESULT_OK == res,
                "CompositeImage.SpriteBoxImpl.Constructor.ColorLoadFailed",
                "Failed to load sprite %s",
                _fullSpritePath.c_str());
  }else{
    PRINT_NAMED_ERROR("SpriteWrapper.LoadSprite.NoPathToLoadFrom", "");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteWrapper::ApplyHS(const Image& grayImg, const HSImageHandle& hsImage, ImageRGBA& outImg) const
{
  std::pair<uint32_t, uint32_t> imageSize = {grayImg.GetNumCols(), grayImg.GetNumRows()};
  Vision::Image* hueImage = nullptr;
  Vision::Image* saturationImage = nullptr;
  bool memoryAllocated = false;

  if(hsImage == nullptr){
    PRINT_NAMED_ERROR("SpriteWrapper.ApplyHS.HSImageNull",
                      "Cannot apply null HS image to grayImg");
    outImg.SetFromGray(grayImg);
    return;
  }else if(hsImage->AreImagesCached()){
    hsImage->GetCachedImages(hueImage, saturationImage, imageSize);
  }else{
    hsImage->AllocateImages(hueImage, saturationImage, imageSize);
    memoryAllocated = true;
  }

  // Create an HSV image from the gray image, replacing the 'hue' channel 
  // with the specified value
  const std::vector<cv::Mat> channels {
    hueImage->get_CvMat_(),
    saturationImage->get_CvMat_(),
    grayImg.get_CvMat_()
  };
  ImageRGB imageHSV;
  cv::merge(channels, imageHSV.get_CvMat_());
  ImageRGB565 im565;
  imageHSV.ConvertHSV2RGB565(im565);
  const bool dimensionsMatch = (outImg.GetNumRows() == grayImg.GetNumRows()) &&
                               (outImg.GetNumCols() == grayImg.GetNumCols());
  if(!dimensionsMatch){
    PRINT_NAMED_INFO("SpriteWrapper.ApplyHS.AllocatingNewRGBAImage", 
                     "Existing dimensions (%d,%d) did not match grayscale dimensions (%d,%d)",
                     outImg.GetNumCols(), outImg.GetNumRows(),
                     grayImg.GetNumCols(),  grayImg.GetNumRows());
    outImg = ImageRGBA(grayImg.GetNumRows(), grayImg.GetNumCols());
  }
  outImg.SetFromRGB565(im565);

  if(memoryAllocated){
    Util::SafeDelete(hueImage);
    Util::SafeDelete(saturationImage);
  }
}


} // namespace Vision
} // namespace Anki
