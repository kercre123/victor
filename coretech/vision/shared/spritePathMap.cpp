/**
 * File: SpritePathMap.h
 *
 * Author: Sam Russell
 * Date:   2/10/2019
 *
 * Description: Defines type of spritePathMap
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "coretech/vision/shared/spritePathMap.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Vision {

const uint16_t SpritePathMap::kEmptySpriteBoxID = std::hash<std::string>()("EmptySpriteBox");
const uint16_t SpritePathMap::kInvalidSpriteID = std::hash<std::string>()("InvalidSpriteBox");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::AssetInfo::AssetInfo(uint16_t wireID,
                                    const bool isSpriteSeq,
                                    const std::string& assetName,
                                    const std::string& filePath)
: wireID(wireID)
, isSpriteSequence(isSpriteSeq)
, assetName(assetName)
, filePath(filePath)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::SpritePathMap()
{
  // Make sure these ID's don't conflict with any of the hashed AssetNames
  _idToInfoMap.emplace(kEmptySpriteBoxID, nullptr);
  _idToInfoMap.emplace(kInvalidSpriteID, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::AddAsset(const std::string& assetName, const std::string& filePath, const bool isSpriteSequence)
{
  {
    const auto& iter = _nameToInfoMap.find(assetName);
    if(iter != _nameToInfoMap.end()){
      PRINT_NAMED_WARNING("SpritePathMap.AddAsset.DuplicateAssetName",
                          "Attempted to AddAsset with name %s, but name is already used",
                          assetName.c_str());
      return false;
    }
  }

  uint16_t newAssetID = static_cast<uint16_t>(std::hash<std::string>()(assetName));
  // Here we use only the bottom 16 bits of the hash. This increases probability of collision, 
  // but given the sample size is just our asset names, the probability is still very small.
  {
    const auto& iter = _idToInfoMap.find(newAssetID);
    if(iter != _idToInfoMap.end()){
      PRINT_NAMED_ERROR("SpritePathMap.AddAsset.InvalidHash",
                        "Asset name: %s produced a hash duplicated by asset name: %s",
                        assetName.c_str(),
                        iter->second->assetName.c_str());
      return false;
    }
  }
  AssetInfo newAssetInfo(newAssetID, isSpriteSequence, assetName, filePath);
  auto emplacePair = _nameToInfoMap.emplace(assetName, newAssetInfo);
  _idToInfoMap.emplace(newAssetID, &emplacePair.first->second);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::VerifyPlaceholderAsset() const
{
  const auto& iter = _nameToInfoMap.find(kPlaceHolderAssetName);
  return ANKI_VERIFY(iter != _nameToInfoMap.end(),
                     "SpritePathMap.VerifyPlaceholderAsset.AssetNotFound",
                     "Placeholder asset specified as: %s not found",
                     kPlaceHolderAssetName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::IsValidAssetName(const std::string& assetName) const
{
  if(kPlaceHolderAssetName == assetName){
    return false;
  }

  const auto& iter = _nameToInfoMap.find(assetName);
  return (iter != _nameToInfoMap.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::IsSpriteSequence(const std::string& assetName) const
{
  return GetInfoForAsset(assetName)->isSpriteSequence;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& SpritePathMap::GetAssetPath(const std::string& assetName) const
{
  return GetInfoForAsset(assetName)->filePath;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t SpritePathMap::GetAssetID(const std::string& assetName) const
{
  return GetInfoForAsset(assetName)->wireID; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& SpritePathMap::GetAssetName(const uint16_t& assetID) const
{
  ANKI_VERIFY(kEmptySpriteBoxID != assetID,
              "SpritePathMap.GetAssetName.EmptySpriteBox",
              "Requested an asset name for an empty SpriteBox, this indicates a usage error");
  return GetInfoForAsset(assetID)->assetName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpritePathMap::AssetInfo* const SpritePathMap::GetInfoForAsset(const uint16_t& assetID) const
{
  const auto& iter = _idToInfoMap.find(assetID);
  if(iter != _idToInfoMap.end()){
    return iter->second;
  }

  return GetInfoForAsset(kPlaceHolderAssetName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpritePathMap::AssetInfo* const SpritePathMap::GetInfoForAsset(const std::string& assetName) const
{
  {
    const auto& iter = _nameToInfoMap.find(assetName);
    if(iter != _nameToInfoMap.end()){
      return &iter->second;
    }

    PRINT_NAMED_ERROR("SpritePathMap.SpriteNotFound",
                      "No sprite found for name: %s. Attempting to return path for %s",
                      assetName.c_str(),
                      kPlaceHolderAssetName);
  }

  const auto& iter = _nameToInfoMap.find(kPlaceHolderAssetName);
  if(iter != _nameToInfoMap.end()){
    return &iter->second;
  }

  PRINT_NAMED_ERROR("SpritePathMap.DefaultSpriteNotFound",
                    "There must be a sprite named %s among the sprite assets, and isn't. CRASH IMMINENT.",
                    kPlaceHolderAssetName);
  return nullptr;
}

} // namespace Vision
} // namespace Anki
