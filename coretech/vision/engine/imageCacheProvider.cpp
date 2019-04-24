/*
 * File: imageCacheProvider.cpp
 *
 * Author: Al Chaussee
 * Created: 4/19/19
 *
 * Description: Manager of ImageCaches. Provides thread safe access to get images from caches.
 *              Designed for multiple readers, single writer
 *
 * Copyright: Anki, Inc. 2019
 */

#include "coretech/vision/engine/imageCacheProvider.h"

namespace Anki {
namespace Vision {

ImageCacheProvider::ImageCacheProvider()
{

}

ImageCacheProvider::~ImageCacheProvider()
{
  std::vector<uint32_t> cleanedIds;
  CleanCaches(cleanedIds);

  if(!cleanedIds.empty())
  {
    PRINT_NAMED_WARNING("ImageCacheProvider.Destroy.LeakingCacheImageIds",
                        "Destruction cleaned caches, imageIDs are being leaked");
  }
}

void ImageCacheProvider::Reset(const ImageBuffer& buffer, std::vector<uint32_t>& cleanedIds)
{
  // Lock for writing
  _cacheMutex.writeLock();

  // Clean old caches
  CleanCaches(cleanedIds);

  // Add a new cache and reset it with this new image
  _caches.emplace_back();
  _caches.back().Reset(buffer);

  _cacheMutex.writeUnlock();
}

void ImageCacheProvider::CleanCaches(std::vector<uint32_t>& cleanedIds)
{
  cleanedIds.clear();

  // CleanCaches is called from Reset which already locks the mutex however
  // Unit tests do directly call CleanCaches so we still need to lock the mutex
  // in those cases
  const bool alreadyLocked = _cacheMutex.isWriteLocked();

  if(!alreadyLocked)
  {
    _cacheMutex.writeLock();
  }

  // For each cache check if there it has entries that are in use
  // if not then remove the cache
  auto iter = _caches.begin();
  while(iter != _caches.end())
  {
    auto& cache = *iter;
    const bool cacheInUse = cache.AreEntriesInUse();
    if(!cacheInUse)
    {
      cleanedIds.push_back(cache.GetBuffer().GetImageId());
      cache.ReleaseMemory();
      iter = _caches.erase(iter);
    }
    else
    {
      iter++;
    }
  }

  if(!alreadyLocked)
  {
    _cacheMutex.writeUnlock();
  }
}

const std::shared_ptr<const Image> ImageCacheProvider::GetGray(ImageCacheSize size, ImageCache::GetType* getType)
{
  // Lock because we are reading from the caches
  _cacheMutex.readLock();

  if(_caches.empty())
  {
    _cacheMutex.readUnlock();
    return nullptr;
  }

  auto img = _caches.back().GetGray(size, getType);
  _cacheMutex.readUnlock();
  return img;
}

const std::shared_ptr<const ImageRGB> ImageCacheProvider::GetRGB(ImageCacheSize size, ImageCache::GetType* getType)
{
  // Lock because we are reading from the caches
  _cacheMutex.readLock();

  if(_caches.empty())
  {
    _cacheMutex.readUnlock();
    return nullptr;
  }

  auto img = _caches.back().GetRGB(size, getType);
  _cacheMutex.readUnlock();
  return img;
}


}
}
