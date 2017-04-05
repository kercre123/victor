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
#include "driver/rtc.h"
#include "driver/crash.h"
#include "driver/factoryData.h"
#include "gpio.h"
#include "backgroundTask.h"
#include "foregroundTask.h"
#include "user_config.h"
#include "anki/cozmo/robot/flash_map.h"
#include "anki/cozmo/robot/buildTypes.h"


// Forward declarations
bool SetFirmwareNote(const u32 offset, u32 note);
u32  GetFirmwareNote(const u32 offset);

/** System calls this method before initalizing the radio.
 * This method is only nessisary to call system_phy_set_rfoption which may only be called here.
 * I think everything else should still happen in user_init and system_init_done
 */
void user_rf_pre_init(void)
{
  crashHandlerInit(); // Set up our own crash handler, so we can record crashes in more detail
}

char wifiPsk[DIGITDASH_WIFI_PSK_LEN+1];
#define WIFI_PSK_BUFF_SZ (sizeof(wifiPsk))  //must follow declaration

struct connection_record connections[AP_MAX_CONNECTIONS];

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

  i2spiInit();
}


static bool wifi_monitor_connection_status(uint32_t param)
{
  u8 numConnected = wifi_softap_get_station_num();
  u8 numRecorded = 0;
  os_printf("WiFi connection change %d\r\n", param);
  if (numConnected > 0)
  {
    int n;
    for (n=0;n<AP_MAX_CONNECTIONS;n++) {
      u8 i;
      for (i=0; i<MAC_ADDR_BYTES; i++)
      {
        if (connections[n].mac[i] != 0) {
          numRecorded++;
          break; }
      }
    }
    if (numConnected > numRecorded)
    {
      os_printf("WIFI has connected station(s) without MAC. Resetting\r\n");
      wifi_set_opmode_current(NULL_MODE);  //off
      wifi_set_opmode_current(SOFTAP_MODE); //ap-mode back on.
      recordBootError(wifi_monitor_connection_status, numConnected-numRecorded);
    }
  }
  return false;
}

static void wifi_event_callback(System_Event_t *evt)
{
  switch (evt->event)
  {
    case EVENT_SOFTAPMODE_STACONNECTED:
    {
      uint8 index = evt->event_info.sta_connected.aid - 1;
      os_memcpy(connections[index].mac, evt->event_info.sta_connected.mac, MAC_ADDR_BYTES);
      break;
    }
    case EVENT_SOFTAPMODE_STADISCONNECTED:
    {
      uint8 index = evt->event_info.sta_disconnected.aid - 1;
      os_memset(connections[index].mac, 0x00, MAC_ADDR_BYTES);
      foregroundTaskPost(wifi_monitor_connection_status, 0);
      break;
    }
  }
}


static void  get_next_dhcp_block(struct dhcps_lease *dhcpInfo)
{
  
  const uint32_t DHCP_MARKER_BLOCK_ADDR = (DHCP_MARKER_SECTOR*SECTOR_SIZE);
  uint32_t dhcp_block_available[DHCP_MARKER_BLOCK_SZ];

  const SpiFlashOpResult result = spi_flash_read(DHCP_MARKER_BLOCK_ADDR, dhcp_block_available, DHCP_MARKER_BLOCK_SZ * sizeof(uint32_t));
  int i = 0;
  if (result == SPI_FLASH_RESULT_OK)
  {
    for (i=0;i<DHCP_MARKER_BLOCK_SZ;i++)
    {
      if (dhcp_block_available[i]) { break; }
    }
  }
  //else just use the beginning of the range

  dhcpInfo->start_ip.addr  = ipaddr_addr(DHCP_START);
  dhcpInfo->end_ip.addr = dhcpInfo->start_ip.addr;
 
  ip4_addr4(&(dhcpInfo->start_ip.addr)) += i * AP_MAX_CONNECTIONS;
  ip4_addr4(&(dhcpInfo->end_ip.addr)) += i * AP_MAX_CONNECTIONS + (AP_MAX_CONNECTIONS-1);
  
  os_printf("Now issuing leases in section %d: " IPSTR " - " IPSTR " range\r\n", i, IP2STR(&dhcpInfo->start_ip.addr), IP2STR(&dhcpInfo->end_ip.addr));
 
  if (i == DHCP_MARKER_BLOCK_SZ)
  { // all used, reset
    spi_flash_erase_sector(DHCP_MARKER_SECTOR);
  }
  else
  { // zero this word
    dhcp_block_available[0] = 0; // zero word
    spi_flash_write(DHCP_MARKER_BLOCK_ADDR+(sizeof(uint32_t)*i), dhcp_block_available, sizeof(uint32_t));
  }
}

int wifi_setup(const char* ssid, const char* pwd, int channel)
{
  struct softap_config ap_config;
  // Create config for Wifi AP
  int8 err = wifi_softap_get_config(&ap_config);
  if (err == false)
  {
    os_printf("Error getting wifi softap config\r\n");
    recordBootError(wifi_softap_get_config, err);
  }
  os_memset(ap_config.ssid, 0, sizeof(ap_config.ssid));
  os_sprintf((char*)ap_config.ssid, ssid);
  if (pwd) {
    int i;
    for (i=0; *pwd; ++i)
    {
      ap_config.password[i] = *pwd++;
    }
    ap_config.password[i] = 0; // Null terminate
    ap_config.authmode = AUTH_WPA2_PSK;
  }
  else {
    ap_config.authmode = AUTH_OPEN;
  }
  ap_config.ssid_len = 0;
  ap_config.max_connection = AP_MAX_CONNECTIONS;
  ap_config.ssid_hidden = 0; // No hidden SSIDs, they create security problems
  ap_config.beacon_interval = 100; // Must be 50 or lower for iOS devices to connect
  if (channel > 0) {
    ap_config.channel = channel;
  }
  //pin channel to legal bounds just in case
  if ((ap_config.channel < 1) || (ap_config.channel > 11)) {
    ap_config.channel = 1;
  }

  if (!wifi_softap_set_config_current(&ap_config)) {
     recordBootError(wifi_softap_set_config_current, false);
  }
  wifi_set_phy_mode(PHY_MODE_11G);

  // Disable radio sleep
  //wifi_set_sleep_type(NONE_SLEEP_T);
  // XXX: This may help streaming performance, but seems to cause slow advertising - not sure which is worse
  //wifi_set_user_fixed_rate(FIXED_RATE_MASK_AP, PHY_RATE_24);
  
  return ap_config.channel;
  
}

static inline uint8_t selectWiFiChannel(void)
{
  uint8_t channel;
  if (!FACTORY_BOOT_SEQUENCE)
  {
    channel = GetStoredWiFiChannel();
    if (channel != 0) // This is a reboot between application firmware versions, keep channel from RTC
    {
      os_printf("Restoring wifi channel from RTC\r\n");
      SetFirmwareNote(0, 0);
      return channel;
    }
    else
    {
      if (GetFirmwareNote(0) != 0)
      { // This is a boot from factory firmware into this fersion
        // Leave the channel what was stored in flash by factory
        os_printf("Restoring factory wifi channel from flash\r\n");
        SetFirmwareNote(0, 0);
        return -1;  //use existing;
      }
    }
  }
  // else select random channel
  os_get_random(&channel, 1);
  return ((channel % 3) * 5) + 1;  //0,6,11
}

void SetWiFiPsk(void)
{
  static const char VALID_PSK_CHARS[] = "0123456789ACDEFGHIJKLMNPQRSTVWXYZ"; // excluding B, O, and U
   if (getFactoryGeneratedPsk(wifiPsk, WIFI_PSK_BUFF_SZ) == false)
   {
      //pre 1.5 robot, generate pwd the old way
      uint32_t randomPSKData[ALPHANUM_WIFI_PSK_LEN];
      getFactoryRandomSeed(randomPSKData, ALPHANUM_WIFI_PSK_LEN);
      char* randomPSKChars = (char*)randomPSKData;
      int i;
      for (i=0; i<ALPHANUM_WIFI_PSK_LEN; ++i)
      {
         wifiPsk[i] = VALID_PSK_CHARS[randomPSKChars[i] % (sizeof(VALID_PSK_CHARS)-1)]; // Sizeof string includes null which we don't want
      }
      wifiPsk[ALPHANUM_WIFI_PSK_LEN]=0;
   }
}

/** User initialization function
 * This function is responsible for setting all the wireless parameters and
 * Setting up any user application code to run on the espressif.
 * It is called automatically from the os main function.
 */
void user_init(void)
{
  char ssid[65];
  int8 err;
  int8 db_mod;

  wifi_status_led_uninstall();

  //find correct dB reduction:
  db_mod = ((getHardwareRevision() & 0xFF) >= COZMO_HARDWARE_REV_1_5) ? TPW_MODIFICATION_V1_5 : TPW_MODIFICATION_V1_0;
  system_phy_set_max_tpw(MAX_TPW + db_mod);

  REG_SET_BIT(0x3ff00014, BIT(0)); //< Set CPU frequency to 160MHz
  err = system_update_cpu_freq(160);
 
  uart_init(BIT_RATE_3000000, BIT_RATE_115200);

  gpio_init();

  os_printf("Espressif booting up...\r\nCPU set freq rslt = %d\r\n", err);
  os_printf("Hardware rev 1.%d, %dB\r\n",getHardwareRevision()&0xFF, db_mod);

  // Setup factory data access methods
  factoryDataInit();

  uint8 macaddr[6];
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  
  if (getSerialNumber() == 0xFFFFffff)
  {
    os_printf("No serial number present, will use MAC instead\r\n");
    os_sprintf(ssid, "Cozmo_%02x%02x%02x", macaddr[3], macaddr[4], macaddr[5]);
  }
  else
  {
    os_sprintf(ssid, "Cozmo_%06X", getSSIDNumber());
  }

  SetWiFiPsk();

  uint8_t channel = selectWiFiChannel();
  
  wifi_set_event_handler_cb(wifi_event_callback);

  // Setup ESP module to AP mode and apply settings
  if (!wifi_set_opmode(SOFTAP_MODE)) {
     recordBootError(wifi_set_opmode, false);
  }

  channel = wifi_setup(ssid, wifiPsk, channel);

  // Store wifi channel for next hot boot
  SetStoredWiFiChannel(channel);
  
  // Disable DHCP server before setting static IP info
  err = wifi_softap_dhcps_stop();
  if (err == false)
  {
    os_printf("Couldn't stop DHCP server\r\n");
    recordBootError(wifi_softap_dhcps_stop, err);
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
    recordBootError(wifi_set_ip_info, err);
  }

  // Configure the DHCP server
  struct dhcps_lease dhcpInfo;
  get_next_dhcp_block(&dhcpInfo);
  err = wifi_softap_set_dhcps_lease(&dhcpInfo);
  if (err == false)
  {
    os_printf("Couldn't set DHCPS lease information\r\n");
    recordBootError(wifi_softap_set_dhcps_lease, err);
  }

  // Start DHCP server
  err = wifi_softap_dhcps_start();
  if (err == false)
  {
    os_printf("Couldn't restart DHCP server\r\n");
    recordBootError(wifi_softap_dhcps_start, err);
  }

  os_printf("SSID: %s\t(Chan: %d)\r\nPSK: %s\r\n", ssid, channel, wifiPsk);

  // Register callbacks
  system_init_done_cb(&system_init_done);
}
