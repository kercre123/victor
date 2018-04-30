/**
* File: hsImageWraper.h
*
* Author: Kevin M. Karol
* Created: 4/23/2018
*
* Description: Class that makes it easy to pass around and reference
* hue/saturation images without worrying about large image copies
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Vision_Shared_HueSatWrapper_H__
#define __Vision_Shared_HueSatWrapper_H__

#include "coretech/common/shared/types.h"
#include <memory>
#include <utility>

namespace Anki {
namespace Vision {

class Image;

// Thin interface to hide caching/clearing functions for handlers
class HueSatWrapper{
public:
  // Pass in pointers to externally managed hue/saturation images
  // Class does not take ownership of the associated memory or have safeguards if
  // the images are deleted while the wrapper persists
  HueSatWrapper(Image* hueImg, Image* saturationImg);

  // If an image size is passed in images will be allocated at construction time
  // Otherwise size must be passed into the allocate image function
  HueSatWrapper(uint8_t hue, uint8_t saturation, 
                 const std::pair<uint32_t, uint32_t>& imageSize = {});

  virtual ~HueSatWrapper();

  // Returns a value that can be used to uniquely identify hue/saturation pairs 
  // for maps and comparisons
  uint16_t GetHSID();
  
  uint8_t GetHue() const;
  uint8_t GetSaturation() const;

  bool AreImagesCached() const { return _hueImg != nullptr;}

  // Memory will be allocated for new hue/saturation images filled with the appropriate values
  // The pointers passed in will be set to point to these images and the caller is responsible for
  // managing the memory after the call
  void AllocateImages(Image*& hueImg, Image*& saturationImg, std::pair<uint32_t, uint32_t> imageSize);

  // Returns hue and saturation images from the internally cached values
  // No allocation cost is incurred if allocateNow was set to true in the constructor
  // Otherwise an error will print and allocation will happen at calltime
  void GetCachedImages(Image*& hueImg, Image*& saturationImg, const std::pair<uint32_t, uint32_t>& imageSize);
  

private:
  // When hue/saturation images are passed in as pointers they may be updated externally without
  // notifying the HueSatWrapper - so hue and saturation are stored as mutables so that they can
  // be updated if necessary to stay in sync with _hue/_saturation image values
  mutable uint8_t _hue = 0;
  mutable uint8_t _saturation = 0;
  // Indicates whether the image values may change due to external updates to the image
  bool _imagesManagedExternally;
  // Ptrs are effectively unique, but making them shared
  // allows custom deleters to operate as necessary
  std::shared_ptr<Image> _hueImg;
  std::shared_ptr<Image> _saturationImg;

  void AllocateImagesInternal(Image*& hueImg, Image*& saturationImg,
                              const std::pair<uint32_t, uint32_t>& imageSize);
  
  // Extracts the value of the first pixel in the image
  // If the image is empty, set the default value
  void GetValueFromImage(const Image& img, uint8_t defaultValue);
};

using HSImageHandle = std::shared_ptr<HueSatWrapper>;
using ConstHSImageHandle = std::shared_ptr<const HueSatWrapper>;

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_Shared_HueSatWrapper_H__
