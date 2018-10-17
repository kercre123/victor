#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/shared/types.h"

#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/engine/imageBuffer/imageBuffer.h"
#include "coretech/vision/engine/imageCache.h"

using namespace Anki;

GTEST_TEST(ImageCache, CachedGetters)
{
  using namespace Anki::Vision;
  
  ImageCache cache;
  
  // Nothing in the cache: should fail
  ASSERT_DEATH(cache.GetRGB(), "");
  ASSERT_DEATH(cache.GetGray(), "");
  
  const s32 nrows = 16;
  const s32 ncols = 32;
  Vision::Image imgGray(nrows, ncols);
  imgGray(0,0) = 42;
  cache.Reset(imgGray);
  
  ASSERT_EQ(false, cache.HasColor());
  ASSERT_EQ(nrows, cache.GetNumRows(ImageCacheSize::Full));
  ASSERT_EQ(ncols, cache.GetNumCols(ImageCacheSize::Full));
  ASSERT_EQ(nrows/2, cache.GetNumRows(ImageCacheSize::Half));
  ASSERT_EQ(ncols/2, cache.GetNumCols(ImageCacheSize::Half));
  ASSERT_EQ(nrows/4, cache.GetNumRows(ImageCacheSize::Quarter));
  ASSERT_EQ(ncols/4, cache.GetNumCols(ImageCacheSize::Quarter));
  ASSERT_EQ(nrows/8, cache.GetNumRows(ImageCacheSize::Eighth));
  ASSERT_EQ(ncols/8, cache.GetNumCols(ImageCacheSize::Eighth));
  
  ImageCache::GetType getType;
  
  // Cached gray image should share data pointer with original
  const Vision::Image& getResult = cache.GetGray(ImageCacheSize::Full, &getType);
  ASSERT_EQ(imgGray.GetDataPointer(), getResult.GetDataPointer());
  
  // Getting sensor-size should not have required any resizing/computation
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
  
  // Compute new resized entry
  const Vision::Image& halfSize = cache.GetGray(ImageCacheSize::Half, &getType);
  ASSERT_EQ(nrows/2, halfSize.GetNumRows());
  ASSERT_EQ(ncols/2, halfSize.GetNumCols());
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
    
  // Second request should be cached
  const Vision::Image& halfSize2 = cache.GetGray(ImageCacheSize::Half, &getType);
  ASSERT_EQ(&halfSize, &halfSize2);
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
    
  // Get full-scale color version (should require computation)
  const Vision::ImageRGB& colorVersion = cache.GetRGB(ImageCacheSize::Full, &getType);
  ASSERT_EQ(nrows, colorVersion.GetNumRows());
  ASSERT_EQ(ncols, colorVersion.GetNumCols());
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  
  // Gray value should be replicated to all three color channels
  ASSERT_EQ(Vision::PixelRGB(42,42,42), colorVersion(0,0));
  
  // Getting colorized gray should not change cache's HasColor status
  ASSERT_EQ(false, cache.HasColor());
  
  // Second request for color should be cached
  const Vision::ImageRGB& colorVersion2 = cache.GetRGB(ImageCacheSize::Full, &getType);
  ASSERT_EQ(&colorVersion, &colorVersion2);
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
  
  // Request for quarter size color should create new entry
  const Vision::ImageRGB& qtrSizeColor = cache.GetRGB(ImageCacheSize::Quarter, &getType);
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
  ASSERT_EQ(nrows/4, qtrSizeColor.GetNumRows());
  ASSERT_EQ(ncols/4, qtrSizeColor.GetNumCols());
  
  // Asking for quarter size gray should compute it from the color one we just created
  const Vision::Image& qtrSizeGray = cache.GetGray(ImageCacheSize::Quarter, &getType);
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  ASSERT_EQ(nrows/4, qtrSizeGray.GetNumRows());
  ASSERT_EQ(ncols/4, qtrSizeGray.GetNumCols());
  
  // Resetting with a new image and asking for half size should resize into an existing entry and use same data
  Vision::Image newImg(nrows,ncols);
  cache.Reset(newImg);
  const Vision::Image& newHalfSize = cache.GetGray(ImageCacheSize::Half, &getType);
  ASSERT_EQ(ImageCache::GetType::ResizeIntoExisting, getType);
  ASSERT_EQ(newHalfSize.GetDataPointer(), halfSize.GetDataPointer());
  
  // Resetting with a new color image should mean asking for full-sized gray will cause a compute
  Vision::ImageRGB newColorImg(nrows,ncols);
  const Vision::PixelRGB colorPixel(5, 10, 15);
  newColorImg(0,0) = colorPixel;
  cache.Reset(newColorImg);
  ASSERT_EQ(true, cache.HasColor());
  const Vision::Image& newGray = cache.GetGray(ImageCacheSize::Full, &getType);
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  ASSERT_EQ(nrows, newGray.GetNumRows());
  ASSERT_EQ(ncols, newGray.GetNumCols());
  
  // Gray value should be (R+2G+B)
  ASSERT_EQ(colorPixel.gray(), newGray(0,0));
  
  // ReleaseMemory should clear out the cache and thus a request for an image should fail
  cache.ReleaseMemory();
  ASSERT_DEATH(cache.GetGray(), "");
  
  // Resetting with a color image and requesting a resized version should trigger a
  // new allocation and not a resize into existing memory, since we called ReleaseMemory
  cache.Reset(newColorImg);
  ASSERT_EQ(true, cache.HasColor());
  cache.GetRGB(ImageCacheSize::Half, &getType);
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
}

GTEST_TEST(ImageCache, PriorityResizeRGB)
{
  using namespace Anki::Vision;

  ImageCache cache;

  const s32 nrows = 16;
  const s32 ncols = 32;
  ImageRGB sensorImg(nrows, ncols);
  cache.Reset(sensorImg);

  // We do not want to compute color data from an existing cached gray entry at the same size
  // if there's color data to resize from instead
  ImageCache::GetType getType;
  const Image imgGray = cache.GetGray(ImageCacheSize::Half, &getType);
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);

  const ImageRGB imgRGB = cache.GetRGB(ImageCacheSize::Half, &getType);
  ASSERT_EQ(ImageCache::GetType::ResizeIntoExisting, getType);
  
  // the grayscale image should not have been invalidated by the request for RGB
  const Image imgGray2 = cache.GetGray(ImageCacheSize::Half, &getType);
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
}

GTEST_TEST(ImageCache, ImageBuffer)
{
  using namespace Anki::Vision;

  ImageCache cache;

  u8 bayerData[6][40] = {{0}};

  bayerData[2][5] = 255; //b
  bayerData[2][6] = 0;   //g
  bayerData[3][5] = 0;   //g
  bayerData[3][6] = 0;   //r

  bayerData[2][15] = 0;   //b
  bayerData[2][16] = 255; //g
  bayerData[3][15] = 255; //g
  bayerData[3][16] = 0;   //r

  bayerData[2][25] = 0;   //b
  bayerData[2][26] = 0;   //g
  bayerData[3][25] = 0;   //g
  bayerData[3][26] = 255; //r

  
  const s32 nrows = 6;
  const s32 ncols = 32;
  const TimeStamp_t t = 1;
  const s32 id = 2;
  ImageBuffer buffer(&(bayerData[0][0]), nrows, ncols, ImageEncoding::BAYER, t, id);
  buffer.SetSensorResolution(nrows, ncols);
  cache.Reset(buffer);

  ImageCache::GetType getType;
  ImageRGB imgRGB = cache.GetRGB(ImageCacheSize::Full, &getType);
  // Reseting imageCache with ImageBuffer means anything at Sensor size
  // will be computed from the existing ImageBuffer which is raw data at
  // Sensor res
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  ASSERT_EQ(imgRGB.GetNumRows(),   nrows);
  ASSERT_EQ(imgRGB.GetNumCols(),   ncols);
  ASSERT_EQ(imgRGB.GetTimestamp(), buffer.GetTimestamp());
  ASSERT_EQ(imgRGB.GetImageId(),   buffer.GetImageId());

  // Check a couple of pixels for the BAYER -> RGB conversion
  // when not downsampling
  ASSERT_EQ(imgRGB.get_CvMat_()[2][4],  PixelRGB(0, 0, 255));
  ASSERT_EQ(imgRGB.get_CvMat_()[2][13], PixelRGB(0, 255, 0));
  ASSERT_EQ(imgRGB.get_CvMat_()[3][12], PixelRGB(0, 255, 0));
  ASSERT_EQ(imgRGB.get_CvMat_()[3][21], PixelRGB(255, 0, 0));

  Image imgGray = cache.GetGray(ImageCacheSize::Full, &getType);
  // Computed from gray directly from raw data entry
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  ASSERT_EQ(imgGray.GetNumRows(),   nrows);
  ASSERT_EQ(imgGray.GetNumCols(),   ncols);
  ASSERT_EQ(imgGray.GetTimestamp(), buffer.GetTimestamp());
  ASSERT_EQ(imgGray.GetImageId(),   buffer.GetImageId());

  
  // Requesting Full should result in halved bayer data
  imgRGB = cache.GetRGB(ImageCacheSize::Half, &getType);
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
  ASSERT_EQ(imgRGB.GetNumRows(),   nrows/2);
  ASSERT_EQ(imgRGB.GetNumCols(),   ncols/2);
  ASSERT_EQ(imgRGB.GetTimestamp(), buffer.GetTimestamp());
  ASSERT_EQ(imgRGB.GetImageId(),   buffer.GetImageId());

  // Check a couple of pixels for the BAYER -> RGB conversion
  // when downsampling
  ASSERT_EQ(imgRGB.get_CvMat_()[1][2],  PixelRGB(0, 0, 255));
  ASSERT_EQ(imgRGB.get_CvMat_()[1][6],  PixelRGB(0, 255, 0));
  ASSERT_EQ(imgRGB.get_CvMat_()[1][10], PixelRGB(255, 0, 0));
}
