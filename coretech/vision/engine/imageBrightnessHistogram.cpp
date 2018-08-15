/**
 * File: imageBrightnessHistogram.cpp
 *
 * Author: Andrew Stein
 * Date:   10/20/2016
 *
 * Description: Implements a histogram sprecifically for image brightness: 256 bins.
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "coretech/vision/engine/imageBrightnessHistogram.h"
#include "coretech/vision/engine/image_impl.h"

namespace Anki {
namespace Vision {

void ImageBrightnessHistogram::Reset()
{
  ImageBrightnessHistogram emptyHist;
  std::swap(*this, emptyHist);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageBrightnessHistogram::IncrementBin(u8 bin)
{
  ++(_counts[bin]);
  ++_totalCount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageBrightnessHistogram::IncrementBin(u8 bin, s32 byAmount)
{
  _counts[bin] += byAmount;
  _totalCount  += byAmount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImageBrightnessHistogram::FillFromImage(const Image& img, s32 subSample, TransformFcn transformFcn)
{
  if(subSample <= 0)
  {
    PRINT_NAMED_ERROR("ImageBrightnessHistogram.FillFromImage.InvalidSubSample",
                      "%d not > 0. Will use 1 instead.", subSample);
    subSample = 1;
  }
  
  s32 nrows = img.GetNumRows();
  s32 ncols = img.GetNumCols();
  if(img.IsContinuous() && subSample==1) {
    ncols *= nrows;
    nrows = 1;
  }

  if(transformFcn)
  {
    for(s32 i=0; i<nrows; i+=subSample)
    {
      const u8* img_i = img.GetRow(i);

      for(s32 j=0; j<ncols; j+=subSample)
      {
        const u8 pixelValue = transformFcn(img_i[j]);
        ++_counts[pixelValue];
      }
    }
  }
  else
  {
    for(s32 i=0; i<nrows; i+=subSample)
    {
      const u8* img_i = img.GetRow(i);

      for(s32 j=0; j<ncols; j+=subSample)
      {
        ++_counts[img_i[j]];
      }
    }
  }

  // Consider that in the loop above, we always start at row 0, and we always start at column 0
  const s32 totalCountToAdd = ((nrows + subSample - 1) / subSample) *
                              ((ncols + subSample - 1) / subSample);
  _totalCount += totalCountToAdd;

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImageBrightnessHistogram::FillFromImage(const Image& img, const Image& weights, s32 subSample, TransformFcn transformFcn)
{
  if(weights.GetNumRows() != img.GetNumRows() ||
     weights.GetNumCols() != img.GetNumCols())
  {
    PRINT_NAMED_ERROR("ImageBrightnessHistogram.FillFromImage.InvalidWeightMaskSize",
                      "Weight mask [%dx%d] not same size as image [%dx%d]",
                      weights.GetNumCols(), weights.GetNumRows(),
                      img.GetNumCols(), img.GetNumRows());
    return RESULT_FAIL;
  }
  
  if(subSample <= 0)
  {
    PRINT_NAMED_ERROR("ImageBrightnessHistogram.FillFromImage.InvalidSubSampleWithWeights",
                      "%d not > 0. Will use 1 instead.", subSample);
    subSample = 1;
  }
  
  s32 nrows = img.GetNumRows();
  s32 ncols = img.GetNumCols();
  if(img.IsContinuous() && weights.IsContinuous() && subSample==1) {
    ncols *= nrows;
    nrows = 1;
  }
  
  for(s32 i=0; i<nrows; i+=subSample)
  {
    const u8* img_i  = img.GetRow(i);
    const u8* weights_i = weights.GetRow(i);
    
    for(s32 j=0; j<ncols; j+=subSample)
    {
      const u8 weight = weights_i[j];
      if(weight > 0)
      {
        u8 pixelValue = img_i[j];
        if(transformFcn)
        {
          pixelValue = transformFcn(pixelValue);
        }
        _counts[pixelValue] += weight;
        _totalCount += weight;
      }
    }
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u8 ImageBrightnessHistogram::ComputePercentile(const f32 p) const
{
  if(_totalCount == 0)
  {
    PRINT_NAMED_WARNING("ImageBrightnessHistogram.ComputePercentile.ZeroTotalCount",
                        "Returning 0 for p=%f because histogram is empty", p);
    return 0;
  }
  
  const s32 desiredCount = std::round(CLIP(p, 0.f, 1.f) * static_cast<f32>(_totalCount));
  u8  value  = 0;
  s32 cumSum = 0;
  while(cumSum < desiredCount)
  {
    cumSum += _counts[value];
    if(value == 255) {
      break;
    }
    ++value;
  }
  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<u8> ImageBrightnessHistogram::ComputePercentiles(const std::set<f32>& p_list) const
{
  std::vector<u8> values;
  
  if(_totalCount == 0)
  {
    PRINT_NAMED_WARNING("ImageBrightnessHistogram.ComputePercentiles.ZeroTotalCount",
                        "Returning 0 for all %zu percentiles because histogram is empty",
                        p_list.size());
    values.resize(p_list.size(), 0);
  }
  else
  {
    u8  value  = 0;
    s32 cumSum = 0;
    
    for(f32 p : p_list)
    {
      const s32 desiredCount = std::round(CLIP(p, 0.f, 1.f) * static_cast<f32>(_totalCount));
      
      while(cumSum < desiredCount)
      {
        cumSum += _counts[value];
        if(value == 255)
        {
          break;
        }
        ++value;
      }
      
      values.push_back(value);
    }
  }
    
  DEV_ASSERT(values.size() == p_list.size(), "ImageBrightnessHistogram.ComputePercentile.SizeMismatch");
  
  return values;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Image ImageBrightnessHistogram::GetDisplayImage(const s32 maxCount) const
{
  Image histImg;
  
  if(maxCount <= 0)
  {
    PRINT_NAMED_WARNING("ImageBrightnessHistogram.GetDisplayImage.BadMaxCount",
                        "MaxCount should be > 0, not %d", maxCount);
    
    histImg.Allocate(1, (s32)_counts.size());
    histImg.FillWith(0);
  }
  else
  {
    histImg.Allocate((s32)_counts.size(), maxCount); // start transposed, for efficiency
    histImg.FillWith(255);
    
    // Shift is the bit-shift amount to achieve scaling the count values such that
    // a perfectly uniform distribution of counts would be 1/4 the image height
    s32 shift = std::log2f(static_cast<f32>(_totalCount*4) / static_cast<f32>(_counts.size()*maxCount));
    
    std::function<s32(s32)> shiftFcn = [](s32 in) -> s32 { return in; };
    
    if(shift > 0) {
      shiftFcn = [shift](s32 in) -> s32 { return (in >> shift); };
    }
    else if(shift < 0) {
      shift = -shift;
      shiftFcn = [shift](s32 in) -> s32 { return (in << shift); };
    }
    
    for(s32 i=0; i<_counts.size(); ++i)
    {
      const s32 count = std::min(maxCount, shiftFcn(_counts[i]));
      u8* histImg_i = histImg.GetRow(i);
      for(s32 j=0; j<(maxCount-count); ++j) {
        histImg_i[j] = 0;
      }
    }
    cv::Mat_<u8> cvImgT = histImg.get_CvMat_().t();
    histImg = Vision::Image(cvImgT);
  }
  
  return histImg;
}

} // namespace Vision
} // namespace Anki
