/**
* File: spriteWrapper.h
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

#ifndef __Vision_Shared_SpriteWrapper_H__
#define __Vision_Shared_SpriteWrapper_H__

#include "coretech/vision/shared/spriteCache/iSpriteWrapper.h"

#include <string>

namespace Anki {
namespace Vision {

// forward declaration
class ImageRGBA;
class Image;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SpriteWrapper : public ISpriteWrapper {
public:
  SpriteWrapper(const std::string& fullSpritePath);
  
  // Transfers ownership of the ptr to the SpriteWrapper
  SpriteWrapper(ImageRGBA* sprite);
  SpriteWrapper(Image* sprite);

  virtual ~SpriteWrapper();

  // Pair represents Grayscale/RGBA
  virtual ImgTypeCacheSpec IsContentCached(const HSImageHandle& hsImage) const override;

  virtual ImageRGBA GetSpriteContentsRGBA(const HSImageHandle& hsImage) override;
  virtual Image GetSpriteContentsGrayscale() override;
  virtual const ImageRGBA& GetCachedSpriteContentsRGBA(const HSImageHandle& hsImage) override;
  virtual const Image& GetCachedSpriteContentsGrayscale() override;

  virtual bool GetFullSpritePath(std::string& fullSpritePath) override;

  // cacheGrayscale defines what combination of Grayscale/RGBA to load into memory
  void CacheSprite(const ImgTypeCacheSpec& typesToCache = {false, false}, const HSImageHandle& hsImage = {});
  void ClearCachedSprite();

private:
  bool ImageMatchesStoredID(const HSImageHandle& hsImage) const;
  
  void LoadSprite(Image* outImage) const;
  void LoadSprite(ImageRGBA* outImage, const HSImageHandle& hsImage) const;
  void ApplyHS(const Image& grayImg, const HSImageHandle& hsImage, ImageRGBA* outImg) const;

  const std::string _fullSpritePath;
  // Keep track of what hue/saturation have been applied to the image if appropriate
  uint16_t _hsID = 0;

  std::unique_ptr<ImageRGBA> _spriteRGBA;
  std::unique_ptr<Image> _spriteGrayscale;



};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_Shared_SpriteWrapper_H__
