/**
 * File: ChannelFilter
 *
 * Author: Zhetao Wang
 * Created: 05/19/2015
 *
 * Description: Class that provides a channel filter (for logs) that can load from json and provides console vars
 * to enable/disable log channels at runtime.
 *
 * Copyright: Anki, inc. 2015
 *
 */
#include "channelFilter.h"

#include "util/logging/logging.h"
#include "util/string/stringUtils.h"
#include <string>
#include <map>

namespace Anki {
namespace Util {

const char* kChannelListKey = "channels";
const char* kChannelNameKey = "channel";
const char* kChannelEnabledKey = "enabled";

ChannelFilter::~ChannelFilter()
{
  for(auto& c : _channelEnableList) {
    delete c.second;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ChannelFilter::Initialize(const Json::Value& config)
{
  // parse config
  if (!config.isNull()) {
    for (const auto& channel : config[kChannelListKey]) {
    
      // parse channel name
      DEV_ASSERT(channel[kChannelNameKey].isString(), "ChannelFilter.Initialize.BadName");
      const std::string& channelName = channel[kChannelNameKey].asString();

      // parse value
      DEV_ASSERT(channel[kChannelEnabledKey].isBool(), "ChannelFilter.Initialize.BadEnableFlag");
      const bool channelEnabled = channel[kChannelEnabledKey].asBool();
      _channelEnableList.emplace(channelName, new ChannelVar(channelName, channelEnabled, true));
    }
  }
  
  // Print which channels are enabled
  {
    std::stringstream enabledStr;
    std::stringstream disabledStr;
    int enCount = 0;
    int disCount = 0;
    for( const auto& pair : _channelEnableList ) {
      if ( pair.second->enable ) {
        enabledStr << ((++enCount==1) ? "":", ");
        enabledStr << "'" << pair.first << "'";
      } else {
        disabledStr << ((++disCount==1) ? "":", ");
        disabledStr << "'" << pair.first << "'";
      }
    }
    if ( enCount == 0 ) {
      enabledStr << "(None were enabled!)";
    }
    if ( disCount == 0 ) {
      disabledStr << "(None were disabled)";
    }
    PRINT_CH_INFO("LOG", "ChannelFilter.Channels", ": Enabled [%s]; Disabled [%s]",
      enabledStr.str().c_str(),
      disabledStr.str().c_str());
  }
  
  _initialized = true;
}

void ChannelFilter::EnableChannel(const std::string& channelName)
{
  // if found, set as true (do not register if not found)
  auto it = _channelEnableList.find(channelName);
  if(it != _channelEnableList.end()) {
    it->second->enable = true;
  } else {
    _channelEnableList.emplace(channelName, new ChannelVar(channelName, true, true));
  }
}

void ChannelFilter::DisableChannel(const std::string& channelName)
{
  // registerif found, set as false
  auto it = _channelEnableList.find(channelName);
  if(it != _channelEnableList.end()) {
    it->second->enable = false;
  } else {
    _channelEnableList.emplace(channelName, new ChannelVar(channelName, false, true));
  }
}

bool ChannelFilter::IsChannelEnabled(const std::string& channelName) const
{
  for(const auto& pair : _channelEnableList) {
    if (pair.second->enable) {
      if (pair.first == "*") {
        // wildcard match, match anything
        return true;
      } else if (StringEndsWith(pair.first, "*")) {
        // prefix match, match up to wildcard character
        const std::string match = pair.first.substr(0, pair.first.length() - 1);
        if (StringStartsWith(channelName, match)) {
          return true;
        }
      } else {
        // exact match
        if (pair.first == channelName) {
          return true;
        }
      }
    }
  }
  return false;
}

} // namespace Util
} // namespace Anki
