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
SpriteHandle SpriteCache::GetSpriteHandle(SpriteName spriteName, const std::set<CacheSpec>& cacheSpecs)
{
  std::lock_guard<std::mutex> guard(_spriteNameMutex);
  // See if handle can be returned from the cache
  auto iter = _wrapperMap.find(spriteName);
  if(iter != _wrapperMap.end()){
    return iter->second;
  }

  // If not, create a new handle
  InternalHandle handle = std::make_shared<SpriteWrapper>(_spriteMap, spriteName);

  // Add it to the cache as appropriate
  for(auto& spec : cacheSpecs){
    const bool cacheGrayscale = (spec == CacheSpec::CacheGrayscaleIndefinitely);
    handle->CacheSprite(cacheGrayscale);
    _wrapperMap.emplace(spriteName, handle);
  }

  return handle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteHandle SpriteCache::GetSpriteHandle(const std::string& fullSpritePath, const std::set<CacheSpec>& cacheSpecs)
{
  std::lock_guard<std::mutex> guard(_pathMutex);
  // See if handle can be returned from the cache
  auto iter = _filePathMap.find(fullSpritePath);
  if(iter != _filePathMap.end()){
    return iter->second;
  }

  // If not, create a new handle
  InternalHandle handle = std::make_shared<SpriteWrapper>(fullSpritePath);

  // Add it to the cache as appropriate
  for(auto& spec : cacheSpecs){
    const bool cacheGrayscale = (spec == CacheSpec::CacheGrayscaleIndefinitely);
    handle->CacheSprite(cacheGrayscale);
    _filePathMap.emplace(fullSpritePath, handle);
  }

  return handle;

}



} // namespace Vision
} // namespace Anki
