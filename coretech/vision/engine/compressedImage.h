/**
 * File: compressedImage.h
 *
 * Author: Al Chaussee
 * Date:   11/8/2018
 *
 * Description: Class for storing a jpg compressed image along with info about the original uncompressed image
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Cozmo_Coretech_Vision_CompressedImage_H__
#define __Anki_Cozmo_Coretech_Vision_CompressedImage_H__

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include <vector>

namespace Anki {
namespace Vision {

class CompressedImage
{
public:

  // Construct a CompressedImage object without actually compressing anything
  // Call Compress to actually compress an image
  CompressedImage() = default;

  // Construct and immediately compress img at quality
  template<class PixelType>
  CompressedImage(const ImageBase<PixelType>& img, s32 quality = 50);

  ~CompressedImage() = default;

  // Compresses img at quality
  template<class PixelType>
  const std::vector<u8>& Compress(const ImageBase<PixelType>& img, s32 quality = 50);

  template<class PixelType>
  bool Decompress(ImageBase<PixelType>& img) const;

  // Display a compressed image (uncompresses it first)
  void Display(const std::string& name) const;

  // Returns the raw compressed image data, will be empty if Compress has not been called
  const std::vector<u8>& GetCompressedBuffer() const { return _compressedBuffer; }

  // Get the timestamp of the original uncompressed image
  TimeStamp_t GetTimestamp() const { return _timestamp; }

  // Get the image id of the original uncompressed image
  u32 GetImageId() const { return _imageId; }

  // Get the number of rows in the original uncompressed image
  s32 GetNumRows() const { return _numRows; }

  // Get the number of cols in the original uncompressed image
  s32 GetNumCols() const { return _numCols; }

  // Get the number of channels in the original uncompressed image
  s32 GetNumChannels() const { return _numChannels; }
  
private:

  // Store metadata about the original image before compressing it
  template<class PixelType>
  void SetMetadata(const ImageBase<PixelType>& img);

  // Reusable buffer to store original image converted to a "showable format"
  // before it is compressed
  std::vector<u8> _uncompressedBuffer;
  
  std::vector<u8> _compressedBuffer;
  
  TimeStamp_t _timestamp   = 0;
  u32         _imageId     = 0;
  s32         _numRows     = 0;
  s32         _numCols     = 0;
  s32         _numChannels = 0;
};
  
}
}

#endif
