extern "C" {
  #include "client.h"
  #include "osapi.h"
  #include "user_interface.h"
  #include "driver/i2spi.h"
}
#include "wifi_configuration.h"
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

#define DEBUG_WIFI_CFG

#ifdef DEBUG_WIFI_CFG
#define db_printf(...) os_printf(__VA_ARGS__)
#else
#define db_printf(...)
#endif

static struct softap_config  apConfig;
static struct station_config staConfig;

#define SET_IP_ERROR_OFFSET (-100)
#define UNSUPPORTED_STRING_OFFSET (-200)
#define FLAGS_ERR_OFFSET (-300)
#define BAD_HOSTNAME_ERR (-400)

namespace Anki {
namespace Cozmo {
namespace WiFiConfiguration {

  uint8_t sessionToken[16];

  static bool sendResult(int32_t result, uint8_t command)
  {
    RobotInterface::AppConnectConfigResult msg;
    msg.result  = result;
    msg.command = command;
    return RobotInterface::SendMessage(msg);
  }


  Result Init()
  {
    if (!wifi_softap_get_config(&apConfig)) return RESULT_FAIL;
    if (!wifi_station_get_config_default(&staConfig)) return RESULT_FAIL;
    os_memset(apConfig.ssid,      0, sizeof(apConfig.ssid));
    os_memset(apConfig.password,  0, sizeof(apConfig.password));
    os_memset(staConfig.ssid,     0, sizeof(apConfig.ssid));
    os_memset(staConfig.password, 0, sizeof(staConfig.password));
    apConfig.ssid_len = 0;
    db_printf("WiFiConfig::Init() OKAY\r\n");

    return RESULT_OK;
  }
  
  bool Off(const bool sleep)
  {
    db_printf("WiFiConfiguration::Off(%d)\r\n", sleep);
    if (sleep) os_printf("WARN: WiFi sleep not yet implemented");
    return wifi_set_opmode_current(NULL_MODE);
  }
  
  void SendRobotIpInfo(const u8 ifId)
  {
    RobotInterface::AppConnectRobotIP msg;
    struct ip_info ipInfo;
    os_memset(&msg, 0, sizeof(RobotInterface::AppConnectRobotIP));
    msg.ifId = ifId;
    msg.success = wifi_get_ip_info(ifId, &ipInfo);
    if (msg.success)
    {
      msg.robotIp      = ipInfo.ip.addr;
      msg.robotGateway = ipInfo.gw.addr;
      msg.robotNetmask = ipInfo.netmask.addr;
      if (ifId == STATION_IF) msg.fromDHCP = wifi_station_dhcpc_status();
      msg.success = wifi_get_macaddr(ifId, msg.macaddr);
    }
    db_printf("WiFiConfig robot ip info: %x %x %x %x:%x:%x:%x:%x:%x, %d, %d, %d\r\n",
      msg.robotIp, msg.robotNetmask, msg.robotGateway,
      msg.macaddr[0], msg.macaddr[1], msg.macaddr[2], msg.macaddr[3], msg.macaddr[4], msg.macaddr[5],
      msg.ifId, msg.fromDHCP, msg.success);
    RobotInterface::SendMessage(msg);
  }
  
  void ProcessConfigString(const RobotInterface::AppConnectConfigString& msg)
  {
    using namespace RobotInterface;
    #ifdef DEBUG_WIFI_CFG
    char pbuf[17];
    const int dstrlen = ets_snprintf(pbuf, 17, "%s", msg.data);
    pbuf[16] = 0;
    os_printf("WiFiConfig string: %d \"%s\" [%d]\r\n", msg.id, pbuf, dstrlen);
    #endif
    
    switch(msg.id)
    {
      case AP_AP_SSID_0:
      case AP_AP_SSID_1:
      {
        const int offset = sizeof(msg.data) * (msg.id - AP_AP_SSID_0); // Offset into the string buffer
        os_memcpy(apConfig.ssid + offset, msg.data, sizeof(msg.data));
        break;
      }
      case AP_AP_PSK_0:
      case AP_AP_PSK_1:
      case AP_AP_PSK_2:
      case AP_AP_PSK_3:
      {
        const int offset = sizeof(msg.data) * (msg.id - AP_AP_PSK_0); // Offset into the string buffer
        os_memcpy(apConfig.password + offset, msg.data, sizeof(msg.data));
        break;
      }
      case AP_STA_SSID_0:
      case AP_STA_SSID_1:
      {
        const int offset = sizeof(msg.data) * (msg.id - AP_STA_SSID_0); // Offset into the string buffer
        os_memcpy(staConfig.ssid + offset, msg.data, sizeof(msg.data));
        break;
      }
      case AP_STA_PSK_0:
      case AP_STA_PSK_1:
      case AP_STA_PSK_2:
      case AP_STA_PSK_3:
      {
        const int offset = sizeof(msg.data) * (msg.id - AP_STA_PSK_0); // Offset into the string buffer
        os_memcpy(staConfig.password + offset, msg.data, sizeof(msg.data));
        break;
      }
      case AP_STA_BSSID:
      {
        os_memcpy(staConfig.bssid, msg.data, sizeof(staConfig.bssid));
        break;
      }
      case AP_STA_HOSTNAME:
      {
        char hostname[32];
        os_memset(hostname, 0, sizeof(hostname)); // Copy to zeroed full length buffer for API safety
        os_memcpy(hostname, msg.data, sizeof(msg.data));
        if (wifi_station_set_hostname(hostname) == false);
        {
          os_printf("WiFiConfig: Couldn't set wifi station hostname to \"%s\"\r\n", hostname);
          sendResult(BAD_HOSTNAME_ERR, EngineToRobot::Tag_appConCfgString);
          return;
        }
        break;
      }
      case AP_SESSION_TOKEN:
      {
        os_memcpy(sessionToken, msg.data, sizeof(msg.data));
        break;
      }
      default:
      {
        sendResult(UNSUPPORTED_STRING_OFFSET - msg.id, EngineToRobot::Tag_appConCfgString);
        return;
      }
    }
    sendResult(0, EngineToRobot::Tag_appConCfgString);
  }
  
#define conditionalFlagsError(cond, error, msg, ...) if ((cond) == false) {\
  os_printf("WiFiConfig: " msg "\r\n", ##__VA_ARGS__); \
  sendResult(error, EngineToRobot::Tag_appConCfgFlags); \
  return; \
}
  
  
  void ProcessConfigFlags(const RobotInterface::AppConnectConfigFlags& msg)
  {
    using namespace RobotInterface;
    
    db_printf("WiFiConfg Process ConfigFlags\r\n");
    
    apConfig.channel         = msg.channel;
    apConfig.authmode        = (AUTH_MODE)msg.authMode;
    apConfig.max_connection  = msg.maxConnections;
    apConfig.ssid_hidden     = (msg.apFlags & AP_AP_HIDDEN) ? true : false;
    apConfig.beacon_interval = msg.beaconInterval;
    staConfig.bssid_set      = (msg.apFlags & AP_BSSID)  ? true : false;
    
    if (msg.rfMax_dBm <= MAX_TPW) // A valid dBm, use 0xff to disable setting
    {
      system_phy_set_max_tpw(msg.rfMax_dBm);
    }
    
    const u8 opMode = (msg.apFlags & AP_OPMODE_MSK) >> AP_OPMODE_SHIFT;
    
    if (wifi_get_opmode() != opMode)
    {
      conditionalFlagsError(wifi_set_opmode_current(opMode), FLAGS_ERR_OFFSET-1, "Failed to set opmode");
    }
    
    if (msg.apFlags & AP_PHY_MASK) // WiFi phy mode set requested
    {
      conditionalFlagsError(wifi_set_phy_mode((phy_mode)((msg.apFlags & AP_PHY_MASK) >> AP_PHY_SHIFT)), FLAGS_ERR_OFFSET-2, "Failed to set phy mode");
    }
    
    if (msg.apFlags & AP_FIXED_RATE_MSK)
    {
      conditionalFlagsError(wifi_set_user_fixed_rate((msg.apFlags & AP_FIXED_RATE_MSK) >> AP_FIXED_SHIFT, msg.wifiFixedRate) == 0, FLAGS_ERR_OFFSET-3, "Failed to set fixed rate");
    }
    
    if (msg.apFlags & AP_STATION)
    {
      if (msg.apFlags & AP_APPLY_SETTINGS) conditionalFlagsError(wifi_station_set_config_current(&staConfig), FLAGS_ERR_OFFSET-10, "Failed to set station config");
      //conditionalFlagsError(wifi_station_set_auto_connect(msg.apFlags & AP_STA_AUTOCON ? 1 : 0), FLAGS_ERR_OFFSET-11, "Couldn't set station auto-connect"); // This writes to flash, don't do it
      conditionalFlagsError(wifi_station_set_reconnect_policy(msg.apFlags & AP_STA_RECON ? true : false), FLAGS_ERR_OFFSET-12, "Couldn't set station reconnect policy");

      if (msg.apFlags & AP_STA_DHCP)
      {
        conditionalFlagsError(wifi_station_dhcpc_start(), FLAGS_ERR_OFFSET-13, "Couldn't start station DHCP client");
        if (msg.staDHCPMaxTry != 0xFF) conditionalFlagsError(wifi_station_dhcpc_set_maxtry(msg.staDHCPMaxTry), FLAGS_ERR_OFFSET-14, "Couldn't set station DHCP max tries");
      }
      else
      {
        conditionalFlagsError(wifi_station_dhcpc_stop(), FLAGS_ERR_OFFSET-15, "Couldn't stop station DHCP client");
      }
    }
    
    if (msg.apFlags & AP_SOFTAP)
    {
      if (msg.apFlags & AP_APPLY_SETTINGS) conditionalFlagsError(wifi_softap_set_config_current(&apConfig), FLAGS_ERR_OFFSET-20, "Failed to set softAP config");
      if (msg.apFlags & AP_LIMIT_RATES)
      {
        conditionalFlagsError(wifi_set_user_sup_rate(msg.apMinMaxSupRate & 0x0f, msg.apMinMaxSupRate >> 4) == 0, FLAGS_ERR_OFFSET-21, "Failed to limit supported wifi rates");
      }
      if (msg.apFlags & AP_AP_DHCP_START)
      {
        uint8_t mode = msg.apFlags & AP_ROUTE ? 0x01 : 0x00;
        conditionalFlagsError(wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode), FLAGS_ERR_OFFSET-23, "Failed to set softap DHCP offer options");
        conditionalFlagsError(wifi_softap_dhcps_start(), FLAGS_ERR_OFFSET-24, "Failed to start softap DHCP server");
        if (msg.apDHCPLeaseTime != 0xFFFFffff) conditionalFlagsError(wifi_softap_set_dhcps_lease_time(msg.apDHCPLeaseTime), FLAGS_ERR_OFFSET-25, "Failed to set softap DHCP lease time");
      }
      else if (msg.apFlags & AP_AP_DHCP_STOP)
      {
        conditionalFlagsError(wifi_softap_dhcps_stop(), FLAGS_ERR_OFFSET-30, "Failed to stop softap DHCP server");
      }
    }
    
    sendResult(0, EngineToRobot::Tag_appConCfgFlags);
  }
  
  void ProcessConfigIPInfo(const RobotInterface::AppConnectConfigIPInfo& msg)
  {
    struct ip_info ipInfo;
    ipInfo.ip.addr      = msg.robotIp;
    ipInfo.gw.addr      = msg.robotGateway;
    ipInfo.netmask.addr = msg.robotNetmask;
    
    db_printf("WiFiConfg PCIPI: %x, %x, %x, %d\r\n", msg.robotIp, msg.robotGateway, msg.robotNetmask, msg.ifId);
    
    if(wifi_set_ip_info(msg.ifId, &ipInfo))
    {
      sendResult(0, RobotInterface::EngineToRobot::Tag_appConCfgIPInfo);
    }
    else
    {
      sendResult(SET_IP_ERROR_OFFSET - msg.ifId, RobotInterface::EngineToRobot::Tag_appConCfgIPInfo);
    }
  }
  
} // namespace WiFiConfiguration
} // namespace Cozmo
} // namespace Anki
