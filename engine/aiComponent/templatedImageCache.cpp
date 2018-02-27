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


#include "engine/aiComponent/templatedImageCache.h"
#include "coretech/vision/engine/compositeImageBuilder.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::ImageRGBA& TemplatedImageCache::BuildImage(const std::string& imageName,
                                                         s32 imageWidth, s32 imageHeight,
                                                         const Json::Value& templateSpec, 
                                                         const std::map<std::string, std::string>& quadrantMap)
{
  auto iter = _imageCache.find(imageName);
  if(iter == _imageCache.end()){
    iter = _imageCache.insert(std::make_pair(imageName, CacheEntry(imageWidth, imageHeight))).first;
  }

  UpdateCacheEntry(iter->second, templateSpec, quadrantMap);
  return iter->second._image;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TemplatedImageCache::UpdateCacheEntry(CacheEntry& cacheEntry,
                                           const Json::Value& templateSpec, 
                                           const std::map<std::string, std::string>& quadrantMap)
{
  Json::Value diffTemplate;
  Json::Value specTemplate;
  int diffIdx = 0;
  const std::string cacheDebugStr = "TemplatedImageCache.UpdateCacheEntry.TemplateKey";
  // Iterate through the templateSpec and construct a diff of the quadrants that should be re-drawn
  for(auto& entry: templateSpec){
    const std::string qName = JsonTools::ParseString(entry, Vision::CompositeImageBuilderKeys::kQuadrantNameKey, cacheDebugStr);
    auto newQuadIter = quadrantMap.find(qName);
    if(newQuadIter == quadrantMap.end()){
      PRINT_NAMED_INFO("TemplatedImageCache.UpdateCacheEntry.MissingQuadrantName",
                       "Quadrant %s missing from new quadrant map",
                       qName.c_str());
      continue;
    }

    // check if the name of the image in the quadrant has changed
    auto cacheQuadIter = cacheEntry._quadrantMap.find(qName);
    if((cacheQuadIter == cacheEntry._quadrantMap.end()) ||
       (cacheQuadIter->second != newQuadIter->second)){
      // To make it easier for the time being grab the full path based on just the image name
      // this might need to be changed when we have a better idea of how to refer to PNGs in engine
      // or ripped out totally if this work moves to the anim process
      diffTemplate[diffIdx] = entry;
      Json::Value specEntry;
      specEntry[Vision::CompositeImageBuilderKeys::kQuadrantNameKey] = qName;
      specEntry[Vision::CompositeImageBuilderKeys::kImagePathKey] = _imagePathMap.find(newQuadIter->second)->second;

      specTemplate[diffIdx] = specEntry;
      cacheEntry._quadrantMap[qName] = newQuadIter->second;
      diffIdx++;
    }
  }
  // use the diff template
  Vision::CompositeImageBuilder::BuildCompositeImage(diffTemplate,
                                                     specTemplate,
                                                     cacheEntry._image,
                                                     false);
}

} // namespace Cozmo
} // namespace Anki
