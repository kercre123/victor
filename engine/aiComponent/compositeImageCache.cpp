/**
 * File: templatedImageCache.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/2/18
 *
 * Description: Caches the images it builds based off of a template and then
 * only re-draws the quadrants that have changed for the next request.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/compositeImageCache.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/vision/engine/compositeImage/compositeImage.h"


namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::ImageRGBA& CompositeImageCache::BuildImage(const std::string& imageName,
                                                         s32 imageWidth, s32 imageHeight,
                                                         const Vision::CompositeImage& image)
{
  auto iter = _imageCache.find(imageName);
  if(iter == _imageCache.end()){
    iter = _imageCache.insert(std::make_pair(imageName, CacheEntry(imageWidth, imageHeight))).first;
  }

  UpdateCacheEntry(iter->second, image);
  return iter->second.preAllocatedImage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageCache::UpdateCacheEntry(CacheEntry& cacheEntry,
                                           const Vision::CompositeImage& image)
{
  // Only render the image diff
  Vision::CompositeImage imageDiff = image.GetImageDiff(cacheEntry.compositeImage);
  cacheEntry.compositeImage = image;
  
  // TMP hack - assume the image paths are relative for face images - iterate over image and update
  // relative paths to full paths
  for(auto& layerPair: imageDiff.GetLayerMap()){
    auto intententionalCopy = layerPair.second.GetImageMap();
    for(auto& spriteBoxPair: intententionalCopy){
      spriteBoxPair.second = _imagePathMap.find(spriteBoxPair.second)->second;
    }
    layerPair.second.SetImageMap(std::move(intententionalCopy));
  }
  
  imageDiff.OverlayImage(cacheEntry.preAllocatedImage);
  // Save the full new image as the base of the cache
}

} // namespace Cozmo
} // namespace Anki
