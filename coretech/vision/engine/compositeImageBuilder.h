/**
* File: compositeImageBuilder.h
*
* Author: Kevin M. Karol
* Created: 1/31/2018
*
* Description: Build a composite image by combining the images specified by CompositeImageImpl
* with the quadrant definitions provided by CompositeImageLayout
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Vision_CompositeImageHelper_H__
#define __Vision_CompositeImageHelper_H__

#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/compositeImage_fwd.h"


namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImageBuilder{
public: 
  
  static void BuildCompositeImage(const Json::Value& layoutSpec,
                                  const Json::Value& implementationSpec,
                                  ImageRGBA& outImage,
                                  bool requireAllArguments = true);

  static void BuildCompositeImage(const CompositeImageLayout& layoutSpec,
                                  const CompositeImageImpl& implementationSpec,
                                  ImageRGBA& outImage,
                                  bool requireAllArguments = true);
};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImageHelper_H__
