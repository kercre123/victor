/**
 * File: wifi.h
 *
 * Author: Mathew Prokos
 * Created: 7/10/2018
 *
 * Description: Routines for scanning and configuring Wifi
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Platform_Switchboard_Anki_Wifi_Wifi_H__
#define __Platform_Switchboard_Anki_Wifi_Wifi_H__

#include <map>
#include <string>
#include <vector>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <glib.h>
#include <NetworkManager.h>

namespace Anki {
namespace Wifi {

    enum WifiIpFlags : uint8_t {
      NONE     = 0,
      HAS_IPV4 = 1 << 0,
      HAS_IPV6 = 1 << 1,
      HAS_BOTH = HAS_IPV4 | HAS_IPV6,
    };

    inline WifiIpFlags operator|(WifiIpFlags a, WifiIpFlags b) {
      return static_cast<WifiIpFlags>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum WifiAuth : uint8_t {
        AUTH_NONE_OPEN       = 0,
        AUTH_NONE_WEP        = 1,
        AUTH_NONE_WEP_SHARED = 2,
        AUTH_IEEE8021X       = 3,
        AUTH_WPA_PSK         = 4,
        AUTH_WPA_EAP         = 5,
        AUTH_WPA2_PSK        = 6,
        AUTH_WPA2_EAP        = 7
        };
    enum WifiConnState : uint8_t {
      UNKNOWN       = 0,
        ONLINE        = 1,
        CONNECTED     = 2,
        DISCONNECTED  = 3,
        };

    enum WifiScanErrorCode : uint8_t {
      SUCCESS                   = 0,
        ERROR_GETTING_PROXY       = 100,
        ERROR_SCANNING            = 101,
        FAILED_SCANNING           = 102,
        ERROR_GETTING_MANAGER     = 103,
        ERROR_GETTING_SERVICES    = 104,
        FAILED_GETTING_SERVICES   = 105,
        };

    enum ConnectWifiResult : uint8_t {
      CONNECT_NONE = 255,
        CONNECT_SUCCESS = 0,
        CONNECT_FAILURE = 1,
        CONNECT_INVALIDKEY = 2,
        };

    class WifiScanResult {
    public:
      WifiAuth    auth;
      bool        encrypted;
      bool        wps;
      uint8_t     signal_level;
      std::string ssid;
      bool        hidden;
      bool        provisioned;
    };

    class WifiConfig {
    public:
      WifiAuth auth;
      bool     hidden;
      std::string ssid; /* hexadecimal representation of ssid name */
      std::string passphrase;
    };

    static const unsigned MAX_NUM_ATTEMPTS = 5;

    struct WifiState {
      std::string ssid;
      WifiConnState connState;
    };

    struct ConnectAsyncData {
      bool completed;
      GCond *cond;
      GError *error;
      GCancellable *cancellable;
    };

    struct ConnectInfo {
      GCond* cond;
      GError* error;
    };

    struct WPAConnectInfo {
      const char *name;
      const char *ssid;
      const char *passphrase;

      guint agentId;
      GDBusConnection *connection;
      bool errRetry;
      uint8_t retryCount;
      ConnectWifiResult status;
    };

    class Wifi {
    public:
      Wifi();
      ~Wifi();

      void Initialize();

      ConnectWifiResult ConnectWifiBySsid(std::string ssid_str,
                                          std::string pw, WifiAuth auth, bool hidden);
      bool DisableAccessPointMode();
      bool EnableAccessPointMode(std::string ssid, std::string pw);
      WifiIpFlags GetIpAddress(struct in_addr* ipv4_32bits, struct in6_addr* ipv6_128bits);
      void GetWifiState(WifiState &wifiState);
      bool IsAccessPointMode();
      WifiScanErrorCode ScanForWifiAccessPoints(std::vector<WifiScanResult>& results);
      bool RemoveWifiService(std::string ssid);
      bool RemoveWifiServiceAll();

    protected:
      bool HaveMatchingConnection(NMClient *client, NMAccessPoint *ap);
      bool RemoveWifiService(NMClient *client, std::string ssid);
      WifiAuth APSecurity2WifiAuth(const NM80211ApSecurityFlags a);
      std::string WifiAuth2Auth(WifiAuth a);
      std::string WifiAuth2Key(WifiAuth a);
      bool reloadConnections(NMClient *);
      void InitializeClient(NMClient *);
      std::string _ap_ssid_hex;
    };
  }// namespace Wifi
} // namespace Anki

#endif //__Platform_Switchboard_Anki_Wifi_Wifi_H__
