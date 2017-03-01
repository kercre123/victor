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

#ifndef __Util_Logging_ChannelFilter_H__
#define __Util_Logging_ChannelFilter_H__

#include "iChannelFilter.h"
#include "util/console/consoleInterface.h"
#include "util/helpers/noncopyable.h"
#include "json/json.h"

#include <string>
#include <unordered_map>

namespace Anki {
namespace Util {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ChannelVar: console var
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct ChannelVar {
  ChannelVar(const std::string& name, bool defaultEnable)
  : enable(defaultEnable)
  , cvar(enable, name.c_str()
  , "Channels") {}
  
  bool enable; // variable where the value is stored
  ConsoleVar<bool> cvar; // consoleVar provider (for console var menu) that links to the storage variable 'enable'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ChannelFilter
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ChannelFilter : public IChannelFilter, Anki::Util::noncopyable {
public:
  ChannelFilter() : _initialized(false){}
  ~ChannelFilter();
  
  // initialize with an optinal json configuration (can be empty)
  void Initialize(const Json::Value& config = Json::Value());
  inline bool IsInitialized() const{ return _initialized; }
  
  void EnableChannel(const std::string& channelName);
  void DisableChannel(const std::string& channelName);
  void RegisterChannel(const std::string& channelName, bool defaultEnableStatus);
  bool IsChannelRegistered(const std::string& channelName) const;
  
  // IChannelFilter API
  virtual bool IsChannelEnabled(const std::string& channelName) const override;
  
private:
  std::unordered_map<std::string, ChannelVar*> _channelEnableList;
  bool _initialized;
};

} // namespace Util
} // namespace Anki

#endif /* defined(__Util_Logging_ChannelFilter_H__) */
