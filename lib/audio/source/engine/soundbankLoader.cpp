/**
 * File: soundbankLoader.cpp
 *
 * Author: Jordan Rivas
 * Created: 9/26/2017
 *
 * Description: This class handles the complexities of loading and unloading sound banks. This is designed to work with
 *              Anki’s RAMS system where assets may or may not be available. Using a build server generated metadata
 *              file “SoundbankBundleInfo.json” to search and load available sound banks
 *              (See anki-audio/build-scripts/bundle_soundbank_products.py).
 *
 * TODO: Implement RAMS
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "audioEngine/soundbankLoader.h"
#include "audioEngine/audioEngineController.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include <algorithm>
#include <set>


namespace Anki {
namespace AudioEngine {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundbankLoader::SoundbankLoader(AudioEngineController& audioEngineController, std::string assetPath)
: _audioEngineController( audioEngineController )
, _assetPath( assetPath )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundbankLoader::LoadDefaultSoundbanks()
{
  // Check if assets are bundled into zip files
  LoadSoundbankBundleMetadata();
  
  // Create a list of available bank names
  BankNameList availableBanks = GetAvailableSoundbankNames();
  
  BankNameList addList;
  BankNameList removeList;
  DiffBankLists( _audioEngineController.GetLoadedBankNames(), availableBanks, addList, removeList );
  
  // Only load Normal soundbanks types
  FilterBankNameList( addList, SoundbankBundleInfo::BankType::Normal );
  
  // Unload soundbanks
  // Always unload "Init" bank last
  for ( auto aBankIt = removeList.rbegin() ; aBankIt != removeList.rend(); ++aBankIt ) {
    _audioEngineController.UnloadSoundbank( *aBankIt );
  }
  
  // Load soundbanks
  // Always load "Init" bank first
  for ( const auto& aBank : addList ) {
    _audioEngineController.LoadSoundbank( aBank );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundbankLoader::LoadDebugSoundbanks(const BankNameList& optOutDevSoundbanks, bool loadBanks)
{
  // Find debug sound banks to load/unload
  BankNameList debugBanks( GetAvailableSoundbankNames() );
  FilterBankNameList( debugBanks, SoundbankBundleInfo::BankType::Debug );
  BankNameList filteredBanks;
  for ( auto debugBankIt : debugBanks ) {
    // Add sound banks to filteredBanks if they don't exist in optOutDevSoundbanks
    const auto optOutIt = std::find( optOutDevSoundbanks.begin(), optOutDevSoundbanks.end(), debugBankIt );
    if ( optOutIt == optOutDevSoundbanks.end() ) {
      filteredBanks.push_back( debugBankIt );
    }
  }
  
  // Load/Unload soundbanks
  for ( auto filteredBanksIt : filteredBanks ) {
    if ( loadBanks ) {
      _audioEngineController.LoadSoundbank( filteredBanksIt );
    }
    else {
      _audioEngineController.UnloadSoundbank( filteredBanksIt );
    }
  }
}

// Load soundbanks helper methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundbankLoader::LoadSoundbankBundleMetadata()
{
  const char* filename = "SoundbankBundleInfo.json";
  const std::string pathToSoundConfig = Anki::Util::FileUtils::FullFilePath({ _assetPath, filename });
  
  BankInfoList list;
  if ( !SoundbankBundleInfo::GetSoundbankBundleInfoMetadata( pathToSoundConfig, list ) ) {
    PRINT_NAMED_WARNING("SoundbankLoader.LoadSoundbankBundleMetadata", "GetSoundbankBundleInfoMetadata() failed");
    return;
  }
  
  // Add all asset zip files to audio engine search path
  std::vector<std::string> pathsToZipFiles = GetAvailableSoundbankZipFiles( list );
  _audioEngineController.AddZipFiles( pathsToZipFiles );
  
  // Group the soundbanks together with their locale counterparts
  _bankBundleInfoMap.clear();
  for ( auto& aBundleInfo : list ) {
    const bool exists = _audioEngineController.CheckFileExists( aBundleInfo.Path );
    if ( exists ) {
      // Add to map
      _bankBundleInfoMap[aBundleInfo.SoundbankName].push_back(std::move( aBundleInfo ));
    } else {
      PRINT_NAMED_WARNING("SoundbankLoader.LoadSoundbankBundleMetadata.DoesNotExist",
                          "Soundbank '%s' does not exist!", aBundleInfo.Path.c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineController::AudioNameList SoundbankLoader::GetAvailableSoundbankNames() const
{
  BankNameList list;
  for ( const auto& kvp : _bankBundleInfoMap ) {
    list.push_back( kvp.first );
  }
  return list;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundbankLoader::FilterBankNameList(BankNameList& bankNameList, SoundbankBundleInfo::BankType bankType) const
{
  const auto removeIt = std::remove_if( bankNameList.begin(),
                                       bankNameList.end(),
                                       [this, bankType](std::string bankName )
                                       {
                                         const auto findIt = _bankBundleInfoMap.find( bankName );
                                         if ( findIt != _bankBundleInfoMap.end() ) {
                                           // We want to find all SoundBankBundleInfos that are not "bankType"
                                           // We can assume that all locales of the bank are the same type
                                           return ( findIt->second[0].Type != bankType );
                                         }
                                         DEV_ASSERT(findIt != _bankBundleInfoMap.end(),
                                                    "SoundbankLoader.FilterBankNameList.bankMap.bankName.NotFound");
                                         // We should always find it in the map if not
                                         return true;
                                       });
  bankNameList.erase( removeIt, bankNameList.end() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::string> SoundbankLoader::GetAvailableSoundbankZipFiles(const BankInfoList& infoList) const
{
  std::vector<std::string> pathsToZipFiles;
  
  for ( auto const& it : infoList ) {
    std::string path;
    const std::string filename = it.BundleName + ".zip";
    const std::string filePath = Anki::Util::FileUtils::FullFilePath({ _assetPath, filename });
    if ( Anki::Util::FileUtils::FileExists( filePath ) ) {
      pathsToZipFiles.push_back( filePath );
    }
  }
  
  return pathsToZipFiles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::string> SoundbankLoader::FindUniqueBankFiles(const std::vector<std::string>& filePaths) const
{
  std::vector<std::string> bankFiles;
  if (filePaths.empty()) {
    return bankFiles;
  }
  
  std::set<std::string> bankSet;
  const std::string suffix = ".bnk";
  const auto endsWithSuffixFunc = [](const std::string& str, const std::string& suffix)
  {
    return str.rfind(suffix) == (str.size()-suffix.size());
  };
  
  for (const auto& aPath : filePaths) {
    if (endsWithSuffixFunc(aPath, suffix)) {
      std::string shortName = aPath.substr(aPath.find_last_of("/")+1);
      const bool inserted = bankSet.insert(shortName).second;
      if (inserted) {
        bankFiles.push_back(shortName);
      }
    }
  }
  
  return bankFiles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundbankLoader::DiffBankLists(const BankNameList& loadedBanks,
                                    BankNameList& updatedList,
                                    BankNameList& out_addList,
                                    BankNameList& out_removeList) const
{
  out_addList.clear();
  out_removeList.clear();
  
  // Check if any banks are loaded
  if (loadedBanks.empty()) {
    // No banks loaded add all in list
    std::copy(std::begin(updatedList), std::end(updatedList),
              std::back_inserter(out_addList));
  }
  else {
    // Diff current banks and updated list to find banks that need to loaded and unloaded from audio engine
    BankNameList currentBanks(loadedBanks);
    
    // Sort bank lists for set_difference
    std::sort(currentBanks.begin(), currentBanks.end());
    std::sort(updatedList.begin(), updatedList.end());
    
    // Find soundbanks that need to be removed
    std::set_difference(currentBanks.begin(), currentBanks.end(),
                        updatedList.begin(), updatedList.end(),
                        std::inserter(out_removeList, out_removeList.begin()));
    // Find soundbanks that need to be added
    std::set_difference(updatedList.begin(), updatedList.end(),
                        currentBanks.begin(), currentBanks.end(),
                        std::inserter(out_addList, out_addList.begin()));
  }
  
  // Special sort. Alphabetically with "Init" always first
  const std::string initBnk = "init";
  const char fileExtensionDot = '.';
  const auto specialSortFunc = [initBnk] (const std::string& a, const std::string& b) {
    // First check if the string is the init bank and if so sort it to be the first object
    std::string aLower = a.substr(0, a.find_last_of(fileExtensionDot)); // Trim off extension
    if (initBnk.size() == aLower.size()) {
      std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
      if (aLower.compare(0, initBnk.size(), initBnk) == 0) {
        return true;
      }
    }
    
    std::string bLower = b.substr(0, b.find_last_of(fileExtensionDot)); // Trim off extension
    if (initBnk.size() == bLower.size()) {
      std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
      if (bLower.compare(0, initBnk.size(), initBnk) == 0) {
        return false;
      }
    }
    // Otherwise normal alphabetical sort
    return a < b;
  };
  
  std::sort(out_addList.begin(), out_addList.end(), specialSortFunc);
  std::sort(out_removeList.begin(), out_removeList.end(), specialSortFunc);
}

}
}
