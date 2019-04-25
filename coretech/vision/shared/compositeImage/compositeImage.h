/**
* File: compositeImage.h
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

#ifndef __Vision_CompositeImage_H__
#define __Vision_CompositeImage_H__

#include "clad/types/compositeImageTypes.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/shared/hueSatWrapper.h"
#include "coretech/vision/shared/spriteCache/spriteWrapper.h"
#include <set>

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImage {
public:

  struct Sprite {
    Sprite(const SpriteBox& spriteBox, const SpriteHandle& spriteHandle)
    : spriteBox(spriteBox)
    , spriteHandle(spriteHandle){}
    SpriteBox spriteBox;
    SpriteHandle spriteHandle;
  };

  using ImageLayer = std::vector<Sprite>; 
  using LayerMap = std::map<LayerName, ImageLayer>;
  using SpriteBoxMap = std::map<SpriteBoxName, Sprite*>;

  CompositeImage(ConstHSImageHandle faceHSImageHandle)
  : _faceHSImageHandle(faceHSImageHandle){};
 
  virtual ~CompositeImage();

  // Merges all layout/image info from other image into this image
  void MergeInImage(const CompositeImage& otherImage);
  void ClearLayerByName(const LayerName name);

  void AddImage(const SpriteBox& spriteBox, const SpriteHandle& spriteHandle);

  // Overlay the SpriteBox contents of the composite image onto the base image in layer order
  // from firstLayer to lastLayer. This allows partial rendering so that content can be deliberately
  // injected at specific layering, e.g. eyes are rendered externally between layers 5 & 6.
  // NOTE: This function assumes there is NO PRE-EXISTING CONTENT which should be preserved in the 
  // target image, and will clear out the image before rendering as necessary.
  void DrawIntoImage(ImageRGBA& baseImage,
                     const LayerName startLayer = LayerName::Layer_1,
                     const LayerName endLayer = LayerName::Layer_10) const;


private:
  void DrawSpriteIntoImage(const Sprite& sprite, ImageRGBA& baseImage, const bool firstImage) const;
  void DrawSpriteImage(const ImageRGBA& spriteImage, 
                       ImageRGBA& baseImage,
                       const Point2f& topCorner,
                       bool firstImage) const;

  // To allow sprite boxes to be rendered the color of the robot's eyes
  // store references to the static face hue/saturation images internally
  const ConstHSImageHandle _faceHSImageHandle;

  LayerMap _layerMap;
  SpriteBoxMap _spriteBoxMap;

};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImage_H__
