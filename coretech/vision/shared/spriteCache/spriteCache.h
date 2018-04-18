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

#include "clad/types/spriteNames.h"
#include "util/helpers/templateHelpers.h"

#include <unordered_map>
#include <set>

namespace Anki {
// Forward declaration
namespace Util{
template<class CladEnum>
class CladEnumToStringMap;
}

namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SpriteCache {
public:
  // Simple enum specifier for how to cache the sprite - currently it's binary
  // but in the future functionality can be added along the lines of "Cache for x seconds"
  enum class CacheSpec{
    CacheGrayscaleIndefinitely,
    CacheRGBAIndefinitely,
    DoNotCache
  };

  SpriteCache(const Vision::SpritePathMap* spriteMap);
  virtual ~SpriteCache();

  const Vision::SpritePathMap* GetSpritePathMap(){ return _spriteMap;}  
  SpriteHandle GetSpriteHandle(SpriteName spriteName, const std::set<CacheSpec>& cacheSpecs = {});
  SpriteHandle GetSpriteHandle(const std::string& fullSpritePath, const std::set<CacheSpec>& cacheSpecs = {});

private:
  using InternalHandle = std::shared_ptr<SpriteWrapper>;
  std::mutex _spriteNameMutex;
  std::mutex _pathMutex;

  const Vision::SpritePathMap* _spriteMap;
  std::unordered_map<SpriteName, InternalHandle,  Util::EnumHasher> _wrapperMap;
  std::unordered_map<std::string, InternalHandle> _filePathMap;


};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_Shared_SpriteCache_H__
