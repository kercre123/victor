/**
 * File: wifi.cpp
 *
 * Author: Mathew Prokos
 * Created: 7/10/2018
 *
 * Description: Routines for scanning and configuring Wifi
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <glib.h>
#include <NetworkManager.h>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <nm-setting-wireless-security.h>
#include <nm-device-wifi.h>
#ifndef GTEST
#include "anki-ble/common/stringutils.h"
#include "exec_command.h"
#endif
#include "wifi.h"
#include "log.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"


#define HOTSPOT_ID "hotspot"
#define HOTSPOT_IP "10.0.0.1"
#define DISCONNECT_TIMEOUT_MS 90000
#define CONNECT_CHECK_TIMEOUT_SEC 60

struct CBState {
  GMainLoop *loop;
  Anki::Wifi::ConnectWifiResult result;
  std::string path;
};

static void RecordDeviceState (NMDevice *device,
                               bool newConnState, bool oldConnState,
                               bool newApMode, bool oldApMode)
{

  static bool sInitial = true;
  const char MAC_MANUFAC_BYTES = 3;
  NMAccessPoint *ap = nullptr;
  const char *bssid = nullptr;
  std::string apMacManufacturerBytes = "";
  bool status = false;

  if(newConnState != oldConnState || newApMode != oldApMode || sInitial) {
    ap = nm_device_wifi_get_active_access_point((NMDeviceWifi*)device);
    if(ap) {
      bssid = nm_access_point_get_bssid(ap);
      if(bssid) {
        // Strip ap MAC of last three bytes
        for(int i = 0; i < MAC_MANUFAC_BYTES; i++) {
          std::stringstream ss;
          ss << std::setfill('0') << std::setw(2) << std::hex << (int)(unsigned char)bssid[i];
          apMacManufacturerBytes += ss.str();
        }
      }
    }

    status = newConnState && !newApMode;

    Log::Write("WiFi connection status changed: [connected=%s / mac=%s]", 
               status ? "true":"false", apMacManufacturerBytes.c_str());

    std::string event = status ? "wifi.connection":"wifi.disconnection";

    DASMSG(wifi_connection_status, event,
           "WiFi connection status changed.");
    DASMSG_SET(s4, apMacManufacturerBytes, "AP MAC manufacturer bytes");
    DASMSG_SEND();
  }

  sInitial = false;
}

static gboolean ConnectionCheckCb(gpointer user_data) {
  Anki::Wifi::Wifi *wifi = (Anki::Wifi::Wifi *)user_data;
  if(wifi) {
    return wifi->ConnectionCheck();
  } else {
    return FALSE;
  }
}


static void ConnectionAddedCb (GObject *client,
                               GAsyncResult *result,
                               gpointer user_data)
{
  NMRemoteConnection *remote;
  GError *error = nullptr;
  struct CBState *state  = (struct CBState*)user_data;
  GMainLoop *loop = state->loop;
  state->result = Anki::Wifi::ConnectWifiResult::CONNECT_SUCCESS;

  remote = nm_client_add_connection_finish (NM_CLIENT (client), result, &error);

  if (error) {
    Log::Error("Error adding connection: %s", error->message);
    state->result = Anki::Wifi::ConnectWifiResult::CONNECT_FAILURE;
    g_error_free (error);
  } else {
    state->path = std::string(nm_connection_get_path (NM_CONNECTION (remote)));
    Log::Write("Added: %s\n", state->path.c_str());
    g_object_unref (remote);
  }
  g_main_loop_quit (loop);
}

static void ActivateNewCb (GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
  GError *error = NULL;
  struct CBState *state = (struct CBState*)user_data;
  state->result = Anki::Wifi::ConnectWifiResult::CONNECT_SUCCESS;

  if (!nm_client_activate_connection_finish (NM_CLIENT (source_object), res, &error)) {
    g_warning ("Failed to add new connection: (%d) %s", error->code, error->message);
    state->result = Anki::Wifi::ConnectWifiResult::CONNECT_FAILURE;
    g_error_free (error);
  }

  if(state->loop) {
    g_main_loop_quit (state->loop);
  }
}

static void AddActivateNewCb (GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
  GError *error = NULL;
  GMainLoop *loop = (GMainLoop*)user_data;

  if (!nm_client_add_and_activate_connection_finish (NM_CLIENT (source_object), res, &error)) {
    g_warning ("Failed to add new connection: (%d) %s", error->code, error->message);
    g_error_free (error);
  }

  if(loop) {
    g_main_loop_quit (loop);
  }
}

static gpointer ConnectionThread(gpointer data)
{
  GMainContext * context = g_main_context_new();
  GMainLoop *loop = g_main_loop_new(context, TRUE);

  Log::Write("Starting Connection Thread");
  if (!loop) {
    Log::Error("Error getting main loop");
    return nullptr;
  }

  GSource *s = g_timeout_source_new_seconds(CONNECT_CHECK_TIMEOUT_SEC);
  g_source_set_callback(s, ConnectionCheckCb, data, NULL);
  g_source_attach(s, context);
  g_source_unref(s);

  g_main_loop_run(loop);
  g_main_loop_unref(loop);
  return nullptr;
}


namespace Anki {
namespace Wifi {

  Wifi::Wifi() {}
  Wifi::~Wifi() {}

  void Wifi::Initialize() {
    _offlineTime_ms = Anki::Vector::HAL::GetTimeStamp();
    _oldConnState = false;
    _oldApMode = false;
    static GThread *thread2 = g_thread_new("init_thread", ConnectionThread, this);
    if (thread2 == nullptr) {
      Log::Write("couldn't spawn init thread");
      return;
    }
  }

  void Wifi::InitializeClient(NMClient *client)
  {
    GError *error = nullptr;
    NMDevice *device = nullptr;

    bool networkEnabled = nm_client_networking_get_enabled(client);
    bool wirelessEnabled = nm_client_wireless_get_enabled(client);

    if(!networkEnabled) {
      if(!nm_client_networking_set_enabled(client, true, &error)) {
        Log::Error("NetworkManager networking:failed:%s", error->message);
        g_error_free (error);
      } else {
        Log::Write("NetworkManager networking:enabled");
      }
    }

    if(!wirelessEnabled) {
      nm_client_wireless_set_enabled(client, true);
      Log::Write("NetworkManager wireless:enabled");
    }

    device = GetWifiDevice(client);
    if(device) {
      nm_device_set_managed(device, true);
      nm_device_set_autoconnect(device, true);
    }
  }

  ConnectWifiResult Wifi::ConnectWifiBySsid(std::string hexSSID, std::string pw, WifiAuth auth, bool hidden)
  {
    NMConnection *connection = nullptr;
    NMSettingConnection *connectionSetting;
    NMSetting *wirelessSetting;
    NMSetting *wirelessSecuritySetting;
    NMSetting *ip4Setting;
    NMSetting *ip6Setting;
    GBytes *ssid;
    std::vector<APConnection> availableConnections;
    std::vector<WifiScanResult> results;
    char *uuid = nullptr;
    NMClient *client = nullptr;
    GMainLoop *loop = nullptr;
    GError *error = nullptr;
    struct CBState state = {nullptr, ConnectWifiResult::CONNECT_FAILURE};
    std::string statusString = "failure";
    std::string errorstring = "None";

    ToUpper(hexSSID);

    if(IsAccessPointMode()) {
      DisableAccessPointMode();
    }

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
      return ConnectWifiResult::CONNECT_FAILURE;
    } else {

      loop = g_main_loop_new (nullptr, FALSE);

      RemoveWifiService(client, hexSSID);

      connection = nm_simple_connection_new ();
      uuid = nm_utils_uuid_generate ();

      connectionSetting = (NMSettingConnection *) nm_setting_connection_new ();
      g_object_set (G_OBJECT (connectionSetting),
                    NM_SETTING_CONNECTION_ID, hexSSID.c_str(),
                    NM_SETTING_CONNECTION_UUID, uuid,
                    NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
                    NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                    nullptr);
      nm_connection_add_setting (connection, NM_SETTING (connectionSetting));
      g_free (uuid);

      wirelessSetting = nm_setting_wireless_new ();
      g_assert(wirelessSetting);
      nm_connection_add_setting (connection, wirelessSetting);

      ssid = nm_utils_hexstr2bin(hexSSID.c_str());

      g_object_set (wirelessSetting,
                    NM_SETTING_WIRELESS_SSID, ssid,
                    NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_INFRA,
                    NM_SETTING_WIRELESS_HIDDEN, hidden,
                    nullptr);
      g_bytes_unref (ssid);

      if(auth != AUTH_NONE_OPEN) {
        wirelessSecuritySetting = nm_setting_wireless_security_new();
        g_assert(wirelessSecuritySetting);

        g_object_set (G_OBJECT (wirelessSecuritySetting),
                      NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, WifiAuth2Key(auth).c_str(),
                      nullptr);

        if(!pw.empty()) {
          g_object_set (G_OBJECT (wirelessSecuritySetting),
                        NM_SETTING_WIRELESS_SECURITY_PSK, pw.c_str(),
                        nullptr);
        }

        if(!WifiAuth2Auth(auth).empty()) {
          g_object_set (G_OBJECT (wirelessSecuritySetting),
                        NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, WifiAuth2Auth(auth).c_str(),
                        nullptr);
        }

        nm_connection_add_setting (connection, wirelessSecuritySetting);
      }


      ip4Setting = nm_setting_ip4_config_new ();
      g_object_set (G_OBJECT (ip4Setting),
                    NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_AUTO,
                    nullptr);
      nm_connection_add_setting (connection, NM_SETTING (ip4Setting));

      ip6Setting = nm_setting_ip6_config_new ();
      g_object_set (G_OBJECT (ip6Setting),
                    NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP6_CONFIG_METHOD_AUTO,
                    nullptr);
      nm_connection_add_setting (connection, NM_SETTING (ip6Setting));

      state.loop = loop;
      nm_client_add_connection_async(client, connection, TRUE, nullptr, ConnectionAddedCb, &state);
      g_main_loop_run(loop);

      ScanForWifiAccessPoints(client,
                              results,
                              &availableConnections,
                              false);

      for(auto const &apc : availableConnections) {
        if(std::string(nm_connection_get_path (NM_CONNECTION (apc.conn))) == state.path) {
          NMDevice *device = GetWifiDevice(client);
          const char *path = nm_object_get_path((NMObject*)apc.ap);
          struct CBState state = {loop, ConnectWifiResult::CONNECT_FAILURE, ""};
          Log::Write("AP for new connection found!");
          nm_client_activate_connection_async(client, apc.conn, device, path, NULL, ActivateNewCb, &state);
          g_main_loop_run(loop);
        }
      }

      g_object_unref(connection);
      g_object_unref (client);
      g_main_loop_unref(loop);
    }

    switch(state.result) {
    case ConnectWifiResult::CONNECT_SUCCESS:
      statusString = "success";
      break;
    case ConnectWifiResult::CONNECT_INVALIDKEY:
      errorstring = "invalid password";
      break;
    default:
      statusString = "failure";
      errorstring = "unknown";
      break;
    }

    DASMSG(wifi_connection_status, "wifi.manual_connect_attempt",
           "WiFi connection attempt.");
    DASMSG_SET(s1, statusString, "Connection attempt result");
    DASMSG_SET(s2, errorstring, "Error reason");
    DASMSG_SET(s3, hidden?"hidden":"visible", "SSID broadcast");
    DASMSG_SEND();

    return state.result;
  }

  bool Wifi::ConnectionCheck() {
    NMClient *client = nullptr;
    std::vector<APConnection> availableConnections;
    bool newConnState = false;
    bool newApMode;
    NMDevice *device;
    GError *error = nullptr;

    std::vector<WifiScanResult> results;
    Anki::Wifi::WifiState wifiState;
    Anki::TimeStamp_t curTime_ms;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    device = GetWifiDevice(client);
    newApMode = nm_device_wifi_get_mode((NMDeviceWifi*)device) == NM_802_11_MODE_AP;

    if(client) {
      switch(nm_client_get_state(client)) {
      case NM_STATE_CONNECTED_LOCAL:
      case NM_STATE_CONNECTED_SITE:
      case NM_STATE_CONNECTED_GLOBAL:
        if(!newApMode) {
          newConnState = true;
        }
        break;
      default:
        newConnState = false;
      }

      //record DAS
      RecordDeviceState (device,
                         newConnState, _oldConnState,
                         newApMode, _oldApMode);

      //set offline timer when disconnected
      if(_oldConnState != newConnState) {
        if(!newConnState) {
          Log::Write("Starting offline timer");
          _offlineTime_ms = Anki::Vector::HAL::GetTimeStamp();
        }
      }

      _oldConnState = newConnState;
      _oldApMode = newApMode;

      if(!newApMode && !newConnState)
      {
        ScanForWifiAccessPoints(client,
                                results,
                                &availableConnections,
                                true);
      }

      // Kick the network if we are not connected but we either see available
      // connections or no scan results. Both of these conditions may indicate a
      // problem
      if(!newConnState
          && !newApMode
          && (!availableConnections.empty() || results.empty()))
      {
        curTime_ms = Anki::Vector::HAL::GetTimeStamp();
        if((curTime_ms - _offlineTime_ms) > DISCONNECT_TIMEOUT_MS) {
          Log::Error("Unable to connect: Restarting Networking");
          std::vector<std::string> restart_wpa = {"/bin/systemctl", "restart", "wpa_supplicant"};
          std::vector<std::string> restart_nm = {"/bin/systemctl", "restart", "NetworkManager"};
          Anki::ExecCommand(restart_wpa);
          Anki::ExecCommand(restart_nm);
          _offlineTime_ms = Anki::Vector::HAL::GetTimeStamp();
        } else if (!availableConnections.empty()) {
          Log::Write("Available connections:%d waiting %d", availableConnections.size(), _offlineTime_ms + DISCONNECT_TIMEOUT_MS - curTime_ms);
        }
      }

      g_object_unref(client);
    }

    return true;
  }

  bool Wifi::DisableAccessPointMode()
  {
    NMClient *client = nullptr;
    GError *error = nullptr;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
    } else {
      _offlineTime_ms = Anki::Vector::HAL::GetTimeStamp();
      RemoveWifiService(client, HOTSPOT_ID);
      g_object_unref (client);
    }
    return false;
  }

  bool Wifi::ReloadConnections(NMClient *client)
  {
    GError *error = nullptr;

    InitializeClient(client);
    nm_client_reload_connections(client, nullptr, &error);
    if(error) {
      Log::Error("Could not reload connections: %s.", error->message);
      g_error_free (error);
      return false;
    }
    return true;
  }

  bool Wifi::EnableAccessPointMode(std::string ssid, std::string pw)
  {
    NMConnection *connection = nullptr;
    NMSettingConnection *connectionSetting;
    NMSetting *wirelessSetting;
    NMSetting *wirelessSecuritySetting;
    NMSetting *ip4Setting;
    GBytes *SSIDBytes;
    char *uuid = nullptr;
    NMClient *client = nullptr;
    GMainLoop *loop = nullptr;
    GError *error = nullptr;
    GPtrArray *addrs;

    SSIDBytes = g_bytes_new (ssid.c_str(), strlen (ssid.c_str()));
    _apSsid_hex = std::string(nm_utils_bin2hexstr(g_bytes_get_data(SSIDBytes,nullptr),
                                                   g_bytes_get_size(SSIDBytes), -1));

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
      return ConnectWifiResult::CONNECT_FAILURE;
    } else {

      NMIPAddress* addr = nm_ip_address_new(AF_INET, HOTSPOT_IP, 24, NULL);

      loop = g_main_loop_new (nullptr, FALSE);

      addrs = g_ptr_array_new();
      g_ptr_array_add(addrs, (gpointer)addr);

      RemoveWifiService(client, HOTSPOT_ID);

      connection = nm_simple_connection_new ();
      uuid = nm_utils_uuid_generate ();

      connectionSetting = (NMSettingConnection *) nm_setting_connection_new ();
      g_object_set (G_OBJECT (connectionSetting),
                    NM_SETTING_CONNECTION_ID, HOTSPOT_ID,
                    NM_SETTING_CONNECTION_UUID, uuid,
                    NM_SETTING_CONNECTION_AUTOCONNECT, FALSE,
                    NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                    nullptr);

      nm_connection_add_setting (connection, NM_SETTING (connectionSetting));
      g_free (uuid);

      wirelessSetting = nm_setting_wireless_new ();
      g_assert(wirelessSetting);
      nm_connection_add_setting (connection, wirelessSetting);


      g_object_set (wirelessSetting,
                    NM_SETTING_WIRELESS_SSID, SSIDBytes,
                    NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_AP,
                    nullptr);
      g_bytes_unref (SSIDBytes);

      wirelessSecuritySetting = nm_setting_wireless_security_new();
      g_assert(wirelessSecuritySetting);

      g_object_set (G_OBJECT (wirelessSecuritySetting),
                    NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                    NM_SETTING_WIRELESS_SECURITY_PSK, pw.c_str(),
                    nullptr);
      nm_connection_add_setting (connection, wirelessSecuritySetting);

      ip4Setting = nm_setting_ip4_config_new ();
      g_object_set (G_OBJECT (ip4Setting),
                    NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_MANUAL,
                    NM_SETTING_IP_CONFIG_ADDRESSES, addrs,
                    nullptr);
      nm_connection_add_setting (connection, NM_SETTING (ip4Setting));

      NMDevice *device = GetWifiDevice(client);
      if(device) {
        const char *path = nm_object_get_path((NMObject*)device);
        if(path) {
          nm_client_add_and_activate_connection_async (client, connection, device, path, nullptr, AddActivateNewCb, loop);
          g_main_loop_run(loop);
        }
      }

      nm_ip_address_unref(addr);
      g_ptr_array_free(addrs, true);
      g_object_unref(connection);
      g_object_unref (client);
      g_main_loop_unref(loop);
    }

    return ConnectWifiResult::CONNECT_SUCCESS;
  }

  WifiIpFlags Wifi::GetIpAddress(struct in_addr* ipv4_32bits, struct in6_addr* ipv6_128bits)
  {
    NMDevice *device = nullptr;
    NMIPConfig * ip4 = nullptr;
    NMIPConfig * ip6 = nullptr;
    WifiIpFlags wifiFlags = WifiIpFlags::NONE;
    NMIPAddress *ipaddr = nullptr;
    GPtrArray *ip4Addresses = nullptr;
    GPtrArray *ip6Addresses = nullptr;
    NMClient *client = nullptr;
    GError *error = nullptr;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
    } else {

      memset(ipv4_32bits, 0, 4);
      memset(ipv6_128bits, 0, 16);

      device = GetWifiDevice(client);

      ip4 = nm_device_get_ip4_config(device);
      if(ip4) {
        ip4Addresses = nm_ip_config_get_addresses(ip4);
      }
      if(ip4Addresses && ip4Addresses->len) {
        for(int i = 0 ; i < ip4Addresses->len; i++) {
          ipaddr = (NMIPAddress*)ip4Addresses->pdata[i];
          nm_ip_address_get_address_binary(ipaddr, (gpointer)&ipv4_32bits->s_addr);
          const char *ip4_str = nm_utils_inet4_ntop (ipv4_32bits->s_addr, nullptr);
          if(nm_utils_ipaddr_valid(AF_INET, ip4_str)) {
            wifiFlags = (WifiIpFlags) wifiFlags | WifiIpFlags::HAS_IPV4;
            break;
          }
        }
      }

      ip6 = nm_device_get_ip6_config(device);
      if(ip6) {
        ip6Addresses = nm_ip_config_get_addresses(ip6);
      }
      if(ip6Addresses && ip6Addresses->len) {
        ipaddr = (NMIPAddress*)ip6Addresses->pdata[0];
        nm_ip_address_get_address_binary(ipaddr, (gpointer)ipv6_128bits->s6_addr);
        const char *ip6_str = nm_utils_inet6_ntop (ipv6_128bits, nullptr);
        if(nm_utils_ipaddr_valid(AF_INET6, ip6_str)) {
          wifiFlags = (WifiIpFlags) wifiFlags | WifiIpFlags::HAS_IPV6;
        }
      }
      g_object_unref (client);
    }
    return wifiFlags;
  }

  void Wifi::GetWifiState(WifiState &wifiState)
  {
    NMDevice *device;
    NMState state;
    const char * ssid=nullptr;
    NMClient *client = nullptr;
    GError *error = nullptr;
    NMActiveConnection *connection=nullptr;

    if(IsAccessPointMode()) {
      wifiState.connState = WifiConnState::DISCONNECTED;
      wifiState.ssid = _apSsid_hex;
      return;
    }

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
    } else {
      state = nm_client_get_state(client);
      switch(state) {
      case NM_STATE_CONNECTED_LOCAL:
      case NM_STATE_CONNECTED_SITE:
        wifiState.connState = WifiConnState::CONNECTED;
        break;
      case NM_STATE_CONNECTED_GLOBAL:
        wifiState.connState = WifiConnState::ONLINE;
        break;
      default:
        wifiState.connState = WifiConnState::DISCONNECTED;
      }

      device = GetWifiDevice(client);

      if(device) {
        connection = nm_device_get_active_connection(device);
      }
      if(connection) {
        ssid = nm_active_connection_get_id(connection);
        if(ssid) {
          wifiState.ssid = std::string(ssid);
        }
      }
      g_object_unref (client);
    }
  }

  bool Wifi::IsAccessPointMode()
  {
    NMDevice *device;
    NMClient *client = nullptr;
    GError *error = nullptr;
    bool isAp = false;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
    } else {
      device = GetWifiDevice(client);
      if(device) {
        isAp = nm_device_wifi_get_mode((NMDeviceWifi*)device) == NM_802_11_MODE_AP;
      }

      g_object_unref (client);
    }

    return isAp;
  }

  NMDevice * Wifi::GetWifiDevice() {
    NMClient *client = nullptr;
    GError *error = nullptr;
    NMDevice *device = nullptr;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
    } else {
      device = GetWifiDevice(client);
      g_object_unref (client);
    }
    return device;
  }

  NMDevice * Wifi::GetWifiDevice(NMClient *client)
  {
    NMDevice *device = nullptr;
    NMDevice *wifiDevice = nullptr;
    NMDeviceType deviceType = NM_DEVICE_TYPE_UNKNOWN;
    const GPtrArray *devices = nullptr;

    devices = nm_client_get_devices(client);
    for (int i=0; i < devices->len; i++) {
      deviceType = NM_DEVICE_TYPE_UNKNOWN;

      device = (NMDevice*)devices->pdata[i];
      if(device) {
        deviceType = nm_device_get_device_type(device);
      }

      if(deviceType == NM_DEVICE_TYPE_WIFI) {
        wifiDevice = device;
      }
    }

    if(!device) {
      Log::Error("GetWifiDevice: Could not find Wifi Device");
    }

    return wifiDevice;
  }


  WifiScanErrorCode Wifi::ScanForWifiAccessPoints(std::vector<WifiScanResult>& results)
  {
    WifiScanErrorCode status = WifiScanErrorCode::FAILED_SCANNING;
    NMClient *client = nullptr;
    GError *error = nullptr;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
    } else {
      status = ScanForWifiAccessPoints(client, results, nullptr, false);
      g_object_unref(client);
    }
    return status;
  }

  WifiScanErrorCode Wifi::ScanForWifiAccessPoints(NMClient *client,
                                                  std::vector<WifiScanResult>& results,
                                                  std::vector<APConnection> *available,
                                                  bool request)
  {
    NMDevice *device;
    const GPtrArray *scanResults = nullptr;
    APConnection apconn;
    GBytes* SSIDBytes = nullptr;
    std::map<std::string, WifiScanResult> resultMap;

    device = GetWifiDevice(client);
    if(device) {
      if(request) {
        nm_device_wifi_request_scan(NM_DEVICE_WIFI(device), nullptr, nullptr);
      }
      scanResults = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(device));
    }

    if(scanResults) {
      for(int i = 0; i < scanResults->len; i++) {
        WifiScanResult result;
        NMAccessPoint *ap = (NMAccessPoint*)scanResults->pdata[i];
        NM80211ApSecurityFlags security_wpa_flags = nm_access_point_get_wpa_flags(ap);
        NM80211ApSecurityFlags security_wpa2_flags = nm_access_point_get_rsn_flags(ap);
        NM80211ApFlags ap_flags = nm_access_point_get_flags(ap);
        gint signal = nm_access_point_get_strength(ap);
        SSIDBytes = nm_access_point_get_ssid(ap);

        result.hidden = true;
        result.auth  = APSecurity2WifiAuth(security_wpa_flags);
        if(result.auth == AUTH_NONE_OPEN) {
          result.auth = APSecurity2WifiAuth(security_wpa2_flags);
        }

        if(result.auth == AUTH_NONE_OPEN) {
          result.auth = ap_flags == NM_802_11_AP_FLAGS_PRIVACY ? WifiAuth::AUTH_NONE_WEP : WifiAuth::AUTH_NONE_OPEN;
        }

        if(SSIDBytes) {
          result.ssid = std::string(nm_utils_bin2hexstr(g_bytes_get_data(SSIDBytes,nullptr), g_bytes_get_size(SSIDBytes), -1));
          ToUpper(result.ssid);
          Log::Write("bssid found: %s %s", result.ssid.c_str(), nm_access_point_get_bssid(ap));
          result.hidden = false;

          result.provisioned = HaveMatchingConnection(client, ap, apconn);
          if(result.provisioned && available) {
            available->push_back(apconn);
          }
          result.encrypted = result.auth == AUTH_NONE_OPEN ? false : true;
          result.wps = ap_flags & NM_802_11_AP_FLAGS_WPS ? true : false;
          result.signal_level = signal;

          if(resultMap.find(result.ssid) != resultMap.end()) {
            if(resultMap[result.ssid].signal_level < result.signal_level)
              resultMap[result.ssid] = result;
          } else {
            resultMap[result.ssid] = result;
          }
        }
      }

      for(const auto &ap : resultMap) {
        results.push_back(ap.second);
      }

    } else {
      Log::Error("No scan results found");
      return WifiScanErrorCode::FAILED_SCANNING;
    }
    return WifiScanErrorCode::SUCCESS;
  }

  bool Wifi::RemoveWifiServiceAll()
  {
    NMClient *client = nullptr;
    GError *error = nullptr;
    bool ret = false;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
      return ret;
    } else {
      std::string empty;
      ret = RemoveWifiService(client, empty);
      g_object_unref (client);
    }

    return ret;
  }

  bool Wifi::RemoveWifiService(std::string hexSSID)
  {
    NMClient *client = nullptr;
    GError *error = nullptr;
    bool ret = false;

    std::lock_guard<std::mutex> lock(_lock);
    client = nm_client_new (nullptr, &error);
    if (!client) {
      Log::Error("Could not connect to NetworkManager: %s.", error->message);
      g_error_free (error);
      return ret;
    } else {
      ret = RemoveWifiService(client, hexSSID);
      g_object_unref (client);
    }

    return ret;
  }

  bool Wifi::RemoveWifiService(NMClient *client, std::string hexSSID)
  {
    NMRemoteConnection *rconnection = nullptr;
    char *uuid = nullptr;
    const GPtrArray *connArray = nullptr;
    bool removed = false;
    GError *error = nullptr;

    ToUpper(hexSSID);

    /* Remove any existing connections for this SSID */
    connArray = nm_client_get_connections(client);
    for(int i=0; i < connArray->len; i++) {
      NMConnection *conn = (NMConnection*)connArray->pdata[i];
      std::string connid = nm_connection_get_id(conn);
      ToUpper(connid);
      if(hexSSID == connid || hexSSID.empty()) {
        if (error) {
          Log::Error("Could not disconnect: %s.", error->message);
          g_error_free (error);
        }

        uuid = (char*)nm_connection_get_uuid(conn);
        if(uuid) {
          rconnection = nm_client_get_connection_by_uuid(client, uuid);
        } else {
          Log::Error("Could not find uuid for: %s.", hexSSID.c_str());
        }
        if(rconnection) {
          nm_remote_connection_delete(rconnection, NULL, NULL);
          removed = true;
        } else {
          Log::Error("Could not find remote connection for: %s.", uuid);
        }
      }
    }
    ReloadConnections(client);
    return removed;
  }

  bool Wifi::HaveMatchingConnection(NMClient *client, NMAccessPoint *ap, APConnection &conn)
  {
    const GPtrArray *connArray = nullptr;

    connArray = nm_client_get_connections(client);
    for(int i=0; i < connArray->len; i++) {
      NMConnection *c = (NMConnection*)connArray->pdata[i];
      if(nm_access_point_connection_valid(ap, c)) {
        conn.ap = ap;
        conn.conn = c;
        return true;
      }
    }
    return false;
  }

  std::string Wifi::WifiAuth2Auth(WifiAuth a)
  {
    switch(a) {
    case AUTH_NONE_OPEN:
      return "open";
    case AUTH_NONE_WEP:
    case AUTH_NONE_WEP_SHARED:
      return "shared";
    default:
      return "";
    }
  }

  std::string Wifi::WifiAuth2Key(WifiAuth a)
  {
    switch(a) {
    case AUTH_NONE_WEP:
    case AUTH_NONE_OPEN:
    case AUTH_NONE_WEP_SHARED:
      return "none";
    case AUTH_IEEE8021X:
      return "ieee8021x";
    case AUTH_WPA_PSK:
    case AUTH_WPA2_PSK:
      return "wpa-psk";
    case AUTH_WPA_EAP:
    case AUTH_WPA2_EAP:
      return "wpa-eap";
    }
  }

  WifiAuth Wifi::APSecurity2WifiAuth(const NM80211ApSecurityFlags a)
  {
    if(a & NM_802_11_AP_SEC_NONE) {
      return AUTH_NONE_OPEN;
    }

    if(a & NM_802_11_AP_SEC_GROUP_WEP40 ||
       a & NM_802_11_AP_SEC_GROUP_WEP104 ||
       a & NM_802_11_AP_SEC_PAIR_WEP40 ||
       a & NM_802_11_AP_SEC_PAIR_WEP104) {
      return AUTH_NONE_WEP_SHARED;
    }

    if(a & NM_802_11_AP_SEC_KEY_MGMT_PSK ||
       a & NM_802_11_AP_SEC_PAIR_TKIP ||
       a & NM_802_11_AP_SEC_GROUP_TKIP) {
      return AUTH_WPA_PSK;
    }

    if(a & NM_802_11_AP_SEC_PAIR_CCMP ||
       a & NM_802_11_AP_SEC_GROUP_CCMP) {
      return AUTH_WPA2_PSK;
    }

    if(a & NM_802_11_AP_SEC_KEY_MGMT_802_1X) {
      return AUTH_IEEE8021X;
    }

    return AUTH_NONE_OPEN;
  }
} // namespace Wifi
} // namespace Anki
