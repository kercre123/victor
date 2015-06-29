/**
 * File: jsonTools.h
 *
 * Author: Brad Neuman / Andrew Stein
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
#include "anki/common/basestation/math/point.h"

namespace Anki
{
  // Forward declaration
  class Pose3d;
  class ColorRGBA;
  
  namespace JsonTools
  {
    
    // Define a specialization of this template (in jsonTools.cpp) for each
    // type T you want to be able to handle
    template<typename T>
    T GetValue(const Json::Value& node);
    
    // Gets a value by name.  Returns true if value set successfully, false otherwise.
    template<typename T>
    bool GetValueOptional(const Json::Value& config, const std::string& key, T& value);
    
    // Gets (fixed-length) array of values by name.  Returns true if values is set
    // successfully, false otherwise.
    template<typename T, size_t N>
    bool GetArrayOptional(const Json::Value& config, const std::string& key, std::array<T,N>& values);
    
    // Gets (variable-length) vector of values by name.  Returns true if values is
    // set successfully, false otherwise.
    template<typename T>
    bool GetVectorOptional(const Json::Value& config, const std::string& key, std::vector<T>& values);
    
    // Get an Anki::Point by name.  Returns true if the point is set successfully,
    // or false otherwise.
    template<typename T, size_t N>
    bool GetPointOptional(const Json::Value& node, const std::string& key, Anki::Point<N,T>& pt);
    
    // Get an Anki::Pose3d object by name (translation, rotation axis/angle).
    // Returns true if Pose is set successfully, false otherwise.
    bool GetPoseOptional(const Json::Value& node, const std::string& key, Anki::Pose3d& pose);
    
    // Get a ColorRGBA either by name, 4-element u8/float array [R G B A], or
    // 3-element u8/float array [R G B] (with A = 255/1.0).
    // NOTE: In the ambiguous case of [1 1 1 (1)], the values will be interpreted as
    // floats and the color will be white.
    bool GetColorOptional(const Json::Value& node, const std::string& key, Anki::ColorRGBA& color);
    
    // Dump the json to stdout (pretty-printed). The depth argument limits
    // the depth of the tree that is printed. It is 0 by default, which
    // means to print the whole tree
    void PrintJson(const Json::Value& config, int maxDepth = 0);
    
#if 0
#pragma mark --- Templated Implementations ---
#endif

    // TODO: Move implementations to a separate _impl.h file
    
    template<typename T>
    bool GetValueOptional(const Json::Value& config, const std::string& key, T& value)
    {
      const Json::Value& child(config[key]);
      if(child.isNull())
        return false;
      
      value = GetValue<T>(child);
      return true;
    }
    
    template<typename T, size_t N>
    bool GetArrayOptional(const Json::Value& config, const std::string& key, std::array<T,N>& values)
    {
      const Json::Value& child(config[key]);
      if(child.isNull() || not child.isArray() || child.size() != N)
        return false;
      
      for(uint32_t i=0; i<N; ++i) {
        values[i] = GetValue<T>(child[i]);
      }
      
      return true;
    }
    
    template<typename T>
    bool GetVectorOptional(const Json::Value& config, const std::string& key, std::vector<T>& values)
    {
      const Json::Value& child(config[key]);
      if(child.isNull() || not child.isArray())
        return false;
      
      values.reserve(child.size());
      for(auto const& element : child) {
        values.emplace_back(GetValue<T>(element));
      }
      
      return true;
    }
    
    template<typename T, size_t N>
    bool GetPointOptional(const Json::Value& node, const std::string& key, Anki::Point<N,T>& pt)
    {
      bool retVal = false;
      
      const Json::Value& jsonVec = node[key];
      if(not jsonVec.isNull() && jsonVec.isArray() && jsonVec.size()==N)
      {
        for(uint32_t i=0; i<N; ++i) {
          pt[i] = GetValue<T>(jsonVec[i]);
        }
        
        retVal = true;
      }
      
      return retVal;
    }
    
    
  } // namespace JsonTools
  
} // namespace Anki

#endif
