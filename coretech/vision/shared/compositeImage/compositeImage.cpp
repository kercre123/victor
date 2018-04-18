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


#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "coretech/vision/engine/image_impl.h"
#include "util/cladHelpers/cladEnumToStringMap.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage::CompositeImage(SpriteCache* spriteCache,
                               const Json::Value& layersSpec,
                               s32 imageWidth,
                               s32 imageHeight)
: _spriteCache(spriteCache)
{
  _width = imageWidth;
  _height = imageHeight;
  for(const auto& layerSpec: layersSpec){
    CompositeImageLayer layer(layerSpec);
    AddLayer(std::move(layer));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImage::CompositeImage(SpriteCache* spriteCache,
                               const LayerMap&& layers,
                               s32 imageWidth,
                               s32 imageHeight)
: _spriteCache(spriteCache)
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
std::vector<CompositeImageChunk> CompositeImage::GetImageChunks() const
{
  CompositeImageChunk baseChunk;
  // Add all of the values that are constant across all chunks
  baseChunk.imageWidth  = _width;
  baseChunk.imageHeight = _height;
  baseChunk.layerMax    = _layerMap.size();

  // Iterate through all layers/sprite boxes adding one chunk per sprite box to the
  // chunks vector
  std::vector<CompositeImageChunk> chunks;
  int layerIdx = 0;
  for(auto& layerPair : _layerMap){
    // stable per layer
    baseChunk.layerName = layerPair.first;
    baseChunk.layerIndex = layerIdx;
    layerIdx++;
    baseChunk.spriteBoxMax = layerPair.second.GetLayoutMap().size();

    int spriteBoxIdx = 0;
    for(auto& spriteBoxPair : layerPair.second.GetLayoutMap()){
      baseChunk.spriteBoxIndex = spriteBoxIdx;
      spriteBoxIdx++;
      baseChunk.spriteBox = spriteBoxPair.second.Serialize();
      baseChunk.spriteName = SpriteName::Count;
      

      Vision::SpriteName spriteName;
      if(layerPair.second.GetSpriteSequenceName(spriteBoxPair.first, spriteName) &&
        Vision::IsSpriteSequence(spriteName, false)){
        baseChunk.spriteName = spriteName;
      }else{
        // perform a reverse lookup on the sprite 
        Vision::SpriteSequence seq;
        std::string fullSpritePath;
        SpriteName spriteName;
        Vision::SpriteHandle handle;
        // Get Handle -> Sprite Path -> Sprite name that maps to that path
        // Each step is reliant on the previous one's value being set
        if(layerPair.second.GetSpriteSequence(spriteBoxPair.first, seq) &&
           seq.GetFrame(0, handle) &&
           handle->GetFullSpritePath(fullSpritePath) &&
           _spriteCache->GetSpritePathMap()->GetKeyForValueConst(fullSpritePath, spriteName)){
          baseChunk.spriteName = spriteName;
        }else{
          PRINT_NAMED_ERROR("CompositeImage.GetImageChunks.SerializingInvalidCompositeImage",
                            "Currently only composite images composed solely of sprite names can be serialized");
        }
      }
      chunks.push_back(baseChunk);
    } // end for(spriteBoxPair)
  } // end for(layerPair)

  return chunks;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::ReplaceCompositeImage(const LayerMap&& layers,
                                           s32 imageWidth,
                                           s32 imageHeight)
{
  _width   = imageWidth;
  _height  = imageHeight;
  _layerMap  = std::move(layers);
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
  
  return CompositeImage(_spriteCache, std::move(outLayerMap));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageRGBA CompositeImage::RenderFrame(const u32 frameIdx, std::set<Vision::LayerName> layersToIgnore) const
{
  ANKI_VERIFY((_height != 0) && (_width != 0),
              "CompositeImage.RenderFrame.InvalidSize",
              "Attempting to render an image with height %d and width %d",
              _height, _width);
  ImageRGBA outImage(_height, _width);
  outImage.FillWith(Vision::PixelRGBA());
  OverlayImageWithFrame(outImage, frameIdx, layersToIgnore);
  return outImage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::OverlayImageWithFrame(ImageRGBA& baseImage,
                                           const u32 frameIdx,
                                           std::set<Vision::LayerName> layersToIgnore,
                                           const Point2i& overlayOffset) const
{
  auto callback = [this, &baseImage, &frameIdx, &overlayOffset, &layersToIgnore]
                     (Vision::LayerName layerName, SpriteBoxName spriteBoxName, 
                      const SpriteBox& spriteBox, const SpriteEntry& spriteEntry){
    if(layersToIgnore.find(layerName) != layersToIgnore.end()){
      return;
    }
    // If implementation quad was found, draw it into the image at the point
    // specified by the layout quad def
    Vision::SpriteHandle  handle;
    if(spriteEntry._spriteSequence.GetFrame(frameIdx, handle)){
      if(handle->IsContentCached()){
        const ImageRGBA& subImage = handle->GetCachedSpriteContentsRGBA();
        DrawSubImage(baseImage, subImage, spriteBox, overlayOffset);
      }else{
        const ImageRGBA& subImage = handle->GetSpriteContentsRGBA();
        DrawSubImage(baseImage, subImage, spriteBox, overlayOffset);
      }
    }
  };
  ProcessAllSpriteBoxes(callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::OverlayImageWithFrame(Image& baseImage,
                                           const u32 frameIdx,
                                           std::set<Vision::LayerName> layersToIgnore,
                                           const Point2i& overlayOffset) const
{
  auto callback = [this, &baseImage, &frameIdx, &overlayOffset, &layersToIgnore]
                     (Vision::LayerName layerName, SpriteBoxName spriteBoxName, 
                      const SpriteBox& spriteBox, const SpriteEntry& spriteEntry){
    if(layersToIgnore.find(layerName) != layersToIgnore.end()){
      return;
    }
    // If implementation quad was found, draw it into the image at the point
    // specified by the layout quad def
    Vision::SpriteHandle  handle;
    if(spriteEntry._spriteSequence.GetFrame(frameIdx, handle)){
      if(handle->IsContentCached()){
        const Image& subImage = handle->GetCachedSpriteContentsGrayscale();
        DrawSubImage(baseImage, subImage, spriteBox, overlayOffset);
      }else{
        const Image& subImage = handle->GetSpriteContentsGrayscale();
        DrawSubImage(baseImage, subImage, spriteBox, overlayOffset);
      }
    }
  };
  ProcessAllSpriteBoxes(callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint CompositeImage::GetFullLoopLength(){
  uint maxSequenceLength = 0;
  auto callback = [&maxSequenceLength](Vision::LayerName layerName, SpriteBoxName spriteBoxName, 
                                       const SpriteBox& spriteBox, const SpriteEntry& spriteEntry){
    const auto numFrames = spriteEntry._spriteSequence.GetNumFrames();
    maxSequenceLength = (numFrames > maxSequenceLength) ? numFrames : maxSequenceLength;
  };

  ProcessAllSpriteBoxes(callback);

  return maxSequenceLength;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImage::ProcessAllSpriteBoxes(AllSpriteBoxDataFunc processCallback) const
{
    for(const auto& layerPair : _layerMap){
      auto& layoutMap = layerPair.second.GetLayoutMap();
      auto& imageMap  = layerPair.second.GetImageMap();
      for(const auto& imagePair : imageMap){
        auto layoutIter = layoutMap.find(imagePair.first);
        if(layoutIter == layoutMap.end()){
          return;
        }

        auto& spriteBox = layoutIter->second;
        processCallback(layerPair.first, imagePair.first, spriteBox, imagePair.second);
      } // end for(imageMap)
  } // end for(_layerMap)
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename ImageType>
void CompositeImage::DrawSubImage(ImageType& baseImage, const ImageType& subImage, 
                                  const SpriteBox& spriteBox, const Point2i& overlayOffset) const
{
  Point2f topCorner(spriteBox.topLeftCorner.x() + overlayOffset.x(),
                    spriteBox.topLeftCorner.y() + overlayOffset.y());
  const bool drawBlankPixels = false;
  baseImage.DrawSubImage(subImage, topCorner, drawBlankPixels);

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


} // namespace Vision
} // namespace Anki
