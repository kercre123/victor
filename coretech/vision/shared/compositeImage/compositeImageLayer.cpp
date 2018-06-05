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

#include "coretech/vision/shared/compositeImage/compositeImageLayer.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"


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
    const int x                = JsonTools::ParseInt32(entry,  kCornerXKey,       templateDebugStr);
    const int y                = JsonTools::ParseInt32(entry,  kCornerYKey,       templateDebugStr);
    const int width            = JsonTools::ParseInt32(entry,  kWidthKey,         templateDebugStr);
    const int height           = JsonTools::ParseInt32(entry,  kHeightKey,        templateDebugStr);

    SpriteRenderConfig renderConfig;
    JsonTools::GetValueOptional(entry,  kHueKey,        renderConfig.hue);
    JsonTools::GetValueOptional(entry,  kSaturationKey,  renderConfig.saturation);

    const std::string renderString = JsonTools::ParseString(entry, kRenderMethodKey, templateDebugStr);
    renderConfig.renderMethod = SpriteRenderMethodFromString(renderString);

    const std::string sbString = JsonTools::ParseString(entry, kSpriteBoxNameKey, templateDebugStr);
    SpriteBoxName sbName = SpriteBoxNameFromString(sbString);
    
    _layoutMap.emplace(std::make_pair(sbName, SpriteBox(sbName, renderConfig, Point2i(x, y), width, height)));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::~CompositeImageLayer()
{

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::GetSpriteSequenceName(SpriteBoxName sbName, Vision::SpriteName& sequenceName)  const
{
  auto imageMapIter = _imageMap.find(sbName);
  if(imageMapIter != _imageMap.end()){
    sequenceName = imageMapIter->second._spriteName;
    return true;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::GetSpriteSequence(SpriteBoxName sbName, Vision::SpriteSequence& seq)  const 
{
  auto imageMapIter = _imageMap.find(sbName);
  if(imageMapIter != _imageMap.end()){
    seq = imageMapIter->second._spriteSequence;
    return true;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::MergeInLayer(const CompositeImageLayer& otherLayer)
{
  // Merge layout info
  for(const auto& entry : otherLayer.GetLayoutMap()){
    AddToLayout(entry.first, entry.second);
  }

  // Merge image info
  for(const auto& entry : otherLayer.GetImageMap()){
    AddToImageMap(entry.first, entry.second);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::AddToLayout(SpriteBoxName sbName, const SpriteBox& spriteBox)
{
  auto resultPair = _layoutMap.emplace(sbName, spriteBox);
  // If map entry already exists, just update existing iterator
  if(!resultPair.second){
    resultPair.first->second = spriteBox;
  }  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::AddToImageMap(SpriteCache* cache, SpriteSequenceContainer* seqContainer,
                                        SpriteBoxName sbName, const Vision::SpriteName& spriteName)
{
  auto resultPair = _imageMap.emplace(sbName, SpriteEntry(cache, seqContainer, spriteName));
  // If map entry already exists, just update existing iterator
  if(!resultPair.second){
    resultPair.first->second = SpriteEntry(cache, seqContainer, spriteName);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::AddToImageMap(SpriteBoxName sbName, const SpriteEntry& spriteEntry)
{
  auto resultPair = _imageMap.emplace(sbName, spriteEntry);
  // If map entry already exists, just update existing iterator
  if(!resultPair.second){
    resultPair.first->second = spriteEntry;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::GetSpriteEntry(const SpriteBox& sb, SpriteEntry& outEntry) const
{
  auto iter = _imageMap.find(sb.spriteBoxName);
  if(iter != _imageMap.end()){
    outEntry = iter->second;
    return true;
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::SetImageMap(const Json::Value& imageMapSpec, 
                                      SpriteCache* cache, SpriteSequenceContainer* seqContainer)
{
  using namespace CompositeImageConfigKeys;

  _imageMap.clear();
  const std::string implDebugStr = "CompositeImageBuilder.BuildCompositeImage.SpecKey";
  for(auto& entry: imageMapSpec){
    const std::string sbString     = JsonTools::ParseString(entry, kSpriteBoxNameKey, implDebugStr);
    SpriteBoxName sbName           = SpriteBoxNameFromString(sbString);
    const std::string spriteString = JsonTools::ParseString(entry, kSpriteNameKey, implDebugStr);
    Vision::SpriteName spriteName   = Vision::SpriteNameFromString(spriteString);
    auto spriteEntry = Vision::CompositeImageLayer::SpriteEntry(cache,
                                                                seqContainer,
                                                                spriteName);

    _imageMap.emplace(std::make_pair(sbName,  std::move(spriteEntry)));
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerializedSpriteBox CompositeImageLayer::SpriteBox::Serialize() const
{
  SerializedSpriteBox serialized;
  serialized.topLeftX = topLeftCorner.x();
  serialized.topLeftY = topLeftCorner.y();
  serialized.width    = width;
  serialized.height   = height;
  serialized.name     = spriteBoxName;
  serialized.renderConfig = renderConfig;

  return serialized;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::SpriteBox::ValidateRenderConfig() const
{
  switch(renderConfig.renderMethod){
    case SpriteRenderMethod::RGBA:
    case SpriteRenderMethod::CustomHue:
    {
      return true;
    }
    default:
    {
      PRINT_NAMED_ERROR("CompositeImageLayer.ValidateRenderConfig",
                        "Sprite Box %s does not have a valid render method",
                        SpriteBoxNameToString(spriteBoxName));
      return false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteEntry::SpriteEntry(SpriteCache* cache,
                                              SpriteSequenceContainer* seqContainer,
                                              Vision::SpriteName spriteName)
: _spriteName(spriteName)
{
  if(Vision::IsSpriteSequence(spriteName, false)){
    auto* seq = seqContainer->GetSequenceByName(spriteName);
    if(seq != nullptr){
      _spriteSequence = *seq;
    }else{
      PRINT_NAMED_ERROR("CompositeImageLayer.SpriteEntry.SequenceNotInContainer",
                        "Could not find sequence for SpriteName %s",
                        SpriteNameToString(spriteName));
    }
  }else{
    _spriteSequence.AddFrame(cache->GetSpriteHandle(spriteName));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteEntry::SpriteEntry(Vision::SpriteSequence&& sequence)
{
  _spriteSequence = std::move(sequence);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteEntry::SpriteEntry(Vision::SpriteHandle spriteHandle)
{
  _spriteSequence.AddFrame(spriteHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::SpriteEntry::operator == (const SpriteEntry& other) const {
  if(_spriteName == Vision::SpriteName::Count){
    PRINT_NAMED_ERROR("CompositeImageLayer.SpriteEntry.==Invalid",
                      "Invalid comparison because spriteName is count");
  }
  return (_spriteName == other._spriteName);
}

}; // namespace Vision
}; // namespace Anki

