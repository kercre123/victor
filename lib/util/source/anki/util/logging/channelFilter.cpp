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
#include <string>
#include <map>

namespace Anki {
namespace Util {

const char* kChannelListKey = "channels";
const char* kChannelNameKey = "channel";
const char* kChannelEnabledKey = "enabled";

ChannelFilter::~ChannelFilter()
{
  for(auto c : _channelEnableList) {
    delete c.second;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ChannelFilter::Initialize(const Json::Value& config)
{
  // always register the default channel enabled by default (this can be overridden by config passed in)
  RegisterChannel(DEFAULT_CHANNEL_NAME, true);

  // parse config
  if (!config.isNull()) {
    for (const auto& channel : config[kChannelListKey]) {
    
      // parse channel name
      DEV_ASSERT(channel[kChannelNameKey].isString(), "ChannelFilter.Initialize.BadName");
      const std::string& channelName = channel[kChannelNameKey].asString();

      // parse value
      DEV_ASSERT(channel[kChannelEnabledKey].isBool(), "ChannelFilter.Initialize.BadEnableFlag");
      const bool channelEnabled = channel[kChannelEnabledKey].asBool();
      
      // Register channel
      RegisterChannel(channelName, channelEnabled);
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
  // storage is case insensitive
  std::string channelNameLowerCase = channelName;
  std::transform(channelNameLowerCase.begin(), channelNameLowerCase.end(), channelNameLowerCase.begin(), ::tolower);

  // if found, set as true (do not register if not found)
  auto it = _channelEnableList.find(channelNameLowerCase);
  if(it != _channelEnableList.end()) {
    it->second->enable = true;
  } else {
    PRINT_NAMED_WARNING("ChannelFilter.EnableChannel.ChannelNotFound",
      "Requested to enable channel '%s' (not found)",
      channelName.c_str());
  }
}

void ChannelFilter::DisableChannel(const std::string& channelName)
{
  // storage is case insensitive
  std::string channelNameLowerCase = channelName;
  std::transform(channelNameLowerCase.begin(), channelNameLowerCase.end(), channelNameLowerCase.begin(), ::tolower);

  // registerif found, set as false
  auto it = _channelEnableList.find(channelNameLowerCase);
  if(it != _channelEnableList.end()) {
    it->second->enable = false;
  } else {
    PRINT_NAMED_WARNING("ChannelFilter.DisableChannel.ChannelNotFound",
      "Requested to disable channel '%s' (not found)",
      channelName.c_str());
  }
}

void ChannelFilter::RegisterChannel(const std::string& channelName, bool defaultEnableStatus)
{
  if(IsInitialized()) {
    PRINT_NAMED_ERROR("ChannelFilter.RegisterChannel", "ChannelFilter already initialized, don't register %s after.", channelName.c_str());
    return;
  }

  // storage is case insensitive
  std::string channelNameLowerCase = channelName;
  std::transform(channelNameLowerCase.begin(), channelNameLowerCase.end(), channelNameLowerCase.begin(), ::tolower);
  
  auto it = _channelEnableList.find(channelNameLowerCase);
  if(it == _channelEnableList.end()) {
    _channelEnableList.emplace(channelNameLowerCase, new ChannelVar(channelNameLowerCase, defaultEnableStatus));
  } else {
    // override
// Will surely show up every time we run. Do not spam
//    PRINT_CH_INFO("LOG", "ChannelFilter.RegisterChannel", "Channel '%s' already registered, setting value to '%s'.",
//      channelNameLowerCase.c_str(), defaultEnableStatus ? "true" : "false");
    it->second->enable = defaultEnableStatus;
  }
}
  
bool ChannelFilter::IsChannelRegistered(const std::string& channelName) const
{
  // storage is case insensitive
  std::string channelNameLowerCase = channelName;
  std::transform(channelNameLowerCase.begin(), channelNameLowerCase.end(), channelNameLowerCase.begin(), ::tolower);

  // registered if found in table
  auto it = _channelEnableList.find(channelNameLowerCase);
  return it != _channelEnableList.end();
}

bool ChannelFilter::IsChannelEnabled(const std::string& channelName) const
{
  // storage is case insensitive
  std::string channelNameLowerCase = channelName;
  std::transform(channelNameLowerCase.begin(), channelNameLowerCase.end(), channelNameLowerCase.begin(), ::tolower);

  // check if registered with a true value
  auto it = _channelEnableList.find(channelNameLowerCase);
  if(it != _channelEnableList.end()) {
    return it->second->enable;
  } else {
    return false;
  }
}

} // namespace Util
} // namespace Anki