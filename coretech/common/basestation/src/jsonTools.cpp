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

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/point_impl.h"

#include "json/json.h"
#include <iostream>
#include <vector>


namespace Anki
{

namespace JsonTools
{
  
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

  template<>
  uint32_t GetValue<uint32_t>(const Json::Value& node) {
    return node.asUInt();
  }

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
  
  
__attribute__((used)) void PrintJson(const Json::Value& config, int maxDepth)
{
  if(maxDepth == 0) {
    Json::StyledWriter writer;
    std::cout<<writer.write(config)<<std::endl;
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

    Json::StyledWriter writer;
    std::cout<<"<note: limited printing depth to "<<maxDepth<<">\n";
    std::cout<<writer.write(tree)<<std::endl;
  }
} // PrintJson()

} // namespace JsonTools

} // namespace Anki

