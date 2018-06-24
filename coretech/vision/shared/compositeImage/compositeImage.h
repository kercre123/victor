/**
* File: compositeImage.h
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

#ifndef __Vision_CompositeImage_H__
#define __Vision_CompositeImage_H__


#include "coretech/vision/shared/compositeImage/compositeImageLayer.h"

#include "clad/types/compositeImageTypes.h"
#include "clad/types/spriteNames.h"
#include "coretech/vision/engine/image.h"
#include <set>

namespace Anki {
// Forward declaration
namespace Util{
template<class CladEnum>
class CladEnumToStringMap;
}

namespace Vision {

class SpriteCache;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImage {
public:
  using LayerLayoutMap = std::map<Vision::LayerName, CompositeImageLayer>;
  using LayerImageMap = std::map<Vision::LayerName, CompositeImageLayer::ImageMap>;
  CompositeImage(SpriteCache* spriteCache,   
                 ConstHSImageHandle faceHSImageHandle,
                 s32 imageWidth = 0,
                 s32 imageHeight = 0)
  : _spriteCache(spriteCache)
  , _faceHSImageHandle(faceHSImageHandle)
  , _width(imageWidth)
  , _height(imageHeight){};
  
  CompositeImage(SpriteCache* spriteCache,
                 ConstHSImageHandle faceHSImageHandle,
                 const Json::Value& layersSpec,
                 s32 imageWidth = 0,
                 s32 imageHeight = 0);

  CompositeImage(SpriteCache* spriteCache,
                 ConstHSImageHandle faceHSImageHandle,
                 const LayerLayoutMap&& layers,
                 s32 imageWidth = 0,
                 s32 imageHeight = 0);
            
  // Create a composite image with the spriteHandle as a static background
  CompositeImage(ConstHSImageHandle faceHSImageHandle,
                 Vision::SpriteHandle spriteHandle,
                 bool shouldRenderRGBA = true,
                 const Point2i& topLeftCorner = {0,0});

  // Create a composite image with the sprite sequence as a background
  CompositeImage(ConstHSImageHandle faceHSImageHandle,
                 const Vision::SpriteSequence* const spriteSeq,
                 bool shouldRenderRGBA = true,
                 const Point2i& topLeftCorner = {0,0});
  
  virtual ~CompositeImage();
  
  bool operator ==(const CompositeImage& other) const{
    return (_spriteCache == other._spriteCache) &&
           (_width == other._width) &&
           (_height == other._height) &&
           (_layerMap == other._layerMap);
  }
  

  std::vector<CompositeImageChunk> GetImageChunks(bool emptySpriteBoxesAreValid = false) const;

  // Clear out the existing image and replace it with the new layer map 
  void ReplaceCompositeImage(const LayerLayoutMap&& layers,
                             s32 imageWidth = 0,
                             s32 imageHeight = 0);

  // Merges all layout/image info from other image into this image
  void MergeInImage(const CompositeImage& otherImage);
                             
  // Returns true if layer added successfully
  // Returns false if the layer name already exists
  bool AddLayer(CompositeImageLayer&& layer);
  void ClearLayerByName(LayerName name);

  const LayerLayoutMap& GetLayerLayoutMap() const { return _layerMap;}
  LayerLayoutMap& GetLayerLayoutMap()             { return _layerMap;}
  // Returns a pointer to the layer within the composite image
  // Returns nullptr if layer by that name does not exist
  CompositeImageLayer* GetLayerByName(LayerName name);

  // Render the composite image to a newly allocated image
  // Any layers specified in layersToIgnore will not be rendered
  ImageRGBA RenderFrame(const u32 frameIdx = 0,
                        std::set<Vision::LayerName> layersToIgnore = {}) const;
  // Overlay the composite image on top of the base image
  // The overlay offset will shift the composite image rendered relative to the base images (0,0)
  // Any layers specified in layersToIgnore will not be rendered
  void OverlayImageWithFrame(ImageRGBA& baseImage,
                             const u32 frameIdx = 0,
                             std::set<Vision::LayerName> layersToIgnore = {},
                             const Point2i& overlayOffset = {}) const;
  
  // Returns the length of the longest subsequence
  uint GetFullLoopLength();
  
  s32 GetWidth(){ return _width;}
  s32 GetHeight(){ return _height;}
  
  // Update all sprite boxes to use the new render method
  void OverrideRenderMethod(Anki::Vision::SpriteRenderMethod renderMethod);

  // The composite image should cache all of its sprites for the time specifed
  void CacheInternalSprites(Vision::SpriteCache* cache, const TimeStamp_t endTime_ms);


  // Utility function which adds the "empty layer" to the image - useful for instances
  // where a blank composite image is needed at the start of an animation so that updates
  // can be applied at a non-zero time
  void AddEmptyLayer(SpriteSequenceContainer* seqContainer, 
                     Vision::LayerName layerName = Vision::LayerName::EmptyBoxLayer);

private:
  using SpriteBox = CompositeImageLayer::SpriteBox;
  using SpriteEntry = CompositeImageLayer::SpriteEntry;
  
  using UseSpriteBoxDataFunc = std::function<void(Vision::LayerName,
                                                  SpriteBoxName, const SpriteBox&, 
                                                  const SpriteEntry&)>;
  using UpdateSpriteBoxDataFunc = std::function<void(Vision::LayerName,
                                                     SpriteBoxName, SpriteBox&,
                                                     SpriteEntry&)>;
  // Call the callback with the data from every spritebox moving from lowest z-index to highest
  // Data is const so can only be used for making decisions
  void ProcessAllSpriteBoxes(UseSpriteBoxDataFunc processCallback) const;
  // Call the callback with the data from every spritebox moving from lowest z-index to highest
  // Data is non-const so it can be updated (e.g. update sprite box render method
  void UpdateAllSpriteBoxes(UpdateSpriteBoxDataFunc updateCallback);
  
  template<typename ImageType>
  void DrawSubImage(ImageType& baseImage, const ImageType& subImage, 
                    const SpriteBox& spriteBox, const Point2i& overlayOffset) const;
  
  // Translates SpriteRenderConfig into a usable format for composite images
  // Returns nullptr if should render RGBA with no image handle
  // Othrewise, returns HSImage to use in render
  HSImageHandle HowToRenderRGBA(const SpriteRenderConfig& config) const;

  SpriteCache* _spriteCache = nullptr;
  // To allow sprite boxes to be rendered the color of the robot's eyes
  // store references to the static face hue/saturation images internally
  const ConstHSImageHandle _faceHSImageHandle;

  s32 _width = 0;
  s32 _height = 0;
  LayerLayoutMap  _layerMap;

};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImage_H__
