/**
* File: compositeImage_fwd.cpp
*
* Author: Kevin M. Karol
* Created: 1/31/2018
*
* Description: Defines the building blocks of CompositeImages which
* are images built from a bunch of sub images loaded into "quadrants" within the composite image
*
* Copyright: Anki, Inc. 2018
*
**/

#include "coretech/vision/engine/compositeImage_fwd.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayout::CompositeImageLayout(const Json::Value& templateSpec)
{
  using namespace CompositeImageBuilderKeys;

  std::vector<QuadrantDefinition> templateVector;
  const std::string templateDebugStr = "CompositeImageBuilder.BuildCompositeImage.TemplateKey";
  for(auto& entry: templateSpec){
    const std::string qName = JsonTools::ParseString(entry, kQuadrantNameKey, templateDebugStr);
    const int x = JsonTools::ParseUInt32(entry, kCornerXKey, templateDebugStr);
    const int y = JsonTools::ParseUInt32(entry, kCornerYKey, templateDebugStr);
    const int width = JsonTools::ParseUInt32(entry, kWidthKey, templateDebugStr);
    const int height = JsonTools::ParseUInt32(entry, kHeightKey, templateDebugStr);
    _quadrants.emplace_back(QuadrantDefinition(qName, Point2i(x, y), width, height));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayout::IsValidImplementation(const CompositeImageImpl& impl, bool requireAllQudrants)
{
  if(requireAllQudrants){
    // just check size here - checks below will fail if quadrant names don't match
    if(!ANKI_VERIFY(_quadrants.size() == impl.GetImplementation().size(),
                    "CompositeImageLayout.IsValidImplementation.AllQuadrantsNotSpecified",
                    "Layout has %zu quadrants, but implementation only has %zu",
                    _quadrants.size(), 
                    impl.GetImplementation().size())){
      return false;
    }
  }
  


  for(const auto& implQuad: impl.GetImplementation()){
    auto compareName = [&implQuad](const QuadrantDefinition& quad){
      return quad.quadrantName == implQuad.quadrantName;
    };
    auto iter = std::find_if(_quadrants.begin(), _quadrants.end(), compareName);
    if(iter == _quadrants.end()){
      PRINT_NAMED_WARNING("CompositeImageLayout.IsValidImplementation.QuadrantNameMismatch",
                          "Implementation has quadrant named %s which is not present in layout",
                          implQuad.quadrantName.c_str());
      return false;
    }
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageImpl::CompositeImageImpl(const Json::Value& implSpec)
{
  using namespace CompositeImageBuilderKeys;

  std::map<std::string, QuadrantImpl> implementationMap;
  const bool isGreyscale = true;
  const std::string implDebugStr = "CompositeImageBuilder.BuildCompositeImage.SpecKey";
  for(auto& entry: implSpec){
    const std::string qName     = JsonTools::ParseString(entry, kQuadrantNameKey, implDebugStr);
    const std::string imagePath = JsonTools::ParseString(entry, kImagePathKey, implDebugStr);
    _quadrants.emplace_back(QuadrantImpl(qName, imagePath, isGreyscale));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageImpl::QuadrantImpl::QuadrantImpl(const std::string& quadrantName, const std::string& imagePath, bool isGreyscale)
: quadrantName(quadrantName)
{
  if(isGreyscale){
    Image greyImage;
    ANKI_VERIFY(RESULT_OK == greyImage.Load(imagePath),
                "CompositeImageBuilder.QuadrantImpl.Constructor.GreyLoadFailed",
                "Failed to load image at path %s",
                imagePath.c_str());
    image = ImageRGBA(greyImage.GetNumRows(), greyImage.GetNumCols());
    image.FillWith(PixelRGBA(0,0,0,0));
    PixelRGBA* dataRGBA = image.GetDataPointer();
    u8* dataGreyscale = greyImage.GetRawDataPointer();

    for(s32 i=0; i<greyImage.GetNumElements(); ++i)
    {
      u8 pixel = dataGreyscale[i];
      dataRGBA[i].r() = pixel;
      dataRGBA[i].g() = dataGreyscale[i];
      dataRGBA[i].b() = dataGreyscale[i];
      dataRGBA[i].a() = dataGreyscale[i];
    }
  }else{
    ANKI_VERIFY(RESULT_OK == image.Load(imagePath),
                "CompositeImageBuilder.QuadrantImpl.Constructor.ColorLoadFailed",
                "Failed to load image at path %s",
                imagePath.c_str());
  }
}


}; // namespace Vision
}; // namespace Anki

