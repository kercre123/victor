/**
 * File: imageCompositor.cpp
 *
 * Author: Arjun Menon
 * Date:   2019-02-12
 *
 * Description: Vision system helper class for compositing images together
 * by averaging a fixed number of them, and applying contrast boost
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "engine/vision/imageCompositor.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/array2d_impl.h"

namespace Anki {
namespace Vector {

namespace {
  const char* kMaxImageCountKey = "MaxImageCount";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageCompositor::ImageCompositor(const Json::Value& config)
: _sumImage(0,0)
, _numImagesComposited(0)
{
  _maxImageCount = JsonTools::ParseUInt32(config, kMaxImageCountKey, "Vision.ImageCompositor.Ctor");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCompositor::ComposeWith(const Vision::Image& img)
{
  if(_sumImage.GetNumRows() == 0) {
    _sumImage.Allocate(img.GetNumRows(), img.GetNumCols());
    Reset();
  }

  if(!ANKI_VERIFY(_sumImage.GetNumRows()==img.GetNumRows() && _sumImage.GetNumCols()==img.GetNumCols(), 
                  "ImageCompositor.ComposeWith.DifferingImageSizes",
                  "Trying to compose images of different sizes not allowed.")) {
    return;
  }

  if(_numImagesComposited >= _maxImageCount) {
    Reset(); // zeros all elements in the sum image
  }

  typedef u32(AddPixelsFuncType)(const u32&, const u8&);
  std::function<AddPixelsFuncType> addPixels = [](const u32& accPixel, const u8& pixel) -> u32 {
    return accPixel + (u32)pixel;
  };
  _sumImage.ApplyScalarFunction(addPixels, img, _sumImage);
  _numImagesComposited++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCompositor::Reset()
{
  typedef u32(ZeroPixelFuncType)(const u32&);
  std::function<ZeroPixelFuncType> zeroPixels = [](const u32& accPixel) -> u32 {
    return 0;
  };
  _sumImage.ApplyScalarFunction(zeroPixels);
  _numImagesComposited = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::Image ImageCompositor::GetCompositeImage() const
{
  Vision::Image outImg(_sumImage.GetNumRows(), _sumImage.GetNumCols());
  typedef u8(MeanPixelFuncType)(const u32&);
  std::function<MeanPixelFuncType> meanPixel = [&](const u32& accPixel) -> u8 {
    return (u8)(accPixel/_numImagesComposited);
  };
  _sumImage.ApplyScalarFunction(meanPixel, outImg);
  return outImg;
}

} // end namespace Vector
} // end namespace Anki