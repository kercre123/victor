/** This file is required by the iot_sdk include osapi.h though it doesn't appear that it actually has to have any
 * contents.
 */
#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

/// Put format strings in flash rather than needing to load them into program memory
#define USE_OPTIMIZE_PRINTF
/// Add missing prototype of os_printf_plus
extern int os_printf_plus(const char * format, ...) __attribute__ ((format (printf, 1, 2)));

/// Based on 1MB flash map
#define USER_NV_PARAM_START 0x7d000
#define USER_NV_START_SEC   0x7d

#define AP_MAX_CONNECTIONS 8
#define AP_SSID_FMT "AnkiEspressif%02x%02x"
#define AP_KEY      "2manysecrets"
#define AP_IP       "172.31.1.1"
#define AP_NETMASK  "255.255.255.0"
#define AP_GATEWAY  "172.31.1.1"
#define DHCP_START  "171.31.1.10"
#define DHCP_END    "172.31.1.15"

#define STATION_SSID "AnkiRobits"
#define STATION_KEY  "KlaatuBaradaNikto!"
#define STATION_IP      "192.168.3.30"
#define STATION_NETMASK "255.255.248.0"
#define STATION_GATEWAY "192.168.2.1"

/** WiFi fix rate configuration
 * This is a custom API added by Espressif for Anki 2015-07-21
 */
enum FIXED_RATE {
          PHY_RATE_48    =    0x8,
          PHY_RATE_24    =    0x9,
          PHY_RATE_12    =    0xA,
          PHY_RATE_6     =    0xB,
          PHY_RATE_54    =    0xC,
          PHY_RATE_36    =    0xD,
          PHY_RATE_18    =    0xE,
          PHY_RATE_9     =    0xF,
};
#define FIXED_RATE_MASK_NONE (0x00)
#define FIXED_RATE_MASK_STA (0x01)
#define FIXED_RATE_MASK_AP  (0x02)
#define FIXED_RATE_MASK_ALL (0x03)

/** Set the WiFi to operate at a fixed rate rather than negotiating
 * @param A value from FIXED_RATE enum.
 */
int wifi_set_user_fixed_rate(uint8 enable_mask, uint8 rate);

/// Rom function to reset the chip
void software_reset(void);

#endif
