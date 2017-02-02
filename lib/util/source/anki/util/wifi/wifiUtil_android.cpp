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

#include "util/wifi/wifiUtil_android.h"

#ifdef ANKI_PLATFORM_ANDROID

#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/signals/simpleSignal.hpp"
#include "util/transport/udpTransport.h"

namespace {

std::string _ssid;
std::string _status;
Signal::Signal<void()> _signal;

void BindTransportToNetwork(Anki::Util::UDPTransport* transport) {
  if (transport->IsConnected()) {
    transport->ResetSocket();
    PRINT_NAMED_INFO("WifiUtil.BindTransport", "reset socket %d", transport->GetSocketId());
  }
}


}

namespace Anki {
namespace Util {

std::string WifiUtil::GetSSID()
{
  return _ssid;
}

std::string WifiUtil::GetConnectionStatus()
{
  return _status;
}

Signal::SmartHandle WifiUtil::RegisterTransport(UDPTransport* transport)
{
  return _signal.ScopedSubscribe(std::bind(&BindTransportToNetwork, transport));
}

}
}

extern "C"
{

void JNICALL
Java_com_anki_util_WifiUtil_NativeScanCallback(JNIEnv* env, jobject clazz,
                                                    jobjectArray ssids, jintArray levels)
{

}

void JNICALL
Java_com_anki_util_WifiUtil_NativeStatusCallback(JNIEnv* env, jobject clazz,
                                                      jstring ssid, jstring status)
{
  using namespace Anki::Util;
  _ssid = JNIUtils::getStringFromJString(env, ssid);
  _status = JNIUtils::getStringFromJString(env, status);
}

void JNICALL
Java_com_anki_util_WifiUtil_NativeBindNetworkCallback(JNIEnv* env, jobject thiz, jlong netId)
{
  // todo: use NDK R13 functionality to bind to netId?
  _signal.emit();
}

void JNICALL
Java_com_anki_util_WifiUtil_NativeBindLollipopCallback(JNIEnv* env, jobject thiz)
{
  // android 5.0 can't send us a network handle
  _signal.emit();
}

}

#endif
