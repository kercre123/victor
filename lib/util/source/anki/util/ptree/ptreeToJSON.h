//
//  ptreeToJSON.h
//  util
//
//  Created by Mark Pauley on 6/17/15.
//
//  Tools to convert ptree objects to
//   json_cpp objects (to quicken our release from ptree)

#ifndef UTIL_PTREETOJSON_H_
#define UTIL_PTREETOJSON_H_

#include <boost/property_tree/ptree_fwd.hpp>
#include <json/json-forwards.h>

namespace Anki {
namespace Util {
  
// ptree => json
bool PTreeToJSON(const boost::property_tree::ptree& input, Json::Value& output);
Json::Value PTreeToJSON(const boost::property_tree::ptree& input);

// json => ptree
// please only use this if it's too difficult to convert a class already depending on ptree.
bool JSONToPTree(const Json::Value& input, boost::property_tree::ptree& output);
boost::property_tree::ptree JSONToPTree(const Json::Value& input);
  
} // Util
} // Anki

#endif /* defined(UTIL_PTREETOJSON_H_) */
