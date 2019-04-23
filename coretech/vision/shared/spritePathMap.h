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

public:
  using AssetID = uint16_t;
  static const AssetID kClearSpriteBoxID;
  static const AssetID kEmptySpriteBoxID;
  static const AssetID kInvalidSpriteID;
private:
  static const char* kPlaceHolderAssetName;
  static const AssetID kPlaceHolderAssetID;
public:

  static AssetID GetAssetID(const std::string& assetName);

  SpritePathMap();

  AssetID AddAsset(const std::string& assetName, const std::string& filePath, const bool isSpriteSequence);
  bool VerifyPlaceholderAsset() const;

  // AssetID's can be obtained from strings and stored in constants in behaviors etc. This is acceptable 
  // since the hash function doesn't change at run time, but because such constants are initialized before
  // the SpritePathMap has loaded up assets, we store the requested strings for verification after loading
  // has finished to provide load-time verification of all requested Assets
  void CheckUnverifiedAssetIDs() const;

  bool IsValidAssetID(const AssetID assetID) const;
  bool IsSpriteSequence(const AssetID assetID) const;

  const std::string& GetPlaceholderAssetPath() const { return GetAssetPath(kPlaceHolderAssetID); } 

  // Returns the filepath to the requested asset. In the event that there is no record
  // corresponding to the request assetName, an error will be logged along with Dev Assert
  // and the path for the "missing_sprite_placeholder" asset will be returned instead.
  const std::string& GetAssetPath(const AssetID assetID) const;
  const std::string& GetAssetPath(const std::string& assetName) const { 
    return GetAssetPath(GetAssetID(assetName)); 
  }

private:
  static AssetID HashAssetName(const std::string& assetName);

  struct AssetInfo{
    AssetInfo();
    AssetInfo(const bool isSpriteSeq, const std::string& assetName, const std::string& filePath);
    bool        isSpriteSequence;
    std::string assetName;
    std::string filePath; 
  };

  const AssetInfo* const GetInfoForAsset(const AssetID assetID) const;

  static bool _loadingComplete;
  static std::unique_ptr<std::vector<std::string>> _unverifiedAssets;
  static std::unordered_map<AssetID, AssetInfo> _idToInfoMap;
};

} // namespace Vision
} // namespace Anki


#endif // __CannedAnimLib_BaseTypes_SpritePathMap_H__
