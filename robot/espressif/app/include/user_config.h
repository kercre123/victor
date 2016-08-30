/** This file is required by the iot_sdk include osapi.h though it doesn't appear that it actually has to have any
 * contents.
 */
#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

/// Put format strings in flash rather than needing to load them into program memory
#define USE_OPTIMIZE_PRINTF
/// Add missing prototype of os_printf_plus
extern int os_printf_plus(const char * format, ...) __attribute__ ((format (printf, 1, 2)));

extern int xPortGetFreeHeapSize(void);    // Faster than system_get_free_heap_size(), reads the same global

/// Based on 1MB flash map
#define FLASH_MEMORY_MAP (0x40200000)

/// Must drop 6dBm below max for FCC
#define MAX_TPW (82-24)

#define AP_MAX_CONNECTIONS 1
#define AP_IP       "172.31.1.1"
#define AP_NETMASK  "255.255.255.0"
#define AP_GATEWAY  "172.31.1.1"
#define DHCP_START  "171.31.1.10"
#define DHCP_END    "172.31.1.15"

#define WIFI_PSK_LEN (12)
extern char wifiPsk[WIFI_PSK_LEN];
#define MAC_ADDR_BYTES (6)
extern uint8_t connectedMac[MAC_ADDR_BYTES];

#endif
