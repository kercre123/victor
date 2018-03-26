/**
* File: compositeImageLayer.cpp
*
* Author: Kevin M. Karol
* Created: 3/19/2018
*
* Description: Defines the sprite box layout for a composite layer and the
* image map for each sprite box
*
* Copyright: Anki, Inc. 2018
*
**/

#include "coretech/vision/engine/compositeImage/compositeImageLayer.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::CompositeImageLayer(const Json::Value& layoutSpec)
{
  using namespace CompositeImageConfigKeys;
  const std::string templateDebugStr = "CompositeImageLayer.Constructor.LayoutKeyIssue.";

  // Load in the layer name
  {
    const std::string layerName = JsonTools::ParseString(layoutSpec, kLayerNameKey, templateDebugStr + kLayerNameKey);
    _layerName = LayerNameFromString(layerName);
  }

  // Verify that a layout is specified
  if(!ANKI_VERIFY(layoutSpec.isMember(kSpriteBoxLayoutKey),
                  (templateDebugStr + kSpriteBoxLayoutKey).c_str(), 
                  "No sprite box layout provided for composite image")){
    return;
  }

  // Load in the sprite boxes from the layout
  for(auto& entry: layoutSpec[kSpriteBoxLayoutKey]){
    const std::string sbString = JsonTools::ParseString(entry, kSpriteBoxNameKey, templateDebugStr);
    const int x                = JsonTools::ParseInt32(entry,  kCornerXKey,       templateDebugStr);
    const int y                = JsonTools::ParseInt32(entry,  kCornerYKey,       templateDebugStr);
    const int width            = JsonTools::ParseInt32(entry,  kWidthKey,         templateDebugStr);
    const int height           = JsonTools::ParseInt32(entry,  kHeightKey,        templateDebugStr);
    
    SpriteBoxName sbName = SpriteBoxNameFromString(sbString);
    _layoutMap.emplace(std::make_pair(sbName, SpriteBox(sbName, Point2i(x, y), width, height)));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::~CompositeImageLayer()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::SetImageMap(const Json::Value& imageMapSpec)
{
  using namespace CompositeImageConfigKeys;

  _imageMap.clear();
  const std::string implDebugStr = "CompositeImageBuilder.BuildCompositeImage.SpecKey";
  for(auto& entry: imageMapSpec){
    const std::string sbString  = JsonTools::ParseString(entry, kSpriteBoxNameKey, implDebugStr);
    const std::string imagePath = JsonTools::ParseString(entry, kSpritePathKey, implDebugStr);
    SpriteBoxName sbName        = SpriteBoxNameFromString(sbString);
    _imageMap.emplace(std::make_pair(sbName, imagePath));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::IsValidImageMap(const ImageMap& imageMap, bool requireAllSpriteBoxes) const
{
  if(requireAllSpriteBoxes){
    // just check size here - checks below will fail if quadrant names don't match
    if(!ANKI_VERIFY(_layoutMap.size() == imageMap.size(),
                    "CompositeImageLayerDef.IsValidImplementation.AllQuadrantsNotSpecified",
                    "Layout has %zu quadrants, but implementation only has %zu",
                    _layoutMap.size(), 
                    imageMap.size())){
      return false;
    }
  }

  for(const auto& pair: imageMap){
    auto iter = _layoutMap.find(pair.first);
    if(iter == _layoutMap.end()){
      PRINT_NAMED_WARNING("CompositeImageLayerDef.IsValidImplementation.spriteBoxNameMismatch",
                          "Implementation has quadrant named %s which is not present in layout",
                          SpriteBoxNameToString(iter->first));
      return false;
    }
  }

  return true;
}


}; // namespace Vision
}; // namespace Anki

