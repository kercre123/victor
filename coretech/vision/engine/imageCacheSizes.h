/**
 * File: imageCacheSizes.h
 *
 * Author:  Al Chaussee
 * Created: 10/04/2018
 *
 * Description: Defines the various image sizes that are supported by ImageCache
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Anki_Cozmo_Basestation_ImageCacheSizes_H__
#define __Anki_Cozmo_Basestation_ImageCacheSizes_H__

namespace Anki {
namespace Vision {

// For requesting different sizes and resize interpolation methods
enum class ImageCacheSize : u8
{
  // Full *sensor* resolution with which this cache was Reset
  Sensor,

  // Full *processing* resolution
  // Sizes below are relative to Full
  Full,

  // Nearest Neighbor
  Half_NN,
  Quarter_NN,
  Double_NN,

  // Linear Interpolation
  Half_Linear,
  Quarter_Linear,
  Double_Linear,

  // Average-Area Downsampling
  Half_AverageArea,
  Quarter_AverageArea,

  // TODO: add other sizes/methods (e.g. cubic interpolation)
};

}
}

#endif
