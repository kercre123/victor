/**
 * File: jsonTools.h
 *
 * Author: Brad Neuman
 * Created: 2014-01-10
 *
 * Description: Utility functions for dealing with jsoncpp objects
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_COMMON_JSONTOOLS_H_
#define _ANKICORETECH_COMMON_JSONTOOLS_H_

#include <stdint.h>
#include <string>
#include <array>

//#include "json/json-forwards.h"
#include "json/json.h" // can't just use forwards b/c of template for arrays

#include "anki/common/basestation/exceptions.h"

namespace Anki
{

namespace JsonTools
{
  
  // Define a specialization of this template for each type you want to be able
  // to handle
  template<typename T>
  T GetValue(const Json::Value& node) {
    CORETECH_THROW("Unknown type conversion for JSON value.");
  }
  
  // Gets a value by name.  Returns true if value set successfully, false otherwise.
  template<typename T>
  bool GetValueOptional(const Json::Value& config, const std::string& key, T& value)
  {
    const Json::Value& child(config[key]);
    if(child.isNull())
      return false;
    
    value = GetValue<T>(child);
    return true;
  }

  // Gets array of values by name.  Returns true if value set successfully, false otherwise.
  template<typename T, size_t N>
  bool GetValueArrayOptional(const Json::Value& config, const std::string& key, std::array<T,N>& value)
  {
    const Json::Value& child(config[key]);
    if(child.isNull() || not child.isArray() || child.size() != N)
      return false;
    
    for(int i=0; i<N; ++i) {
      value[i] = GetValue<T>(child[i]);
    }
    return true;
  }

  
  
// Dump the json to stdout (pretty-printed). The depth argument limits
// the depth of the tree that is printed. It is 0 by default, which
// means to print the whole tree
void PrintJson(const Json::Value& config, int maxDepth = 0);


}

}

#endif
