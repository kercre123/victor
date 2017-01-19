/**
 * File: wifiUtil_android
 *
 * Author: baustin
 * Created: 11/21/2016
 *
 * Description: Receives wifi info from Java to make available to C++
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Util_Wifi_WifiUtil_Android_H__
#define __Anki_Util_Wifi_WifiUtil_Android_H__

#include "util/helpers/ankiDefines.h"

#ifdef ANKI_PLATFORM_ANDROID

#include "util/jni/jniUtils.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Util {

class UDPTransport;

class WifiUtil {
public:
  static std::string GetSSID();
  static std::string GetConnectionStatus();
  static Signal::SmartHandle RegisterTransport(UDPTransport* transport) __attribute__((warn_unused_result));
};

}
}

#endif // ANKI_PLATFORM_ANDROID
#endif // __Anki_Util_Wifi_WifiUtil_Android_H__
