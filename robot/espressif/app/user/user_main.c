#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "client.h"
#include "driver/uart.h"
#include "driver/spi.h"
#include "nv_params.h"
#include "task0.h"
#include "telnet.h"
#include "upgrade_controller.h"
#include "user_config.h"

#define USER_TASK_INTERVAL_MS 1000

/** User "idle" task
 * Called by OS with lowest priority.
 */
/*LOCAL bool ICACHE_FLASH_ATTR userTask(uint32_t param)
{
  return false;
}*/

static ETSTimer userTimer;

LOCAL void ICACHE_FLASH_ATTR userIntervalTask(void *timer_arg)
{
  static int32 tick = 0;
  int32 tock = system_get_time();
  telnetPrintf("%d\t%dus\r\n", tock, tock-tick-(USER_TASK_INTERVAL_MS*1000));
  tick = tock;
}


/** Handle wifi events passed by the OS
 */
void ICACHE_FLASH_ATTR wifi_event_callback(System_Event_t *evt)
{
  switch (evt->event)
  {
    case EVENT_STAMODE_CONNECTED:
    {
      os_printf("Station connected to %s %d\r\n", evt->event_info.connected.ssid, evt->event_info.connected.channel);
#ifndef COZMO_AS_AP
      // Create ip config
      struct ip_info ipinfo;
      ipinfo.gw.addr = ipaddr_addr(STATION_GATEWAY);
      ipinfo.ip.addr = ipaddr_addr(STATION_IP);
      ipinfo.netmask.addr = ipaddr_addr(STATION_NETMASK);

      // Assign ip config
      if (wifi_set_ip_info(STATION_IF, &ipinfo) == false)
      {
        os_printf("Couldn't set IP info\r\n");
      }
#endif
      break;
    }
    case EVENT_STAMODE_DISCONNECTED:
    {
      os_printf("Station disconnected from %s because %d\r\n",
                evt->event_info.disconnected.ssid,
                evt->event_info.disconnected.reason);
      break;
    }
    case EVENT_STAMODE_AUTHMODE_CHANGE:
    {
      os_printf("Station authmode %d -> %d\r\n",
                evt->event_info.auth_change.old_mode,
                evt->event_info.auth_change.new_mode);
      break;
    }
    case EVENT_STAMODE_GOT_IP:
    {
      os_printf("Station got IP " IPSTR ", " IPSTR ", " IPSTR "\r\n",
                IP2STR(&evt->event_info.got_ip.ip),
                IP2STR(&evt->event_info.got_ip.mask),
                IP2STR(&evt->event_info.got_ip.gw));
      break;
    }
    case EVENT_SOFTAPMODE_STACONNECTED:
    {
      os_printf("AP station %d jointed: " MACSTR "\r\n",
                evt->event_info.sta_connected.aid,
                MAC2STR(evt->event_info.sta_connected.mac));
      // Fix the wifi rate rather than allowing negotiation
      break;
    }
    case EVENT_SOFTAPMODE_STADISCONNECTED:
    {
      os_printf("AP station %d left: " MACSTR "\r\n",
                evt->event_info.sta_connected.aid,
                MAC2STR(evt->event_info.sta_connected.mac));
      break;
    }
    default:
    {
      os_printf("Unhandled wifi event: %d\r\n", evt->event);
    }
  }
}


/** System calls this method before initalizing the radio.
 * This method is only nessisary to call system_phy_set_rfoption which may only be called here.
 * I think everything else should still happen in user_init and system_init_done
 */
void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
  system_phy_set_rfoption(1); // Do all the calibration, don't care how much power we burn
  system_phy_set_max_tpw(82); // Set the maximum  TX power allowed
}

/** Callback after all the chip system initalization is done.
 * We shouldn't do any networking until after this is done.
 */
static void ICACHE_FLASH_ATTR system_init_done(void)
{
  int8 err;
  if (wifi_get_opmode() & SOFTAP_MODE)
  {
    // Create ip config
    struct ip_info ipinfo;
    ipinfo.gw.addr = ipaddr_addr(AP_GATEWAY);
    ipinfo.ip.addr = ipaddr_addr(AP_IP);
    ipinfo.netmask.addr = ipaddr_addr(AP_NETMASK);

    // Assign ip config
    err = wifi_set_ip_info(SOFTAP_IF, &ipinfo);
    if (err == false)
    {
      os_printf("Couldn't set IP info\r\n");
    }

    // Configure DHCP range
    struct dhcps_lease dhcpconf;
    dhcpconf.start_ip.addr = ipaddr_addr(DHCP_START);
    dhcpconf.end_ip.addr   = ipaddr_addr(DHCP_END);
    err = wifi_softap_set_dhcps_lease(&dhcpconf);
    if (err == false)
    {
      os_printf("Couldn't set DHCP server lease information\r\n");
    }
    uint8 dhcps_offer_mode = 0; // Disable default gateway information
    err = wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &dhcps_offer_mode);
    if (err == false)
    {
      os_printf("Couldn't configure DHCPS offer mode\r\n");
    }

    // Start DHCP server
    err = wifi_softap_dhcps_start();
    if (err == false)
    {
      os_printf("Couldn't start DHCP server\r\n");
    }
  }

  // Set up the remote debugging port
  telnetInit();

  // Enable upgrade controller
  upgradeControllerInit();

  // Setup Basestation client
  clientInit();

  // Enable UART0 RX interrupt
  // Only after clientInit
  uart_start();

  // Set up shared background tasks
  task0Init();
  //task0Post(userTask, 0);

  os_timer_disarm(&userTimer);
  os_timer_setfn(&userTimer, userIntervalTask, NULL);
  os_timer_arm(&userTimer, USER_TASK_INTERVAL_MS, true);

  os_printf("CPU Freq: %d MHz\r\n", system_get_cpu_freq());

  os_printf("user initalization complete\r\n");
}

/** User initialization function
 * This function is responsible for setting all the wireless parameters and
 * Setting up any user application code to run on the espressif.
 * It is called automatically from the os main function.
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
    NVParams* nvpars;
    int8 err;

    wifi_status_led_uninstall();

    REG_SET_BIT(0x3ff00014, BIT(0)); //< Set CPU frequency to 160MHz
    err = system_update_cpu_freq(160);

    gpio_init();

    uart_init(BIT_RATE_5000000, BIT_RATE_115200);

    os_printf("Espressif booting up...\r\nCPU set freq rslt = %d\r\n", err);

    nvpars = (NVParams*)os_zalloc(sizeof(NVParams));
    if (nvpars == NULL)
    {
      os_printf("Couldn't allocate memory for NV parameters\r\n");
    }
    os_memset(nvpars, 0, sizeof(NVParams));
    if (system_param_load(USER_NV_START_SEC, 0, nvpars, sizeof(NVParams)) == false)
    {
      os_printf("Couldn't read non-volatile parameters from flash. Will use defaults\r\n");
    }
    if (nvpars->PREFIX != NV_PARAMS_PREFIX)
    {
      os_printf("Non-voltatile parameters prefix incorrect, using defaults\r\n");
      nvpars->wifiOpMode  = SOFTAP_MODE;
      nvpars->wifiChannel = 11;
      // Get the mac address
      uint8 macaddr[6];
      err = wifi_get_macaddr(SOFTAP_IF, macaddr);
      if (err == false)
      {
        os_printf("Error getting mac address info\r\n");
      }
      os_sprintf(nvpars->ssid, AP_SSID_FMT, macaddr[4], macaddr[5]);
      os_sprintf(nvpars->pkey, AP_KEY);
    }

    if (nvpars->wifiOpMode & SOFTAP_MODE) // Cozmo as Access poiont
    {
      struct softap_config ap_config;
      os_printf("Configuring as access point \"%s\"\r\n", nvpars->ssid);
      // Create config for Wifi AP
      err = wifi_softap_get_config(&ap_config);
      if (err == false)
      {
        os_printf("Error getting wifi softap config\r\n");
      }

      os_sprintf((char*)ap_config.ssid, nvpars->ssid);
      os_sprintf((char*)ap_config.password, nvpars->pkey);
      ap_config.ssid_len = 0;
      ap_config.channel = nvpars->wifiChannel;
      ap_config.authmode = AUTH_WPA2_PSK;
      ap_config.max_connection = 8;
      ap_config.ssid_hidden = 0; // No hidden SSIDs, they create security problems
      ap_config.beacon_interval = 33; // Must be 50 or lower for iOS devices to connect

      // Setup ESP module to AP mode and apply settings
      wifi_set_opmode(SOFTAP_MODE);
      wifi_softap_set_config(&ap_config);
      wifi_set_phy_mode(PHY_MODE_11G);
      // Disable radio sleep
      wifi_set_sleep_type(NONE_SLEEP_T);
      wifi_set_user_fixed_rate(PHY_RATE_24);

      // Disable DHCP server before setting static IP info
      wifi_softap_dhcps_stop();
    }
    else // Cozmo as station
    {
      struct station_config sta_config;
      os_printf("Configuring as station to %s\r\n", nvpars->ssid);
      err = wifi_station_get_config_default(&sta_config);
      if (err != 0)
      {
        os_printf("Error getting wifi station default config\r\n");
      }

      // Setup station parameters
      os_sprintf((char*)sta_config.ssid, STATION_SSID);
      os_sprintf((char*)sta_config.password, STATION_KEY);
      #ifdef STATION_BSSID
      os_sprintf(sta_config.bssid, STATION_BSSID)
      sta_config.bssid_set = 1;
      #else
      sta_config.bssid_set = 0;
      #endif

      // Setup ESP module to station mode and apply settings
      // Setup ESP module to AP mode and apply settings
      wifi_set_opmode(STATION_MODE);
      wifi_station_set_config(&sta_config);
      wifi_set_phy_mode(PHY_MODE_11G);
      // Disable radio sleep
      wifi_set_sleep_type(NONE_SLEEP_T);

      // Don't do DHCP client
      wifi_station_dhcpc_stop();
    }

    if (nvpars != NULL)
    {
      os_free(nvpars);
      nvpars = NULL;
    }

    // Register callbacks
    system_init_done_cb(&system_init_done);
    wifi_set_event_handler_cb(wifi_event_callback);

}
