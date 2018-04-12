/**
* File: compositeImageBuilder.h
*
* Author: Kevin M. Karol
* Created: 3/29/2018
*
* Description: Build a composite image out of clad chunks
* Provides a clean interface that prevents compositeImages from being used while
* chunks are still pending
*
*
* Copyright: Anki, Inc. 2018
*
**/


#include "coretech/vision/shared/compositeImage/compositeImageBuilder.h"

namespace Anki {
namespace Vision {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageBuilder::CompositeImageBuilder(const Util::CladEnumToStringMap<Vision::SpriteName>* spriteMap,
                                             const CompositeImageChunk& chunk)
: _spriteMap(spriteMap)
, _expectedNumLayers(chunk.layerMax)
, _width(chunk.imageWidth)
, _height(chunk.imageHeight)
{
  AddImageChunk(chunk);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageBuilder::~CompositeImageBuilder()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageBuilder::AddImageChunk(const CompositeImageChunk& chunk)
{
  // Find or create the layer within the builder
  auto layerIter = _layerMap.find(chunk.layerName);
  if(layerIter == _layerMap.end()){
    CompositeImageLayer::LayoutMap empty;
    CompositeImageLayer layer(chunk.layerName, std::move(empty));
    layerIter = _layerMap.emplace(chunk.layerName, std::move(layer)).first;
    _expectedSpriteBoxesPerLayer.emplace(chunk.layerName, chunk.spriteBoxMax);
  }

  const auto& layout = layerIter->second.GetLayoutMap();
  if(ANKI_VERIFY(layout.find(chunk.spriteBox.name) == layout.end(),
                 "CompositeImageBuilder.AddImageChunk.SpriteBoxAlreadySet",
                 "Sprite box %s already set for image %s",
                 SpriteBoxNameToString(chunk.spriteBox.name),
                 LayerNameToString(chunk.layerName))){
    // Add sprite box to map
    CompositeImageLayer::SpriteBox sb(chunk.spriteBox);
    layerIter->second.AddToLayout(chunk.spriteBox.name, std::move(sb));
    
    // Add image name to map
    layerIter->second.AddToImageMap(chunk.spriteBox.name, chunk.spriteName);
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageBuilder::CanBuildImage()
{
  // Ensure all layers have been received
  if(_layerMap.size() != _expectedNumLayers){
    return false;
  }

  // ensure all layers have received all sprite boxes
  for(const auto& layer: _layerMap){
    auto expectedIter = _expectedSpriteBoxesPerLayer.find(layer.first);
    if(expectedIter == _expectedSpriteBoxesPerLayer.end()){
      return false;
    }
    if(expectedIter->second != layer.second.GetLayoutMap().size()){
      return false;
    }
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageBuilder::GetCompositeImage(CompositeImage& outImage)
{
  if(CanBuildImage()){
    auto intentionalCopy = _layerMap;
    outImage.ReplaceCompositeImage(std::move(intentionalCopy), _width, _height);
    outImage.SetSpriteMap(_spriteMap);
    return true;
  }
  return false; 
}


} // namespace Vision
} // namespace Anki
