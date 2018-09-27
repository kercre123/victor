/**
 * File: imageBuffer.h
 *
 * Author:  Al Chaussee
 * Created: 09/19/2018
 *
 * Description: Wrapper around raw image data
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Coretech_Vision_Engine_ImageBuffer_H__
#define __Anki_Coretech_Vision_Engine_ImageBuffer_H__

#include "coretech/common/shared/types.h"

#include "clad/types/imageFormats.h"

namespace Anki {
namespace Vision {

class ImageRGB;
  
class ImageBuffer
{
public:
  ImageBuffer() {};
  ImageBuffer(u8* data, s32 numRows, s32 numCols, ImageEncoding format, TimeStamp_t timestamp, s32 id);

  // Set whether or not to downsample the image data if it is BAYER formatted
  void SetDownsampleIfBayer(bool b) { _downsampleIfBayer = b; }

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

  // Returns scale factor relating ImageCache's Sensor and Full resolution
  // for this image
  f32 GetScaleFactorFromSensorRes() const;
  
  TimeStamp_t GetTimestamp() const { return _timestamp; }

  u32 GetImageId() const { return _imageId; }

  ImageEncoding GetFormat() const { return _format; }

  // Assuming raw data is always color so converted RGB will have 3 channels
  u32 GetNumChannels() const { return 3; }

  bool HasValidData() const { return _rawData != nullptr; }

  void Invalidate() { _rawData = nullptr; _imageId = 0; _timestamp = 0; }

  // Converts raw image data to rgb
  // Returns true if conversion was successful
  // TODO: VIC-7267 Add GetGray version and be able to specify downsampling factor
  bool GetRGB(ImageRGB& rgb) const;

  // Converts YUV420sp formatted data of an image of size rows x cols
  // to RGB
  // NEON optimized
  static void ConvertYUV420spToRGB(const u8* yuv, s32 rows, s32 cols,
                                   ImageRGB& rgb);

  // Converts a Bayer BGGR 10bit image to RGB
  // Downsamples so output is half the resolution of the bayer image
  static void DownsampleBGGR10ToRGB(const u8* bayer, s32 rows, s32 cols,
                                    ImageRGB& rgb);

  // Converts a Bayer BGGR 10bit image to RGB
  // Demosaics so output is same resolution as bayer image
  static void DemosaicBGGR10ToRGB(const u8* bayer, s32 rows, s32 cols,
                                  ImageRGB& rgb);
  
private:

  // Calculates number of rows and cols a converted RGB image will have
  void GetNumRowsCols(s32& rows, s32& cols) const;
  
  u8*           _rawData           = nullptr;
  s32           _rawNumRows        = 0;
  s32           _rawNumCols        = 0;
  ImageEncoding _format            = ImageEncoding::NoneImageEncoding;
  bool          _downsampleIfBayer = false;
  u32           _imageId           = 0;
  TimeStamp_t   _timestamp         = 0;
  s32           _sensorNumRows     = 0;
  s32           _sensorNumCols     = 0;
  
};
  
}
}

#endif
