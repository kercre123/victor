/**
 * File: compressedImage.cpp
 *
 * Author: Al Chaussee
 * Date:   11/8/2018
 *
 * Description: Class for storing a jpg compressed image along with info about the original uncompressed image
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/engine/compressedImage.h"

#include "coretech/vision/engine/image_impl.h"

#include "util/helpers/templateHelpers.h"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

namespace Anki {
namespace Vision {

// Template specializations for RGB and Gray images
template<>
CompressedImage::CompressedImage(const ImageBase<PixelRGB>& img, s32 quality)
{
  Compress(img, quality);
}

template<>
CompressedImage::CompressedImage(const ImageBase<u8>& img, s32 quality)
{
  Compress(img, quality);
}

template<class PixelType>
void CompressedImage::SetMetadata(const ImageBase<PixelType>& img)
{
  _timestamp   = img.GetTimestamp();
  _imageId     = img.GetImageId();
  _numRows     = img.GetNumRows();
  _numCols     = img.GetNumCols();
  _numChannels = img.GetNumChannels();
}

template<class PixelType>
const std::vector<u8>& CompressedImage::Compress(const ImageBase<PixelType>& img, s32 quality)
{
  _compressedBuffer.clear();

  SetMetadata(img);

  _uncompressedBuffer.resize(img.GetNumElements() * img.GetNumChannels());
  cv::Mat_<PixelType> mat(img.GetNumRows(),
                          img.GetNumCols(),
                          reinterpret_cast<PixelType*>(_uncompressedBuffer.data()));

  // Convert to BGR so that imencode works
  img.ConvertToShowableFormat(mat);

  const std::vector<int> compressionParams = {CV_IMWRITE_JPEG_QUALITY, quality};

  cv::imencode(".jpg", mat, _compressedBuffer, compressionParams);

  return _compressedBuffer;
}

template<>
bool CompressedImage::Decompress(ImageBase<PixelRGB>& img) const
{
  if(GetNumChannels() != 3)
  {
    PRINT_NAMED_WARNING("CompressedImage.RGBDecompress.NotRGB",
                        "Trying to decompress to rgb but original image was not rgb");
    return false;
  }

  auto mat = cv::imdecode(_compressedBuffer, CV_LOAD_IMAGE_COLOR);
  img.SetFromShowableFormat(mat);
  
  return true;
}

template<>
bool CompressedImage::Decompress(ImageBase<u8>& img) const
{
  if(GetNumChannels() != 1)
  {
    PRINT_NAMED_WARNING("CompressedImage.GrayDecompress.NotGray",
                        "Trying to decompress to gray but original image was not gray");
    return false;
  }

  auto mat = cv::imdecode(_compressedBuffer, CV_LOAD_IMAGE_GRAYSCALE);
  img.SetFromShowableFormat(mat);
  
  return true;
}


void CompressedImage::Display(const std::string& name) const
{
  auto img = cv::imdecode(GetCompressedBuffer(),
                          (GetNumChannels() > 1 ? CV_LOAD_IMAGE_COLOR : CV_LOAD_IMAGE_GRAYSCALE));
  cv::imshow(name.c_str(), img);
  cv::waitKey(5);
}

}
}

