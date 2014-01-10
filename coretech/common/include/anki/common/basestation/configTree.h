/**
 * File: configTree.h
 *
 * Author: Brad Neuman
 * Created: 2014-01-06
 *
 * Description: Config tree implementation using jsoncpp library, but
 * done in a way to hide all of the calls so it should be easy to swap
 * out the underlying library if needed
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_COMMON_JSONCONFIGTREE_H_
#define _ANKICORETECH_COMMON_JSONCONFIGTREE_H_

#include "json/json-forwards.h"
#include <string>
#include <vector>

namespace Anki
{

class ConfigTree
{
public:

  ~ConfigTree();
  ConfigTree();

  // Read data from specified json file, returns true if it worked
  bool ReadJson(const std::string& filename);

  // leaf accessors. These only work if the ConfigTree is a leaf
  std::string AsString();
  int AsInt(); 
  float AsFloat();
  bool AsBool();

  // accessors. Return true if value set (in arguments), false
  // otherwise, looks up the given key
  bool GetValueOptional(const std::string& key, std::string& value);
  bool GetValueOptional(const std::string& key, int& value); 
  bool GetValueOptional(const std::string& key, float& value);
  bool GetValueOptional(const std::string& key, bool& value);

  // accessors with a default value
  std::string GetValue(const std::string& key, const std::string& defaultValue);
  std::string GetValue(const std::string& key, const char* defaultValue);
  int GetValue(const std::string& key, const int defaultValue); 
  float GetValue(const std::string& key, const float defaultValue);
  bool GetValue(const std::string& key, const bool defaultValue);

  // Index by a string to get a sub-tree
  // TODO:(bn) define if you can use dots in this name to traverse
  // multiple levels, probably not
  ConfigTree operator[](const std::string& key);

  // Index by an int, only valid if this node is a list
  ConfigTree operator[](unsigned int index);

  // Number of children is 0 for a leaf, nonzero if there are children
  unsigned int GetNumberOfChildren() const;

  // Returns a vector of keys at this node
  std::vector<std::string> GetKeys() const;

private:
  ConfigTree(Json::Value tree);

  Json::Value *tree_;
};

}


#endif

