//
//  StringMapBuilder.cpp
//  StringMapBuilder
//
//  Created by damjan stulic on 10/26/13.
//  Copyright (c) 2013 damjan stulic. All rights reserved.
//

#include "StringMapBuilder.h"
#include "util/parsingConstants/parsingConstants.h"
#include <iostream>
#include <fstream>
#include "boost/property_tree/ptree.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <string>
#include <map>
#include <vector>
#include <boost/foreach.hpp>

#define PRINT_DEBUG_FLAG 0

using ptree = boost::property_tree::ptree;


StringMapBuilder::MapChangeInfo::MapChangeInfo()
  : _keysHaveBeenAdded(false)
  , _missingKeysHaveBeenRemoved(false)
  , _missingKeysAreStillPresent(false)
  , _foundDuplicateKeys(false)
  , _failedToPopulate(false)
{
}


StringMapBuilder::MapChangeInfo& StringMapBuilder::MapChangeInfo::operator |= (const StringMapBuilder::MapChangeInfo& rhs)
{
  _keysHaveBeenAdded          |= rhs._keysHaveBeenAdded;
  _missingKeysHaveBeenRemoved |= rhs._missingKeysHaveBeenRemoved;
  _missingKeysAreStillPresent |= rhs._missingKeysAreStillPresent;
  _foundDuplicateKeys         |= rhs._foundDuplicateKeys;
  _failedToPopulate           |= rhs._failedToPopulate;
  return *this;
}


bool StringMapBuilder::MapChangeInfo::MadeAnyChanges() const
{
  // Note: No changes are made by _foundDuplicateKeys
  return _keysHaveBeenAdded || _missingKeysHaveBeenRemoved;
}


bool StringMapBuilder::MapChangeInfo::HadAnyErrors() const
{
  // Note: Adding keys isn't considered an error
  return _missingKeysAreStillPresent || _foundDuplicateKeys || _failedToPopulate;
}


void StringMapBuilder::AddMapForKey(const StringToInt& map, const string& key, ptree &target)
{
  ptree data;
  for (StringToInt::const_iterator it = map.begin(); it != map.end(); it ++)
  {
    //cout << it->first << " -> " << it->second << "\n";
    ptree node;
    node.put(AnkiUtil::kP_STRING_ID, it->first);
    node.put(AnkiUtil::kP_INT_ID, it->second);
    data.push_back(make_pair("", node));
  }
  target.put_child(key, data);
}



// Note: CompareMaps doesn't just "compare", it fills in ids in map using any ids found in stringMap.json
StringMapBuilder::MapChangeInfo StringMapBuilder::CompareMaps(StringToInt &map, string key, ptree config, bool removeMissingKeys)
{
  StringMapBuilder::MapChangeInfo mapChangeInfo;
 
  int maxIntId = 0;
  
  // find old values and put them into new config
  boost::optional<ptree &> optionalList = config.get_child_optional(key);
  if(optionalList)
  {
    // get data out of ptree
    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, optionalList.get())
    {
      // v.second is ptree
      // get item id
      string stringId = v.second.get(AnkiUtil::kP_STRING_ID, AnkiUtil::kP_UNDEFINED);
      int intId = v.second.get(AnkiUtil::kP_INT_ID, -1);
      
      //cout << "looking for " << key << " " << stringId << " - " << intId << "\n";
      // save max value
      if (maxIntId < intId)
        maxIntId = intId;
      
      StringToInt::iterator it = map.find(stringId);
      if (it != map.end())
      {
        it->second = intId;
      }
      else
      {
        // a key in target is no longer present - e.g. a key has been removed, if removeMissingKeys is true this old key is kept for backwards compatability
        cout << "found missing key " << key << " . " << stringId << "\n";
        if (removeMissingKeys)
        {
          mapChangeInfo._missingKeysHaveBeenRemoved = true;
        }
        else
        {
          mapChangeInfo._missingKeysAreStillPresent = true;
          map[stringId] = intId;
        }
      }
    }
  }
  maxIntId ++;
  
  // now find missing int ids
  vector<int> skippedIntIds;
  bool usedIds[maxIntId];
  for (unsigned int i = 0; i < maxIntId; ++i)
  {
    usedIds[i] = false;
  }
  
  for (StringToInt::iterator it = map.begin(); it != map.end(); ++it)
  {
    if (it->second >= 0)
    {
      assert(it->second < maxIntId);
      if (usedIds[it->second])
      {
        // This id was already being used by another string!
        cout << "WARNING: found duplicate id " << it->second << " on: " << key << "." << it->first << " - previously used on:\n";
        {
          for (StringToInt::iterator it2 = map.begin(); it2 != it; ++it2)
          {
            if (it2->second == it->second)
            {
              cout << "  " << key << "." << it2->first << " = " << it2->second << "\n";
            }
          }
        }
        
        mapChangeInfo._foundDuplicateKeys = true;
      }
      else
      {
        usedIds[it->second] = true;
      }
    }
  }
  
  for (unsigned int i = 1; i < maxIntId; ++i)
  {
    if (!usedIds[i])
    {
      skippedIntIds.push_back(i);
    }
  }
  
  // now find new items in the map and assign them ids
  ptree data;
  for (StringToInt::iterator it = map.begin(); it != map.end(); it ++)
  {
    if (it->second < 0)
    {
      if (skippedIntIds.size() > 0 && removeMissingKeys)
      {
        // reuse old keys that have been removed
        it->second = skippedIntIds[0];
        skippedIntIds.erase(skippedIntIds.begin());
      }
      else
      {
        // new key
        it->second = maxIntId++;
      }
      
      mapChangeInfo._keysHaveBeenAdded = true;
      cout << "assigned new id to " << key << " . " << it->first << " = " << it->second << "\n";
    }
  }
  
  return mapChangeInfo;
}


// searches through the given ptree for entries with "key"
// if entry contains string data, or a list of string data, then those values are added to the map
bool StringMapBuilder::PopulateMapWithSearchKey(StringToInt &map, const ptree& config, const string& searchForKey, bool allowMultipleIdOcurrences)
{
  bool ok = true;

  // for every key in config
  for( auto& entry : config)
  {
    const std::string& key = entry.first;
    //cout << "searching through key [" << key << "]" << "\n";

    // if has data at node, and key matches,
    if (!entry.second.data().empty() && key == searchForKey) {

      // Verify duplicated ids
      if ( !allowMultipleIdOcurrences && map.find(entry.second.data()) != map.end() ) {
        cout << "WARNING: found duplicate id " << entry.second.data() << endl;
        ok = false;
      }
    
      // add data to map
      map[entry.second.data()] = -1;
      //cout << "found raw key [" << entry.second.data() << "]" << "\n";
    }

    // if has sub tree
    if ( !entry.second.empty() ) {
      // look for simple list elements in the subtree
      for( auto& subEntry : entry.second) {
        //cout << "searching through sub key [" << subEntry.first << "]" << "\n";
        // if sub entry, has no key and has no sub trees
        // consider it simple list, and check if the key matches
        if (subEntry.first.empty() && subEntry.second.empty()
          && !subEntry.second.data().empty()) {
          //cout << " simple list found at key " << key << "\n";

          if (key == searchForKey) {
          
            // Verify duplicated ids
            if ( !allowMultipleIdOcurrences && map.find(subEntry.second.data()) != map.end() ) {
              cout << "WARNING: found duplicate id " << entry.second.data() << endl;
              ok = false;
            }
          
            map[subEntry.second.data()] = -1;
            //cout << "found list key [" << subEntry.second.data() << "]" << "\n";
          }
        }
      }
      // now parse this subtree
      const bool okSubTree = PopulateMapWithSearchKey(map, entry.second, searchForKey, allowMultipleIdOcurrences);
      ok = ok && okSubTree;
    }

  }
  
  return ok;
}


bool StringMapBuilder::PopulateMap(StringToInt &map, const ptree& config, const string& listKey, const string& idKey, bool allowMultipleIdOcurrences)
{
  bool ok = true;

  // verify that keys exists, so that ptree does not throw exception
  boost::optional<const ptree &> optionalList = config.get_child_optional(listKey);
  if (optionalList)
  {
    BOOST_FOREACH(const ptree::value_type &v, *optionalList)
    {
    
      string id;
      if (idKey.empty())
      {
        // if missing, do not create an entry for the default id
        boost::optional<string> optionalValue = v.second.get_value_optional<string>();
        if ( !optionalValue ) {
          continue;
        }

        id = optionalValue.get();
      }
      else
      {
        // if missing, do not create an entry for the default id
        boost::optional<string> optionalKey = v.second.get_optional<string>(idKey);
        if ( !optionalKey ) {
          continue;
        }
  
        id = optionalKey.get();
      }
      
      // Verify duplicated ids
      if ( !allowMultipleIdOcurrences && map.find(id) != map.end() ) {
        cout << "WARNING: found duplicate id " << id << endl;
        ok = false;
      }
      
      map[id] = -1;
      //cout << "parsed " << listKey << " [" << idKey << "] " << " . " << id << "\n";
    }
  }
  
  return ok;
}


bool StringMapBuilder::PopulateNestedMap(StringToInt &map, const ptree& config, const string& listKey, const string& nestedListKey, const string& idKey, bool allowMultipleIdOcurrences)
{
  bool ok = true;

  // verify that keys exists, so that ptree does not throw exception
  boost::optional<const ptree &> optionalList = config.get_child_optional(listKey);
  if (optionalList)
  {
    BOOST_FOREACH(const ptree::value_type &v, *optionalList)
    {
      boost::optional<const ptree &> optionalNestedList = v.second.get_child_optional(nestedListKey);
      if (optionalNestedList)
      {
        BOOST_FOREACH(const ptree::value_type &nestedV, *optionalNestedList)
        {
          boost::optional<string> optionalid = nestedV.second.get_optional<string>(idKey);
          if (optionalid && !optionalid.get().empty())
          {
            // Verify duplicated ids
            if ( !allowMultipleIdOcurrences && map.find(optionalid.get()) != map.end() ) {
              cout << "WARNING: found duplicate id " << optionalid.get() << endl;
              ok = false;
            }
          
            map[optionalid.get()] = -1;
            //cout << "parsed " << listKey << " . " << nestedListKey << " . " << optionalid.get() << "\n";
          }
        }
      }
    }
  }
  
  return ok;
}


bool StringMapBuilder::PopulateMapFromDefFile(StringToInt &map, const string& fileName, const string& delimiters, const uint32_t fieldIndex, bool allowMultipleIdOcurrences)
{
  bool ok = true;

  // open file
  std::string line;
  std::ifstream myfile (fileName);
  if (myfile.is_open())
  {
    while ( std::getline (myfile,line) )
    {
      std::vector<std::string> words;
      {
        std::size_t prev = 0, pos;
        while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos)
        {
          if (pos > prev) {
            words.push_back(line.substr(prev, pos - prev));
          }
          prev = pos+1;
        }
        if (prev < line.length()) {
          words.push_back(line.substr(prev, std::string::npos));
        }
      }
      if (fieldIndex < words.size()) {
      
        // Verify duplicated ids
        if ( !allowMultipleIdOcurrences && map.find(words[fieldIndex]) != map.end() ) {
          cout << "WARNING: found duplicate id " << words[fieldIndex] << endl;
          ok = false;
        }
      
        //cout << "found string " << words[fieldIndex] << '\n';
        map[words[fieldIndex]] = -1;
      }
    }
    myfile.close();
  } else  {
    cout << "Unable to open file " << fileName << "\n";
  }
  return ok;
}


bool StringMapBuilder::ReadPtreeFromFile(const string& fileName, ptree &config)
{
  static map<string, ptree* > loadedPtrees;
  map<string, ptree* >::const_iterator it = loadedPtrees.find(fileName);
  if (it != loadedPtrees.end())
  {
    config = *it->second;
    return true;
  }

  if (PRINT_DEBUG_FLAG)
    cout << "reading config json from " << fileName << "\n";
  bool succesful = false;
  try {
    boost::property_tree::json_parser::read_json(fileName, config);
    loadedPtrees[fileName] = new ptree(config);
    succesful = true;
  }
  // catch c++ exceptions
  catch (exception& e)
  {
    cout << "Error " << e.what() << "\n";
  }
  // catch all other issues
  catch (...)
  {
    cout << "Error of unknown type\n";
  }
  return succesful;
}


bool StringMapBuilder::ParseListKeyNode(ptree &ptreeNode, ListAndKey &listKeyNode)
{
  // get nested list key
  boost::optional<string> optionalSearchForKey = ptreeNode.get_optional<string>("search_for_key");
  if (optionalSearchForKey) {
    listKeyNode.searchForKey = optionalSearchForKey.get();
    return true;
  }

  // get list key
  boost::optional<string> optionalListKey = ptreeNode.get_optional<string>("list_key");
  if (!optionalListKey)
  {
    cout << "source did not contain \"list_key\"\n";
    return false;
  }

  // get nested list key
  boost::optional<string> optionalNestedListKey = ptreeNode.get_optional<string>("nested_list_key");

  // get id key
  boost::optional<string> optionalIdKey = ptreeNode.get_optional<string>("id_key");
  if (!optionalIdKey)
  {
    cout << "source did not contain \"id_key\"\n";
    return false;
  }

  listKeyNode.listKey = optionalListKey.get();
  if (optionalNestedListKey)
    listKeyNode.nestedListKey = optionalNestedListKey.get();
  listKeyNode.idKey = optionalIdKey.get();

  return true;
}


bool StringMapBuilder::Build(const string& configRoot, const string& workRoot, const string& configJson, const bool testOnly)
{

  // parse config json
  ptree internalConfig;
  if (!ReadPtreeFromFile(configJson, internalConfig))
  {
    cout << "config file could not be opened\n";
    return false;
  }



  boost::optional<string> optionalTargetFile = internalConfig.get_optional<string>("target");
  if (!optionalTargetFile)
  {
    cout << "config file did not contain \"target\" key\n";
    return false;
  }

  // get exiting map data
  ptree targetStringMap;
  if (!ReadPtreeFromFile(configRoot + optionalTargetFile.get(), targetStringMap))
  {
    cout << "original string map file could not be opened [" << configRoot << optionalTargetFile.get() << "], creating from scratch\n";
  }
  
  StringMapBuilder::MapChangeInfo mapChangeInfo;
  ptree generatedMap;

  boost::optional<ptree &> optionalList = internalConfig.get_child_optional("sources");
  if (optionalList)
  {
    // for each source
    BOOST_FOREACH(ptree::value_type &listItem, *optionalList)
    {
      // get file, or files
      vector<string> files;
      boost::optional<string> optionalFileName = listItem.second.get_optional<string>("file");
      if (optionalFileName)
      {
        files.push_back(optionalFileName.get());
      }
      else
      {
        boost::optional<ptree &> optionalFileList = listItem.second.get_child_optional("files");
        if (optionalFileList)
        {
          BOOST_FOREACH(ptree::value_type &fileNode, *optionalFileList)
          {
            string fileName = fileNode.second.data();
            files.push_back(fileName);
          }
        }
      }

      if (files.size() == 0)
      {
        cout << "source did not contain any files to parse, make sure you include \"file\" or \"files\"\n";
        break;
      }


      vector<ListAndKey> listDefinitions;
      // if there is not "list_and_keys"
      boost::optional<ptree &> optionalListAndKeyList = listItem.second.get_child_optional("list_and_keys");
      if (optionalListAndKeyList)
      {
        BOOST_FOREACH(ptree::value_type &listKeyNode, *optionalListAndKeyList)
        {
          ListAndKey listAndKey;
          if (ParseListKeyNode(listKeyNode.second, listAndKey))
            listDefinitions.push_back(listAndKey);
        }
      }
      else
      {
        ListAndKey listAndKey;
        if (ParseListKeyNode(listItem.second, listAndKey))
          listDefinitions.push_back(listAndKey);
      }

      // get target key
      boost::optional<string> optionalTargetKey = listItem.second.get_optional<string>("string_map_target_key");
      if (!optionalTargetKey)
      {
        cout << "source did not contain \"string_map_target_key\"\n";
        break;
      }

      bool removeMissingKeys = listItem.second.get("remove_missing_keys", true);
      bool allowMultipleIdOcurrences = listItem.second.get("allow_multiple_id_ocurrences", false);

      if (PRINT_DEBUG_FLAG)
      {
        cout << "working on: \n";
        cout << "string_map_target_key [" <<  optionalTargetKey.get() << "]\n";
        cout << "remove_missing_keys [" <<  (removeMissingKeys ? "true" : "false") << "]\n";
        for (unsigned int j = 0; j < listDefinitions.size(); ++j)
        {
          if (listDefinitions[j].searchForKey.empty()) {
            cout << "{\n  list_key [" << listDefinitions[j].listKey << "]\n";
            if (!listDefinitions[j].nestedListKey.empty()) {
              cout << "  nested_list_key [" << listDefinitions[j].nestedListKey << "]\n";
            }
            cout << "  id_key [" << listDefinitions[j].idKey << "]\n}\n";
          } else {
            cout << "{\n  search_for_key [" << listDefinitions[j].searchForKey << "]\n}\n";
          }
        }
      }
      
      bool sourcesPopulateOk = true;
      
      StringToInt stringsToCheck;
      for (unsigned int i = 0; i < files.size(); ++i)
      {
        if (files[i].rfind(".def") != std::string::npos ) {
          //cout << "processing " << files[i] << " as .def file\n";
          string delims = "()";
          sourcesPopulateOk = PopulateMapFromDefFile(stringsToCheck, configRoot + files[i], delims, 1, allowMultipleIdOcurrences) && sourcesPopulateOk;
          continue;
        }
        ptree someConfig;
        if (!ReadPtreeFromFile(configRoot + files[i], someConfig))
        {
          // if we could not read a file do not even generate the stringMap.json, it could mess up keys because
          // the ids actually failed to load
          cout << "could not open file: [" << configRoot << files[i] << "]. Aborting stringMap generation...\n";
          return false;
        }

        for (unsigned int j = 0; j < listDefinitions.size(); ++j)
        {
          if (!listDefinitions[j].searchForKey.empty()) {
            sourcesPopulateOk = PopulateMapWithSearchKey(stringsToCheck, someConfig, listDefinitions[j].searchForKey, allowMultipleIdOcurrences) && sourcesPopulateOk;
          } else if (!listDefinitions[j].nestedListKey.empty()) {
            sourcesPopulateOk = PopulateNestedMap(stringsToCheck, someConfig, listDefinitions[j].listKey, listDefinitions[j].nestedListKey, listDefinitions[j].idKey, allowMultipleIdOcurrences) && sourcesPopulateOk;
          } else {
           sourcesPopulateOk = PopulateMap(stringsToCheck, someConfig, listDefinitions[j].listKey, listDefinitions[j].idKey, allowMultipleIdOcurrences) && sourcesPopulateOk;
          }
        }
      }
      
      if ( !sourcesPopulateOk )
      {
        mapChangeInfo._failedToPopulate = true;
        cout << "Maps for target '" << optionalTargetKey.get() << "' failed to populate!\n\n";
      }
      else
      {
        // compare data
        mapChangeInfo |= CompareMaps(stringsToCheck, optionalTargetKey.get(), targetStringMap, removeMissingKeys);
        
        // generate new config file
        AddMapForKey(stringsToCheck, optionalTargetKey.get(), generatedMap);

        if (PRINT_DEBUG_FLAG)
          cout << "\n\n";
      }
    }
  }
  else
  {
    if (PRINT_DEBUG_FLAG)
      cout << "nothing to do, config file did not contain \"sources\"\n";
  }

  if ( !mapChangeInfo.HadAnyErrors() )
  {
    if (mapChangeInfo.MadeAnyChanges())
    {
      if (testOnly)
      {
        cout << "test mode - not writing config json to " << configRoot << optionalTargetFile.get() << "\n";
      }
      else
      {
        string fileName = configRoot + optionalTargetFile.get();

        cout << "writing config json to " << fileName << "\n";
        try {
          boost::property_tree::json_parser::write_json(fileName, generatedMap, std::locale(), true);
        }
        // catch c++ exceptions
        catch (exception& e)
        {
          cout << "Error " << e.what() << "\n";
        }
        // catch all other issues
        catch (...)
        {
          cout << "Error of unknown type\n";
        }
      }
    }
    else
    {
      cout << "String Map Config OKAY\n";
    }
  } else {
      cout << "String Map Config UNMODIFIED due to previous errors\n";
  }

  const bool error = mapChangeInfo.HadAnyErrors();
  return !error; // return is positive
}

