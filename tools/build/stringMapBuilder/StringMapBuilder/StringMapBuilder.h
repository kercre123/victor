//
//  StringMapBuilder.h
//  StringMapBuilder
//
//  Created by damjan stulic on 10/26/13.
//  Copyright (c) 2013 damjan stulic. All rights reserved.
//

#ifndef __StringMapBuilder__StringMapBuilder__
#define __StringMapBuilder__StringMapBuilder__

#include <string>
#include <map>
#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace boost::property_tree;

typedef map<string, short> StringToInt;


struct ListAndKey {
  string listKey;
  string nestedListKey;
  string idKey;
  string searchForKey;
};


/**
 * Utility class for writing string maps and testing their validity
 *
 * @author   damjan
 */
class StringMapBuilder {
public:
  static bool Build(const string& configRoot, const string& workRoot, const string& configJson, const bool testOnly);
private:
  
  struct MapChangeInfo
  {
    MapChangeInfo();
    MapChangeInfo& operator |= (const MapChangeInfo& rhs);
    
    bool MadeAnyChanges() const;
    bool HadAnyErrors()   const;

    bool _keysHaveBeenAdded;
    bool _missingKeysHaveBeenRemoved;
    bool _missingKeysAreStillPresent;
    bool _foundDuplicateKeys;
    bool _failedToPopulate;
  };
  
  static void AddMapForKey(const StringToInt& map, const string& key, ptree &target);
  static MapChangeInfo CompareMaps(StringToInt &map, string key, ptree config, bool removeMissingKeys);
  static bool PopulateMap(StringToInt &map, const ptree& config, const string& listKey, const string& idKey, bool allowMultipleIdOcurrences);
  static bool PopulateMapWithSearchKey(StringToInt &map, const ptree& config, const string& searchForKey, bool allowMultipleIdOcurrences);
  static bool PopulateNestedMap(StringToInt &map, const ptree& config, const string& listKey, const string& nestedListKey, const string& idKey, bool allowMultipleIdOcurrences);
  static bool PopulateMapFromDefFile(StringToInt &map, const string& fileName, const string& delimiters, const uint32_t fieldIndex, bool allowMultipleIdOcurrences);
  static bool ReadPtreeFromFile(const string& fileName, ptree &config);
  static bool ParseListKeyNode(ptree &ptreeNode, ListAndKey &listKeyNode);
};


#endif /* defined(__StringMapBuilder__StringMapBuilder__) */
