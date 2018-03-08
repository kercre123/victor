/**
 * File: wifi.cpp
 *
 * Author: seichert
 * Created: 1/22/2018
 *
 * Description: Routines for scanning and configuring WiFi
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "connmanbus.h"
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <sys/socket.h>
#include "exec_command.h"
#include "fileutils.h"
#include "log.h"
#include "wifi.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace Anki {

static int CalculateSignalLevel(const int strength, const int min, const int max, const int numLevels) {
  if (strength < min) {
    return 0;
  } else if (strength >= max) {
    return (numLevels - 1);
  } else {
    float inputRange = (max - min);
    float outputRange = numLevels;
    return (int)((float)(strength - min) * outputRange / inputRange);
  }
}

std::vector<WiFiScanResult> ScanForWiFiAccessPoints() {
  std::vector<WiFiScanResult> results;
  ConnManBusTechnology* tech_proxy;
  GError* error;

  error = nullptr;
  tech_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);
  if (error) {
    loge("error getting proxy for net.connman /net/connman/technology/wifi");
    return results;
  }

  gboolean success = conn_man_bus_technology_call_scan_sync(tech_proxy,
                                                            nullptr,
                                                            &error);
  g_object_unref(tech_proxy);
  if (error) {
    loge("error asking connman to scan for wifi access points");
    return results;
  }

  if (!success) {
    loge("connman failed to scan for wifi access points");
    return results;
  }

  ConnManBusManager* manager_proxy;
  manager_proxy = conn_man_bus_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/",
                                                              nullptr,
                                                              &error);
  if (error) {
    loge("error getting proxy for net.connman /");
    return results;
  }

  GVariant* services = nullptr;
  success = conn_man_bus_manager_call_get_services_sync(manager_proxy,
                                                        &services,
                                                        nullptr,
                                                        &error);
  g_object_unref(manager_proxy);
  if (error) {
    loge("Error getting services from connman");
    return results;
  }

  if (!success) {
    loge("connman failed to get list of services");
    return results;
  }

  for (gsize i = 0 ; i < g_variant_n_children(services); i++) {
    WiFiScanResult result{WiFiAuth::AUTH_NONE_OPEN, false, false, 0, ""};
    GVariant* child = g_variant_get_child_value(services, i);
    GVariant* attrs = g_variant_get_child_value(child, 1);
    bool type_is_wifi = false;
    bool iface_is_wlan0 = false;

    for (gsize j = 0 ; j < g_variant_n_children(attrs); j++) {
      GVariant* attr = g_variant_get_child_value(attrs, j);
      GVariant* key_v = g_variant_get_child_value(attr, 0);
      GVariant* val_v = g_variant_get_child_value(attr, 1);
      GVariant* val = g_variant_get_variant(val_v);
      const char* key = g_variant_get_string(key_v, nullptr);

      // Make sure this is a wifi service and not something else
      if (g_str_equal(key, "Type")) {
        if (g_str_equal(g_variant_get_string(val, nullptr), "wifi")) {
          type_is_wifi = true;
        } else {
          type_is_wifi = false;
          break;
        }
      }

      // Make sure this is for the wlan0 interface and not p2p0
      if (g_str_equal(key, "Ethernet")) {
        for (gsize k = 0 ; k < g_variant_n_children(val); k++) {
          GVariant* ethernet_attr = g_variant_get_child_value(val, k);
          GVariant* ethernet_key_v = g_variant_get_child_value(ethernet_attr, 0);
          GVariant* ethernet_val_v = g_variant_get_child_value(ethernet_attr, 1);
          GVariant* ethernet_val = g_variant_get_variant(ethernet_val_v);
          const char* ethernet_key = g_variant_get_string(ethernet_key_v, nullptr);
          if (g_str_equal(ethernet_key, "Interface")) {
            if (g_str_equal(g_variant_get_string(ethernet_val, nullptr), "wlan0")) {
              iface_is_wlan0 = true;
            } else {
              iface_is_wlan0 = false;
              break;
            }
          }
        }
      }

      if (g_str_equal(key, "Name")) {
        result.ssid = std::string(g_variant_get_string(val, nullptr));
      }

      if (g_str_equal(key, "Strength")) {
        result.signal_level =
            (uint8_t) CalculateSignalLevel((int) g_variant_get_byte(val),
                                           0, // min
                                           100, // max
                                           4 /* number of levels */);
      }

      if (g_str_equal(key, "Security")) {
        for (gsize k = 0 ; k < g_variant_n_children(val); k++) {
          GVariant* security_val = g_variant_get_child_value(val, k);
          const char* security_val_str = g_variant_get_string(security_val, nullptr);
          if (g_str_equal(security_val_str, "wps")) {
            result.wps = true;
          }
          if (g_str_equal(security_val_str, "none")) {
            result.auth = WiFiAuth::AUTH_NONE_OPEN;
            result.encrypted = false;
          }
          if (g_str_equal(security_val_str, "wep")) {
            result.auth = WiFiAuth::AUTH_NONE_WEP;
            result.encrypted = true;
          }
          if (g_str_equal(security_val_str, "ieee8021x")) {
            result.auth = WiFiAuth::AUTH_IEEE8021X;
            result.encrypted = true;
          }
          if (g_str_equal(security_val_str, "psk")) {
            result.auth = WiFiAuth::AUTH_WPA2_PSK;
            result.encrypted = true;
          }
        }
      }

    }

    if (type_is_wifi && iface_is_wlan0) {
      results.push_back(result);
    }
  }

  return results;
}

std::vector<uint8_t> PackWiFiScanResults(const std::vector<WiFiScanResult>& results) {
  std::vector<uint8_t> packed_results;
  // Payload is (<auth><encrypted><wps><signal_level><ssid>\0)*
  for (auto const& r : results) {
    packed_results.push_back(r.auth);
    packed_results.push_back(r.encrypted);
    packed_results.push_back(r.wps);
    packed_results.push_back(r.signal_level);
    std::copy(r.ssid.begin(), r.ssid.end(), std::back_inserter(packed_results));
    packed_results.push_back(0);
  }
  return packed_results;
}

void HandleOutputCallback(int rc, const std::string& output) {
  // noop
}

bool ConnectWiFiBySsid(std::string ssid, std::string pw, GAsyncReadyCallback cb, gpointer userData) {
  ConnManBusTechnology* tech_proxy;
  GError* error;
  bool success;

  error = nullptr;
  tech_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);
  if (error) {
    loge("error getting proxy for net.connman /net/connman/technology/wifi");
    return false;
  }

  ConnManBusManager* manager_proxy;
  manager_proxy = conn_man_bus_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/",
                                                              nullptr,
                                                              &error);
  if (error) {
    loge("error getting proxy for net.connman /");
    return false;
  }

  GVariant* services = nullptr;
  success = conn_man_bus_manager_call_get_services_sync(manager_proxy,
                                                        &services,
                                                        nullptr,
                                                        &error);

  g_object_unref(manager_proxy);
  if (error) {
    loge("Error getting services from connman");
    return false;
  }

  if (!success) {
    loge("connman failed to get list of services");
    return false;
  }

  GVariant* serviceVariant = nullptr;
  bool foundService = false;

  for (gsize i = 0 ; i < g_variant_n_children(services); i++) {
    if(foundService) {
      break;
    }

    GVariant* child = g_variant_get_child_value(services, i);
    GVariant* attrs = g_variant_get_child_value(child, 1);

    bool matchedName = false;
    bool matchedInterface = false;
    bool matchedType = false;
    bool serviceOnline = false;

    for (gsize j = 0 ; j < g_variant_n_children(attrs); j++) {
      GVariant* attr = g_variant_get_child_value(attrs, j);
      GVariant* key_v = g_variant_get_child_value(attr, 0);
      GVariant* val_v = g_variant_get_child_value(attr, 1);
      GVariant* val = g_variant_get_variant(val_v);
      const char* key = g_variant_get_string(key_v, nullptr);

      if(g_str_equal(key, "Name")) {
        if(std::string(g_variant_get_string(val, nullptr)) == ssid) {
          matchedName = true;
        } else {
          matchedName = false;
        }
      }

      if(g_str_equal(key, "Type")) {
        if (g_str_equal(g_variant_get_string(val, nullptr), "wifi")) {
          matchedType = true;
        } else {
          matchedType = false;
        }
      }

      // Make sure this is for the wlan0 interface and not p2p0
      if (g_str_equal(key, "Ethernet")) {
        for (gsize k = 0 ; k < g_variant_n_children(val); k++) {
          GVariant* ethernet_attr = g_variant_get_child_value(val, k);
          GVariant* ethernet_key_v = g_variant_get_child_value(ethernet_attr, 0);
          GVariant* ethernet_val_v = g_variant_get_child_value(ethernet_attr, 1);
          GVariant* ethernet_val = g_variant_get_variant(ethernet_val_v);
          const char* ethernet_key = g_variant_get_string(ethernet_key_v, nullptr);
          if (g_str_equal(ethernet_key, "Interface")) {
            if (g_str_equal(g_variant_get_string(ethernet_val, nullptr), "wlan0")) {
              matchedInterface = true;
            } else {
              matchedInterface = false;
              break;
            }
          }
        }
      }

      if (g_str_equal(key, "State")) {
        if (g_str_equal(g_variant_get_string(val, nullptr), "online") ||
            g_str_equal(g_variant_get_string(val, nullptr), "ready") ) {
          serviceOnline = true;
        }
      }
    }

    if(matchedName && matchedInterface && matchedType) {
      // this is our service
      serviceVariant = child;
      foundService = true;

      if(serviceOnline) {
        // early out--we are already connected!
        //return true;
      }
    }
  }

  if(!foundService) {
    loge("Could not find service...");
    return false;
  }

  // Set config
  std::map<std::string, std::string> networks;
  networks.insert(std::pair<std::string, std::string>(ssid, pw));
  SetWiFiConfig(networks, HandleOutputCallback);

  GVariantIter iter;
  GVariant *serviceChild;
  const gchar* objectPath = nullptr;

  g_variant_iter_init (&iter, serviceVariant);
  while ((serviceChild = g_variant_iter_next_value (&iter)))
  {
    if(g_str_equal(g_variant_get_type_string(serviceChild),"o")) {
      objectPath = g_variant_get_string (serviceChild, nullptr);
    }

    g_variant_unref (serviceChild);
  }

  std::string s = std::string(objectPath);
  printf("Trying to connect to %s\n", s.c_str());

  ConnManBusService* service = conn_man_bus_service_proxy_new_for_bus_sync(
                                  G_BUS_TYPE_SYSTEM,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  "net.connman",
                                  objectPath,
                                  nullptr,
                                  &error);

  //conn_man_bus_service_call_connect(service, nullptr, cb, userData);

  /*GAsyncResult result;

  gboolean didConnect = conn_man_bus_service_call_connect_finish (
    service,
    &result,
    &error);

  printf("Did connect? %d\n", didConnect);

  if(error != nullptr) {
    printf("Did connect? %s\n",error->message);
  }*/

  gboolean didConnect = conn_man_bus_service_call_connect_sync (
                                 service,
                                 nullptr,
                                 &error);

  if(!didConnect && error != nullptr) {
    // 24 -- timeout ? wrong password
    // 36 -- Already connected
    printf("Error code: %d domain: %d\n", error->code, error->domain);

    printf(std::string(error->message).c_str());
  }  

  return (bool)didConnect;
}

void EnableWiFiInterface(const bool enable, ExecCommandCallback callback) {
  if (enable) {
    ExecCommandInBackground({"connmanctl", "enable", "wifi"}, callback);
    ExecCommandInBackground({"echo", "Give me about 15 seconds to start WiFi and get an IP...."}, callback);
    ExecCommandInBackground({"ifconfig", "wlan0"}, callback, 15L * 1000L);
  } else {
    ExecCommandInBackground({"connmanctl", "disable", "wifi"}, callback);
    ExecCommandInBackground({"ifconfig", "wlan0"}, callback);
  }
}

static std::string GetPathToWiFiConfigFile()
{
  return "/data/lib/connman/wifi.config";
}

std::map<std::string, std::string> UnPackWiFiConfig(const std::vector<uint8_t>& packed) {
  std::map<std::string, std::string> networks;
  // The payload is (<SSID>\0<PSK>\0)*
  for (auto it = packed.begin(); it != packed.end(); ) {
    auto terminator = std::find(it, packed.end(), 0);
    if (terminator == packed.end()) {
      break;
    }
    std::string ssid(it, terminator);
    it = terminator + 1;
    terminator = std::find(it, packed.end(), 0);
    if (terminator == packed.end()) {
      break;
    }
    std::string psk(it, terminator);
    networks.emplace(ssid, psk);
    it = terminator + 1;
  }
  return networks;
}

void SetWiFiConfig(const std::map<std::string, std::string> networks, ExecCommandCallback callback) {
  std::ostringstream wifiConfigStream;

  int count = 0;
  for (auto const& kv : networks) {
    if (count > 0) {
      wifiConfigStream << std::endl;
    }
    wifiConfigStream << "[service_wifi_" << count++ << "]" << std::endl
                     << "Type = wifi" << std::endl
                     << "IPv4 = auto" << std::endl
                     << "IPv6 = auto" << std::endl
                     << "Name = " << kv.first << std::endl
                     << "Passphrase = " << kv.second << std::endl;
  }

  int rc = WriteFileAtomically(GetPathToWiFiConfigFile(), wifiConfigStream.str());

  if (rc) {
    std::string error = "Failed to write wifi config. rc = " + std::to_string(rc);
    callback(rc, "Failed to write wifi config.");
    return;
  }

  ExecCommandInBackground({"connmanctl", "enable", "wifi"}, callback);
  ExecCommandInBackground({"echo", "Give me about 15 seconds to setup WiFi...."}, callback);
  ExecCommandInBackground({"ifconfig", "wlan0"}, callback, 15L * 1000L);
}

bool CanConnectToHostName(char* hostName) {
  struct sockaddr_in addr;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // we will try to make tcp connection using IPv4
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);

  char ipAddr[100];

  if(strlen(hostName) > 100) {
    // don't allow host names larger than 100 chars
    return false;
  }

  // Try to get IP from host name (will do DNS resolve)
  bool gotIp = GetIpFromHostName(hostName, ipAddr);

  if(!gotIp) {
    return false;
  }

  if(inet_pton(AF_INET, ipAddr, &addr.sin_addr) <= 0) {
    // can't resolve hostname
    return false;
  }

  if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    // can't connect to hostname
    return false;
  }

  close(sockfd);
  
  // success, return true!
  return true;
}

bool GetIpFromHostName(char* hostName, char* ipAddressOut) {
  struct hostent* hostEntry;
  struct in_addr** ip;

  hostEntry = gethostbyname(hostName);

  if(hostEntry == nullptr) {
    return false;
  }

  ip = (struct in_addr**)hostEntry->h_addr_list;

  if(ip[0] == nullptr) {
    return false;
  }

  // we have an ip!
  strcpy(ipAddressOut, inet_ntoa(*ip[0]));

  return true;
}

bool HasInternet() {
  return CanConnectToHostName("google.com") || CanConnectToHostName("amazon.com");
}

} // namespace Anki