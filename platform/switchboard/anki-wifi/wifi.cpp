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

#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include "exec_command.h"
#include "fileutils.h"
#include "anki-ble/stringutils.h"
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

WifiScanErrorCode ScanForWiFiAccessPoints(std::vector<WiFiScanResult>& results) {
  results.clear();

  bool disabledApMode = DisableAccessPointMode();
  if(!disabledApMode) {
    Log::Write("Not in access point mode or could not disable AccessPoint mode");
  } else {
    Log::Write("Disabled AccessPoint mode.");
  }

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
    return WifiScanErrorCode::ERROR_GETTING_PROXY;
  }

  gboolean success = conn_man_bus_technology_call_scan_sync(tech_proxy,
                                                            nullptr,
                                                            &error);
  g_object_unref(tech_proxy);
  if (error) {
    loge("error asking connman to scan for wifi access points");
    return WifiScanErrorCode::ERROR_SCANNING;
  }

  if (!success) {
    loge("connman failed to scan for wifi access points");
    return WifiScanErrorCode::FAILED_SCANNING;
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
    return WifiScanErrorCode::ERROR_GETTING_MANAGER;
  }

  GVariant* services = nullptr;
  success = conn_man_bus_manager_call_get_services_sync(manager_proxy,
                                                        &services,
                                                        nullptr,
                                                        &error);
  g_object_unref(manager_proxy);
  if (error) {
    loge("Error getting services from connman");
    return WifiScanErrorCode::ERROR_GETTING_SERVICES;
  }

  if (!success) {
    loge("connman failed to get list of services");
    return WifiScanErrorCode::FAILED_GETTING_SERVICES;
  }

  // Get hidden flag
  std::string configSsid = "";
  std::string fieldString = "Hidden";
  bool configIsHidden = GetConfigField(fieldString, configSsid) == "true";

  for (gsize i = 0 ; i < g_variant_n_children(services); i++) {
    WiFiScanResult result{WiFiAuth::AUTH_NONE_OPEN, false, false, 0, "", false};
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
      result.ssid = GetHexSsidFromServicePath(GetObjectPathForService(child));

      if(result.ssid == configSsid) {
        result.hidden = configIsHidden;
      }

      results.push_back(result);
    }
  }

  return WifiScanErrorCode::SUCCESS;
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

static gpointer connectionThread(gpointer data)
{
  GMainLoop *loop = g_main_loop_new(NULL, true);

  if (!loop) {
      loge("error getting main loop");
      return nullptr;
  }

  g_main_loop_run(loop);
  g_main_loop_unref(loop);
  return nullptr;
}

static GMutex connectMutex;

void connectCallback(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
  struct ConnectAsyncData *connectData = (struct ConnectAsyncData *)user_data;

  // sanity check
  gpointer res_user_data = g_async_result_get_user_data(result);
  if (res_user_data != user_data) {
    loge("%s: res_user_data != user_data, bailing out", __func__);
    return;
  }

  if (!g_cancellable_is_cancelled(connectData->cancellable)) {
    conn_man_bus_service_call_connect_finish (connectData->service,
                                              result,
                                              &connectData->error);
  }

  g_mutex_lock(&connectMutex);
  connectData->completed = true;
  g_cond_signal(connectData->cond);
  g_mutex_unlock(&connectMutex);
}

static const char* const hiddenAgentPath = "/tmp/vic-switchboard/connman-agent";

static void HiddenAPCallback(GDBusConnection *connection,
                      const gchar *sender,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *method_name,
                      GVariant *parameters,
                      GDBusMethodInvocation *invocation,
                      gpointer user_data)
{
  struct WPAConnectInfo *wpaConnectInfo = (struct WPAConnectInfo *)user_data;

  if (strcmp(object_path, hiddenAgentPath)) {
    return; //not us
  }

  if (strcmp(interface_name, "net.connman.Agent")) {
    return; // not our Interface
  }

  if (!strcmp(method_name, "RequestInput")) {
    // Ok, let's provide the input
    gchar *obj;
    GVariant *dict;

    g_variant_get(parameters, "(oa{sv})", &obj, &dict);
    logi("%s: object %s", __func__, obj);

    GVariantBuilder *dict_builder = g_variant_builder_new(G_VARIANT_TYPE("(a{sv})"));
    g_variant_builder_open(dict_builder, G_VARIANT_TYPE("a{sv}"));
    if (wpaConnectInfo->name) {
      logi("%s: found 'Name'", __func__);
      g_variant_builder_add(dict_builder, "{sv}", "Name", g_variant_new_string(wpaConnectInfo->name));
    }
    if (wpaConnectInfo->ssid) {
      logi("%s: found 'SSID'", __func__);
      g_variant_builder_add(dict_builder, "{sv}", "SSID", g_variant_new_bytestring(wpaConnectInfo->ssid));
    }
    if (wpaConnectInfo->passphrase) {
      logi("%s: found 'Passphrase'", __func__);
      g_variant_builder_add(dict_builder, "{sv}", "Passphrase", g_variant_new_string(wpaConnectInfo->passphrase));
    }
    g_variant_builder_close(dict_builder);

    GVariant *response = g_variant_builder_end(dict_builder);
    g_variant_builder_unref(dict_builder);
    logi("%s", g_variant_print(response, true));

    g_dbus_method_invocation_return_value(invocation, response);
  }
}

GDBusInterfaceVTable hiddenAPVtable = {
  .method_call = HiddenAPCallback,
};

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='net.connman.Agent'>"
  "    <method name='RequestInput'>"
  "      <arg type='o' name='service' direction='in'/>"
  "      <arg type='a{sv}' name='fields' direction='in'/>"
  "      <arg type='a{sv}' name='input' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

bool RegisterAgentHidden(struct WPAConnectInfo *wpaConnectInfo)
{
  GError *error = nullptr;

  GDBusConnection *gdbusConn = g_bus_get_sync(G_BUS_TYPE_SYSTEM,
                                              nullptr,
                                              &error);

  static GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  if (!introspection_data) {
    return false;
  }

  ConnManBusManager *manager;
  manager = conn_man_bus_manager_proxy_new_sync(gdbusConn,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                "net.connman",
                                                "/",
                                                nullptr,
                                                &error);
  if (error) {
    loge("error getting manager");
    return false;
  }

  guint agentId = 0;
  agentId = g_dbus_connection_register_object(gdbusConn,
                                              hiddenAgentPath,
                                              introspection_data->interfaces[0],
                                              &hiddenAPVtable,
                                              wpaConnectInfo,
                                              nullptr,
                                              &error);
  if (agentId == 0 || error != NULL) {
    loge("Error registering agent object");
    return false;
  }

  if (!conn_man_bus_manager_call_register_agent_sync(manager,
                                                     hiddenAgentPath,
                                                     nullptr,
                                                     &error)) {
    g_dbus_connection_unregister_object(gdbusConn, agentId);
    loge("error registering agent");
    return false;
  }

  wpaConnectInfo->agentId = agentId;
  wpaConnectInfo->connection = gdbusConn;
  wpaConnectInfo->manager = manager;
  return true;
}

bool UnregisterAgentHidden(struct WPAConnectInfo *wpaConnectInfo)
{
  GError *error = nullptr;

  if (!wpaConnectInfo) {
    return false;
  }

  conn_man_bus_manager_call_unregister_agent_sync (wpaConnectInfo->manager,
                                                   hiddenAgentPath,
                                                   nullptr,
                                                   &error);

  if (error) {
    return false;
  }

  g_dbus_connection_unregister_object(wpaConnectInfo->connection,
                                      wpaConnectInfo->agentId);

  g_object_unref(wpaConnectInfo->manager);
  g_object_unref(wpaConnectInfo->connection);
  return true;
}

bool ConnectWiFiBySsid(std::string ssid, std::string pw, uint8_t auth, bool hidden, GAsyncReadyCallback cb, gpointer userData) {
  ConnManBusTechnology* tech_proxy;
  GError* error;
  bool success;

  static GThread *thread = g_thread_new("connect", connectionThread, nullptr);

  if (thread == nullptr) {
    loge("couldn't spawn connection thread");
    return false;
  }

  std::string nameFromHex = hexStringToAsciiString(ssid);

  Log::Write("NameFromHex: {%s}", nameFromHex.c_str());

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
  GVariant* currentServiceVariant = nullptr;
  bool foundService = false;

  for (gsize i = 0 ; i < g_variant_n_children(services); i++) {
    if(foundService) {
      break;
    }

    GVariant* child = g_variant_get_child_value(services, i);
    GVariant* attrs = g_variant_get_child_value(child, 1);

    bool hasName = false;
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
        if(std::string(g_variant_get_string(val, nullptr)) == nameFromHex) {
          matchedName = true;
        } else {
          matchedName = false;
        }
        hasName = true;
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
          currentServiceVariant = child;
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
        return true;
      }
    } else if(hidden && !hasName) {
      serviceVariant = child;
      foundService = true;
    }
  }

  if(!foundService) {
    loge("Could not find service...");
    return false;
  }

  // Set config
  SetWiFiConfig(ssid, pw, (WiFiAuth)auth, hidden);

  std::string servicePath = GetObjectPathForService(serviceVariant);
  Log::Write("Initiating connection to %s.", servicePath.c_str());

  // before connecting, lets disconnect from our current different network
  if(currentServiceVariant != nullptr) {
    // we have a connected service and it *isn't* the one we are currently connected to
    std::string currentOPath = GetObjectPathForService(currentServiceVariant);
    ConnManBusService* currentService = GetServiceForPath(currentOPath);
    bool disconnected = DisconnectFromWifiService(currentService);

    if(disconnected) {
      Log::Write("Disconnected from %s.", currentOPath.c_str());
    }
  }

  // Get the ConnManBusService for our object path
  Log::Write("Service path: %s", servicePath.c_str());
  ConnManBusService* service = GetServiceForPath(servicePath);
  if(service == nullptr) {
    return false;
  }

  WPAConnectInfo connectInfo = {};
  bool agent_registered = false;
  if(hidden) {
    connectInfo.name = nameFromHex.c_str();
    connectInfo.passphrase = pw.c_str();

    agent_registered = RegisterAgentHidden(&connectInfo);
    if (!agent_registered) {
      loge("could not register agent, bailing out");
      return false;
    }
  }

  // Try to connect to our service
  bool connect = ConnectToWifiService(service);

  if (!connect) {
    Log::Write("Retry connecting one more time");
    connect = ConnectToWifiService(service);
  }

  if (agent_registered) {
    Log::Write("unregistering agent");
    UnregisterAgentHidden(&connectInfo);
  }
  return connect;
}

ConnManBusService* GetServiceForPath(std::string objectPath) {
  GError* error = nullptr;
  ConnManBusService* service = conn_man_bus_service_proxy_new_for_bus_sync(
                                  G_BUS_TYPE_SYSTEM,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  "net.connman",
                                  objectPath.c_str(),
                                  nullptr,
                                  &error);

  if(service == nullptr || error != nullptr) {
    Log::Write("Could not find service for object path: %s", objectPath.c_str());
  }

  return service;
}

bool ConnectToWifiService(ConnManBusService* service) {

  if(service == nullptr) {
    return false;
  }

  GCond connectCond;
  g_cond_init(&connectCond);

  g_mutex_lock(&connectMutex);

  struct ConnectAsyncData connAsyncData;

  connAsyncData.completed = false;
  connAsyncData.error = nullptr;
  connAsyncData.cond = &connectCond;
  connAsyncData.service = service;
  connAsyncData.cancellable = g_cancellable_new();
  if (connAsyncData.cancellable == nullptr) {
    loge("%s: out of memory", __func__);
    return false;
  }
  conn_man_bus_service_call_connect (service,
                                     connAsyncData.cancellable,
                                     connectCallback,
                                     (gpointer)&connAsyncData);

  gint64 end_time = g_get_monotonic_time () + 10 * G_TIME_SPAN_SECOND;
  bool timedOut = false;
  while (!connAsyncData.completed) {
    timedOut = !g_cond_wait_until(&connectCond, &connectMutex, end_time);
    if (timedOut) {
      g_cancellable_cancel(connAsyncData.cancellable);
      g_cond_wait(&connectCond, &connectMutex);
      break;
    }
  }
  g_mutex_unlock(&connectMutex);

  bool didConnect = !timedOut && connAsyncData.error == nullptr;
  if(!didConnect) {
    Log::Write("Error connecting to wifi: %s",
               timedOut ?
               "timed out waiting on conditional" : connAsyncData.error->message);
  }
  g_object_unref(connAsyncData.cancellable);
  return didConnect;
}

bool DisconnectFromWifiService(ConnManBusService* service) {
  if(service == nullptr) {
    return false;
  }
  
  GError* error = nullptr;
  return conn_man_bus_service_call_disconnect_sync (service, nullptr, &error);
}

void EnableWiFiInterface(const bool enable, ExecCommandCallback callback) {
  if (enable) {
    ExecCommandInBackground({"connmanctl", "enable", "wifi"}, callback);
  } else {
    ExecCommandInBackground({"connmanctl", "disable", "wifi"}, callback);
    ExecCommandInBackground({"ifconfig", "wlan0"}, callback);
  }
}

std::string GetObjectPathForService(GVariant* service) {
  GVariantIter iter;
  GVariant *serviceChild;
  const gchar* objectPath = nullptr;

  g_variant_iter_init (&iter, service);
  while ((serviceChild = g_variant_iter_next_value (&iter)))
  {
    if(g_str_equal(g_variant_get_type_string(serviceChild),"o")) {
      objectPath = g_variant_get_string (serviceChild, nullptr);
    }

    g_variant_unref (serviceChild);
  }

  return std::string(objectPath);
}

static std::string GetPathToWiFiConfigFile()
{
  return "/data/lib/connman/wifi.config";
}

static std::string GetPathToWiFiConfigDir()
{
  return "/data/lib/connman";
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

bool RemoveAllConfigDirs() {
  std::string dirConfig = GetPathToWiFiConfigDir();

  DIR *d;
  struct dirent *dir;
  d = opendir(dirConfig.c_str());

  if(d) {
    while((dir = readdir(d)) != nullptr) {
      struct stat statInfo;
      std::string fullPath = dirConfig + "/" + std::string(dir->d_name);

      int statSuccess = stat(fullPath.c_str(), &statInfo);
      Log::Write("Found: %s", dir->d_name);

      if(statSuccess != 0) {
        Log::Write("Stat broke: %d", errno);
        continue;
      }

      Log::Write("Stat was successful.");

      // S_ISDIR(statInfo.st_mode) // wtf
      if((strncmp(dir->d_name, "wifi_", 5) == 0)) {
        Log::Write("Deleting: %s", dir->d_name);
        int deleteSuccess = remove(fullPath.c_str());

        if(deleteSuccess != 0) {
          Log::Write("Failed to delete [%d].", errno);
        }
      }
    }
    closedir(d);
  }

  return true;
}

void SetWiFiConfig(std::string ssid, std::string password, WiFiAuth auth, bool isHidden) {
  std::vector<WiFiConfig> networks;
  WiFiConfig config;
  config.auth = auth;
  config.hidden = isHidden;
  config.ssid = ssid;
  config.passphrase = password;

  networks.push_back(config);
  SetWiFiConfig(networks, HandleOutputCallback);
}

void SetWiFiConfig(const std::vector<WiFiConfig>& networks, ExecCommandCallback callback) {
  std::ostringstream wifiConfigStream;

  RemoveAllConfigDirs();

  int count = 0;
  for (auto const& config : networks) {
    if (count > 0) {
      wifiConfigStream << std::endl;
    }
    // Exclude networks with ssids that are not in hex format
    if (!IsHexString(config.ssid)) {
      loge("SetWiFiConfig. '%s' is NOT a hexadecimal string.", config.ssid.c_str());
      continue;
    }
    // Exclude networks with unsupported auth types
    if ((config.auth != WiFiAuth::AUTH_NONE_OPEN)
        && (config.auth != WiFiAuth::AUTH_NONE_WEP)
        && (config.auth != WiFiAuth::AUTH_NONE_WEP_SHARED)
        && (config.auth != WiFiAuth::AUTH_WPA_PSK)
        && (config.auth != WiFiAuth::AUTH_WPA2_PSK)) {
      loge("SetWiFiConfig. Unsupported auth type : %d for '%s'",
           config.auth,
           hexStringToAsciiString(config.ssid).c_str());
      continue;
    }

    std::string security;
    switch (config.auth) {
      case WiFiAuth::AUTH_NONE_WEP:
        /* fall through */
      case WiFiAuth::AUTH_NONE_WEP_SHARED:
        security = "wep";
        break;
      case WiFiAuth::AUTH_WPA_PSK:
        /* fall through */
      case WiFiAuth::AUTH_WPA2_PSK:
        security = "psk";
        break;
      case WiFiAuth::AUTH_NONE_OPEN:
        /* fall through */
      default:
        security = "none";
        break;
    }

    std::string hidden(config.hidden ? "true" : "false");
    wifiConfigStream << "[service_wifi_" << count++ << "]" << std::endl
                     << "Type = wifi" << std::endl
                     << "IPv4 = dhcp" << std::endl
                     << "IPv6 = auto" << std::endl
                     << "SSID=" << config.ssid << std::endl
                     << "Security=" << security << std::endl
                     << "Hidden=" << hidden << std::endl;
    if (!config.passphrase.empty()) {
      wifiConfigStream << "Passphrase=" << config.passphrase << std::endl;
    }
  }

  int rc = WriteFileAtomically(GetPathToWiFiConfigFile(), wifiConfigStream.str());

  if (rc) {
    std::string error = "Failed to write wifi config. rc = " + std::to_string(rc);
    callback(rc, "Failed to write wifi config.");
    return;
  }

  ExecCommandInBackground({"connmanctl", "enable", "wifi"}, callback);
}

WiFiState GetWiFiState() {
  // Return WiFiState object
  WiFiState wifiState;
  wifiState.ssid = "";
  wifiState.connState = WiFiConnState::UNKNOWN;

  GError* error = nullptr;

  ConnManBusManager* manager_proxy;
  manager_proxy = conn_man_bus_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/",
                                                              nullptr,
                                                              &error);
  if (error) {
    loge("error getting proxy for net.connman /");
    return wifiState;
  }

  GVariant* services = nullptr;
  bool success = conn_man_bus_manager_call_get_services_sync(manager_proxy,
                                                        &services,
                                                        nullptr,
                                                        &error);
  g_object_unref(manager_proxy);
  if (error) {
    loge("Error getting services from connman");
    return wifiState;
  }

  if (!success) {
    loge("connman failed to get list of services");
    return wifiState;
  }

  for (gsize i = 0 ; i < g_variant_n_children(services); i++) {
    GVariant* child = g_variant_get_child_value(services, i);
    GVariant* attrs = g_variant_get_child_value(child, 1);

    bool isAssociated = false;
    std::string connectedSsid;
    WiFiConnState connState = WiFiConnState::UNKNOWN;

    for (gsize j = 0 ; j < g_variant_n_children(attrs); j++) {
      GVariant* attr = g_variant_get_child_value(attrs, j);
      GVariant* key_v = g_variant_get_child_value(attr, 0);
      GVariant* val_v = g_variant_get_child_value(attr, 1);
      GVariant* val = g_variant_get_variant(val_v);
      const char* key = g_variant_get_string(key_v, nullptr);

      // Make sure this is a wifi service and not something else
      if (g_str_equal(key, "Type")) {
        if (!g_str_equal(g_variant_get_string(val, nullptr), "wifi")) {
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
            if (!g_str_equal(g_variant_get_string(ethernet_val, nullptr), "wlan0")) {
              isAssociated = false;
              break;
            }
          }
        }
      }

      if (g_str_equal(key, "State")) {
        std::string state = std::string(g_variant_get_string(val, nullptr));
        std::string servicePath = GetObjectPathForService(child);
        connectedSsid = GetHexSsidFromServicePath(servicePath);

        if(state == "ready") {
          isAssociated = true;
          connState = WiFiConnState::CONNECTED;
        } else if(state == "online") {
          isAssociated = true;
          connState = WiFiConnState::ONLINE;
        }
      }
    }

    if(isAssociated) {
      wifiState.ssid = connectedSsid;
      wifiState.connState = connState;
      break;
    }
  }

  return wifiState;
}

std::string GetHexSsidFromServicePath(const std::string& servicePath) {
  // Take a dbus wifi service path and extract the ssid HEX
  std::string wifiPrefix = "/net/connman/service/wifi";
  std::string prefix = wifiPrefix + "_000000000000_";
  std::string hexString = "";

  if(servicePath.compare(0, wifiPrefix.length(), wifiPrefix) != 0) {
    // compare strings all the way up to and including "wifi"
    return "! Invalid Ssid";
  }

  for(int i = prefix.length(); i < servicePath.length(); i++) {
    if(servicePath[i] == '_') {
      break;
    }
    hexString.push_back(servicePath[i]);
  }

  return hexString;
}

bool CanConnectToHostName(char* hostName) {
  struct sockaddr_in addr = {0};

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd == -1) {
    return false;
  }

  // we will try to make tcp connection using IPv4
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);

  char ipAddr[100];

  if(strlen(hostName) > 100) {
    // don't allow host names larger than 100 chars
    close(sockfd);
    return false;
  }

  // Try to get IP from host name (will do DNS resolve)
  bool gotIp = GetIpFromHostName(hostName, ipAddr);

  if(!gotIp) {
    close(sockfd);
    return false;
  }

  if(inet_pton(AF_INET, ipAddr, &addr.sin_addr) <= 0) {
    // can't resolve hostname
    close(sockfd);
    return false;
  }

  if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    // can't connect to hostname
    close(sockfd);
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

bool IsAccessPointMode() {
  GError* error = nullptr;

  GVariant* properties;

  ConnManBusTechnology* tech_proxy = nullptr;
  tech_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);

  if(error != nullptr) {
    return false;
  }

  bool success = (bool)conn_man_bus_technology_call_get_properties_sync (
    tech_proxy,
    &properties,
    nullptr,
    &error);

  if(error != nullptr || !success) {
    return false;
  }

  for (gsize j = 0 ; j < g_variant_n_children(properties); j++) {
    GVariant* attr = g_variant_get_child_value(properties, j);
    GVariant* key_v = g_variant_get_child_value(attr, 0);
    GVariant* val_v = g_variant_get_child_value(attr, 1);
    GVariant* val = g_variant_get_variant(val_v);
    const char* key = g_variant_get_string(key_v, nullptr);

    // Make sure this is a wifi service and not something else
    if (g_str_equal(key, "Tethering")) {
      return (bool)g_variant_get_boolean(val);
    }
  }

  return false;
}

bool EnableAccessPointMode(std::string ssid, std::string pw) {
  GError* error = nullptr;

  GVariant* ssid_s = g_variant_new_string(ssid.c_str());
  GVariant* ssid_v = g_variant_new_variant(ssid_s);

  GVariant* pw_s = g_variant_new_string(pw.c_str());
  GVariant* pw_v = g_variant_new_variant(pw_s);

  GVariant* enable_b = g_variant_new_boolean(true);
  GVariant* enable_v = g_variant_new_variant(enable_b);

  ConnManBusTechnology* tech_proxy = nullptr;
  tech_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);

  if(error != nullptr) {
    return false;
  }

  conn_man_bus_technology_call_set_property_sync (
    tech_proxy,
    "TetheringIdentifier",
    ssid_v,
    nullptr,
    &error);

  if(error != nullptr) {
    return false;
  }

  conn_man_bus_technology_call_set_property_sync (
    tech_proxy,
    "TetheringPassphrase",
    pw_v,
    nullptr,
    &error);

  if(error != nullptr) {
    return false;
  }

  conn_man_bus_technology_call_set_property_sync (
    tech_proxy,
    "Tethering",
    enable_v,
    nullptr,
    &error);

  if(error != nullptr) {
    return false;
  }

  return true;
}

bool DisableAccessPointMode() {
  GError* error = nullptr;

  GVariant* enable_b = g_variant_new_boolean(false);
  GVariant* enable_v = g_variant_new_variant(enable_b);

  ConnManBusTechnology* tech_proxy = nullptr;
  tech_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);

  if(error != nullptr) {
    return false;
  }

  conn_man_bus_technology_call_set_property_sync (
    tech_proxy,
    "Tethering",
    enable_v,
    nullptr,
    &error);

  if(error != nullptr) {
    return false;
  }

  return true;
}

WiFiIpFlags GetIpAddress(uint8_t* ipv4_32bits, uint8_t* ipv6_128bits) {
  WiFiIpFlags wifiFlags = WiFiIpFlags::NONE;

  struct ifaddrs* ifaddrs;

  // get ifaddrs
  getifaddrs(&ifaddrs);

  struct ifaddrs* current = ifaddrs;

  // clear memory
  memset(ipv4_32bits, 0, 4);
  memset(ipv6_128bits, 0, 16);

  const char* interface = IsAccessPointMode()? "tether" : "wlan0";

  while(current != nullptr) {
    int family = current->ifa_addr->sa_family;

    if ((family == AF_INET || family == AF_INET6) && (strcmp(current->ifa_name, interface) == 0)) {
      if(family == AF_INET) {
        // Handle IPv4
        struct sockaddr_in *sa = (struct sockaddr_in*)current->ifa_addr;
        memcpy(ipv4_32bits, &sa->sin_addr, sizeof(sa->sin_addr));
        wifiFlags = wifiFlags | WiFiIpFlags::HAS_IPV4;
      } else {
        // Handle IPv6
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)current->ifa_addr;
        memcpy(ipv6_128bits, &sa6->sin6_addr, sizeof(sa6->sin6_addr));
        wifiFlags = wifiFlags | WiFiIpFlags::HAS_IPV6;
      }
    }

    current = current->ifa_next;
  }

  // free ifaddrs
  freeifaddrs(ifaddrs);

  return wifiFlags;
}

std::string GetConfigField(std::string& field, std::string& outSsid) {
  // This method returns the value of given 'field' (and sets SSID) in 'outSsid'
  // Currently this method assumes that there is only one wifi network in the config.

  std::vector<uint8_t> bytes;
  bool readSuccess = ReadFileIntoVector(GetPathToWiFiConfigFile(), bytes);

  if(!readSuccess) {
    // If we can't read file, return empty string
    return "";
  }

  std::string fileContents(reinterpret_cast<char const*>(bytes.data()), bytes.size());

  std::string configField = "";

  std::string line;
  std::stringstream ss(fileContents);
  const std::string delim = "=";
  const std::string ssid = "SSID";

  while(std::getline(ss, line, '\n')) {
    size_t index = line.find(delim);

    if(index == std::string::npos) {
      continue;
    }

    std::string fieldName = line.substr(0, index);

    if(fieldName == field) {
      configField = line.substr(index + 1);
    } else if(fieldName == ssid) {
      outSsid = line.substr(index + 1);
    }
  }

  return configField;
}

} // namespace Anki
