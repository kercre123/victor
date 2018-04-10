/** File: compositeImage.cpp
*
* Author: Kevin M. Karol
* Created: 2/16/2018
*
* Description: Defines an image with multiple named layers:
*   1) Each layer is defined by a composite image layout
*   2) Layers are drawn on top of each other in a strict priority order
*
* Copyright: Anki, Inc. 2018
*
**/


#include "coretech/vision/engine/compositeImage/compositeImage.h"
#include "coretech/vision/engine/image_impl.h"
#include "util/cladHelpers/cladEnumToStringMap.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Vision {



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage::CompositeImage(const Json::Value& layersSpec,
                               s32 imageWidth,
                               s32 imageHeight)
{
  _width = imageWidth;
  _height = imageHeight;
  for(const auto& layerSpec: layersSpec){
    CompositeImageLayer layer(layerSpec);
    AddLayer(std::move(layer));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage::CompositeImage(const LayerMap&& layers,
                               s32 imageWidth,
                               s32 imageHeight)
{
  _width   = imageWidth;
  _height  = imageHeight;
  _layerMap  = std::move(layers);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage::~CompositeImage()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImage::AddLayer(CompositeImageLayer&& layer)
{
  const auto pair = _layerMap.emplace(layer.GetLayerName(), std::move(layer));
  return pair.second;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer* CompositeImage::GetLayerByName(LayerName name)
{
  auto iter = _layerMap.find(name);
  if(iter != _layerMap.end()){
    return &iter->second;
  }else{
    return nullptr;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage CompositeImage::GetImageDiff(const CompositeImage& compImg) const
{ 
  //NOTE ON READABILITY: In all instances throughout this function:
  // .first = enum value for layername/sprite box name
  // .second = actual layer/sprite box implementation class

  LayerMap outLayerMap;
  auto& compLayerMap = compImg.GetLayerMap();
  for(const auto& layerPair : _layerMap){
    // If the layer doesn't exist in the compImg, add the whole layer to the outLayerMap
    auto compLayerIter = compLayerMap.find(layerPair.first);
    if(compLayerIter == compLayerMap.end()){
      outLayerMap.emplace(layerPair.first, layerPair.second);
      continue;
    }
    // Build a new layer that's a diff of the sprite boxes
    CompositeImageLayer::ImageMap  diffLayerPathMap;
    CompositeImageLayer::LayoutMap diffLayerLayoutMap;
    const CompositeImageLayer::ImageMap& compImageMap = compLayerIter->second.GetImageMap();
    for(const auto& imageMapPair: layerPair.second.GetImageMap()){
      // If the sprite box doesn't exist in compImg, or the images don't match, add it to the diff layer
      auto compImageIter = compImageMap.find(imageMapPair.first);
      if((compImageIter == compImageMap.end()) ||
         (compImageIter->second != imageMapPair.second)){
        // Find the image path and sprite box for the changed sprite box name
        diffLayerPathMap.emplace(imageMapPair.first, imageMapPair.second);
        auto spriteBoxIter = layerPair.second.GetLayoutMap().find(imageMapPair.first);
        if(ANKI_VERIFY(spriteBoxIter != layerPair.second.GetLayoutMap().end(),
                       "CompositeImage.GetImageDiff.MissingLayoutKey",
                       "")){
          diffLayerLayoutMap.emplace(imageMapPair.first, spriteBoxIter->second);
        }
      }
    }
    CompositeImageLayer layer(layerPair.first, std::move(diffLayerLayoutMap));
    layer.SetImageMap(std::move(diffLayerPathMap));
    outLayerMap.emplace(layerPair.first, layer);
  }
  
  return CompositeImage(std::move(outLayerMap));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageRGBA CompositeImage::RenderImage(std::set<Vision::LayerName> layersToIgnore) const
{
  ANKI_VERIFY((_height != 0) && (_width != 0),
              "CompositeImage.RenderImage.InvalidSize",
              "Attempting to render an image with height %d and width %d",
              _height, _width);
  ImageRGBA outImage(_height, _width);
  outImage.FillWith(Vision::PixelRGBA());
  OverlayImage(outImage, layersToIgnore);
  return outImage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::OverlayImage(ImageRGBA& baseImage,
                                  std::set<Vision::LayerName> layersToIgnore,
                                  const Point2i& overlayOffset) const
{
  for(const auto& layerPair : _layerMap){
    // Skip rendering any layers that should be ignored
    if(layersToIgnore.find(layerPair.first) != layersToIgnore.end()){
      continue;
    }

    auto& layoutMap = layerPair.second.GetLayoutMap();
    auto& imageMap  = layerPair.second.GetImageMap();
    for(const auto& imagePair : imageMap){
      auto layoutIter = layoutMap.find(imagePair.first);
      if(layoutIter != layoutMap.end()){
        auto& spriteBox = layoutIter->second;
        // If implementation quad was found, draw it into the image at the point
        // specified by the layout quad def
        const ImageRGBA& subImage = LoadSprite(imagePair.second);
        Point2f topCorner(spriteBox.topLeftCorner.x() + overlayOffset.x(),
                          spriteBox.topLeftCorner.y() + overlayOffset.y());
        baseImage.DrawSubImage(subImage, topCorner);

        // dev only verification that image size is as expected
        ANKI_VERIFY(spriteBox.width == subImage.GetNumCols(), 
                    "CompositeImageBuilder.BuildCompositeImage.InvalidWidth",
                    "Quadrant Name:%s Expected Width:%d, Image Width:%d",
                    SpriteBoxNameToString(spriteBox.spriteBoxName), spriteBox.width, subImage.GetNumCols());
        ANKI_VERIFY(spriteBox.height == subImage.GetNumRows(), 
                    "CompositeImageBuilder.BuildCompositeImage.InvalidHeight",
                    "Quadrant Name:%s Expected Height:%d, Image Height:%d",
                    SpriteBoxNameToString(spriteBox.spriteBoxName), spriteBox.height, subImage.GetNumRows());
      }
    } // end for(imageMap)
  } // end for(_layerMap)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageRGBA CompositeImage::LoadSprite(Vision::SpriteName spriteName) const
{
  DEV_ASSERT(_spriteMap != nullptr, "CompositeImage.AttemptingToLoadSpriteWithoutMap");
  ImageRGBA outImage;
  const bool isGrayscale = true;
  const std::string fullImagePath = _spriteMap->GetValue(spriteName);
  if(isGrayscale){
    Image grayImage;
    auto res = grayImage.Load(fullImagePath);
    ANKI_VERIFY(RESULT_OK == res,
                "CompositeImage.SpriteBoxImpl.Constructor.GrayLoadFailed",
                "Failed to load sprite %s",
                SpriteNameToString(spriteName));
    outImage = ImageRGBA(grayImage.GetNumRows(), grayImage.GetNumCols());
    outImage.SetFromGray(grayImage);
  }else{
    auto res = outImage.Load(fullImagePath);
    ANKI_VERIFY(RESULT_OK == res,
                "CompositeImage.SpriteBoxImpl.Constructor.ColorLoadFailed",
                "Failed to load sprite %s",
                SpriteNameToString(spriteName));
  }
  return outImage;
}


} // namespace Vision
} // namespace Anki
