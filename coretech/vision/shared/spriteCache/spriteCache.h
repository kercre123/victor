/**
* File: spriteCache.h
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

#ifndef __Vision_Shared_SpriteCache_H__
#define __Vision_Shared_SpriteCache_H__

#include "coretech/vision/shared/spriteCache/iSpriteWrapper.h"
#include "coretech/vision/shared/spriteCache/spriteWrapper.h"
#include "coretech/vision/shared/spritePathMap.h"
#include "coretech/common/shared/types.h"

#include "util/helpers/templateHelpers.h"

#include <map>
#include <mutex>
#include <unordered_map>
#include <set>

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SpriteCache {
public:
  using ImgTypeCacheSpec = ISpriteWrapper::ImgTypeCacheSpec;

  SpriteCache(const Vision::SpritePathMap* spriteMap);
  virtual ~SpriteCache();

  const Vision::SpritePathMap* GetSpritePathMap(){ return _spritePathMap;}  

  // Returns a SpriteHandle for an independent sprite given the filename
  SpriteHandle GetSpriteHandleForNamedSprite(const std::string& spriteName, 
                                             const HSImageHandle& hueAndSaturation = {});

  // Returns a SpriteHandle for a Sprite given the full path to the sprite
  SpriteHandle GetSpriteHandleForSpritePath(const std::string& fullSpritePath,
                                            const HSImageHandle& hueAndSaturation = {});

  void Update(BaseStationTime_t currTime_nanosec);

  // Functions for caching/clearing sprites out of cache memory
  void CacheSprite(const SpriteHandle& handle,
                   ImgTypeCacheSpec cacheSpec,
                   BaseStationTime_t cacheFor_ms = 0, 
                   const HSImageHandle& hueAndSaturation = {});

  void ClearCachedSprite(SpriteHandle handle, 
                         ImgTypeCacheSpec cacheSpec, 
                         const HSImageHandle& hueAndSaturation = {});

private:
  using InternalHandle = std::shared_ptr<SpriteWrapper>;

  const Vision::SpritePathMap* _spritePathMap;
  std::mutex _hueSaturationMapMutex;

  BaseStationTime_t _lastUpdateTime_nanosec;

  using SpriteNameToHandleMap = std::unordered_map<std::string, InternalHandle>;

  std::unordered_map<uint16_t, SpriteNameToHandleMap> _hueSaturationMap;

  // Map of expiration time -> Internal Handle
  // This ordering allows the update loop to efficiently clear stale entries
  // since they are stored in time order
  std::multimap<BaseStationTime_t, InternalHandle> _cacheTimeoutMap;

  SpriteNameToHandleMap& GetHandleMapForHue(const HSImageHandle& hueAndSaturation);

  InternalHandle GetSpriteHandleInternal(const std::string& fullSpritePath, 
                                         const HSImageHandle& hueAndSaturation = {});

  // In most cases this is the equivelent of a downcast from SpriteHandle to InternalHandle
  // If the SpriteHandle doesn't exist withn the cache, it will be added as part of this function call
  InternalHandle ConvertToInternalHandle(SpriteHandle handle,
                                         const HSImageHandle& hueAndSaturation = {});

};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_Shared_SpriteCache_H__
