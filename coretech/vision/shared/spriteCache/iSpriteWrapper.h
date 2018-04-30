/**
* File: SpriteHandle.h
*
* Author: Kevin M. Karol
* Created: 4/13/2018
*
* Description: Defines type for SpriteHandles used to reference SpriteWrappers
* managed by the SpriteCache
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Vision_Shared_SpriteCache_SpriteHandle_H__
#define __Vision_Shared_SpriteCache_SpriteHandle_H__

#include "coretech/vision/shared/hueSatWrapper.h"
#include <memory>

namespace Anki {
namespace Vision {

// forward declaration
class ImageRGBA;
class Image;

// Thin interface to hide caching/clearing functions for handlers
class ISpriteWrapper{
public:
  // struct that species which images are/should be cached
  struct ImgTypeCacheSpec {
    ImgTypeCacheSpec()
    : grayscale(false)
    , rgba(false) {}
    ImgTypeCacheSpec(bool g, bool r)
    : grayscale(g)
    , rgba(r) {}
    
    bool grayscale;
    bool rgba;
  };

  // Returns a pair of bools representing whether Grayscale/RGBA are cached respectively
  // If hsImage is passed in the bool for RGBA will represent whether the cached RGBA represents
  // the image with that hue/saturation overlaid
  // This allows functions with no knowledge of the wrapper they're working with 
  // and wheather it's cached or not to gracefully use the best available function
  virtual ImgTypeCacheSpec IsContentCached(const HSImageHandle& hsImage = {}) const = 0;
  // Returns a copy of the image - this function can be called whether
  // the image has been cached or not
  virtual ImageRGBA GetSpriteContentsRGBA(const HSImageHandle& hsImg = {}) = 0;
  virtual Image GetSpriteContentsGrayscale() = 0;
  // Returns a refernce to the image - this function should only be called
  // if the image has already been cached
  // Additionally, its return value should not be stored since the 
  // cache may be cleared
  virtual const ImageRGBA& GetCachedSpriteContentsRGBA(const HSImageHandle& hsImg = {}) = 0;
  virtual const Image& GetCachedSpriteContentsGrayscale() = 0;

  // Only valid for sprites defined from a SpriteName/Path 
  // This function should be removed when VIC-2414 is implemented
  virtual bool GetFullSpritePath(std::string& fullSpritePath) = 0;
};

using SpriteHandle = std::shared_ptr<ISpriteWrapper>;

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_Shared_SpriteCache_SpriteHandle_H__