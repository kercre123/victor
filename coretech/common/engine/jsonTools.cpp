/**
 * File: jsonTools.cpp
 *
 * Authors: Brad Neuman, Andrew Stein
 * Created: 2014-01-10
 *
 * Description: Utility functions for dealing with jsoncpp objects
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/point_impl.h"

#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#include "json/json.h"
#include <iostream>
#include <sstream>
#include <vector>


namespace Anki
{

namespace JsonTools
{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float ParseFloat(const Json::Value& config, const char* key, const std::string& debugName) {
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isNumeric(), (debugName + ".ParseFloat.NotValidFloat").c_str(), "%s", key);
  return val.asFloat();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t ParseUint8(const Json::Value& config, const char* key, const std::string& debugName) {
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isNumeric(), (debugName + ".ParseUint8.NotValidUint8").c_str(), "%s", key);
  return Anki::Util::numeric_cast<uint8_t>(val.asInt());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int8_t ParseInt8(const Json::Value& config, const char* key, const std::string& debugName) {
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isNumeric(), (debugName + ".ParseUint8.NotValidInt8").c_str(), "%s", key);
  return Anki::Util::numeric_cast<int8_t>(val.asInt());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int32_t ParseInt32(const Json::Value& config, const char* key, const std::string& debugName)
{
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isNumeric(), (debugName + ".ParseInt.NotValidInt32").c_str(), "%s", key);
  return val.asInt();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t ParseUInt32(const Json::Value& config, const char* key, const std::string& debugName)
{
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isNumeric(), (debugName + ".ParseUint.NotValidUInt32").c_str(), "%s", key);
  return val.asUInt();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ParseBool(const Json::Value& config, const char* key, const std::string& debugName) {
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isBool(), (debugName + ".ParseBool.NotValidBool").c_str(), "%s", key);
  return val.asBool();
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ParseString(const Json::Value& config, const char* key, const std::string& debugName) {
  const auto & val = config[key];
  DEV_ASSERT_MSG(val.isString(), (debugName + ".ParseString.NotValidString").c_str(), "%s", key);
  return val.asString();
};

  
// Specializations of GetValue for all types we care about

  // Floating point
  template<>
  float GetValue<float>(const Json::Value& node) {
    return node.asFloat();
  }
  
  template<>
  double GetValue<double>(const Json::Value& node) {
    return node.asDouble();
  }
  
  // Unsigned int
  template<>
  uint8_t GetValue<uint8_t>(const Json::Value& node) {
    return node.asUInt();
  }

  template<>
  uint16_t GetValue<uint16_t>(const Json::Value& node) {
    return node.asUInt();
  }

#if !defined(__ANDROID__) && !defined(VICOS)
  template<>
  uint32_t GetValue<uint32_t>(const Json::Value& node) {
    return node.asUInt();
  }
#endif //size_t is the same as uint32_t for arm

  // Signed int
  template<>
  int8_t GetValue<int8_t>(const Json::Value& node) {
    return node.asInt();
  }
  
  template<>
  int16_t GetValue<int16_t>(const Json::Value& node) {
    return node.asInt();
  }
  
  template<>
  int32_t GetValue<int32_t>(const Json::Value& node) {
    return node.asInt();
  }
 
  template<>
  size_t GetValue<size_t>(const Json::Value& node) {
    return node.asUInt();
  }
  
  // Char
  template<>
  char GetValue<char>(const Json::Value& node) {
    return static_cast<char>(node.asInt());
  }
  
  // String
  template<>
  std::string GetValue<std::string>(const Json::Value& node) {
    return node.asString();
  }
  
  // Boolean
  template<>
  bool GetValue<bool>(const Json::Value& node) {
    return node.asBool();
  }
  
  bool GetPoseOptional(const Json::Value& config, const std::string& key, Anki::Pose3d& pose)
  {
    bool retVal = false;
    
    const Json::Value& child = config[key];
    if(not child.isNull()) {
      Anki::Point3f axis, translation;
      if(child.isMember("Angle") &&
         GetPointOptional(child, "Axis", axis) &&
         GetPointOptional(child, "Translation", translation))
      {
        const Anki::Radians angle(child["Angle"].asFloat());
        pose.SetRotation(angle, axis);
        
        pose.SetTranslation(translation);
        
        retVal = true;
      }
    }
    
    return retVal;
  } // GetPoseOptional()
  
  
  bool GetColorOptional(const Json::Value& jsonRoot, const std::string& key, Anki::ColorRGBA& color)
  {
    bool retVal = false;
    if(jsonRoot.isMember(key)) {
      const Json::Value& node = jsonRoot[key];
      if(node.isString()) {
        color = NamedColors::GetByString(node.asString());
        retVal = true;
        
      } else if(node.isArray() && (node.size() == 3 || node.size() == 4)) {
        const f32 r = node[0].asFloat();
        const f32 g = node[1].asFloat();
        const f32 b = node[2].asFloat();
        
        const f32 a = node.size() == 4 ? node[3].asFloat() : -1.f;
        
        if(r <= 1.f && g <= 1.f && b <= 1.f) {
          color.SetR(r);
          color.SetG(g);
          color.SetB(b);
          if(a >= 0.f) {
            color.SetAlpha(a);
          }
        } else {
          color.r() = static_cast<u8>(r);
          color.g() = static_cast<u8>(g);
          color.b() = static_cast<u8>(b);
          if(a >= 0.f) {
            color.alpha() = static_cast<u8>(a);
          }
        }
        
        retVal = true;
        
      } else {
        PRINT_NAMED_WARNING("JsonTools.GetColorOptional", "Expecting color in Json to be a string or 3 or 4 element array");
      }
    } // if node.isMember(key)
    
    return retVal;
  }

  bool GetAngleOptional(const Json::Value& jsonRoot, const std::string& key, Radians& angle, bool storedInDegrees)
  {
    float storedVal;
    const bool found = GetValueOptional(jsonRoot, key, storedVal);
    if(found) {
      if(storedInDegrees) {
        angle = DEG_TO_RAD(storedVal);
      } else {
        angle = storedVal;
      }
    }
    return found;
  }
  
namespace {
  
__attribute__((used)) void PrintJsonInternal(const Json::Value& config, int maxDepth, const std::function<void(std::string)>& outputFunction)
{
  if(maxDepth == 0) {
    Json::StyledWriter writer;
    const std::string& outString = writer.write(config);
    outputFunction(outString);
  }
  else {
    // we need to replace everything below the max depth with "...",
    // using a breadth-first iteration because I don't want to be
    // iterating over things while I'm messing with their children
    std::vector<Json::Value::iterator> Q1, Q2;
    Json::Value tree(config);

    for(Json::Value::iterator it = tree.begin();
        it != tree.end();
        ++it) {
      Q1.push_back(it);
    }

    for(unsigned int currDepth = 1; currDepth < maxDepth; ++currDepth) {
      Q2.clear();
      for(std::vector<Json::Value::iterator>::iterator qIt = Q1.begin();
          qIt != Q1.end();
          ++qIt) {
        if((**qIt).size() > 1) {
          for(Json::Value::iterator it = (**qIt).begin();
              it != (**qIt).end();
              ++it) {
            Q2.push_back(it);
          }
        }
      }

      Q1.swap(Q2);
    }

    // now everything at the maxDepth is in Q1
    for(std::vector<Json::Value::iterator>::iterator qIt = Q1.begin();
        qIt != Q1.end();
        ++qIt) {
      if((**qIt).size() > 1)
        **qIt = "...";
    }

    // add warning and print
    {
      Json::StyledWriter writer;
      std::stringstream outputSS;
      outputSS<<"<note: limited printing depth to "<<maxDepth<<">\n";
      outputSS<<writer.write(tree);
      const std::string& outString = outputSS.str();
      outputFunction(outString);
    }
  }
} // PrintJsonInternal()

}; // annon namespace for internal linkage

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
__attribute__((used)) void PrintJsonCout(const Json::Value& config, int maxDepth)
{
  auto function = []( const std::string& str ) {
    std::cout << str << std::endl;
  };
  PrintJsonInternal(config, maxDepth, function);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
__attribute__((used)) void PrintJsonDebug(const Json::Value& config, const std::string& eventName, int maxDepth)
{
#if ALLOW_DEBUG_LOGGING
  auto function = [&eventName]( const std::string& str ) {
    PRINT_NAMED_DEBUG(eventName.c_str(), "\n%s", str.c_str());
  };
  PrintJsonInternal(config, maxDepth, function);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
__attribute__((used)) void PrintJsonInfo(const Json::Value& config, const std::string& eventName, int maxDepth)
{
  auto function = [&eventName]( const std::string& str ) {
    PRINT_NAMED_INFO(eventName.c_str(), "\n%s", str.c_str());
  };
  PrintJsonInternal(config, maxDepth, function);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
__attribute__((used)) void PrintJsonError(const Json::Value& config, const std::string& eventName, int maxDepth)
{
  auto function = [&eventName]( const std::string& str ) {
    PRINT_NAMED_ERROR(eventName.c_str(), "\n%s", str.c_str());
  };
  PrintJsonInternal(config, maxDepth, function);
}


} // namespace JsonTools

} // namespace Anki

