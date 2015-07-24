/** This file is required by the iot_sdk include osapi.h though it doesn't appear that it actually has to have any
 * contents.
 */
#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

#define COZMO_AS_AP

#ifdef COZMO_AS_AP

#define AP_SSID_FMT "AnkiEspressif%02x%02x"
#define AP_KEY      "2manysecrets"
#define AP_IP       "172.31.1.1"
#define AP_NETMASK  "255.255.255.0"
#define AP_GATEWAY  "0.0.0.0"
#define DHCP_START  "171.31.1.2"
#define DHCP_END    "172.31.1.9"

#else

#define STATION_SSID "AnkiRobits"
#define STATION_KEY  "KlaatuBaradaNikto!"
#define STATION_IP      "192.168.3.30"
#define STATION_NETMASK "255.255.248.0"
#define STATION_GATEWAY "192.168.2.1"

#endif

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
/** Set the WiFi to operate at a fixed rate rather than negotiating
 * @param A value from FIXED_RATE enum.
 */
void wifi_set_user_fixed_rate(u8 rate);

#endif
