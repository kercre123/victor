/**
* File: hsImageWraper.cpp
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


#include "coretech/vision/shared/hueSatWrapper.h"
#include "coretech/vision/engine/image_impl.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HueSatWrapper::HueSatWrapper(Image* hueImg, Image* saturationImg)
: _imagesManagedExternally(true)
{
  const bool imagesAreValid = ANKI_VERIFY(!hueImg->IsEmpty() &&
                                          !saturationImg->IsEmpty(),
                                          "HueSatWrapper.Constructor.InvalidImages",
                                          "Is Hue image empty: %s, Is saturation image empty: %s",
                                          hueImg->IsEmpty() ? "Yes" : "No",
                                          saturationImg->IsEmpty() ? "Yes" : "No");
  if(imagesAreValid){
    _hue = hueImg->GetRow(0)[0];
    _saturation =  saturationImg->GetRow(0)[0];
    
    // Create a deleter that does nothing since HueSatWrapper doesn't control image memory
    auto emptyDeleter = [](Image* img){};
    _hueImg = std::shared_ptr<Image>(hueImg, emptyDeleter);
    _saturationImg = std::shared_ptr<Image>(saturationImg, emptyDeleter);

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HueSatWrapper::HueSatWrapper(uint8_t hue, uint8_t saturation, 
                               const ImageSize& imageSize)
: _hue(hue)
, _saturation(saturation)
, _imagesManagedExternally(false)
{
  if(imageSize.numCols != 0){
    Image* hueImg;
    Image* saturationImg;
    AllocateImagesInternal(hueImg, saturationImg, imageSize);
    _hueImg = std::shared_ptr<Image>(hueImg);
    _saturationImg = std::shared_ptr<Image>(saturationImg);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HueSatWrapper::~HueSatWrapper()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t HueSatWrapper::GetHSID()
{
  return (static_cast<uint16_t>(GetHue() << 8) | GetSaturation());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t HueSatWrapper::GetHue() const
{
  if(_imagesManagedExternally &&
     (_hueImg != nullptr) &&
     (_hueImg->GetNumRows() > 0) &&
     (_hueImg->GetNumCols() > 0)){
    _hue = _hueImg->GetRow(0)[0];
  }
  return _hue;

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t HueSatWrapper::GetSaturation() const
{
  if(_imagesManagedExternally &&
     (_saturationImg != nullptr) &&
     (_saturationImg->GetNumRows() > 0) &&
     (_saturationImg->GetNumCols() > 0)){
    _saturation = _saturationImg->GetRow(0)[0];
  }
  return _saturation;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HueSatWrapper::AreImagesCached(const ImageSize& imageSize) const
{
  return (_hueImg != nullptr) &&
         (_hueImg->GetNumRows() == imageSize.numRows) &&
         (_hueImg->GetNumCols() == imageSize.numCols);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HueSatWrapper::AllocateImages(Image*& hueImg, Image*& saturationImg,
                                   const ImageSize& imageSize)
{
  if((_hueImg != nullptr) &&
     (_hueImg->GetNumRows() == imageSize.numRows) &&
     (_hueImg->GetNumCols() == imageSize.numCols)){
    PRINT_NAMED_WARNING("HueSatWrapper.AllocateImages.AllocatingMemoryUnnecessarily",
                        "Hue and saturation images are cached for hue %d and saturation %d, but new allocation requested",
                        _hue, _saturation);
  }
  AllocateImagesInternal(hueImg, saturationImg, imageSize);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HueSatWrapper::AllocateImagesInternal(Image*& hueImg, Image*& saturationImg,
                                           const ImageSize& imageSize)
{
  hueImg = new Image(imageSize.numRows, imageSize.numCols);
  hueImg->FillWith(_hue);
  saturationImg = new Image(imageSize.numRows, imageSize.numCols);
  saturationImg->FillWith(_saturation);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HueSatWrapper::GetCachedImages(Image*& hueImg, Image*& saturationImg, 
                                    const ImageSize& imageSize)
{
  if((_hueImg == nullptr) ||
     (_hueImg->GetNumRows() != imageSize.numRows) ||
     (_hueImg->GetNumCols() != imageSize.numCols)){
    PRINT_NAMED_ERROR("HueSatWrapper.GetCachedImages.ImagesNotCached",
                      "Called get cached images on a wrapper that doesn't have cached images");
    _hueImg = std::unique_ptr<Image>(new Image(imageSize.numRows, imageSize.numCols));
    _saturationImg = std::unique_ptr<Image>(new Image(imageSize.numRows, imageSize.numCols));
  }
  hueImg = _hueImg.get();
  saturationImg = _saturationImg.get();
}


}; // namespace Vision
}; // namespace Anki
