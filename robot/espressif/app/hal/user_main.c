#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "eagle_soc.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "client.h"
#include "driver/uart.h"
#include "driver/i2spi.h"
#include "driver/crash.h"
#include "gpio.h"
#include "backgroundTask.h"
#include "foregroundTask.h"
#include "user_config.h"
#include "flash_map.h"

/** System calls this method before initalizing the radio.
 * This method is only nessisary to call system_phy_set_rfoption which may only be called here.
 * I think everything else should still happen in user_init and system_init_done
 */
void user_rf_pre_init(void)
{
  crashHandlerInit(); // Set up our own crash handler, so we can record crashes in more detail
  //system_phy_set_rfoption(1); // Do all the calibration, don't care how much power we burn
}

char wifiPsk[WIFI_PSK_LEN];

/// Forward declarations
typedef void (*NVInitDoneCB)(const int8_t);
int8_t NVInit(const bool garbageCollect, NVInitDoneCB finishedCallback);
void NVWipeAll(void);

static void ICACHE_FLASH_ATTR nv_init_done(const int8_t result)
{
  // Enable I2SPI start only after clientInit and checkAndClearBootloaderConfig
  i2spiInit();

  os_printf("Application Firmware Init Complete\r\n");
}

/** Callback after all the chip system initalization is done.
 * We shouldn't do any networking until after this is done.
 */
static void system_init_done(void)
{
  // Setup Basestation client
  clientInit();

  // Set up shared background tasks
  backgroundTaskInit();
  
  // Set up shared foreground tasks
  foregroundTaskInit();

  // Check the file system integrity
  // Must be called after backgroundTaskInit and foregroundTaskInit
  NVInit(true, nv_init_done);
}

/** User initialization function
 * This function is responsible for setting all the wireless parameters and
 * Setting up any user application code to run on the espressif.
 * It is called automatically from the os main function.
 */
void user_init(void)
{
  static const char VALID_PSK_CHARS[] = "0123456789ACDEFGHIJKLMNPQRSTVWXYZ"; // excluding B, O, and U
  int i;
  char ssid[65];
  int8 err;

  wifi_status_led_uninstall();
  //system_phy_set_tpw_via_vdd33(system_get_vdd33());
  system_phy_set_max_tpw(MAX_TPW-12); // take off another 3dB in the office where there are so many cozmos. need to make this a debug feature
  
  REG_SET_BIT(0x3ff00014, BIT(0)); //< Set CPU frequency to 160MHz
  err = system_update_cpu_freq(160);

  uart_init(BIT_RATE_230400, BIT_RATE_115200);

  gpio_init();

  os_printf("Espressif booting up...\r\nCPU set freq rslt = %d\r\n", err);

  uint8 macaddr[6];
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  
  uint32 randomPSKData[WIFI_PSK_LEN];
  getFactoryRandomSeed(randomPSKData, WIFI_PSK_LEN);
  char* randomPSKChars = (char*)randomPSKData;
  
  if (getSerialNumber() == 0xFFFFffff)
  {
    os_printf("No serial number present, will use MAC instead\r\n");
    os_sprintf(ssid, "Cozmo_%02x%02x%02x", macaddr[3], macaddr[4], macaddr[5]);
  }
  else
  {
    os_sprintf(ssid, "Cozmo_%06X", getSSIDNumber());
  }

  struct softap_config ap_config;
  
  // Create config for Wifi AP
  err = wifi_softap_get_config(&ap_config);
  if (err == false)
  {
    os_printf("Error getting wifi softap config\r\n");
  }
  
  os_sprintf((char*)ap_config.ssid, ssid);
  for (i=0; i<WIFI_PSK_LEN; ++i)
  {
    
    ap_config.password[i] = wifiPsk[i] = VALID_PSK_CHARS[randomPSKChars[i] % (sizeof(VALID_PSK_CHARS)-1)]; // Sizeof string includes null which we don't want
  }
  ap_config.password[WIFI_PSK_LEN] = 0; // Null terminate
  ap_config.ssid_len = 0;
  ap_config.channel = (macaddr[5] % 11) + 1;
  ap_config.authmode = AUTH_WPA2_PSK;
  ap_config.max_connection = AP_MAX_CONNECTIONS;
  ap_config.ssid_hidden = 0; // No hidden SSIDs, they create security problems
  ap_config.beacon_interval = 33; // Must be 50 or lower for iOS devices to connect

  // Setup ESP module to AP mode and apply settings
  wifi_set_opmode(SOFTAP_MODE);
  wifi_softap_set_config(&ap_config);
  wifi_set_phy_mode(PHY_MODE_11G);
  // Disable radio sleep
  //wifi_set_sleep_type(NONE_SLEEP_T);
  // XXX: This may help streaming performance, but seems to cause slow advertising - not sure which is worse
  //wifi_set_user_fixed_rate(FIXED_RATE_MASK_AP, PHY_RATE_24);
  
  // Disable DHCP server before setting static IP info
  err = wifi_softap_dhcps_stop();
  if (err == false)
  {
    os_printf("Couldn't stop DHCP server\r\n");
  }

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

  // Configure the DHCP server
  struct dhcps_lease dhcpInfo;
  dhcpInfo.start_ip.addr = ipaddr_addr(DHCP_START);
  dhcpInfo.end_ip.addr   = ipaddr_addr(DHCP_END);
  err = wifi_softap_set_dhcps_lease(&dhcpInfo);
  if (err == false)
  {
    os_printf("Couldn't set DHCPS lease information\r\n");
  }

  // Start DHCP server
  err = wifi_softap_dhcps_start();
  if (err == false)
  {
    os_printf("Couldn't restart DHCP server\r\n");
  }

  os_printf("SSID: %s\r\nPSK: %s\r\n", ap_config.ssid, ap_config.password);

  // Register callbacks
  system_init_done_cb(&system_init_done);
}
