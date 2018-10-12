/**
 * File: imageBuffer.h
 *
 * Author:  Al Chaussee
 * Created: 09/19/2018
 *
 * Description: Wrapper around raw image data
 *              Safe to copy as no image data will actually be copied
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Coretech_Vision_Engine_ImageBuffer_H__
#define __Anki_Coretech_Vision_Engine_ImageBuffer_H__

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageCacheSizes.h"
#include "clad/types/imageFormats.h"

namespace Anki {
namespace Vision {

class ImageRGB;
  
class ImageBuffer
{
public:
  ImageBuffer() {};

  // Expects data to be at size ImageCacheSize::Sensor for all formats except for RawRGB.
  // Sim is the only thing capturing images in RawRGB format and it does so at 640x360 (Full) so
  // internal functions that deal with RawRGB data need to account for this
  ImageBuffer(u8* data, s32 numRows, s32 numCols, ImageEncoding format, TimeStamp_t timestamp, s32 id);

  void SetImageId(u32 id) { _imageId = id; }

  // Set timestamp at which the raw image data was captured
  void SetTimestamp(u32 timestamp) { _timestamp = timestamp; }

  // Set the original resolution of the image sensor
  // Basically max resolution an image can be from this sensor
  void SetSensorResolution(s32 rows, s32 cols) { _sensorNumRows = rows; _sensorNumCols = cols; }

  // Returns number of rows a converted RGB image will have
  s32 GetNumRows() const;

  // Returns number of cols a converted RGB image will have
  s32 GetNumCols() const;

  TimeStamp_t GetTimestamp() const { return _timestamp; }

  u32 GetImageId() const { return _imageId; }

  ImageEncoding GetFormat() const { return _format; }

  // Depending on the format of the raw data there may either be 1 or 3 channels
  u32 GetNumChannels() const { return (_format == ImageEncoding::RawGray ? 1 : 3); }

  // Returns whether or not the buffer has valid data
  bool HasValidData() const { return _rawData != nullptr; }

  const u8* GetDataPointer() const { return _rawData; }

  void Invalidate() { _rawData = nullptr; _imageId = 0; _timestamp = 0; }

  // Converts raw image data to rgb
  // Returns true if conversion was successful
  // The specific ResizeMethod is ignored so for "Half_NN" we only care about
  // the "Half" and ignore the "NN"
  bool GetRGB(ImageRGB& rgb, ImageCacheSize size, ResizeMethod method) const;

  // Converts raw image data to gray
  // Returns true if conversion was successful
  // The specific ResizeMethod is ignored so for "Half_NN" we only care about
  // the "Half" and ignore the "NN"
  bool GetGray(Image& gray, ImageCacheSize size, ResizeMethod method) const;

private:

  // Calculates number of rows and cols a converted RGB image will have
  void GetNumRowsCols(s32& rows, s32& cols) const;

  // Get RGB image at size from various formats
  bool GetRGBFromBAYER(ImageRGB& rgb, ImageCacheSize size, ResizeMethod method) const;
  bool GetRGBFromRawRGB(ImageRGB& rgb, ImageCacheSize size, ResizeMethod method) const;
  bool GetRGBFromYUV420sp(ImageRGB& rgb, ImageCacheSize size, ResizeMethod method) const;
  bool GetRGBFromRawGray(ImageRGB& rgb, ImageCacheSize size, ResizeMethod method) const;

  
  // Get Gray image at size from various formats
  bool GetGrayFromBAYER(Image& gray, ImageCacheSize size, ResizeMethod method) const;
  bool GetGrayFromRawRGB(Image& gray, ImageCacheSize size, ResizeMethod method) const;
  bool GetGrayFromYUV420sp(Image& gray, ImageCacheSize size, ResizeMethod method) const;
  bool GetGrayFromRawGray(Image& gray, ImageCacheSize size, ResizeMethod method) const;

  u8*           _rawData           = nullptr;
  s32           _rawNumRows        = 0;
  s32           _rawNumCols        = 0;
  ImageEncoding _format            = ImageEncoding::NoneImageEncoding;
  u32           _imageId           = 0;
  TimeStamp_t   _timestamp         = 0;
  s32           _sensorNumRows     = 0;
  s32           _sensorNumCols     = 0;
  
};
  
}
}

#endif
