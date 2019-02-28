/**
 * File: imageCompositor.h
 *
 * Author: Arjun Menon
 * Date:   2019-02-12
 *
 * Description: Vision system helper class for compositing images together
 * by averaging a fixed number of them, and applying contrast boost
 *
 * Copyright: Anki, Inc. 2019
 **/

#ifndef __Anki_Coretech_Vision_Engine_ImageCompositor_H__
#define __Anki_Coretech_Vision_Engine_ImageCompositor_H__

#include "coretech/common/shared/array2d.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Vision {

// forward declarations
class Image;

class ImageCompositor
{
  public:
    // Configuration specifies the maximum number of images composited together
    ImageCompositor(const Json::Value& config);

    // Takes a grayscale image and adds it to the buffer of images 
    //  with which the composite is computed
    void ComposeWith(const Vision::Image& img);

    // Users of the ImageCompositor can query how many images are composited
    //  and subsequently decide the schedule of when to perform further work
    size_t GetNumImagesComposited() const { return _numImagesComposited; }

    // Clears the buffer of images to create the composite with.
    // This is externally called by users of ImageCompositor.
    void Reset();

    // Computes the average image from the buffered images so far.
    // Additionally performs a contrast boosting adjustment.
    void GetCompositeImage(Vision::Image& outImg) const;

  private:

    // When adjusting the constrast of the average image,
    //  we set all pixels whose intensity is above the
    //  percentile (specified below) to the max brightness.
    const f32 _kPercentileForMaxIntensity;

    // Accumulator sum image for creating the average image
    Array2d<f32> _sumImage;

    u32 _numImagesComposited;

    // Timestamp of the last image incorporated into the image buffer
    TimeStamp_t _lastImageTimestamp;
};


} // end namespace Vector
} // end namespace Anki

#endif /* __Anki_Coretech_Vision_Engine_ImageCompositor_H__ */