/**
 * File: templatedImageCache.h
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

#ifndef __Cozmo_Basestation_AIComponent_TemplatedImageCache_H__
#define __Cozmo_Basestation_AIComponent_TemplatedImageCache_H__

#include "coretech/vision/engine/colorPixelTypes.h"
#include "coretech/vision/engine/image.h"
#include "engine/robotDataLoader.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

  
class TemplatedImageCache : public IDependencyManagedComponent<AIComponentID>, 
                            private Util::noncopyable
{
public:
  TemplatedImageCache(const RobotDataLoader::ImagePathMap& imageMap)
  : IDependencyManagedComponent<AIComponentID>(this, AIComponentID::TemplatedImageCache)
  , _imagePathMap(imageMap){}
  // Pass in the template and a map of quadrant name to image name
  // If an image with the same name has been built and cached, only the quadrants
  // different from the cached image will be re-drawn 
  const Vision::ImageRGBA& BuildImage(const std::string& imageName,
                                      s32 imageWidth, s32 imageHeight,
                                      const Json::Value& templateSpec, 
                                      const std::map<std::string, std::string>& quadrantMap);

  // Give behaviors a way to access the imagePathMap
  const RobotDataLoader::ImagePathMap& GetImagePathMap(){ return _imagePathMap;}
private:
  struct CacheEntry{
    CacheEntry(s32 imageWidth, s32 imageHeight)
    : _image(imageHeight, imageWidth){
      _image.FillWith(Vision::PixelRGBA());
    }
    std::map<std::string, std::string> _quadrantMap;
    Vision::ImageRGBA _image;
  };

  const RobotDataLoader::ImagePathMap& _imagePathMap;
  std::map<std::string, CacheEntry> _imageCache;

  void UpdateCacheEntry(CacheEntry& cacheEntry,
                        const Json::Value& templateSpec, 
                        const std::map<std::string, std::string>& quadrantMap);

}; // class TemplatedImageCache

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_AIComponent_TemplatedImageCache_H__
