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

#ifndef __Util_Logging_ChannelFilter_H__
#define __Util_Logging_ChannelFilter_H__

#include <string>
#include <unordered_map>
#include <util/console/consoleInterface.h>

namespace Anki {
namespace Util {

struct ChannelVar {
  ChannelVar(const std::string& name, bool defaultEnable)
  : enable(defaultEnable)
  , cvar(enable, name.c_str()
  , "Channels") {}
  
  bool enable;
  ConsoleVar<bool> cvar;
};

class ChannelFilter {
public:
  ChannelFilter()
  : _initialized(false){}
  ~ChannelFilter();
  void Initialize();
  inline bool IsInitialized() const{ return _initialized; }
  void EnableChannel(const std::string& channelName);
  void DisableChannel(const std::string& channelName);
  void RegisterChannel(const std::string& channelName, bool defaultEnableStatus);
  bool IsChannelRegistered(const std::string& channelName) const;
  bool IsChannelEnabled(const std::string& channelName) const;
private:
  std::unordered_map<std::string, ChannelVar*> _channelEnableList;
  bool _initialized;
};

} // namespace Util
} // namespace Anki

#endif /* defined(__Util_Logging_ChannelFilter_H__) */
