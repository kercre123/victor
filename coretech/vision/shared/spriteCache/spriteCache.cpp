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

#include "util/helpers/boundedWhile.h"
#include "util/math/math.h"

namespace Anki {
namespace Vision {

namespace{
}

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
                                          const HSImageHandle& hueAndSaturation)
{
  return GetSpriteHandleInternal(spriteName, hueAndSaturation);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteHandle SpriteCache::GetSpriteHandle(const std::string& fullSpritePath, 
                                          const HSImageHandle& hueAndSaturation)
{
  return GetSpriteHandleInternal(fullSpritePath, hueAndSaturation);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCache::InternalHandle SpriteCache::GetSpriteHandleInternal(SpriteName spriteName, 
                                                                 const HSImageHandle& hueAndSaturation)
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
  wrapperMap.emplace(spriteName, handle);
  return handle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCache::InternalHandle SpriteCache::GetSpriteHandleInternal(const std::string& fullSpritePath, 
                                                                 const HSImageHandle& hueAndSaturation)
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
  filePathMap.emplace(fullSpritePath, handle);

  return handle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCache::InternalHandle SpriteCache::ConvertToInternalHandle(SpriteHandle handle,
                                                                 const HSImageHandle& hueAndSaturation)
{
  InternalHandle internalHandle;
  {
    std::lock_guard<std::mutex> guard(_hueSaturationMapMutex);
    auto& handleMap = GetHandleMapForHue(hueAndSaturation);

    for(auto& pair : handleMap._wrapperMap){
      if(pair.second.get() == handle.get()){
        internalHandle = pair.second;
        break;
      }
    }

    if(internalHandle == nullptr){
      for(auto& pair : handleMap._filePathMap){
        if(pair.second.get() == handle.get()){
          internalHandle = pair.second;
          break;
        }
      }
    }
  } // guard falls out of scope to allow call to GetSpriteHandleInternal
  
  
  if(internalHandle == nullptr){
    std::string fullSpritePath;
    if(handle->GetFullSpritePath(fullSpritePath)){
      internalHandle = GetSpriteHandleInternal(fullSpritePath, hueAndSaturation);
    }
  }
  

  return internalHandle;
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteCache::Update(BaseStationTime_t currTime_nanosec)
{
  _lastUpdateTime_nanosec = currTime_nanosec;

  auto iter = _cacheTimeoutMap.begin();
  const auto upperBound = _cacheTimeoutMap.size() + 1;
  BOUNDED_WHILE(upperBound, iter != _cacheTimeoutMap.end()){
    if(iter->first != 0){
      if(iter->first < _lastUpdateTime_nanosec){
        iter->second->ClearCachedSprite();
        iter = _cacheTimeoutMap.erase(iter);
      }else{
        break;
      }
    }else{
      ++iter;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteCache::CacheSprite(const SpriteHandle& handle, 
                              ImgTypeCacheSpec cacheSpec,
                              BaseStationTime_t cacheFor_ms, 
                              const HSImageHandle& hueAndSaturation)
{
  InternalHandle internalHandle = ConvertToInternalHandle(handle, hueAndSaturation);
  
  if(internalHandle != nullptr){
    internalHandle->CacheSprite(cacheSpec, hueAndSaturation);
    const auto expire_ns = _lastUpdateTime_nanosec + Util::MilliSecToNanoSec(cacheFor_ms);
    _cacheTimeoutMap.emplace(expire_ns, internalHandle);
  }else{
    PRINT_NAMED_WARNING("SpriteCache.CacheSprite.UnableToFindSpriteToCache", "");
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteCache::ClearCachedSprite(SpriteHandle handle, 
                                    ImgTypeCacheSpec cacheSpec, 
                                    const HSImageHandle& hueAndSaturation)
{
  InternalHandle internalHandle = ConvertToInternalHandle(handle, hueAndSaturation);

  for(auto iter = _cacheTimeoutMap.begin(); iter != _cacheTimeoutMap.end(); ++iter){
    if(iter->second == internalHandle){
      internalHandle->ClearCachedSprite();
      _cacheTimeoutMap.erase(iter);
      break;
    }
  }
}


} // namespace Vision
} // namespace Anki
