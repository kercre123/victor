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
#include <nm-setting-wireless-security.h>
#include <nm-device-wifi.h>
#ifndef GTEST
#include "anki-ble/common/stringutils.h"
#endif
#include "wifi.h"
#include "log.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"


#define HOTSPOT_ID "hotspot"
#define HOTSPOT_IP "10.0.0.1"

struct CBState {
  GMainLoop *loop;
  Anki::Wifi::ConnectWifiResult result;
};

NMDevice * getWifiDevice(NMClient *client)
{
  NMDevice *device = nullptr;
  NMDevice *wifiDevice = nullptr;
  NMDeviceType device_type = NM_DEVICE_TYPE_UNKNOWN;
  const GPtrArray *devices = nullptr;

  devices = nm_client_get_devices(client);
  for (int i=0; i < devices->len; i++) {
    device_type = NM_DEVICE_TYPE_UNKNOWN;

    device = (NMDevice*)devices->pdata[i];
    if(device) {
      device_type = nm_device_get_device_type(device);
    }

    if(device_type == NM_DEVICE_TYPE_WIFI) {
      wifiDevice = device;
    }
  }

  if(!device) {
    Log::Error("getWifiDevice: Could not find Wifi Device");
  }

  return wifiDevice;
}

static void device_state_changed_cb (NMDevice *device, guint new_state, guint old_state,
                                     guint reason, gpointer user_data)
{

  const char MAC_MANUFAC_BYTES = 3;
  NMDeviceState state = nm_device_get_state(device);
  NMAccessPoint *ap = nm_device_wifi_get_active_access_point((NMDeviceWifi*)device);
  const char *bssid = nullptr;
  std::string apMacManufacturerBytes = "";

  if(ap) {
    bssid = nm_access_point_get_bssid(ap);
    if(bssid) {
      // Strip ap MAC of last three bytes
      for(int i = 0; i < MAC_MANUFAC_BYTES; i++) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(unsigned char)bssid[i];
        apMacManufacturerBytes += ss.str();
      }
      std::string connectionStatus = state & NM_DEVICE_STATE_ACTIVATED ? "Connected." : "Disconnected.";
      DASMSG(wifi_connection_status, "wifi.status",
             "Wifi connection status has changed.");
      DASMSG_SET(s1, connectionStatus, "Connection status");
      DASMSG_SET(s2, apMacManufacturerBytes, "AP MAC manufacturer bytes");
      DASMSG_SEND();
    }
  }
}


static void connection_added_cb (GObject *client,
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
    Log::Write("Added: %s\n", nm_connection_get_path (NM_CONNECTION (remote)));
    g_object_unref (remote);
  }
  g_main_loop_quit (loop);
}

static void activate_new_cb (GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
  GError *error = NULL;
  GMainLoop *loop = (GMainLoop*)user_data;


  if (!nm_client_add_and_activate_connection_finish (NM_CLIENT (source_object), res, &error)) {
    g_warning ("Failed to add new connection: (%d) %s", error->code, error->message);
    g_error_free (error);
  }

  g_main_loop_quit (loop);
}

static gpointer ConnectionThread(gpointer data)
{
  NMDevice *device = nullptr;
  NMClient *client = nullptr;
  GError *error = nullptr;
  GMainContext * context = g_main_context_new();
  GMainLoop *loop = g_main_loop_new(context, true);

  if (!loop) {
    Log::Error("Error getting main loop");
    return nullptr;
  }

  client = nm_client_new (nullptr, &error);
  if (!client) {
    Log::Error("Could not connect to NetworkManager: %s.", error->message);
    g_error_free (error);
  } else {
    device = getWifiDevice(client);
    g_signal_connect(G_OBJECT(device), "state-changed", (GCallback)device_state_changed_cb, NULL);
  }

  g_main_loop_run(loop);
  g_object_unref (client);
  g_main_loop_unref(loop);
  return nullptr;
}


namespace Anki {
namespace Wifi {

    Wifi::Wifi() {}
    Wifi::~Wifi() {}

    void Wifi::Initialize() {
      GError *error = nullptr;
      NMClient *client = nullptr;

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
      } else {
        InitializeClient(client);
        g_object_unref (client);
      }

      static GThread *thread2 = g_thread_new("init_thread", ConnectionThread, nullptr);
      if (thread2 == nullptr) {
        Log::Write("couldn't spawn init thread");
        return;
      }

    }

    void Wifi::InitializeClient(NMClient *client)
    {
      GError *error = nullptr;
      NMDevice *device = nullptr;

      bool network_enabled = nm_client_networking_get_enabled(client);
      bool wireless_enabled = nm_client_wireless_get_enabled(client);


      if(!network_enabled) {
        if(!nm_client_networking_set_enabled(client, true, &error)) {
          Log::Error("NetworkManager networking:failed:%s", error->message);
          g_error_free (error);
        } else {
          Log::Write("NetworkManager networking:enabled");
        }
      }

      if(!wireless_enabled) {
        nm_client_wireless_set_enabled(client, true);
        Log::Write("NetworkManager wireless:enabled");
      }

      device = getWifiDevice(client);
      if(device) {
        nm_device_set_managed(device, true);
        nm_device_set_autoconnect(device, true);
      }
    }

    ConnectWifiResult Wifi::ConnectWifiBySsid(std::string hex_ssid, std::string pw, WifiAuth auth, bool hidden)
    {

      /* Build up the 'wired' Setting */
      NMConnection *connection = nullptr;
      NMSettingConnection *s_con;
      NMSetting *s_wireless;
      NMSetting *s_wireless_security;
      NMSetting *s_ip4;
      NMSetting *s_ip6;
      GBytes *ssid;
      char *uuid = nullptr;
      NMClient *client = nullptr;
      GMainLoop *loop = nullptr;
      GError *error = nullptr;
      struct CBState state = {nullptr, ConnectWifiResult::CONNECT_FAILURE};

      ToUpper(hex_ssid);

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
        return ConnectWifiResult::CONNECT_FAILURE;
      } else {

        loop = g_main_loop_new (nullptr, FALSE);

        RemoveWifiService(client, hex_ssid);

        connection = nm_simple_connection_new ();
        uuid = nm_utils_uuid_generate ();

        s_con = (NMSettingConnection *) nm_setting_connection_new ();
        g_object_set (G_OBJECT (s_con),
                      NM_SETTING_CONNECTION_ID, hex_ssid.c_str(),
                      NM_SETTING_CONNECTION_UUID, uuid,
                      NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
                      NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                      nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_con));
        g_free (uuid);

        s_wireless = nm_setting_wireless_new ();
        g_assert(s_wireless);
        nm_connection_add_setting (connection, s_wireless);

        ssid = nm_utils_hexstr2bin(hex_ssid.c_str());

        g_object_set (s_wireless,
                      NM_SETTING_WIRELESS_SSID, ssid,
                      NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_INFRA,
                      NM_SETTING_WIRELESS_HIDDEN, hidden,
                      nullptr);
        g_bytes_unref (ssid);

        if(auth != AUTH_NONE_OPEN) {
          s_wireless_security = nm_setting_wireless_security_new();
          g_assert(s_wireless_security);

          g_object_set (G_OBJECT (s_wireless_security),
                        NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, WifiAuth2Key(auth).c_str(),
                        nullptr);

          if(!pw.empty()) {
            g_object_set (G_OBJECT (s_wireless_security),
                          NM_SETTING_WIRELESS_SECURITY_PSK, pw.c_str(),
                          nullptr);
          }

          if(!WifiAuth2Auth(auth).empty()) {
            g_object_set (G_OBJECT (s_wireless_security),
                          NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, WifiAuth2Auth(auth).c_str(),
                          nullptr);
          }

          nm_connection_add_setting (connection, s_wireless_security);
        }


        s_ip4 = nm_setting_ip4_config_new ();
        g_object_set (G_OBJECT (s_ip4),
                      NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_AUTO,
                      nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip4));

        s_ip6 = nm_setting_ip6_config_new ();
        g_object_set (G_OBJECT (s_ip6),
                      NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP6_CONFIG_METHOD_AUTO,
                      nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip6));

        state.loop = loop;
        nm_client_add_connection_async(client,connection,TRUE, nullptr, connection_added_cb, &state);
        g_main_loop_run(loop);

        g_object_unref(connection);
        g_object_unref (client);
        g_main_loop_unref(loop);
      }
      return state.result;
    }

    bool Wifi::DisableAccessPointMode()
    {
      NMClient *client = nullptr;
      GError *error = nullptr;

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
      } else {
        RemoveWifiService(client, HOTSPOT_ID);
        g_object_unref (client);
      }
      return false;
    }

    bool Wifi::reloadConnections(NMClient *client)
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
      NMSettingConnection *s_con;
      NMSetting *s_wireless;
      NMSetting *s_wireless_security;
      NMSetting *s_ip4;
      GBytes *ssid_bytes;
      char *uuid = nullptr;
      NMClient *client = nullptr;
      GMainLoop *loop = nullptr;
      GError *error = nullptr;
      GPtrArray *addrs;

      ssid_bytes = g_bytes_new (ssid.c_str(), strlen (ssid.c_str()));
      _ap_ssid_hex = std::string(nm_utils_bin2hexstr(g_bytes_get_data(ssid_bytes,nullptr),
                                                 g_bytes_get_size(ssid_bytes), -1));

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

        s_con = (NMSettingConnection *) nm_setting_connection_new ();
        g_object_set (G_OBJECT (s_con),
                      NM_SETTING_CONNECTION_ID, HOTSPOT_ID,
                      NM_SETTING_CONNECTION_UUID, uuid,
                      NM_SETTING_CONNECTION_AUTOCONNECT, FALSE,
                      NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                      nullptr);

        nm_connection_add_setting (connection, NM_SETTING (s_con));
        g_free (uuid);

        s_wireless = nm_setting_wireless_new ();
        g_assert(s_wireless);
        nm_connection_add_setting (connection, s_wireless);


        g_object_set (s_wireless,
                      NM_SETTING_WIRELESS_SSID, ssid_bytes,
                      NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_AP,
                      nullptr);
        g_bytes_unref (ssid_bytes);

        s_wireless_security = nm_setting_wireless_security_new();
        g_assert(s_wireless_security);

        g_object_set (G_OBJECT (s_wireless_security),
                      NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                      NM_SETTING_WIRELESS_SECURITY_PSK, pw.c_str(),
                      nullptr);
        nm_connection_add_setting (connection, s_wireless_security);

        s_ip4 = nm_setting_ip4_config_new ();
        g_object_set (G_OBJECT (s_ip4),
                      NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_MANUAL,
                      NM_SETTING_IP_CONFIG_ADDRESSES, addrs,
                      nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip4));

        NMDevice *device = getWifiDevice(client);
        if(device) {
          const char *path = nm_object_get_path((NMObject*)device);
          if(path) {
            nm_client_add_and_activate_connection_async (client, connection, device, path, nullptr, activate_new_cb, loop);
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
      GPtrArray *ip4_addresses = nullptr;
      GPtrArray *ip6_addresses = nullptr;
      NMClient *client = nullptr;
      GError *error = nullptr;

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
      } else {

        memset(ipv4_32bits, 0, 4);
        memset(ipv6_128bits, 0, 16);

        device = getWifiDevice(client);

        ip4 = nm_device_get_ip4_config(device);
        if(ip4) {
          ip4_addresses = nm_ip_config_get_addresses(ip4);
        }
        if(ip4_addresses && ip4_addresses->len) {
          for(int i = 0 ; i < ip4_addresses->len; i++) {
            ipaddr = (NMIPAddress*)ip4_addresses->pdata[i];
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
          ip6_addresses = nm_ip_config_get_addresses(ip6);
        }
        if(ip6_addresses && ip6_addresses->len) {
          ipaddr = (NMIPAddress*)ip6_addresses->pdata[0];
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
        wifiState.ssid = _ap_ssid_hex;
        return;
      }

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

        device = getWifiDevice(client);
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

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
      } else {
        device = getWifiDevice(client);
        if(device) {
          return nm_device_wifi_get_mode((NMDeviceWifi*)device) == NM_802_11_MODE_AP;
        }
      }

      return false;
    }

    WifiScanErrorCode Wifi::ScanForWifiAccessPoints(std::vector<WifiScanResult>& results)
    {
      NMDevice *device;
      const GPtrArray *scan_results = nullptr;
      GBytes* ssid_bytes = nullptr;
      NMClient *client = nullptr;
      GError *error = nullptr;
      std::map<std::string, WifiScanResult> resultMap;

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
        return WifiScanErrorCode::FAILED_SCANNING;
      } else {
        device = getWifiDevice(client);
        if(device) {
          nm_device_wifi_request_scan(NM_DEVICE_WIFI(device), nullptr, nullptr);
          scan_results = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(device));
        }

        if(scan_results) {
          for(int i = 0; i < scan_results->len; i++) {
            WifiScanResult result;
            NMAccessPoint *ap = (NMAccessPoint*)scan_results->pdata[i];
            NM80211ApSecurityFlags security_wpa_flags = nm_access_point_get_wpa_flags(ap);
            NM80211ApSecurityFlags security_wpa2_flags = nm_access_point_get_rsn_flags(ap);
            NM80211ApFlags ap_flags = nm_access_point_get_flags(ap);
            gint signal = nm_access_point_get_strength(ap);
            ssid_bytes = nm_access_point_get_ssid(ap);

            result.hidden = true;
            result.auth  = APSecurity2WifiAuth(security_wpa_flags);
            if(result.auth == AUTH_NONE_OPEN) {
              result.auth = APSecurity2WifiAuth(security_wpa2_flags);
            }

            if(result.auth == AUTH_NONE_OPEN) {
              result.auth = ap_flags == NM_802_11_AP_FLAGS_PRIVACY ? WifiAuth::AUTH_NONE_WEP : WifiAuth::AUTH_NONE_OPEN;
            }

            if(ssid_bytes) {
              result.ssid = std::string(nm_utils_bin2hexstr(g_bytes_get_data(ssid_bytes,nullptr), g_bytes_get_size(ssid_bytes), -1));
              ToUpper(result.ssid);
              Log::Write("bssid found: %s %s", result.ssid.c_str(), nm_access_point_get_bssid(ap));
              result.hidden = false;

              result.provisioned = HaveMatchingConnection(client, ap);
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
        g_object_unref (client);
      }
      return WifiScanErrorCode::SUCCESS;
    }

    bool Wifi::RemoveWifiServiceAll()
    {
      NMClient *client = nullptr;
      GError *error = nullptr;
      bool ret = false;

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

    bool Wifi::RemoveWifiService(std::string hex_ssid)
    {
      NMClient *client = nullptr;
      GError *error = nullptr;
      bool ret = false;

      client = nm_client_new (nullptr, &error);
      if (!client) {
        Log::Error("Could not connect to NetworkManager: %s.", error->message);
        g_error_free (error);
        return ret;
      } else {
        ret = RemoveWifiService(client, hex_ssid);
        g_object_unref (client);
      }

      return ret;
    }

    bool Wifi::RemoveWifiService(NMClient *client, std::string hex_ssid)
    {
      NMRemoteConnection *rconnection = nullptr;
      char *uuid = nullptr;
      const GPtrArray *conn_array = nullptr;
      bool removed = false;
      GError *error = nullptr;

      ToUpper(hex_ssid);

      /* Remove any existing connections for this SSID */
      conn_array = nm_client_get_connections(client);
      Log::Write("Forgetting : %s.", hex_ssid.c_str());
      for(int i=0; i < conn_array->len; i++) {
        NMConnection *conn = (NMConnection*)conn_array->pdata[i];
        Log::Write("Forgetting - found : %s.", nm_connection_get_id(conn));
        std::string connid = nm_connection_get_id(conn);
        ToUpper(connid);
        if(hex_ssid == connid || hex_ssid.empty()) {
          Log::Write("Forgetting - match : %s.", hex_ssid.c_str());

          if (error) {
            Log::Error("Could not disconnect: %s.", error->message);
            g_error_free (error);
          }

          uuid = (char*)nm_connection_get_uuid(conn);
          if(uuid) {
            rconnection = nm_client_get_connection_by_uuid(client, uuid);
          } else {
            Log::Error("Could not find uuid for: %s.", hex_ssid.c_str());
          }
          if(rconnection) {
            nm_remote_connection_delete(rconnection, NULL, NULL);
            removed = true;
          } else {
            Log::Error("Could not find remote connection for: %s.", uuid);
          }
        }
      }
      reloadConnections(client);
      return removed;
    }

    bool Wifi::HaveMatchingConnection(NMClient *client, NMAccessPoint *ap)
    {
      const GPtrArray *conn_array = nullptr;

      conn_array = nm_client_get_connections(client);
      for(int i=0; i < conn_array->len; i++) {
        NMConnection *conn = (NMConnection*)conn_array->pdata[i];
        if(nm_access_point_connection_valid(ap, conn)) {
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
