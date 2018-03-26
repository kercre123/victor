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

#include "coretech/common/engine/exceptions.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/colorRGBA.h"

namespace Anki
{
  // Forward declaration
  class Pose3d;
  class ColorRGBA;
  
  namespace JsonTools
  {
  
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Set of functions that asserts the expected key with the expected type to be present
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    // config: input json
    // key:  key to look for
    // debugName: name of place calling the function, to append to the assert event (for logging/debugging)
    // ParseX: find the given key in the given json and assert that it's present and matches the given type
    float ParseFloat(const Json::Value& config, const char* key, const std::string& debugName);
    uint8_t ParseUint8(const Json::Value& config, const char* key, const std::string& debugName);
    int8_t ParseInt8(const Json::Value& config, const char* key, const std::string& debugName);
    int32_t ParseInt32(const Json::Value& config, const char* key, const std::string& debugName);
    uint32_t ParseUInt32(const Json::Value& config, const char* key, const std::string& debugName);
    bool ParseBool(const Json::Value& config, const char* key, const std::string& debugName);
    std::string ParseString(const Json::Value& config, const char* key, const std::string& debugName);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
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
    
    // Get an angle back in Radians, possibly stored in degrees in the Json
    bool GetAngleOptional(const Json::Value& jsonRoot, const std::string& key, Radians& angle, bool storedInDegrees);
    
    // Get an array of ColorRGBA as u32 by name
    template<size_t N>
    bool GetColorValuesToArrayOptional(const Json::Value& jsonRoot,
                                       const std::string& key,
                                       std::array<u32, N>& arr,
                                       bool parseAsFloat = false);
    
    // Dump the json to the selected output (pretty-printed). The depth argument limits
    // the depth of the tree that is printed. It is 0 by default, which
    // means to print the whole tree
    void PrintJsonCout(const Json::Value& config, int maxDepth = 0);  // std::cout
    void PrintJsonDebug(const Json::Value& config, const std::string& eventName, int maxDepth = 0); // print_named_debug
    void PrintJsonInfo(const Json::Value& config, const std::string& eventName, int maxDepth = 0);  // print_named_info
    void PrintJsonError(const Json::Value& config, const std::string& eventName, int maxDepth = 0);  // print_named_error
    
    // Returns true if a key other than something in expectedKeys is found at the root level. sets
    // the badKey if an unexpected key is found
    bool HasUnexpectedKeys(const Json::Value& config, const std::vector<const char*>& expectedKeys, std::vector<std::string>& badKeys);
    
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
    
    template<size_t N>
    bool GetColorValuesToArrayOptional(const Json::Value& jsonRoot,
                                       const std::string& key,
                                       std::array<u32, N>& arr,
                                       bool parseAsFloat)
    {
      if(jsonRoot.isMember(key))
      {
        const Json::Value& values = jsonRoot[key];
        
        if(arr.size() != values.size())
        {
          PRINT_NAMED_ERROR("JsonTools.GetColorValuesToArrayOptional.DiffSizes",
                            "The json array and destination array are different sizes. Expected %zu, got %u. Key: %s",
                            arr.size(), values.size(), key.c_str());
          return false;
        }
        
        for(u8 i = 0; i < (int)arr.size(); ++i)
        {
          if(parseAsFloat){
            ColorRGBA color(values[i][0].asFloat(),
                            values[i][1].asFloat(),
                            values[i][2].asFloat(),
                            values[i][3].asFloat()
                            );
            arr[i] = color.AsRGBA();
          }else{
            ColorRGBA color(static_cast<u8>(values[i][0].asUInt()),
                            static_cast<u8>(values[i][1].asUInt()),
                            static_cast<u8>(values[i][2].asUInt()),
                            static_cast<u8>(values[i][3].asUInt())
                            );
            arr[i] = color.AsRGBA();
          }
        }
        return true;
      }
      return false;
    }
    
    
  } // namespace JsonTools
  
} // namespace Anki

#endif
