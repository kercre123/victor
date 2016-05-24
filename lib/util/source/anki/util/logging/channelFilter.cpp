/**
 * File: ChannelFilter
 *
 * Author: Zhetao Wang
 * Created: 05/19/2015
 *
 * Description:
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

ChannelFilter::~ChannelFilter()
{
  for(auto c : _channelEnableList) {
    delete c.second;
  }
}

void ChannelFilter::Initialize()
{
  PRINT_NAMED_INFO("ChannelFilter.Initialize", "ChannelFilter initializing.");
  RegisterChannel(DEFAULT_CHANNEL_NAME, true);
  _initialized = true;
}

void ChannelFilter::EnableChannel(const std::string& channelName)
{
  auto it = _channelEnableList.find(channelName);
  if(it != _channelEnableList.end()) {
    it->second->enable = true;
  }
}

void ChannelFilter::DisableChannel(const std::string& channelName)
{
  auto it = _channelEnableList.find(channelName);
  if(it != _channelEnableList.end()) {
    it->second->enable = false;
  }
}

void ChannelFilter::RegisterChannel(const std::string& channelName, bool defaultEnableStatus)
{
  if(IsInitialized()) {
    PRINT_NAMED_ERROR("ChannelFilter.RegisterChannel", "ChannelFilter already initialized, don't register %s after.", channelName.c_str());
    return;
  }
  auto it = _channelEnableList.find(channelName);
  if(it == _channelEnableList.end()) {
    _channelEnableList.emplace(channelName, new ChannelVar(channelName, defaultEnableStatus));
  }
}
  
bool ChannelFilter::IsChannelRegistered(const std::string& channelName) const
{
  auto it = _channelEnableList.find(channelName);
  return it != _channelEnableList.end();
}

bool ChannelFilter::IsChannelEnabled(const std::string& channelName) const
{
  auto it = _channelEnableList.find(channelName);
  if(it != _channelEnableList.end()) {
    return it->second->enable;
  } else {
    return false;
  }
}

} // namespace Util
} // namespace Anki