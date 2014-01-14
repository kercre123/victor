/**
 * File: jsonTools.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-01-10
 *
 * Description: Utility functions for dealing with jsoncpp objects
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/basestation/jsonTools.h"
#include "json/json.h"
#include <vector>

using namespace std;

namespace Anki
{

namespace JsonTools
{

bool GetValueOptional(const Json::Value& config, const std::string& key, std::string& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asString();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, uint8_t& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asUInt();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, uint16_t& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asUInt();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, uint32_t& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asUInt();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, int8_t& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asInt();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, int16_t& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asInt();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, int32_t& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asInt();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, float& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asFloat();
  return true;
}

bool GetValueOptional(const Json::Value& config, const std::string& key, double& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;

  value = child.asDouble();
  return true;
}

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
    vector<Json::Value::iterator> Q1, Q2;
    Json::Value tree(config);

    for(Json::Value::iterator it = tree.begin();
        it != tree.end();
        ++it) {
      Q1.push_back(it);
    }

    for(unsigned int currDepth = 1; currDepth < maxDepth; ++currDepth) {
      Q2.clear();
      for(vector<Json::Value::iterator>::iterator qIt = Q1.begin();
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
    for(vector<Json::Value::iterator>::iterator qIt = Q1.begin();
        qIt != Q1.end();
        ++qIt) {
      if((**qIt).size() > 1)
        **qIt = "...";
    }

    Json::StyledWriter writer;
    std::cout<<"<note: limited printing depth to "<<maxDepth<<">\n";
    std::cout<<writer.write(tree)<<std::endl;
  }
}

}

}
