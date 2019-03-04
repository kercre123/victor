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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/math/point.h"
#include "coretech/vision/shared/compositeImage/compositeImageLayoutModifier.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"


namespace Anki {
namespace Vision {

namespace{
// Keep consistent with value in compositeImageTypes.clad
// Should be half the value so that points can be interleaved
const uint8_t kMaxModifierSequenceSerializationLength = 125;
}

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
bool CompositeImageLayer::operator==(const CompositeImageLayer& other) const{
  return _layerName == other._layerName &&
         _layoutMap == other._layoutMap &&
         _imageMap  == other._imageMap;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CompositeImageLayer::GetAssetID(SpriteBoxName sbName)  const
{
  auto imageMapIter = _imageMap.find(sbName);
  if(imageMapIter != _imageMap.end()){
    return imageMapIter->second.GetAssetID();
  }
  return Vision::SpritePathMap::kEmptySpriteBoxID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::GetFrame(SpriteBoxName sbName, const u32 index,
                                   Vision::SpriteHandle& handle) const
{
  auto imageMapIter = _imageMap.find(sbName);
  if(imageMapIter != _imageMap.end()){
    return imageMapIter->second.GetFrame(index, handle);
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
                                        SpriteBoxName sbName, const std::string& assetName)
{
  auto resultPair = _imageMap.emplace(sbName, SpriteEntry(cache, seqContainer, assetName));
  // If map entry already exists, just update existing iterator
  if(!resultPair.second){
    resultPair.first->second = SpriteEntry(cache, seqContainer, assetName);
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
void CompositeImageLayer::AddOrUpdateSpriteBoxWithEntry(const SpriteBox& spriteBox, const SpriteEntry& spriteEntry)
{
  AddToLayout(spriteBox.spriteBoxName, spriteBox);
  AddToImageMap(spriteBox.spriteBoxName, spriteEntry);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::ClearSpriteBoxByName(const SpriteBoxName& sbName)
{
  auto iter = _imageMap.find(sbName);
  if(iter != _imageMap.end()){
    _imageMap.erase(iter);
  }else{
    LOG_ERROR("CompositeImageLayer.ClearSpriteBoxByName.SpriteBoxNotFound",
              "Attempted to clear SB: %s, which is not present on layer: %s",
              EnumToString(sbName),
              EnumToString(GetLayerName()));
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
    const std::string sbString  = JsonTools::ParseString(entry, kSpriteBoxNameKey, implDebugStr);
    SpriteBoxName sbName        = SpriteBoxNameFromString(sbString);
    const std::string assetName = JsonTools::ParseString(entry, kAssetNameKey, implDebugStr);
    auto spriteEntry = Vision::CompositeImageLayer::SpriteEntry(cache,
                                                                seqContainer,
                                                                assetName);

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

  if(layoutModifier != nullptr){
    const auto& seq = layoutModifier->GetModifierSequence();
    if(seq.size() <= kMaxModifierSequenceSerializationLength){
      int idx = 0;
      for(const auto& point: seq){
        serialized.layoutModifier.interleavedModifierList[idx] = point.x();
        idx++;
        serialized.layoutModifier.interleavedModifierList[idx] = point.y();
        idx++;

      }
      serialized.layoutModifierInterleavedLength = idx;
    }else{
      PRINT_NAMED_ERROR("CompositeImageLayer.SpriteBox.Serialize.ModifierIssue",
                        "Can't serialize modifier of length %zu, max length is %d",
                        seq.size(), kMaxModifierSequenceSerializationLength);
    }
  }

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
bool CompositeImageLayer::SpriteBox::operator==(const SpriteBox& other) const
{
  return (spriteBoxName == other.spriteBoxName) &&
         (renderConfig  == other.renderConfig) &&
         (topLeftCorner == other.topLeftCorner) &&
         (width         == other.width) &&
         (height        == other.height);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteBox& CompositeImageLayer::SpriteBox::operator=(SpriteBox other)
{
  spriteBoxName = other.spriteBoxName;
  renderConfig  = other.renderConfig;
  topLeftCorner = other.topLeftCorner;
  width  = other.width;
  height = other.height;

  if(layoutModifier != nullptr){
    layoutModifier = std::make_unique<CompositeImageLayoutModifier>(*other.layoutModifier);
  }

  return *this;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteBox::SpriteBox(const SpriteBox& spriteBox)
: spriteBoxName(spriteBox.spriteBoxName)
, renderConfig(spriteBox.renderConfig)
, topLeftCorner(spriteBox.topLeftCorner)
, width(spriteBox.width)
, height(spriteBox.height)
{
  if(spriteBox.layoutModifier != nullptr){
    layoutModifier = std::make_unique<CompositeImageLayoutModifier>(*spriteBox.layoutModifier);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteBox::SpriteBox(const SerializedSpriteBox& spriteBox)
: spriteBoxName(spriteBox.name)
, renderConfig(spriteBox.renderConfig)
, topLeftCorner(spriteBox.topLeftX, spriteBox.topLeftY)
, width(spriteBox.width)
, height(spriteBox.height)
{
  ValidateRenderConfig();

  if(spriteBox.layoutModifierInterleavedLength > 0){
    layoutModifier = std::make_unique<CompositeImageLayoutModifier>();
    const auto modifierLength = spriteBox.layoutModifierInterleavedLength/2;
    for(int i = 0; i < modifierLength; i++){
      Point2i point(spriteBox.layoutModifier.interleavedModifierList[i],
                    spriteBox.layoutModifier.interleavedModifierList[i + 1]);
      layoutModifier->UpdatePositionForFrame(i, point);
    }
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::SpriteBox::GetPositionForFrame(const u32 frameIdx, Point2i& outTopLeftCorner,
                                                         int& outWidth, int& outHeight) const
{
  if(layoutModifier != nullptr){
    layoutModifier->GetPositionForFrame(frameIdx, outTopLeftCorner);
  }else{
    outTopLeftCorner = topLeftCorner;
  }


  outWidth = width;
  outHeight = height;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompositeImageLayer::SpriteBox::SetLayoutModifier(CompositeImageLayoutModifier*& modifier)
{
  layoutModifier.reset(modifier);
  modifier = nullptr;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayer::SpriteEntry::SpriteEntry(SpriteCache* cache,
                                              SpriteSequenceContainer* seqContainer,
                                              const std::string& assetName,
                                              uint frameStartOffset)
: _frameStartOffset(frameStartOffset)
, _serializable(true)
{
  auto* spritePathMap = cache->GetSpritePathMap();
  if(ANKI_VERIFY(nullptr != spritePathMap,
                 "SpriteEntry.Ctor.NullSpritePathMap",
                 "SpritePathMap must not be null to construct SpriteEntry's")){
    if(!spritePathMap->IsValidAssetName(assetName)){
      // SpriteEntries will handle missing assets internally since we have SpriteBox info when
      // we try to render them. That way we get the benefit of size information that we would lose
      // if we rendered the "missing" asset offered by the SpritePathMap
      _assetID = Vision::SpritePathMap::kInvalidSpriteID;
      PRINT_NAMED_WARNING("SpriteEntry.Ctor.InvalidAssetName",
                          "No asset named %s could be found. Target spriteBox will render an X",
                          assetName.c_str());
    } else {
      _assetID = spritePathMap->GetAssetID(assetName);
      if(spritePathMap->IsSpriteSequence(assetName)){
        _spriteSequence = *seqContainer->GetSpriteSequence(assetName);
      } else {
        _spriteSequence.AddFrame(cache->GetSpriteHandleForNamedSprite(assetName));
      }
    }
  } else {
    _assetID = Vision::SpritePathMap::kInvalidSpriteID;
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
uint16_t CompositeImageLayer::SpriteEntry::GetAssetID() const
{
  if(_serializable){
    return _assetID;
  }

  LOG_ERROR("CompositeImageLayer.SpriteEntry.GetAssetID.NotSerializable",
            "Attempted to serialize SpriteEntry with non-serializable assets. Target SpriteBox will render an X.");
  // If you're here for the above error message, you tried to use hand constructed Sprites/SpriteSequences in a
  // CompositeImage. This is unsupported for now due to inter-process messaging constraints.
  return SpritePathMap::kInvalidSpriteID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::SpriteEntry::GetFrame(const u32 index, Vision::SpriteHandle& handle) const
{
  if(index < _frameStartOffset){
    return false;
  }

  return _spriteSequence.GetFrame(index - _frameStartOffset, handle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayer::SpriteEntry::operator == (const SpriteEntry& other) const {
  return (_assetID == other._assetID) &&
         (_frameStartOffset == other._frameStartOffset);
}

}; // namespace Vision
}; // namespace Anki
