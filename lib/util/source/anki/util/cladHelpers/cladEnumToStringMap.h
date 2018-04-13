/**
 * File: cladEnumToStringMap.h
 *
 * Authors: Kevin M. Karol
 * Created: 4/3/18
 *
 * Description: Stores map from clad enums to strings - useful for
 * mapping between Clad values used in code and strings for file paths etc.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Util_CladHelpers_CladEnumToStringMap_H__
#define __Util_CladHelpers_CladEnumToStringMap_H__

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "json/json.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include <unordered_map>

namespace Anki {
namespace Util {

template <class CladEnum>
class CladEnumToStringMap
{
public:

  bool Load(const Anki::Util::Data::DataPlatform* data, const std::string& path, const std::string& fileNameKey);
  
  std::vector<CladEnum> GetAllKeys() const;
  std::string GetValue(CladEnum ev) const;

  // Caching the lookup map will allocate additional memory for the class but make
  // future lookups faster
  // Returns true if outKey is set, false if lookup failed
  bool GetKeyForValue(const std::string& value, 
                      CladEnum& outKey, 
                      bool shouldCacheLookupMap = false);

  bool GetKeyForValueConst(const std::string& value, 
                           CladEnum& outKey) const;
  
  bool HasKey(CladEnum ev) const;
  bool Erase(CladEnum ev);
  void UpdateValue(CladEnum ev, const std::string& newValue);

  
private:
  std::unordered_map<CladEnum, std::string, Anki::Util::EnumHasher> _cladMap;
  std::unordered_map<std::string, CladEnum> _reverseLookupMap;

}; // class CladEnumToStringMap
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
bool CladEnumToStringMap<CladEnum>::Load(const Anki::Util::Data::DataPlatform* data, 
                                         const std::string& path, 
                                         const std::string& fileNameKey)
{
  if (nullptr == data )
  {
    return false;
  }
  bool allStringsLoaded = false;

  const std::string fullPath = data->pathToResource(Util::Data::Scope::Resources, path);
  
  const bool fileExists = Anki::Util::FileUtils::FileExists(fullPath);
  ANKI_VERIFY(fileExists, "CladEnumToStringMap.Load.FileDoesNotExistAtExpectedPath",
              "file %s does not exist", fullPath.c_str());
  // Read the file in
  if(fileExists)
  {
    Json::Value pairArray;
    const bool success = data->readAsJson(fullPath, pairArray);
    if( success)
    {
      allStringsLoaded = true;
      const int numPairs = pairArray.size();
      for(int i = 0; i < numPairs; ++i)
      {
        const Json::Value& singleEvent = pairArray[i];
        auto memberNames = singleEvent.getMemberNames();
        if(ANKI_VERIFY(memberNames.size() == 2,
                       "CladEnumToStringMap.Load.ImproperEntryMembers",
                       "Entry in file %s has %zu members, expected 2",
                       fullPath.c_str(), memberNames.size())){
          const std::string debugName = "CladEnumToStringMap.MissingKey.";
          const char* kCladEventKey = "CladEvent";

          std::string filePath = JsonTools::ParseString(singleEvent, fileNameKey.c_str(), debugName + fileNameKey);
          const std::string enumAsString = JsonTools::ParseString(singleEvent, kCladEventKey, debugName + kCladEventKey);
          CladEnum enumValue;
          const bool success = EnumFromString(enumAsString, enumValue);
          ANKI_VERIFY(success, "CladEnumToStringMap.Load.EnumFromStringFailure", 
                      "%s did not match enum value", enumAsString.c_str());
          if(success){
            _cladMap.emplace(enumValue, std::move(filePath));
          }else{
            allStringsLoaded = false;
          } // else(success)
        }else{
          allStringsLoaded = false;
        } // else(memberNames.size() == 2)
      } // end for numPairs
    }
  }
  
  return allStringsLoaded;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
std::vector<CladEnum> CladEnumToStringMap<CladEnum>::GetAllKeys() const
{
  std::vector<CladEnum> outVector;
  for(auto& pair: _cladMap){
    outVector.push_back(pair.first);
  }

  return outVector;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
std::string CladEnumToStringMap<CladEnum>::GetValue(CladEnum enumValue) const
{
  auto retVal = _cladMap.find(enumValue);
  if(retVal == _cladMap.end())
  {
    PRINT_NAMED_WARNING("CladEnumToStringMap.GetValue",
                        "Clad enum '%s' does not map to file",
                        EnumToString(enumValue));
    return "";
  }
  
  PRINT_CH_DEBUG("Unfiltered", "GetValueForCladFound",
                 "%s -> %s",
                 EnumToString(enumValue),
                 retVal->second.c_str());
  
  return retVal->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
bool CladEnumToStringMap<CladEnum>::GetKeyForValue(const std::string& value, CladEnum& outKey, bool shouldCacheLookupMap)
{
  if(shouldCacheLookupMap && _reverseLookupMap.empty()){
    const auto& allKeys = GetAllKeys();
    for(const auto& key: allKeys){
      _reverseLookupMap.emplace(GetValue(key), key);
    }
  }
  return GetKeyForValueConst(value, outKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
bool CladEnumToStringMap<CladEnum>::GetKeyForValueConst(const std::string& value, CladEnum& outKey) const
{
  if(!_reverseLookupMap.empty()){
    auto resultPair = _reverseLookupMap.find(value);
    const bool success = resultPair != _reverseLookupMap.end();
    if(success){
      outKey = resultPair->second;
    }
    return success;
  }else{
    const auto& allKeys = GetAllKeys();
    for(const auto& key: allKeys){
      if(GetValue(key) == value){
        outKey = key;
        return true;
      }
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
bool CladEnumToStringMap<CladEnum>::HasKey(CladEnum enumValue) const
{
  auto retVal = _cladMap.find(enumValue);
  return retVal != _cladMap.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
bool CladEnumToStringMap<CladEnum>::Erase(CladEnum ev)
{
  return (_cladMap.erase(ev) > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class CladEnum>
void CladEnumToStringMap<CladEnum>::UpdateValue(CladEnum ev, const std::string& newValue)
{
  _cladMap[ev] = newValue;
}



} // namespace Util
} // namespace Anki

#endif // __Util_CladHelpers_CladEnumToStringMap_H__
