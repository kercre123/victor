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


#include "coretech/vision/engine/compositeImage/compositeImageLayer.h"

#include "clad/types/compositeImageTypes.h"
#include "coretech/vision/engine/image.h"
#include <set>

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImage {
public:
  using LayerMap = std::map<Vision::LayerName, CompositeImageLayer>;
  CompositeImage(){};
  CompositeImage(const Json::Value& layersSpec,
                 s32 imageWidth = 0,
                 s32 imageHeight = 0);

  CompositeImage(const LayerMap&& layers,
                 s32 imageWidth = 0,
                 s32 imageHeight = 0);

  virtual ~CompositeImage();

  // Returns true if layer added successfully
  // Returns false if the layer name already exists
  bool AddLayer(CompositeImageLayer&& layer);

  const LayerMap& GetLayerMap() const { return _layerMap;}
  LayerMap& GetLayerMap()             { return _layerMap;}
  // Returns a pointer to the layer within the composite image
  // Returns nullptr if layer by that name does not exist
  CompositeImageLayer* GetLayerByName(LayerName name);
  
  // Returns an image that consists of all implementation differences
  // between this image and the one passed in - layouts are assumed to be the same
  // NOTE: The images/quadrants returned are the ones which are:
  //   1) Present in this image and not in the compImg
  //   2) The image paths internal to this image different from the compImg
  // i.e. no image paths present in compImg will be returned
  CompositeImage GetImageDiff(const CompositeImage& compImg) const;

  // Render the composite image to a newly allocated image
  // Any layers specified in layersToIgnore will not be rendered
  ImageRGBA RenderImage(std::set<Vision::LayerName> layersToIgnore = {}) const;
  // Overlay the composite image on top of the base image
  // The overlay offset will shift the composite image rendered relative to the base images (0,0)
  // Any layers specified in layersToIgnore will not be rendered
  void OverlayImage(ImageRGBA& baseImage,
                    std::set<Vision::LayerName> layersToIgnore = {},
                    const Point2i& overlayOffset = {}) const;

private:
  s32 _width = 0;
  s32 _height = 0;
  LayerMap  _layerMap;

  ImageRGBA LoadImageFromPath(const std::string& imagePath) const;

};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImage_H__
