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

#define STACK_START (0x40000000)
#define STACK_END   (0x3fffe000)

#define STACK_DEBUG 0

#if STACK_DEBUG
#define STACK_LEFT(ARG) do { if((ARG)) { int a; os_printf(__FILE__ " %d: %x\r\n", __LINE__, ((unsigned int)&a) - STACK_END); }} while(0)
#define STACK_LEFT_PERIODIC(MS) do { \
  static uint32_t lastTime__FILE____LINE__ = 0; \
  const uint32_t now = system_get_time(); \
  if ((now - lastTime__FILE____LINE__) > ((MS) * 1000)) {\
    STACK_LEFT(1); \
    lastTime__FILE____LINE__ = now; \
  } \
} while(0)
#define ISR_STACK_LEFT(M) do { \
  int a;\
  const unsigned int left = (unsigned int)&a - STACK_END; \
  if (left < 0x0db0) { \
    os_put_char((M)); \
    os_put_hex(left, 3); \
    os_put_char('\r'); os_put_char('\n'); \
  } \
} while(0)
#else
#define STACK_LEFT(...)
#define STACK_LEFT_PERIODIC(...)
#define ISR_STACK_LEFT(...)
#endif

/// Must drop 6dBm below max for FCC
#define MAX_TPW (82-24)

#define AP_MAX_CONNECTIONS 4
#define AP_IP       "172.31.1.1"
#define AP_NETMASK  "255.255.255.0"
#define AP_GATEWAY  "172.31.1.1"
// Last digit should match AP_MAX_CONNECTIONS
#define DHCP_START  "172.31.1.4"
// Shorten by two groups because don't want to allow 0,1,255 in group
#define DHCP_MARKER_BLOCK_SZ  ((255 - (2*AP_MAX_CONNECTIONS)) / (AP_MAX_CONNECTIONS))

#define WIFI_PSK_LEN (12)
extern char wifiPsk[WIFI_PSK_LEN];
#define MAC_ADDR_BYTES (6)

struct connection_record{
   uint8_t mac[MAC_ADDR_BYTES];
};
extern struct connection_record connections[AP_MAX_CONNECTIONS];

#endif
