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
#define FLASH_MEMORY_MAP (0x40200000)

#define AP_MAX_CONNECTIONS 4
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

#endif
