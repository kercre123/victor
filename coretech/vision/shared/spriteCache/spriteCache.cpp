/**
* File: spriteCache.cpp
*
* Author: Kevin M. Karol
* Created: 4/12/2018
*
* Description: Provides a uniform interface for accessing sprites defined on disk
* which can either be cached in memory, read from disk when the request is received
* or otherwise intelligently managed
*
* Copyright: Anki, Inc. 2018
*
**/


#include "coretech/vision/shared/spriteCache/spriteCache.h"


namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCache::SpriteCache(const Vision::SpritePathMap* spriteMap)
: _spriteMap(spriteMap)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCache::~SpriteCache()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteHandle SpriteCache::GetSpriteHandle(SpriteName spriteName, 
                                          const HSImageHandle& hueAndSaturation, 
                                          const std::set<CacheSpec>& cacheSpecs)
{
  std::lock_guard<std::mutex> guard(_hueSaturationMapMutex);

  auto& wrapperMap = GetHandleMapForHue(hueAndSaturation)._wrapperMap;
  // See if handle can be returned from the cache
  auto iter = wrapperMap.find(spriteName);
  if(iter != wrapperMap.end()){
    return iter->second;
  }

  // If not, create a new handle
  InternalHandle handle = std::make_shared<SpriteWrapper>(_spriteMap, spriteName);

  // Add it to the cache as appropriate
  ISpriteWrapper::ImgTypeCacheSpec typesToCache;
  for(auto& spec : cacheSpecs){
    typesToCache.grayscale = (spec == CacheSpec::CacheGrayscaleIndefinitely);
    typesToCache.rgba = (spec == CacheSpec::CacheRGBAIndefinitely);
  }
  handle->CacheSprite(typesToCache, hueAndSaturation);
  wrapperMap.emplace(spriteName, handle);

  return handle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteHandle SpriteCache::GetSpriteHandle(const std::string& fullSpritePath, 
                                          const HSImageHandle& hueAndSaturation, 
                                          const std::set<CacheSpec>& cacheSpecs)
{
  std::lock_guard<std::mutex> guard(_hueSaturationMapMutex);

  auto& filePathMap = GetHandleMapForHue(hueAndSaturation)._filePathMap;
  // See if handle can be returned from the cache
  auto iter = filePathMap.find(fullSpritePath);
  if(iter != filePathMap.end()){
    return iter->second;
  }

  // If not, create a new handle
  InternalHandle handle = std::make_shared<SpriteWrapper>(fullSpritePath);

  // Add it to the cache as appropriate
  ISpriteWrapper::ImgTypeCacheSpec typesToCache;
  for(auto& spec : cacheSpecs){
    typesToCache.grayscale = (spec == CacheSpec::CacheGrayscaleIndefinitely);
    typesToCache.rgba = (spec == CacheSpec::CacheRGBAIndefinitely);
  }
  
  handle->CacheSprite(typesToCache, hueAndSaturation);
  filePathMap.emplace(fullSpritePath, handle);

  return handle;

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
auto SpriteCache::GetHandleMapForHue(const HSImageHandle& hueAndSaturation) -> HandleMaps&
{
  const uint16_t compressedKey = hueAndSaturation != nullptr ? hueAndSaturation->GetHSID() : 0;
  auto iter = _hueSaturationMap.find(compressedKey);
  if(iter == _hueSaturationMap.end()){
    iter = _hueSaturationMap.emplace(compressedKey, HandleMaps()).first;
  }

  return iter->second;
}


} // namespace Vision
} // namespace Anki
