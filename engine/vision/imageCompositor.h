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

#ifndef __Anki_Cozmo_Basestation_ImageCompositor_H__
#define __Anki_Cozmo_Basestation_ImageCompositor_H__

#include "coretech/common/engine/robotTimeStamp.h"

#include "coretech/vision/engine/compressedImage.h"
#include "coretech/vision/engine/debugImageList.h"
#include "coretech/vision/engine/image.h"
#include "coretech/common/shared/array2d.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include <list>
#include <string>

namespace Anki {

// Forward declaration
namespace Vision {
class Camera;
class ImageCache;
}

namespace Vector {

// Forward declaration:
struct VisionPoseData;

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
    //  (i.e. every 4 or 8 images)
    size_t GetNumImagesComposited() const { return _numImagesComposited; }

    // Clears the buffer of images to create the composite with
    void Reset();

    Vision::Image GetCompositeImage() const;

  protected:

  private:

    // Accumulator sum image for creating the average image
    Array2d<u32> _sumImage;

    // Adding another image beyond this count will first Reset()
    //  the accumulated image so far
    u32 _maxImageCount;

    u32 _numImagesComposited;
};


} // end namespace Vector
} // end namespace Anki

#endif /* __Anki_Cozmo_Basestation_imageCompositor_H__ */