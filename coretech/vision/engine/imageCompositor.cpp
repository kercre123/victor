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

#include "coretech/vision/engine/imageCompositor.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageBrightnessHistogram.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/array2d_impl.h"
#include "util/console/consoleInterface.h"
#include <algorithm>
#include <limits>
#include <string>

namespace Anki {
namespace Vision {

namespace {
  const char* kPercentileForMaxIntensityKey = "percentileForMaxIntensity";
  const std::string debugName = "Vision.ImageCompositor";

  CONSOLE_VAR(u32, kImageHistogramSubsample, "Vision.ImageCompositor", 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageCompositor::ImageCompositor(const Json::Value& config)
: _kPercentileForMaxIntensity(JsonTools::ParseFloat(config, kPercentileForMaxIntensityKey, debugName + ".Ctor"))
, _sumImage(0,0)
, _numImagesComposited(0)
{
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

  Array2d<f32> imgAsFloat;
  img.get_CvMat_().convertTo(imgAsFloat.get_CvMat_(), CV_32FC1);
  _sumImage += imgAsFloat;
  _numImagesComposited++;
  _lastImageTimestamp = img.GetTimestamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCompositor::Reset()
{
  _sumImage.FillWith(0.f);
  _numImagesComposited = 0;
  _lastImageTimestamp = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageCompositor::GetCompositeImage(Vision::Image& outImg) const
{
  // A composite image is the average of the sum image, with contrast boosting
  
  // Computes the average image, as a grayscale
  const f32 averageFactor = 1.f / GetNumImagesComposited();
  _sumImage.get_CvMat_().convertTo(outImg.get_CvMat_(), CV_8UC1, averageFactor, 0);

  // Rescale the pixels in the specified percentile to be maximum brightness
  ImageBrightnessHistogram hist;
  hist.FillFromImage(outImg, kImageHistogramSubsample);
  const u8 brightIntensityVal = hist.ComputePercentile(_kPercentileForMaxIntensity);
  const f32 scalingFactor = ((f32)std::numeric_limits<u8>::max()) / brightIntensityVal;
  outImg.get_CvMat_().convertTo(outImg.get_CvMat_(), CV_8UC1, scalingFactor, 0);

  outImg.SetTimestamp(_lastImageTimestamp);
}

} // end namespace Vector
} // end namespace Anki