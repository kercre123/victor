/**
 * File: wifiConfigure.h
 *
 * Author: Lee Crippen
 * Created: 03/21/16
 *
 * Description: Handles the ios specific http server functionality for wifi configuration.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __iOS_WifiConfigure_H__
#define __iOS_WifiConfigure_H__

#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class WifiConfigure : Util::noncopyable
{
public:
  WifiConfigure();
  virtual ~WifiConfigure();
  
  bool UpdateMobileconfig(const char* wifiSSID, const char* wifiPasskey);
  void InstallMobileconfig();
  
private:
  struct WifiConfigureImpl;
  std::unique_ptr<WifiConfigureImpl> _impl;
  
}; // class WifiConfigure
} // namespace Cozmo
} // namespace Anki


#endif // __iOS_WifiConfigure_H__
