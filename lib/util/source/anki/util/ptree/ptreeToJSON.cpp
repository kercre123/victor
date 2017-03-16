//
//  ptreeToJSON.cpp
//  util
//
//  Created by Mark Pauley on 6/17/15.
//
//

#include "ptreeToJSON.h"
#include "util/ptree/includePtree.h"
#include "util/math/numericCast.h"
#include <json/json.h>
#include <regex>

namespace Anki {
namespace Util {

#pragma mark - Type Detectors
struct BoolTranslator
{
  // Converts a string to bool
  boost::optional<bool> get_value(const std::string& str)
  {
    static const std::regex bool_true { R"(^true$)", std::regex::icase };
    static const std::regex bool_false { R"(^false$)", std::regex::icase };
    
    if (!str.empty()) {
      if (std::regex_match(str, bool_true)) {
        return boost::optional<bool>(true);
      }
      else if(std::regex_match(str, bool_false)) {
        return boost::optional<bool>(false);
      }
    }
    return boost::optional<bool>(boost::none);
  }
  
  boost::optional<std::string> put_value(const bool& b)
  {
    return boost::optional<std::string>(b ? "true" : "false");
  }
};

struct UnsignedIntTranslator
{
  // Converts a string to unsigned int
  boost::optional<Json::UInt64> get_value(const std::string& str)
  {
    static const std::regex int_pattern = std::regex { R"(^[0-9]+$)" };
    
    if (!str.empty() && std::regex_match(str, int_pattern)) {
      return boost::optional<Json::UInt64>(numeric_cast<Json::UInt64>(std::stoull(str)));
    }
    return boost::optional<Json::UInt64>(boost::none);
  }
  
  boost::optional<std::string> put_value(const Json::UInt64& i)
  {
    return boost::optional<std::string>(std::to_string(i));
  }
};

struct IntTranslator
{
  // Converts a string to signed int
  boost::optional<Json::Int64> get_value(const std::string& str)
  {
    static const std::regex int_pattern = std::regex { R"(^-[0-9]+$)" };
    if (!str.empty()) {
      if (std::regex_match(str, int_pattern)) {
        return boost::optional<Json::Int64>(std::stoll(str));
      }
    }
    return boost::optional<Json::Int64>(boost::none);
  }
  
  boost::optional<std::string> put_value(const Json::Int64& i)
  {
    return boost::optional<std::string>(std::to_string(i));
  }
};
  
struct HexStringTranslator
{
  // checks if a string fits the 0x... hex pattern, and if so lets it through as a string (no conversion)
  boost::optional<std::string> get_value(const std::string& str)
  {
    static const std::regex hex_pattern = std::regex { R"(^0x[0-9a-fA-F]+$)" };
    if (!str.empty() && std::regex_match(str, hex_pattern)) {
      return boost::optional<std::string>(str);
    }
    return boost::optional<std::string>(boost::none);
  }
  
  boost::optional<std::string> put_value(const std::string& i)
  {
    return boost::optional<std::string>(i);
  }
};

#pragma mark - Value Conversion
static inline bool _PTreeConvertValue(const boost::property_tree::ptree& input, Json::Value& output)
{
  // ptree only stores things as strings, we need to tease the type out
  //  by using a type-sieve.
  
  // Check for a boolean
  {
    Util::BoolTranslator boolDetector;
    auto boolOpt = input.get_value_optional<bool>(boolDetector);
    if (boolOpt) {
      output = boolOpt.get();
      return true;
    }
  }
  
  // Check for an unsigned number
  {
    Util::UnsignedIntTranslator uintDetector;
    auto uintOpt = input.get_value_optional<Json::UInt64>(uintDetector);
    if (uintOpt) {
      output = uintOpt.get();
      return true;
    }
  }
  
  // Check for a signed number
  {
    Util::IntTranslator intDetector;
    auto intOpt = input.get_value_optional<Json::Int64>(intDetector);
    if(intOpt) {
      output = intOpt.get();
      return true;
    }
  }
  
  // Check for a hex string
  {
    Util::HexStringTranslator hexDetector;
    auto hexOpt = input.get_value_optional<std::string>(hexDetector);
    if(hexOpt) {
      output = hexOpt.get();
      return true;
    }
  }
  
  // Check for a real number
  {
    auto realOpt = input.get_value_optional<double>();
    if(realOpt) {
      output = realOpt.get();
      return true;
    }
  }
  
  // Fall back to string
  auto stringOpt = input.get_value_optional<std::string>();
  if(stringOpt) {
    output = input.get_value<std::string>();
    return true;
  }
  return false;
}

#pragma mark - Public Entrypoints
// Recursively convert PTree objects to json.cpp objects
// Completely wipes out whatever value is in output.
bool PTreeToJSON(const boost::property_tree::ptree& input, Json::Value& output)
{
  if (input.empty()) {
    // Convert Value (no children)
    _PTreeConvertValue(input, output);
    return true;
  }
  else if(input.count("") > 0) {
    // Convert array (indexed by int)
    output = Json::Value(Json::arrayValue);
    for(const auto& pair : input) {
      if(pair.first == "") { // limit
        Json::Value arrayMember(Json::nullValue);
        if(PTreeToJSON(pair.second, arrayMember)) {
          //TODO: use std::move
          output.append(arrayMember);
        }
        else {
          return false;
        }
      }
    }
    return true;
  }
  else {
    // Convert Object (indexed by key)
    output = Json::Value(Json::objectValue);
    for(const auto& pair : input) {
      //TODO: remove all temp objects created here by using r-values
      Json::Value childValue(Json::nullValue);
      if(PTreeToJSON(pair.second, childValue)) {
        //TODO: use std::move
        assert("" != pair.first);
        assert(!childValue.isNull());
        output[pair.first] = childValue;
      }
      else {
        return false;
      }
    }
    return true;
  }
  return false;
}

// Convenience r-value creator
Json::Value PTreeToJSON(const boost::property_tree::ptree& input)
{
  Json::Value result { Json::objectValue };
  PTreeToJSON(input, result);
  return result;
}

// Recursively convert json.cpp objects into ptree objects
//  please only use this if it is too much work to convert a class already depending on ptree
bool JSONToPTree(const Json::Value& input, boost::property_tree::ptree& output)
{
  output.clear();
  
  switch (input.type()) {
    case Json::nullValue:
      return false; // don't support null values in ptree
      
    case Json::intValue:
      output.put_value(input.asLargestInt());
      return true;
      
    case Json::uintValue:
      output.put_value(input.asLargestUInt());
      return true;
    
    case Json::realValue:
      output.put_value(input.asDouble());
      return true;
    
    case Json::stringValue:
      output.put_value(input.asString());
      return true;
      
    case Json::booleanValue:
      output.put_value(input.asBool());
      return true;
      
    case Json::arrayValue:
      for(const Json::Value& arrayChild : input) {
        boost::property_tree::ptree arrayChildTree;
        JSONToPTree(arrayChild, arrayChildTree);
        output.push_back(std::make_pair("", arrayChildTree));
      }
      return true;
      
    case Json::objectValue:
      for(const std::string& key : input.getMemberNames()) {
        const Json::Value& child = input[key];
        boost::property_tree::ptree objectChildTree;
        JSONToPTree(child, objectChildTree);
        output.put_child(key, objectChildTree);
      }
      return true;
  }
  
  return false;
}

// Convenience r-value creator
boost::property_tree::ptree JSONToPTree(const Json::Value& input)
{
  boost::property_tree::ptree result;
  JSONToPTree(input, result);
  return result;
}
  
}
}
