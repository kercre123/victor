/**
 * File: imageBrightnessHistogram.h
 *
 * Author: Andrew Stein
 * Date:   10/20/2016
 *
 * Description: Defines a histogram sprecifically for image brightness: 256 bins.
 *
 * Copyright: Anki, Inc. 2016
 **/


#ifndef __Anki_Vision_Basestation_ImageBrightnessHistogram_H__
#define __Anki_Vision_Basestation_ImageBrightnessHistogram_H__

#include "coretech/vision/engine/image.h"

#include <array>
#include <set>

namespace Anki {
namespace Vision {
 
  class ImageBrightnessHistogram
  {
  public:
    
    ImageBrightnessHistogram() : _counts{}, _totalCount(0) { } // Initialize to all-zero
    
    // Empty an existing, already-populated histogram
    void Reset();
    
    // Simply adjust the value of a single bin
    void IncrementBin(u8 bin);
    void IncrementBin(u8 bin, s32 byAmount);
    
    // Populate histogram with data from given image, with optional subsampling.
    // TransformFcn is applied to each pixel value before adding it to the histogram.
    // NOTE: The histogram is NOT cleared before, so you can accumulate multiple images
    //       into one histogram with repeated calls.
    using TransformFcn = std::function<u8(u8)>;
    Result FillFromImage(const Image& img, s32 subSample=1, TransformFcn transformFcn = {});
    
    // Populate histogram using weights. Each pixel from the image contributes its corresponding
    // weight to the histogram. Thus, zero-weighted pixels do not contribute. Use a binary
    // weight image to effectively do simple masking. Image and Mask must be same size.
    Result FillFromImage(const Image& img, const Image& weights, s32 subSample=1, TransformFcn transformFcn = {});
    
    s32 GetTotalCount() const { return _totalCount; }
    
    const std::array<s32,256>& GetCounts() const { return _counts; }
    
    // Compute one or more percentile values. (p should be on the interval [0,1])
    // In the case of a set of percentiles, there will be one element in the output
    // vector for each element in the input, in the same order as the input set.
    // Note that the inputs will be sorted thanks to the set, so the outputs will
    // be in the _sorted_ order, not necessarily the order you input them.
    u8 ComputePercentile(const f32 p) const;
    std::vector<u8> ComputePercentiles(const std::set<f32>& p_list) const;
    
    // Return a histogram as an image for display. MaxCount is the height of the image.
    // The width will be 256 (one column per brightness bin).
    Image GetDisplayImage(const s32 maxCount) const;
    
  private:
    
    std::array<s32,256> _counts;
    s32                 _totalCount;
    
  }; // class ImageBrightnessHistogram
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_Basestation_ImageBrightnessHistogram_H__ */
