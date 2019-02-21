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
#include <algorithm>

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
  // We don't know what image dimensions we're compositing at the time of construction.
  // This is controlled by the scale factor applied to input images prior to Marker Detection.
  // Read the dimensions from the input image, and determine the dimensions of the accumulator.
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

  #if(ANKICORETECH_USE_OPENCV)
  Array2d<f32> imgAsFloat;
  img.get_CvMat_().convertTo(imgAsFloat.get_CvMat_(), CV_32FC1);
  _sumImage += imgAsFloat;
  _numImagesComposited++;
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCompositor::Reset()
{
  _sumImage.FillWith(0.f);
  _numImagesComposited = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::Image ImageCompositor::GetCompositeImage() const
{
  Vision::Image outImg(_sumImage.GetNumRows(), _sumImage.GetNumCols());

  // A composite image is the average of the sum image, with contrast boosting
  #if(ANKICORETECH_USE_OPENCV)
  Array2d<f32> avgImage(_sumImage);
  avgImage.get_CvMat_() *= invNumImageComposited;

  // Threshold of pixel values above which to be set to Max Brightness.
  // Computed by finding the 99th percentile intensity value.
  std::vector<f32> pixels(_sumImage.get_CvMat_().begin(), _sumImage.get_CvMat_().end());
  std::sort(pixels.begin(), pixels.end());
  f32 sum99pct = pixels[ std::max((int)std::floor(pixels.size() * .99f), 0) ];

  // Note: we don't need to divide out the number of images
  //  since the arithmetic works out that numImagesComposited
  //  cancels out in the scaling factor.
  const f32 scaling = 0.9f * 255 / sum99pct;

  avgImage.get_CvMat_().convertTo(outImg.get_CvMat_(), CV_8UC1, scaling, 0);
  #endif

  return outImg;
}

} // end namespace Vector
} // end namespace Anki