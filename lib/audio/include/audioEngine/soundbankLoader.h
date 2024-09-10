/**
 * File: soundbankLoader.h
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


#ifndef __AnkiAudio_SoundbankLoader_H__
#define __AnkiAudio_SoundbankLoader_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/soundbankBundleInfo.h"
#include <string>
#include <unordered_map>
#include <vector>


namespace Anki {
namespace AudioEngine {
class AudioEngineController;


class AUDIOENGINE_EXPORT SoundbankLoader {
  
  using BankNameList = std::vector<std::string>;
  
public:
  
  SoundbankLoader(AudioEngineController& audioEngineController, std::string assetPath);
  
  // Find all available soundbank in app bundle and RAMS downloadable content
  // Unload assets that are no longer available and load new assets that have become available
  // Only load "SoundbankBundleInfo::BankType::Normal" soundbanks
  void LoadDefaultSoundbanks();

  // Load or unload all debug sound banks except those in optOutDevSoundbanks list
  void LoadDebugSoundbanks(const BankNameList& optOutDevSoundbanks, bool loadBanks = true);
  

private:
  
  using BankInfoList = std::vector<AudioEngine::SoundbankBundleInfo>;
  
  // Parent Audio Engine Controller
  AudioEngineController& _audioEngineController;
  // Audio asset path
  std::string _assetPath;
  
  // Track RAMS soundbank bundle info structs
  // NOTE: Map is keyed by Soundbank's name and the value is a list of SoundbankBundleInfo struct for each locale
  // { "bankName" : [ SoundbankBundleInfo, SoundbankBundleInfo ] }
  std::unordered_map<std::string, BankInfoList> _bankBundleInfoMap;
  
  // Load soundbanks helper methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Read Soundbank Bundle Info JSON file, update Zip paths and populate _bankBundleInfoMap with valid soundbanks
  void LoadSoundbankBundleMetadata();
  
  // Return a list of available unordered soundbanks names
  // NOTE: Must call LoadSoundbankBundleMetadata() to load soundbank bundle metadata before using this method
  BankNameList GetAvailableSoundbankNames() const;
  
  // Filter bank name list to only include banks of specified type
  // NOTE: Must call LoadSoundbankBundleMetadata() to load soundbank bundle metadata before using this method
  void FilterBankNameList(BankNameList& bankNameList, AudioEngine::SoundbankBundleInfo::BankType bankType) const;
  
  // Find all available soundbank zip files
  // Pass in list of bundle names, this function will add zip extension to help find the package
  std::vector<std::string> GetAvailableSoundbankZipFiles(const BankInfoList& infoList) const;
  
  // Create a list of unique soundbank names, remove redundant banks which occurs from multiple locales
  std::vector<std::string> FindUniqueBankFiles(const std::vector<std::string>& filePaths) const;
  
  // Determine which banks need to be added and removed
  // Expect out values to be sorted (Special Sort: Alphabetically with "Init" always first)
  // NOTE: Parameter "updateList" vector order may be altered
  void DiffBankLists(const BankNameList& loadedBanks, BankNameList& updatedList,
                     BankNameList& out_addList, BankNameList& out_removeList) const;

};
  
}
}

#endif /* __AnkiAudio_SoundbankLoader_H__ */
