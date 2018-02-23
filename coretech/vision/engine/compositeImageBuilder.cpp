/**
* File: compositeImageBuilder.cpp
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


#include "coretech/vision/engine/compositeImageBuilder.h"


namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageBuilder::BuildCompositeImage(const Json::Value& layoutSpec,
                                                const Json::Value& implementationSpec,
                                                ImageRGBA& outImage,
                                                bool requireAllArguments)
{
  using namespace CompositeImageBuilderKeys;
  CompositeImageLayout layout(layoutSpec);
  CompositeImageImpl   impl(implementationSpec);

  CompositeImageBuilder::BuildCompositeImage(layout, impl, outImage, requireAllArguments);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageBuilder::BuildCompositeImage(const CompositeImageLayout& layoutSpec,
                                                const CompositeImageImpl& implementationSpec,
                                                ImageRGBA& outImage,
                                                bool requireAllArguments)
{
  const auto& implementationVector = implementationSpec.GetImplementation();
  ANKI_VERIFY(!requireAllArguments ||
              (layoutSpec.GetLayout().size() == implementationSpec.GetImplementation().size()),
              "CompositeImageBuilder.BuildCompositeImage.SizeMismatch",
              "");
  for(const auto& layoutEntry : layoutSpec.GetLayout()){
    // Find the implementation quad for each layout quad
    auto compareName = [&layoutEntry](const CompositeImageImpl::QuadrantImpl& implQuad){
      return implQuad.quadrantName == layoutEntry.quadrantName;
    };
    auto iter = std::find_if(implementationVector.begin(), implementationVector.end(), compareName);
    if(iter == implementationVector.end()){
      if(requireAllArguments){
        PRINT_NAMED_WARNING("CompositeImageBuilder.BuildCompositeImage.SpecMissingQuadrant",
                            "Quadrant name %s not found in implementation spec",
                            layoutEntry.quadrantName.c_str());
      }
      continue;
    }
    
    // If implementation quad was found, draw it into the image at the point
    // specified by the layout quad def
    const ImageRGBA& subImage = iter->image;
    Point2f topCorner(layoutEntry.topLeftCorner.x(),layoutEntry.topLeftCorner.y());
    outImage.DrawSubImage(subImage, topCorner);

    // dev only verification that image size is as expected
    ANKI_VERIFY(layoutEntry.width == subImage.GetNumCols(), 
                "CompositeImageBuilder.BuildCompositeImage.InvalidWidth",
                "Quadrant Name:%s Expected Width:%d, Image Width:%d",
                layoutEntry.quadrantName.c_str(), layoutEntry.width, subImage.GetNumCols());
    ANKI_VERIFY(layoutEntry.height == subImage.GetNumRows(), 
                "CompositeImageBuilder.BuildCompositeImage.InvalidHeight",
                "Quadrant Name:%s Expected Height:%d, Image Height:%d",
                layoutEntry.quadrantName.c_str(), layoutEntry.height, subImage.GetNumRows());
  }
}


}; // namespace Vision
}; // namespace Anki
