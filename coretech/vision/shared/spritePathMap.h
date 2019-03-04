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

#ifndef __CannedAnimLib_BaseTypes_SpritePathMap_H__
#define __CannedAnimLib_BaseTypes_SpritePathMap_H__

#include <string>
#include <unordered_map>

namespace Anki {
namespace Vision {

class SpritePathMap
{
private:
  const char* kPlaceHolderAssetName = "missing_asset_placeholder";

public:

  static const uint16_t kEmptySpriteBoxID;
  static const uint16_t kInvalidSpriteID;

  SpritePathMap();

  bool AddAsset(const std::string& assetName, const std::string& filePath, const bool isSpriteSequence);
  bool VerifyPlaceholderAsset() const;

  bool IsValidAssetName(const std::string& assetName) const;
  bool IsSpriteSequence(const std::string& assetName) const;

  const std::string& GetPlaceholderAssetPath() const { return GetAssetPath(kPlaceHolderAssetName); } 

  // Returns the filepath to the requested asset. In the event that there is no record
  // corresponding to the request assetName, an error will be logged along with Dev Assert
  // and the path for the "missing_sprite_placeholder" asset will be returned instead.
  const std::string& GetAssetPath(const std::string& assetName) const;
  uint16_t GetAssetID(const std::string& assetName) const;

  // Used to decode AssetID's to asset names after transmission Engine<->Anim. 
  const std::string& GetAssetName(const uint16_t& assetID) const;

private:
  struct AssetInfo{
    AssetInfo(uint16_t wireID, const bool isSpriteSeq, const std::string& assetName, const std::string& filePath);
    uint16_t    wireID;
    bool        isSpriteSequence;
    std::string assetName;
    std::string filePath; 
  };

  const AssetInfo* const GetInfoForAsset(const uint16_t& assetID) const;
  const AssetInfo* const GetInfoForAsset(const std::string& assetName) const;

  // Primary container mapping asset name to assetInfo
  std::unordered_map<std::string, AssetInfo> _nameToInfoMap;
  // Secondary container of pointers into _nameToInfoMap for reverse lookup by "wireID"
  std::unordered_map<uint16_t, AssetInfo*>   _idToInfoMap;
};

} // namespace Vision
} // namespace Anki


#endif // __CannedAnimLib_BaseTypes_SpritePathMap_H__
