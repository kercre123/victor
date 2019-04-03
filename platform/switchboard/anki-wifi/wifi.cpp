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
#include "exec_command.h"
#include "fileutils.h"
#include "anki-ble/stringutils.h"
#include "log.h"
#include "wifi.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace Anki {

static const char* kConnmanErrorAlreadyEnabledString =
  "GDBus.Error:net.connman.Error.AlreadyEnabled: Already enabled";

static const char* kConnmanErrorAlreadyDisabledString =
  "GDBus.Error:net.connman.Error.AlreadyDisabled: Already disabled";

static ConnManBusManager* GetConnManBusManager() {
  ConnManBusManager* manager_proxy;
  GError* error = nullptr;

  manager_proxy = conn_man_bus_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/",
                                                              nullptr,
                                                              &error);
  if (!manager_proxy) {
    loge("error getting proxy for net.connman /: %s", error->message);
  }

  return manager_proxy;
}

static WifiScanErrorCode GetConnManServices(GVariant** services) {
  *services = nullptr;

  ConnManBusManager* manager_proxy = GetConnManBusManager();
  if (!manager_proxy) {
    return WifiScanErrorCode::ERROR_GETTING_MANAGER;
  }

  GError* error = nullptr;
  gboolean success = conn_man_bus_manager_call_get_services_sync(manager_proxy,
                                                                 services,
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

  return WifiScanErrorCode::SUCCESS;
}

static ConnManBusTechnology* GetConnManBusTechnologyForWiFi() {
  ConnManBusTechnology* wifi_proxy;
  GError* error = nullptr;

  wifi_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);
  if (!wifi_proxy) {
    loge("error getting proxy for net.connman /net/connman/technology/wifi: %s",
         error->message);
  }

  return wifi_proxy;
}

static bool SetWiFiProperty(ConnManBusTechnology* wifi_proxy,
                            const char* property_name,
                            GVariant* value,
                            const char* already_error) {

  bool need_to_unref_wifi_proxy = false;
  if (!wifi_proxy) {
    wifi_proxy = GetConnManBusTechnologyForWiFi();
    if (!wifi_proxy) {
      return false;
    }
    need_to_unref_wifi_proxy = true;
  }

  GError* error = nullptr;
  gchar* value_string = g_variant_print(value, false);
  bool success = (bool) conn_man_bus_technology_call_set_property_sync (
    wifi_proxy,
    property_name,
    value,
    nullptr,
    &error);

  if (!success) {
    if (already_error && (0 == strcmp(error->message, already_error))) {
      success = true;
    } else {
      loge("Error asking connman to set wifi/%s to %s : %s",
           property_name, (char *) value_string, error->message);
    }
    g_error_free(error);
  }
  g_free(value_string);

  if (need_to_unref_wifi_proxy) {
    g_object_unref(wifi_proxy);
  }

  return success;
}

static bool SetStringWiFiProperty(ConnManBusTechnology* wifi_proxy,
                                  const char* property_name,
                                  const char* property_value,
                                  const char* already_error)
{
  GVariant* value_s = g_variant_new_string((gchar *) property_value);
  GVariant* value_v = g_variant_new_variant(value_s);

  return SetWiFiProperty(wifi_proxy,
                         property_name,
                         value_v,
                         already_error);
}

static bool SetBooleanWiFiProperty(ConnManBusTechnology* wifi_proxy,
                                   const char* property_name,
                                   const bool value,
                                   const char* already_error) {

  GVariant* value_b = g_variant_new_boolean((gboolean) value);
  GVariant* value_v = g_variant_new_variant(value_b);

  return SetWiFiProperty(wifi_proxy,
                         property_name,
                         value_v,
                         already_error);
}

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

  if (!EnableWiFi()) {
    Log::Write("Failed to enable wifi");
  }

  ConnManBusTechnology* wifi_proxy = GetConnManBusTechnologyForWiFi();
  if (!wifi_proxy) {
    return WifiScanErrorCode::ERROR_GETTING_PROXY;
  }

  GError* error = nullptr;
  gboolean success = conn_man_bus_technology_call_scan_sync(wifi_proxy,
                                                            nullptr,
                                                            &error);
  g_object_unref(wifi_proxy);
  if (error) {
    loge("error asking connman to scan for wifi access points: %s", error->message);
    return WifiScanErrorCode::ERROR_SCANNING;
  }

  if (!success) {
    loge("connman failed to scan for wifi access points");
    return WifiScanErrorCode::FAILED_SCANNING;
  }

  GVariant* services = nullptr;
  WifiScanErrorCode result = GetConnManServices(&services);
  if (result != WifiScanErrorCode::SUCCESS) {
    return result;
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

void HandleOutputCallback(int rc) {
  // noop
}

static gpointer ConnectionThread(gpointer data)
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

void ConnectCallback(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
  g_mutex_lock(&connectMutex);
  struct ConnectInfo* data = (ConnectInfo*)user_data;

  conn_man_bus_service_call_connect_finish(data->service,
                                          result,
                                          &data->error);

  g_cond_signal(data->cond);
  g_mutex_unlock(&connectMutex);
}

static const char* const agentPath = "/tmp/vic_switchboard/connman_agent";

static void AgentCallback(GDBusConnection *connection,
                      const gchar *sender,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *method_name,
                      GVariant *parameters,
                      GDBusMethodInvocation *invocation,
                      gpointer user_data)
{
  struct WPAConnectInfo *wpaConnectInfo = (struct WPAConnectInfo *)user_data;

  if (strcmp(object_path, agentPath)) {
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

    g_dbus_method_invocation_return_value(invocation, response);
  }

  if (!strcmp(method_name, "ReportError")) {
    gchar *obj;
    gchar *err;

    g_variant_get(parameters, "(os)", &obj, &err);

    if (!strcmp(err, "invalid-key")) {
      wpaConnectInfo->status = ConnectWifiResult::CONNECT_INVALIDKEY;
      g_dbus_method_invocation_return_value(invocation, NULL);
      return;
    }

    if(++wpaConnectInfo->retryCount < MAX_NUM_ATTEMPTS) {
      Log::Write("Connection Error: Retrying");
      g_dbus_method_invocation_return_dbus_error(invocation, "net.connman.Agent.Error.Retry", "");
    }
  }

}

GDBusInterfaceVTable agentVtable = {
  .method_call = AgentCallback,
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
  "    <method name='ReportError'>"
  "      <arg type='o' name='service' direction='in'/>"
  "      <arg type='s' name='error' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

bool RegisterAgent(struct WPAConnectInfo *wpaConnectInfo)
{
  GError *error = nullptr;

  GDBusConnection *gdbusConn = g_bus_get_sync(G_BUS_TYPE_SYSTEM,
                                              nullptr,
                                              &error);

  static GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  if (!introspection_data) {
    return false;
  }

  ConnManBusManager *manager = GetConnManBusManager();

  if (!manager) {
    loge("error getting manager");
    return false;
  }

  guint agentId = 0;
  agentId = g_dbus_connection_register_object(gdbusConn,
                                              agentPath,
                                              introspection_data->interfaces[0],
                                              &agentVtable,
                                              wpaConnectInfo,
                                              nullptr,
                                              &error);
  if (agentId == 0 || error != NULL) {
    loge("Error registering agent object");
    return false;
  }

  if (!conn_man_bus_manager_call_register_agent_sync(manager,
                                                     agentPath,
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

bool UnregisterAgent(struct WPAConnectInfo *wpaConnectInfo)
{
  GError *error = nullptr;

  if (!wpaConnectInfo) {
    return false;
  }

  conn_man_bus_manager_call_unregister_agent_sync (wpaConnectInfo->manager,
                                                   agentPath,
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

ConnectWifiResult ConnectWiFiBySsid(std::string ssid, std::string pw, uint8_t auth, bool hidden, GAsyncReadyCallback cb, gpointer userData) {
  static GThread *thread = g_thread_new("connect", ConnectionThread, nullptr);

  if (thread == nullptr) {
    loge("couldn't spawn connection thread");
    return ConnectWifiResult::CONNECT_FAILURE;
  }

  std::string nameFromHex = hexStringToAsciiString(ssid);

  GVariant* services = nullptr;
  WifiScanErrorCode result = GetConnManServices(&services);
  if (result != WifiScanErrorCode::SUCCESS) {
    return ConnectWifiResult::CONNECT_FAILURE;
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

      g_variant_unref(attr);
      g_variant_unref(key_v);
      g_variant_unref(val_v);
      g_variant_unref(val);
    }

    if(matchedName && matchedInterface && matchedType) {
      // this is our service
      serviceVariant = child;
      foundService = true;

      if(serviceOnline) {
        // early out--we are already connected!
        return ConnectWifiResult::CONNECT_SUCCESS;
      }
    } else if(hidden && !hasName) {
      serviceVariant = child;
      foundService = true;
    }

    g_variant_unref(attrs);
    if(child != serviceVariant && child != currentServiceVariant) {
      g_variant_unref(child);
    }
  }

  if(!foundService) {
    loge("Could not find service...");
    g_variant_unref(services);
    if(currentServiceVariant != nullptr) {
      g_variant_unref(currentServiceVariant);
    }
    return ConnectWifiResult::CONNECT_FAILURE;
  }

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
    g_variant_unref(services);
    g_variant_unref(serviceVariant);
    return ConnectWifiResult::CONNECT_FAILURE;
  }

  WPAConnectInfo connectInfo = {};
  connectInfo.name = nameFromHex.c_str();
  connectInfo.passphrase = pw.c_str();
  connectInfo.status = ConnectWifiResult::CONNECT_NONE;

  if (!RegisterAgent(&connectInfo)) {
    loge("could not register agent, bailing out");
    g_variant_unref(services);
    g_variant_unref(serviceVariant);
    g_object_unref(service);
    return ConnectWifiResult::CONNECT_FAILURE;
  }

  // Try to connect to our service
  ConnectWifiResult connectStatus = ConnectToWifiService(service);

  if(connectInfo.status != ConnectWifiResult::CONNECT_NONE) {
    // If we set the status in the agent callback, use it
    // (it is probably the invalid key error)
    connectStatus = connectInfo.status;
  }

  Log::Write("unregistering agent");
  (void) UnregisterAgent(&connectInfo);

  g_variant_unref(services);
  g_variant_unref(serviceVariant);
  g_object_unref(service);

  return connectStatus;
}

bool RemoveWifiService(std::string ssid) {
  std::string nameFromHex = hexStringToAsciiString(ssid);

  GVariant* services = nullptr;
  WifiScanErrorCode result = GetConnManServices(&services);
  if (result != WifiScanErrorCode::SUCCESS) {
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

    bool hasName = false;
    bool matchedName = false;
    bool matchedInterface = false;
    bool matchedType = false;

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
              g_variant_unref(ethernet_attr);
              g_variant_unref(ethernet_key_v);
              g_variant_unref(ethernet_val_v);
              g_variant_unref(ethernet_val);
              break;
            }
          }

          g_variant_unref(ethernet_attr);
          g_variant_unref(ethernet_key_v);
          g_variant_unref(ethernet_val_v);
          g_variant_unref(ethernet_val);
        }
      }

      g_variant_unref(attr);
      g_variant_unref(key_v);
      g_variant_unref(val_v);
      g_variant_unref(val);
    }

    if(matchedName && matchedInterface && matchedType) {
      // this is our service
      serviceVariant = child;
      foundService = true;
    }

    g_variant_unref(attrs);
    if(child != serviceVariant) {
      g_variant_unref(child);
    }
  }

  if(!foundService) {
    loge("Could not find service...");
    g_variant_unref(services);
    return false;
  }

  std::string servicePath = GetObjectPathForService(serviceVariant);
  Log::Write("Removing %s.", servicePath.c_str());

  // Get the ConnManBusService for our object path
  Log::Write("Service path: %s", servicePath.c_str());
  ConnManBusService* service = GetServiceForPath(servicePath);
  if(service == nullptr) {
    g_variant_unref(services);
    g_variant_unref(serviceVariant);
    return false;
  }

  GError* error = nullptr;
  bool success = conn_man_bus_service_call_remove_sync(
    service,
    nullptr,
    &error);

  g_object_unref(service);
  g_variant_unref(services);
  g_variant_unref(serviceVariant);

  return success && !error;
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

ConnectWifiResult ConnectToWifiService(ConnManBusService* service) {

  if(service == nullptr) {
    return ConnectWifiResult::CONNECT_FAILURE;
  }

  GCond connectCond;
  g_cond_init(&connectCond);
  g_mutex_lock(&connectMutex);

  struct ConnectInfo data;

  data.error = nullptr;
  data.cond = &connectCond;
  data.service = service;

  conn_man_bus_service_call_connect (
    service,
    nullptr,
    ConnectCallback,
    (gpointer)&data);

  g_cond_wait(&connectCond, &connectMutex);
  g_mutex_unlock(&connectMutex);

  if(data.error != nullptr) {
    Log::Write("Connect error: %s", data.error->message);
  }

  return (data.error == nullptr)? ConnectWifiResult::CONNECT_SUCCESS:
    ConnectWifiResult::CONNECT_FAILURE;
}

bool DisconnectFromWifiService(ConnManBusService* service) {
  if(service == nullptr) {
    return false;
  }
  
  GError* error = nullptr;
  return conn_man_bus_service_call_disconnect_sync (service, nullptr, &error);
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

  int rc = WriteFileAtomically(GetPathToWiFiConfigFile(),
                               wifiConfigStream.str(),
                               kModeUserGroupReadWrite,
                               kNetUid,
                               kAnkiGid);

  if (rc) {
    std::string error = "Failed to write wifi config. rc = " + std::to_string(rc);
    callback(rc);
    return;
  }

  ExecCommandInBackground({"sudo", "/usr/bin/connmanctl", "enable", "wifi"}, callback);
}

WiFiState GetWiFiState() {
  // Return WiFiState object
  WiFiState wifiState;
  wifiState.ssid = "";
  wifiState.connState = WiFiConnState::UNKNOWN;

  GVariant* services = nullptr;
  WifiScanErrorCode result = GetConnManServices(&services);
  if (result != WifiScanErrorCode::SUCCESS) {
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

  ConnManBusTechnology* wifi_proxy = nullptr;
  wifi_proxy = conn_man_bus_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                              "net.connman",
                                                              "/net/connman/technology/wifi",
                                                              nullptr,
                                                              &error);

  if(error != nullptr) {
    loge("error asking connman about access point mode: %s", error->message);
    return false;
  }

  bool success = (bool)conn_man_bus_technology_call_get_properties_sync (
    wifi_proxy,
    &properties,
    nullptr,
    &error);

  if(error != nullptr || !success) {
    if (error) {
      loge("error asking connman to get properties about access point mode: %s", error->message);
    }
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
  ConnManBusTechnology* wifi_proxy = GetConnManBusTechnologyForWiFi();
  if (!wifi_proxy) {
    return false;
  }

  if (!SetStringWiFiProperty(wifi_proxy, "TetheringIdentifier", ssid.c_str(), nullptr)) {
    g_object_unref(wifi_proxy);
    return false;
  }

  if (!SetStringWiFiProperty(wifi_proxy, "TetheringPassphrase", pw.c_str(), nullptr)) {
    g_object_unref(wifi_proxy);
    return false;
  }

  if (!SetBooleanWiFiProperty(wifi_proxy, "Tethering", true, kConnmanErrorAlreadyEnabledString)) {
    g_object_unref(wifi_proxy);
    return false;
  }

  g_object_unref(wifi_proxy);
  return true;
}

bool DisableAccessPointMode() {
  return SetBooleanWiFiProperty(nullptr,
                                "Tethering",
                                false,
                                kConnmanErrorAlreadyDisabledString);
}

bool EnableWiFi() {
  return SetBooleanWiFiProperty(nullptr,
                                "Powered",
                                true,
                                kConnmanErrorAlreadyEnabledString);
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
