/*
 * File: imageCacheProvider.h
 *
 * Author: Al Chaussee
 * Created: 4/19/19
 *
 * Description: Manager of ImageCaches. Provides thread safe access to get images from caches.
 *              Designed for multiple readers, single writer
 *
 * Copyright: Anki, Inc. 2019
 */

#ifndef __Anki_Vision_VisionProcessor_H__
#define __Anki_Vision_VisionProcessor_H__

#include "coretech/vision/engine/iImageProcessor_fwd.h"
#include "coretech/vision/engine/imageCache.h"

#include <list>

namespace Anki {
namespace Vision {

// Implementation of C++17 std::shared_mutex
class SharedMutex
{
public:

  SharedMutex() : _writeWaiting(false) { }
  ~SharedMutex() = default;

  void readLock()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this] { return !_writeWaiting; });
    _numReaders++;
  }

  void readUnlock()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    if(_numReaders == 0)
    {
      return;
    }

    _numReaders--;
    if(_numReaders == 0)
    {
      _cond.notify_all();
    }
  }

  void writeLock()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this] { return !_writeWaiting; });
    _writeWaiting = true;
    _cond.wait(lock, [this] { return _numReaders == 0; });
  }

  void writeUnlock()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _writeWaiting = false;
    _cond.notify_all();
  }

  bool isWriteLocked() const
  {
    return _writeWaiting;
  }

private:
  std::atomic<bool> _writeWaiting;
  uint32_t _numReaders = 0;
  std::mutex _mutex;
  std::condition_variable _cond;
};

// Container of ImageCaches that are actively in use
class ImageCacheProvider
{
public:
  ImageCacheProvider();
  ~ImageCacheProvider();

  // Reset the ICP with a new image, will clean up any unused caches
  void Reset(const ImageBuffer& buffer, std::vector<uint32_t>& cleanedIds);

  // Get the most recent image of a specific type/size
  const std::shared_ptr<const Image> GetGray(ImageCacheSize size = ImageCache::GetDefaultImageCacheSize(),
                                             ImageCache::GetType* getType = nullptr);

  const std::shared_ptr<const ImageRGB> GetRGB(ImageCacheSize size  = ImageCache::GetDefaultImageCacheSize(),
                                               ImageCache::GetType* getType = nullptr);

  uint32_t GetLatestImageId() { return _caches.back().GetBuffer().GetImageId(); }

  size_t GetNumCaches() const { return _caches.size(); }

  // Remove caches that are no longer in use
  void CleanCaches(std::vector<uint32_t>& cleanedIds);

private:

  std::list<ImageCache> _caches;

  SharedMutex _cacheMutex;
};

}
}

#endif
