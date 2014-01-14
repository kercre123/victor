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
#include "json/json-forwards.h"
#include <string>

namespace Anki
{

namespace JsonTools
{

// Accessors which return true if the value is set in arguments, or
// false otherwise
bool GetValueOptional(const Json::Value& config, const std::string& key, std::string& value);

bool GetValueOptional(const Json::Value& config, const std::string& key, uint8_t& value);
bool GetValueOptional(const Json::Value& config, const std::string& key, uint16_t& value);
bool GetValueOptional(const Json::Value& config, const std::string& key, uint32_t& value);
bool GetValueOptional(const Json::Value& config, const std::string& key, int8_t& value);
bool GetValueOptional(const Json::Value& config, const std::string& key, int16_t& value);
bool GetValueOptional(const Json::Value& config, const std::string& key, int32_t& value);

bool GetValueOptional(const Json::Value& config, const std::string& key, float& value);
bool GetValueOptional(const Json::Value& config, const std::string& key, double& value);

// Dump the json to stdout (pretty-printed). The depth argument limits
// the depth of the tree that is printed. It is 0 by default, which
// means to print the whole tree
void PrintJson(const Json::Value& config, int maxDepth = 0);


}

}

#endif
