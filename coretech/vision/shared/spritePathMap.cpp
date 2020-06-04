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

#define LOG_CHANNEL "SpritePathMap"

namespace Anki {
namespace Vision {

bool SpritePathMap::_loadingComplete = false;
std::unique_ptr<std::vector<std::string>> SpritePathMap::_unverifiedAssets;
std::unordered_map<SpritePathMap::AssetID, SpritePathMap::AssetInfo> SpritePathMap::_idToInfoMap;

const SpritePathMap::AssetID SpritePathMap::kClearSpriteBoxID = SpritePathMap::HashAssetName("clear_sprite_box");
const SpritePathMap::AssetID SpritePathMap::kEmptySpriteBoxID = SpritePathMap::HashAssetName("empty_sprite_box");
const SpritePathMap::AssetID SpritePathMap::kInvalidSpriteID = SpritePathMap::HashAssetName("invalid_sprite_box");

const char* SpritePathMap::kPlaceHolderAssetName= "missing_asset_placeholder";
const SpritePathMap::AssetID SpritePathMap::kPlaceHolderAssetID =
  SpritePathMap::HashAssetName(kPlaceHolderAssetName);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::AssetInfo::AssetInfo()
:isSpriteSequence(false)
, assetName("")
, filePath("")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::AssetInfo::AssetInfo(const bool isSpriteSeq,
                                    const std::string& assetName,
                                    const std::string& filePath)
: isSpriteSequence(isSpriteSeq)
, assetName(assetName)
, filePath(filePath)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::SpritePathMap()
{
  // Make sure these ID's don't conflict with any of the hashed AssetNames
  _idToInfoMap.emplace(kClearSpriteBoxID, AssetInfo{false, "ClearSpriteBox", ""});
  _idToInfoMap.emplace(kEmptySpriteBoxID, AssetInfo{false, "EmptySpriteBox", ""});
  _idToInfoMap.emplace(kInvalidSpriteID, AssetInfo{false, "InvalidSpriteBox", ""});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::AssetID SpritePathMap::HashAssetName(const std::string& assetName)
{
  // Here we use only the bottom 16 bits of the hash. This increases probability of collision, 
  // but given the sample size is just our asset names, the probability is still very small.
  return static_cast<AssetID>(std::hash<std::string>()(assetName));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::AssetID SpritePathMap::GetAssetID(const std::string& assetName)
{
  if(!_loadingComplete){
    // We haven't loaded assets yet. Hash the name, return the ID, and make a note
    // so we can verify the asset after loading is finished
    if(nullptr == _unverifiedAssets){
      _unverifiedAssets = std::make_unique<std::vector<std::string>>();
    }
    _unverifiedAssets->push_back(assetName);
    return HashAssetName(assetName);
  }

  AssetID assetID = HashAssetName(assetName);
  if(_idToInfoMap.end() == _idToInfoMap.find(assetID)){
    LOG_ERROR("SpritePathMap.GetAssetID.InvalidAssetName",
              "No asset with name %s is available. Returning asset ID for %s",
              assetName.c_str(),
              kPlaceHolderAssetName);
    return kPlaceHolderAssetID;
  }
  return assetID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpritePathMap::AssetID SpritePathMap::AddAsset(const std::string& assetName,
                                               const std::string& filePath,
                                               const bool isSpriteSequence)
{
  AssetID newAssetID = HashAssetName(assetName);

  AssetInfo newAssetInfo(isSpriteSequence, assetName, filePath);
  auto emplacePair = _idToInfoMap.emplace(newAssetID, newAssetInfo);
  if(!emplacePair.second){
    if(emplacePair.first->second.assetName == assetName){
      LOG_WARNING("SpritePathMap.AddAsset.DuplicateAssetName",
                  "Attempted to Add Asset with name %s, but name is already used",
                  assetName.c_str());
    } else {
      LOG_ERROR("SpritePathMap.AddAsset.InvalidHash",
                "Asset name: %s produced a hash duplicated by asset name: %s",
                assetName.c_str(),
                emplacePair.first->second.assetName.c_str());
    }
    return kInvalidSpriteID;
  }

  return newAssetID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::VerifyPlaceholderAsset() const
{
  const auto& iter = _idToInfoMap.find(kPlaceHolderAssetID);
  return ANKI_VERIFY(iter != _idToInfoMap.end(),
                     "SpritePathMap.VerifyPlaceholderAsset.AssetNotFound",
                     "Placeholder asset specified as: %s not found",
                     kPlaceHolderAssetName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::IsValidAssetID(const AssetID assetID) const
{
  if(kInvalidSpriteID == assetID){
    return false;
  } 

  const auto& iter = _idToInfoMap.find(assetID);
  return (iter != _idToInfoMap.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpritePathMap::IsSpriteSequence(const AssetID assetID) const
{
  return GetInfoForAsset(assetID)->isSpriteSequence;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& SpritePathMap::GetAssetPath(const AssetID assetID) const
{
  return GetInfoForAsset(assetID)->filePath;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpritePathMap::AssetInfo* const SpritePathMap::GetInfoForAsset(const AssetID assetID) const
{
  const auto& iter = _idToInfoMap.find(assetID);
  if(_idToInfoMap.end() != iter){
    return &iter->second;
  }

  if(VerifyPlaceholderAsset()){
    LOG_ERROR("SpritePathMap.GetAssetID.InvalidAssetID",
              "No asset with ID %d is available. Returning asset info for %s",
              assetID,
              kPlaceHolderAssetName);
    return GetInfoForAsset(kPlaceHolderAssetID);
  }

  PRINT_NAMED_ERROR("SpritePathMap.DefaultSpriteNotFound",
                    "There must be a sprite named %s among the sprite assets, and isn't. CRASH IMMINENT.",
                    kPlaceHolderAssetName);
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpritePathMap::CheckUnverifiedAssetIDs() const
{
  _loadingComplete = true;

  if(nullptr == _unverifiedAssets){
    return;
  }

  for(const auto& assetName : *_unverifiedAssets){
    ANKI_VERIFY(_idToInfoMap.end() != _idToInfoMap.find(HashAssetName(assetName)),
                "SpritePathMap.GetAssetID.InvalidAssetName",
                "No asset with name %s is available. Ensure that it is present in EXTERNALS",
                assetName.c_str());
  }

  _unverifiedAssets.reset(nullptr);
}

} // namespace Vision
} // namespace Anki
