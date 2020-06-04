/**
* File: compositeImage.cpp
*
* Author: Sam Russell 
* Created: 2/16/2018
* Refactor: 4/23/2019
*
* Description: Defines an image as an aggregate of `SpriteBox`es
*   1) Defines How SpriteBoxes should be rendered to a final image
*   2) Layers are drawn on top of each other in a strict priority order
*
* Copyright: Anki, Inc. 2019
*
**/

#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/engine/image.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "CompositeImage"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage::~CompositeImage()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::MergeInImage(const CompositeImage& otherImage)
{
  for(const auto& layerIter : otherImage._layerMap){
    const auto& layer = layerIter.second;
    for(const auto& sprite : layer){
      AddImage(sprite.spriteBox, sprite.spriteHandle);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::ClearLayerByName(LayerName name)
{
  const auto numRemoved = _layerMap.erase(name);
  if(numRemoved == 0){
    LOG_WARNING("CompositeImage.ClearLayerByName.LayerNotFound",
                "Layer %s not found in composite image",
                LayerNameToString(name));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::AddImage(const SpriteBox& spriteBox, const SpriteHandle& spriteHandle)
{
  const auto& iter = _spriteBoxMap.find(spriteBox.name);
  if(_spriteBoxMap.end() == iter){
    // New, unique SpriteBox. Add layer and add to layer as appropriate
    const auto& layerEmplacePair = _layerMap.emplace(spriteBox.layer, ImageLayer());
    auto& layer = layerEmplacePair.first->second;
    layer.emplace_back(spriteBox, spriteHandle);
  } else {
    // Duplicate SpriteBox. Notify and replace.
    LOG_WARNING("CompositeImage.AddImage.SpriteBoxOverwritten",
                "SpriteBoxes must be unique within a CompositeImage. Overwriting prior SpriteBox named: %s",
                EnumToString(spriteBox.name));
    (*_spriteBoxMap[spriteBox.name]).spriteBox = spriteBox;
    (*_spriteBoxMap[spriteBox.name]).spriteHandle = spriteHandle;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::DrawIntoImage(ImageRGBA& baseImage, const LayerName firstLayer, const LayerName lastLayer) const
{
  ANKI_CPU_PROFILE("CompositeImage::DrawIntoImage");

  if(firstLayer > lastLayer){
    LOG_ERROR("CompositeImage.DrawIntoImage.InvalidLayers",
              "Requested firstLayer %s is drawn after reqested lastLayer %s",
              EnumToString(firstLayer),
              EnumToString(lastLayer));
    return;
  }

  bool firstImage = true;
  for(const auto& layerPair : _layerMap){
    if(layerPair.first < firstLayer){
      continue;
    }
    for(const auto& sprite : layerPair.second){
      DrawSpriteIntoImage(sprite, baseImage, firstImage);
      firstImage = false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::DrawSpriteIntoImage(const Sprite& sprite, ImageRGBA& baseImage, const bool firstImage) const
{
  Point2f topCorner = {static_cast<float>(sprite.spriteBox.xPos), static_cast<float>(sprite.spriteBox.yPos)};

  switch(sprite.spriteBox.renderMethod){
    case SpriteRenderMethod::RGBA:
    {
      // Check to see if the RGBA image is cached
      if(sprite.spriteHandle->IsContentCached().rgba){
        const ImageRGBA& spriteImage = sprite.spriteHandle->GetCachedSpriteContentsRGBA();
        DrawSpriteImage(spriteImage, baseImage, topCorner, firstImage);
      }else{
        const ImageRGBA& spriteImage = sprite.spriteHandle->GetSpriteContentsRGBA();
        DrawSpriteImage(spriteImage, baseImage, topCorner, firstImage);
      }
      break;
    }
    case SpriteRenderMethod::EyeColor:
    {
      Vision::HueSatWrapper::ImageSize imageSize(static_cast<uint32_t>(sprite.spriteBox.height),
                                                 static_cast<uint32_t>(sprite.spriteBox.width));
      std::shared_ptr<Vision::HueSatWrapper> hsImageHandle;
      
      if(nullptr == _faceHSImageHandle){
        LOG_ERROR("CompositeImage.DrawSpriteIntoImage.ShouldRenderInEyeHueButCant",
                  "HS Image handle missing - image will be renderd with 0,0 hue saturation");
      }
      
        // TODO: Kevin K. Copy is happening here due to way we can resize image handles with cached data
        // do something better
      auto hue = _faceHSImageHandle->GetHue();
      auto sat = _faceHSImageHandle->GetSaturation();
      hsImageHandle = std::make_shared<Vision::HueSatWrapper>(hue,
                                                              sat,
                                                              imageSize);
      
      // Render the sprite - use the cached RGBA image if possible
      if(sprite.spriteHandle->IsContentCached(hsImageHandle).rgba){
        const ImageRGBA& spriteImage = sprite.spriteHandle->GetCachedSpriteContentsRGBA(hsImageHandle);
        DrawSpriteImage(spriteImage, baseImage, topCorner, firstImage);
      }else{
        const ImageRGBA& spriteImage = sprite.spriteHandle->GetSpriteContentsRGBA(hsImageHandle);
        DrawSpriteImage(spriteImage, baseImage, topCorner, firstImage);
      }
      break;
    }
    default:
    {
      LOG_ERROR("CompositeImage.DrawSpriteIntoImage.InvalidRenderMethod",
                "Sprite Box %s does not have a valid render method",
                SpriteBoxNameToString(sprite.spriteBox.name));
      break;
    }
  } // end switch
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::DrawSpriteImage(const ImageRGBA& spriteImage,
                                     ImageRGBA& baseImage,
                                     const Point2f& topCorner,
                                     bool firstImage) const
{
  // if the first layer rendered is going to be full-screen and draw all pixels, then there's
  // no need to clear the buffer (with FillWith) since all of those blank pixels will be
  // immediately overwritten. This saves about 1/5 ms when those conditions are true.
  if( firstImage &&
      ( (spriteImage.GetNumRows() != baseImage.GetNumRows()) ||
        (spriteImage.GetNumCols() != baseImage.GetNumCols()) ) ){
    ANKI_CPU_PROFILE("img->FillWith"); // This takes roughly 0.205 ms on robot.
    baseImage.FillWith(Vision::PixelRGBA());
  }

  // Always use the faster 'draw blank pixels' rendering on the first
  // image, since it's always drawn over the initial blank image
  baseImage.DrawSubImage(spriteImage, topCorner, firstImage);
}

} // namespace Vision
} // namespace Anki
