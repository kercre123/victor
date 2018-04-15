/**
 * File: wifi.h
 *
 * Author: seichert
 * Created: 1/22/2018
 *
 * Description: Routines for scanning and configuring WiFi
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#pragma once

#include "exec_command.h"
#include "connmanbus.h"

#include <map>
#include <string>
#include <vector>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

namespace Anki {

enum WiFiIpFlags : uint8_t {
  NONE     = 0,
  HAS_IPV4 = 1 << 0,
  HAS_IPV6 = 1 << 1,
};

inline WiFiIpFlags operator|(WiFiIpFlags a, WiFiIpFlags b) {
  return static_cast<WiFiIpFlags>(static_cast<int>(a) | static_cast<int>(b));
}

enum WiFiAuth : uint8_t {
      AUTH_NONE_OPEN       = 0,
      AUTH_NONE_WEP        = 1,
      AUTH_NONE_WEP_SHARED = 2,
      AUTH_IEEE8021X       = 3,
      AUTH_WPA_PSK         = 4,
      AUTH_WPA_EAP         = 5,
      AUTH_WPA2_PSK        = 6,
      AUTH_WPA2_EAP        = 7
};

enum WiFiConnState : uint8_t {
  UNKNOWN       = 0,
  ONLINE        = 1,
  CONNECTED     = 2,
  DISCONNECTED  = 3,
};

class WiFiScanResult {
 public:
  WiFiAuth    auth;
  bool        encrypted;
  bool        wps;
  uint8_t     signal_level;
  std::string ssid;
};

class WiFiConfig {
 public:
  WiFiAuth auth;
  bool     hidden;
  std::string ssid; /* hexadecimal representation of ssid name */
  std::string passphrase;
};

struct WiFiState {
  std::string ssid;
  WiFiConnState connState;
};

struct ConnectAsyncData {
  bool completed;
  GCond *cond;
  GError *error;
  GCancellable *cancellable;
  ConnManBusService *service;
};

struct WPAConnectInfo {
  const char *name;
  const char *ssid;
  const char *passphrase;

  guint agentId;
  GDBusConnection *connection;
  ConnManBusManager *manager;
};

std::string GetObjectPathForService(GVariant* service);
bool ConnectToWifiService(ConnManBusService* service);
bool DisconnectFromWifiService(ConnManBusService* service);
ConnManBusService* GetServiceForPath(std::string objectPath);
void SetWiFiConfig(std::string ssid, std::string password, WiFiAuth auth, bool isHidden);
std::string GetHexSsidFromServicePath(const std::string& servicePath);

bool ConnectWiFiBySsid(std::string ssid, std::string pw, uint8_t auth, bool hidden, GAsyncReadyCallback cb, gpointer userData);
std::vector<WiFiScanResult> ScanForWiFiAccessPoints();
std::vector<uint8_t> PackWiFiScanResults(const std::vector<WiFiScanResult>& results);
void EnableWiFiInterface(const bool enable, ExecCommandCallback callback);
std::map<std::string, std::string> UnPackWiFiConfig(const std::vector<uint8_t>& packed);
void SetWiFiConfig(const std::vector<WiFiConfig>& networks, ExecCommandCallback);
void HandleOutputCallback(int rc, const std::string& output);
bool GetIpFromHostName(char* hostname, char* ip);
bool IsAccessPointMode();
bool EnableAccessPointMode(std::string ssid, std::string pw);
bool DisableAccessPointMode();
WiFiIpFlags GetIpAddress(uint8_t* ipv4_32bits, uint8_t* ipv6_128bits);
WiFiState GetWiFiState();

} // namespace Anki
