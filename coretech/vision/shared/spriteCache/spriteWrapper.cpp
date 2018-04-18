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

#include "coretech/vision/engine/image.h"
#include "util/cladHelpers/cladEnumToStringMap.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(const Util::CladEnumToStringMap<SpriteName>* spriteMap,
                             SpriteName spriteName)
: _fullSpritePath(spriteMap->GetValue(spriteName))
, _isGrayscaleSpriteSet(false)
, _spriteIsCached(false)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(const std::string& fullSpritePath)
: _fullSpritePath(fullSpritePath)
, _isGrayscaleSpriteSet(false)
, _spriteIsCached(false)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(ImageRGBA* sprite)
: _isGrayscaleSpriteSet(false)
, _spriteIsCached(true)
{
  _spriteRGBA.reset(sprite);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::SpriteWrapper(Image* sprite)
: _isGrayscaleSpriteSet(true)
, _spriteIsCached(true)
{
  _spriteGrayscale.reset(sprite);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteWrapper::~SpriteWrapper()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageRGBA SpriteWrapper::GetSpriteContentsRGBA()
{
  if(_spriteIsCached){
    if(_isGrayscaleSpriteSet){
      ImageRGBA outImage;
      outImage.SetFromGray(*_spriteGrayscale);
      return outImage;
    }else{
      return *_spriteRGBA;
    }
  }else{
    ImageRGBA outImage;
    LoadSprite(outImage);
    return outImage;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Image SpriteWrapper::GetSpriteContentsGrayscale()
{  
  if(_spriteIsCached){
    if(_isGrayscaleSpriteSet){
      return *_spriteGrayscale;
    }else{
      Image outImage = _spriteRGBA->ToGray();
      return outImage;
    }
  }else{
    Image outImage;
    LoadSprite(outImage);
    return outImage;
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ImageRGBA& SpriteWrapper::GetCachedSpriteContentsRGBA()
{
  if(!_spriteIsCached || _isGrayscaleSpriteSet){
    PRINT_NAMED_ERROR("SpriteWrapper.GetCachedGetCachedSpriteContents.InvalidContentAccess",
                      "Access to %s was requested as a reference, but sprite is not cached",
                      _fullSpritePath.c_str());
    const bool cacheGrayscale = false;
    CacheSprite(cacheGrayscale);
  }
  return *_spriteRGBA;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Image& SpriteWrapper::GetCachedSpriteContentsGrayscale()
{
  if(!_spriteIsCached || !_isGrayscaleSpriteSet){
    PRINT_NAMED_ERROR("SpriteWrapper.GetCachedGetCachedSpriteContents.InvalidContentAccess",
                      "Access to %s was requested as a reference, but sprite is not cached",
                      _fullSpritePath.c_str());
    const bool cacheGrayscale = true;
    CacheSprite(cacheGrayscale);
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
void SpriteWrapper::CacheSprite(bool cacheGrayscale)
{
  if(_spriteIsCached && (_isGrayscaleSpriteSet == cacheGrayscale)){
    PRINT_NAMED_WARNING("SpriteWrapper.CacheSprite.SpriteAlreadyCached",
                        "CacheSprite called on %s which is already cached",
                        _fullSpritePath.c_str());
    return;
  }

  if(_fullSpritePath.empty()){
    PRINT_NAMED_ERROR("SpriteWrapper.CacheSprite.NoPathToCache",
                      "CacheSprite called on sprite with empty path - can't load image");
    // Check to make sure memory is allocated even if it's total garbage - better to
    // display a grabage image than crash on a nullptr issue
    if(cacheGrayscale && (_spriteGrayscale == nullptr)){
      _spriteGrayscale.reset(new Image());
    }else if(!cacheGrayscale && (_spriteRGBA == nullptr)){
      _spriteRGBA.reset(new ImageRGBA());
    }

    return;
  }

  // Ensure we don't have extra allocated memory 
  _spriteGrayscale.reset();
  _spriteRGBA.reset();
  
  _spriteIsCached = true;

  if(cacheGrayscale){
    _isGrayscaleSpriteSet = true;
    _spriteGrayscale.reset(new Image());
    LoadSprite(*_spriteGrayscale);
  }else{
    _isGrayscaleSpriteSet = false;
    _spriteRGBA.reset(new ImageRGBA());
    LoadSprite(*_spriteRGBA);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteWrapper::ClearCachedSprite()
{
  if(!_spriteIsCached){
    PRINT_NAMED_WARNING("SpriteWrapper.ClearCachedSprite.SpriteIsNotCached",
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
  _spriteIsCached = false;
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
void SpriteWrapper::LoadSprite(ImageRGBA& outImage) const
{
  auto res = outImage.Load(_fullSpritePath.c_str());
  ANKI_VERIFY(RESULT_OK == res,
              "CompositeImage.SpriteBoxImpl.Constructor.ColorLoadFailed",
              "Failed to load sprite %s",
              _fullSpritePath.c_str());
}

} // namespace Vision
} // namespace Anki
